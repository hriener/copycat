#include <mockturtle/networks/aig.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <lorina/aiger.hpp>
#include <kitty/print.hpp>

using namespace mockturtle;

struct trace
{
public:
  trace() = default;

  trace( uint32_t length )
    : values( length )
  {
  }

  std::string name;
  std::vector<bool> values;
}; /* trace */

class waveform
{
public:
  waveform( uint32_t num_signals, uint32_t length )
    : traces( num_signals, trace( length ) )
  {
  }

  void print( std::ostream& os = std::cout ) const
  {
    for ( const auto& trace : traces )
    {
      os << trace.name << ' ';
      for ( const auto& v : trace.values )
      {
        os << v;
      }
      os << '\n';
      os.flush();
    }
  }

  trace& operator[]( uint32_t index )
  {
    return traces[index];
  }

protected:
  std::vector<trace> traces;
}; /* wavform */

int main( int argc, char* argv[] )
{
  if ( argc != 4 )
  {
    std::cout << "[i] usage: simulate <filename> <time_steps> <seed>" << std::endl;
    return -1;
  }

  /* parameters */
  std::string filename{ argv[1] };
  uint64_t num_time_steps = std::atol( argv[2] );
  uint64_t random_seed = std::atol( argv[3] );

  std::default_random_engine::result_type seed = random_seed;

  /* read the aig */
  std::cout << "[i] read " << filename << std::endl;
  aig_network aig;
  auto const result = lorina::read_aiger( filename, aiger_reader( aig ) );
  assert( result == lorina::return_code::success );

  /* simulate the aig */
  std::cout << "[i] simulate " << filename << std::endl;

  /* initialize simulator */
  auto gen = std::bind( std::uniform_int_distribution<>(0,1), std::default_random_engine( seed ) );
  std::vector<bool> assignments( aig.num_cis() );
  for ( auto i = 0u; i < aig.num_cis(); ++i )
    assignments[i] = gen();

  using simulator_t = default_simulator<bool>;
  simulator_t sim( assignments );

  waveform wf( aig.num_cis() + aig.num_cos(), num_time_steps );

  auto counter = 0;
  aig.foreach_pi( [&]( const auto& node ){
      (void)node;
      wf[counter].name = "pi";
      ++counter;
    });
  aig.foreach_ri( [&]( const auto& f ){
      (void)f;
      wf[counter].name = "ri";
      ++counter;
    });
  aig.foreach_po( [&]( const auto& f ){
      (void)f;
      wf[counter].name = "po";
      ++counter;
    });
  aig.foreach_ro( [&]( const auto& node ){
      (void)node;
      wf[counter].name = "ro";
      ++counter;
    });

  for ( auto k = 0u; k < num_time_steps; ++k )
  {
    /* simulate nodes */
    auto const v = simulate_nodes<bool, aig_network, simulator_t>( aig, sim );

    /* update traces */
    counter = 0;
    aig.foreach_pi( [&]( const auto& node ){
        (void)node;
        bool const value = v[node];
        wf[counter].values[k] = value;
        ++counter;
      });
    aig.foreach_ri( [&]( const auto& f ){
        (void)f;
        bool const value = aig.is_complemented( f ) ? ~v[ aig.get_node( f )] : v[ aig.get_node( f )];
        wf[counter].values[k] = value;
        ++counter;
      });
    aig.foreach_po( [&]( const auto& f ){
        (void)f;
        bool const value = aig.is_complemented( f ) ? ~v[ aig.get_node( f )] : v[ aig.get_node( f )];
        wf[counter].values[k] = value;
        ++counter;
      });
    aig.foreach_ro( [&]( const auto& node ){
        (void)node;
        bool const value = v[node];
        wf[counter].values[k] = value;
        ++counter;
      });

    /* randomize for next iteration */
    ++seed;
    gen = std::bind( std::uniform_int_distribution<>(0, 1), std::default_random_engine( seed ) );

    /* prepare inputs for next iteration */
    counter = 0;
    aig.foreach_pi( [&]( const auto& node ){
        (void)node;
        bool const value = gen();
        assignments[counter] = value;
        ++counter;
      });
    aig.foreach_ri( [&]( const auto& f ){
        bool const value = aig.is_complemented( f ) ? ~v[ aig.get_node( f )] : v[ aig.get_node( f )];
        assignments[counter] = value;
        ++counter;
      });
  }

  /* print traces */
  wf.print();

  return 0;
}
