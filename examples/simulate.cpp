#include <mockturtle/networks/aig.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <lorina/aiger.hpp>
#include <sstream>

using namespace mockturtle;

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

  /* read stimuli */
  auto const stimuli = read_stimuli( stimuli_file );

  /* simulate the aig */
  std::cout << "[i] simulate: " << model_file << " with inputs " << stimuli_file << std::endl;

  /* initialize simulator */
  auto const num_time_steps = stimuli.size();
  std::vector<bool> assignments( aig.num_cis(), num_time_steps );
  for ( auto i = 0u; i < aig.num_pis(); ++i )
    assignments[i] = stimuli[0][i];
  for ( auto i = aig.num_pis(); i < aig.num_cis(); ++i )
    assignments[i] = false;

  using simulator_t = default_simulator<bool>;
  simulator_t sim( assignments );

  uint32_t counter;
  for ( auto k = 0u; k < num_time_steps; ++k )
  {
    /* simulate nodes */
    auto const v = simulate_nodes<bool, aig_network, simulator_t>( aig, sim );

    /* print values */
    aig.foreach_ro( [&]( const auto& node ){
        bool const value = v[node];
        std::cout << value;
      });
    std::cout << ' ';
    aig.foreach_pi( [&]( const auto& node ){
        bool const value = v[node];
        std::cout << value;
      });
    std::cout << ' ';
    aig.foreach_po( [&]( const auto& f ){
        bool const value = aig.is_complemented( f ) ? !v[ aig.get_node( f )] : v[ aig.get_node( f )];
        std::cout << value;
      });
    std::cout << ' ';
    aig.foreach_ri( [&]( const auto& f ){
        bool const value = aig.is_complemented( f ) ? !v[ aig.get_node( f )] : v[ aig.get_node( f )];
        std::cout << value;
      });
    std::cout << std::endl;

    /* prepare inputs for next iteration */
    counter = 0;
    aig.foreach_pi( [&]( const auto& node ){
        (void)node;
        assignments[counter] = stimuli[k][counter];
        ++counter;
      });
    aig.foreach_ri( [&]( const auto& f ){
        bool const value = v[ aig.get_node( f )];
        assignments[counter] = aig.is_complemented( f ) ? !value : value;
        ++counter;
      });
  }

  return 0;
}
