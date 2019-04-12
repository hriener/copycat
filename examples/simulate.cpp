#include <mockturtle/networks/aig.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <lorina/aiger.hpp>
#include <sstream>

using namespace mockturtle;

template<typename Ntk, typename RandomGenerator>
class random_simulator
{
public:
  explicit random_simulator( Ntk& ntk, RandomGenerator& gen )
    : ntk( ntk )
    , gen( gen )
  {
  }

  bool initialize_pi( uint32_t index )
  {
    (void)index;
    return gen();
  }

  bool initialize_ro( uint32_t index )
  {
    (void)index;
    return gen();
  }

  bool compute_pi( uint32_t index, uint32_t time_frame )
  {
    (void)index;
    assert( time_frame > 0u );
    return gen();
  }

protected:
  Ntk const& ntk;
  RandomGenerator& gen;
}; /* random_simulator */

template<typename Ntk>
class stimuli_simulator
{
public:
  explicit stimuli_simulator( Ntk const& ntk, std::vector<std::vector<bool>> const& stimuli )
    : ntk( ntk )
    , stimuli( stimuli )
  {}

  bool initialize_pi( uint32_t index )
  {
    return stimuli.at( 0 ).at( index );
  }

  bool initialize_ro( uint32_t index )
  {
    (void)index;
    return false;
  }

  bool compute_pi( uint32_t index, uint32_t time_frame )
  {
    assert( time_frame > 0u );
    return stimuli.at( time_frame ).at( index );
  }

protected:
  Ntk const& ntk;
  std::vector<std::vector<bool>> const& stimuli;
}; /* stimuli_simulator */

template<typename Ntk>
class simulation_value_printer
{
public:
  explicit simulation_value_printer( Ntk const& ntk, std::ostream& os )
    : ntk( ntk )
    , os( os )
  {
  }

  void on_time_frame_start( uint32_t time_frame )
  {
    (void)time_frame;
  }

  void on_ro( uint32_t index, bool value )
  {
    os << value;
    if ( index == ntk.num_registers()-1 )
      os << ' ';
  }

  void on_pi( uint32_t index, bool value )
  {
    os << value;
    if ( index == ntk.num_pis()-1 )
      os << ' ';
  }

  void on_po( uint32_t index, bool value )
  {
    os << value;
    if ( index == ntk.num_pis()-1 )
      os << ' ';
  }

  void on_ri( uint32_t index, bool value )
  {
    os << value;
    if ( index == ntk.num_registers()-1 )
      os << ' ';
  }

  void on_time_frame_end( uint32_t time_frame )
  {
    (void)time_frame;
    os << std::endl;
  }

protected:
  Ntk const& ntk;
  std::ostream& os;
}; /* simulation_value_printer */

std::vector<std::vector<bool>> read_stimuli( std::string const& filename )
{
  std::vector<std::vector<bool>> stimuli;
  std::ifstream infile( filename );
  std::string line;
  while ( std::getline( infile, line ) )
  {
    std::istringstream iss( line );
    std::vector<bool> assignments( line.size() );
    for ( auto i = 0u; i < line.size(); ++i )
    {
      assignments[i] = line[i] == '1' ? true : false;
    }
    stimuli.emplace_back( assignments );
  }
  return stimuli;
}

template<typename Ntk, typename Simulator, typename Callback>
void simulate( Ntk const& ntk, Simulator& sim, uint32_t num_time_steps, Callback& callback )
{
  /* initialize simulator */
  std::vector<bool> assignments( ntk.num_cis() );
  for ( auto i = 0u; i < ntk.num_pis(); ++i )
    assignments[i] = sim.initialize_pi( i );
  for ( auto i = ntk.num_pis(); i < ntk.num_cis(); ++i )
    assignments[i] = sim.initialize_ro( i );

  using combinational_simulator_t = default_simulator<bool>;
  combinational_simulator_t comb_sim( assignments );

  uint32_t index;
  for ( auto k = 1u; k < num_time_steps; ++k )
  {
    callback.on_time_frame_start( k );

    /* simulate nodes */
    auto const v = simulate_nodes<bool, Ntk, combinational_simulator_t>( ntk, comb_sim );

    /* invoke callback */
    ntk.foreach_ro( [&]( const auto& node, auto index ){
        callback.on_ro( index, v[node] );
      });
    ntk.foreach_pi( [&]( const auto& node, auto index ){
        callback.on_pi( index, v[node] );
      });
    ntk.foreach_po( [&]( const auto& f, auto index ){
        bool const value = ntk.is_complemented( f ) ? !v[ ntk.get_node( f )] : v[ ntk.get_node( f )];
        callback.on_po( index, value );
      });
    ntk.foreach_ri( [&]( const auto& f, auto index ){
        bool const value = ntk.is_complemented( f ) ? !v[ ntk.get_node( f )] : v[ ntk.get_node( f )];
        callback.on_ri( index, value );
      });

    /* prepare inputs for next iteration */
    index = 0;
    ntk.foreach_pi( [&]( const auto& node ){
        (void)node;
        assignments[index] = sim.compute_pi( index, k );
        index++;
      });
    ntk.foreach_ri( [&]( const auto& f ){
        bool const value = v[ ntk.get_node( f )];
        assignments[index] = ntk.is_complemented( f ) ? !value : value;
        index++;
      });

    callback.on_time_frame_end( k );
  }
}

int main( int argc, char* argv[] )
{
  if ( argc != 3 )
  {
    std::cout << "usage: simulate <aig model> <inputs>" << std::endl;
    return -1;
  }

  /* parameters */
  std::string model_file{argv[1]};
  std::string stimuli_file{argv[2]};

  /* read the aig */
  aig_network aig;
  auto const result = lorina::read_aiger( model_file, aiger_reader( aig ) );
  assert( result == lorina::return_code::success );

  /* simulate the aig */
  std::cout << "[i] simulate: " << model_file << std::endl;

  /* random simulation */
  std::default_random_engine::result_type seed = 0xcafeaffe;
  auto gen = std::bind( std::uniform_int_distribution<>(0,1), std::default_random_engine( seed ) );

  random_simulator sim( aig, gen );
  simulation_value_printer printer( aig, std::cout );
  simulate( aig, sim, /* #time steps = */16, printer );

#if 0
  /* simulation with stimuli */
  stimuli_simulator sim( aig, read_stimuli( stimuli_file ) );
  simulation_value_printer printer( aig, std::cout );
  simulate( aig, sim, stimuli.size(), printer );
#endif

  return 0;
}
