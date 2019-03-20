#include <mockturtle/networks/aig.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <lorina/aiger.hpp>
#include <kitty/print.hpp>

using namespace mockturtle;

template<typename TT>
class value_simulator
{
public:
  TT compute_constant( bool value ) const
  {
    TT tt;
    return value ? ~tt : tt;
  }

  TT compute_ci( uint32_t index ) const
  {
    return values.at( index );
  }

  TT compute_not( TT const& value ) const
  {
    return ~value;
  }

public:
  std::vector<TT> values;
}; /* value_simulator */

int main( int argc, char* argv[] )
{
  if ( argc != 2 )
  {
    std::cout << "[i] no input file" << std::endl;
    return -1;
  }
  
  std::string filename{argv[1]};

  /* read the aig */
  std::cout << "[i] read " << filename << std::endl;  
  aig_network aig;
  auto const result = lorina::read_aiger( filename, aiger_reader( aig ) );
  assert( result == lorina::return_code::success );

  /* simulate the aig */
  std::cout << "[i] simulate " << filename << std::endl;

  auto const num_vars = 8u; // simulate 2^num_vars innputs in parallel
  auto const num_time_steps = 10u;

  using tt_t = kitty::static_truth_table<num_vars>;
  using simulator_t = value_simulator<tt_t>;
  simulator_t sim;

  /* initialize values with random numbers */
  std::default_random_engine::result_type seed = 0xcafeaffe;

  sim.values.resize( aig.num_cis() );
  aig.foreach_ci( [&]( const auto& node, auto index ){
      tt_t tt;
      kitty::create_random( tt, seed + index );
      sim.values[index] = tt;
    });

  for ( auto k = 0u; k < num_time_steps; ++k )
  {
    ++seed;
    auto const v = simulate_nodes<tt_t, aig_network, simulator_t>( aig, sim );

    auto counter = 0;
    aig.foreach_pi( [&]( const auto& n, const auto index ){
        tt_t tt;
        kitty::create_random( tt, seed + counter );
        sim.values[counter++] = tt;
      });

    aig.foreach_ri( [&]( const auto& f, const auto index ){
        auto const& tt = aig.is_complemented( f ) ? ~v[ aig.get_node( f )] : v[ aig.get_node( f )];

        sim.values[counter++] = tt;
      });

    for ( auto i = 0; i < (1 << num_vars); ++i )
    {
      std::string input;
      aig.foreach_ci( [&]( const auto& n ){
          auto const bit = kitty::get_bit( v[ n ], i );
          input += bit ? '1' : '0';
        });

      std::string output;
      aig.foreach_co( [&]( const auto& f ){
          auto const tt = aig.is_complemented( f ) ? ~v[ aig.get_node( f ) ] : v[ aig.get_node( f ) ];
          auto const bit = kitty::get_bit( tt, i );
          output += bit ? '1' : '0';
        });
    }
  }

  return 0;
}
