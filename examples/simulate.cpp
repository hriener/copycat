#include <copycat/algorithms/sequential_simulation.hpp>
#include <copycat/generators/waveform_generator.hpp>
#include <copycat/waveform.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <lorina/aiger.hpp>
#include <sstream>

using namespace copycat;
using namespace mockturtle;

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
  std::default_random_engine::result_type seed = 0; // 0xcafeaffe;
  auto gen = std::bind( std::uniform_int_distribution<>(0,1), std::default_random_engine( seed ) );

#if 0
  random_simulator sim( aig, gen );
  simulation_value_printer printer( aig, std::cout );
  simulate( aig, sim, /* #time steps = */16, printer );
#endif

#if 0
  /* simulation with stimuli */
  stimuli_simulator sim( aig, read_stimuli( stimuli_file ) );
  simulation_value_printer printer( aig, std::cout );
  simulate( aig, sim, stimuli.size(), printer );
#endif

  random_simulator sim( aig, gen );
  waveform wf( /* #signals = */ aig.num_cis() + aig.num_pos(), /* #time steps = */ 16 );
  waveform_generator waveform_gen( aig, wf );
  simulate( aig, sim, /* #time steps = */16, waveform_gen );
  wf.print();

  return 0;
}
