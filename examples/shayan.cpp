#include <copycat/algorithms/sequential_simulation.hpp>
#include <copycat/generators/waveform_generator.hpp>
#include <copycat/waveform.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <lorina/aiger.hpp>
#include <fmt/format.h>

template<typename Ntk>
class simulation_value_printer
{
public:
  explicit simulation_value_printer( Ntk const& ntk, std::ostream& os )
    : ntk( ntk )
    , os( os )
    , in(  ntk.num_cis(), ' ' )
    , out( ntk.num_cos(), ' ' )
  {
  }

  void on_time_frame_start( uint32_t time_frame )
  {
    (void)time_frame;
  }

  void on_ro( uint32_t index, bool value )
  {
    assert( ntk.num_pis() + index < in.size() );
    in[ntk.num_pis() + index] = value ? '1' : '0';
  }

  void on_pi( uint32_t index, bool value )
  {
    assert( index < in.size() );
    in[index] = value ? '1' : '0';
  }

  void on_ri( uint32_t index, bool value )
  {
    assert( ntk.num_pis() + index < in.size() );
    out[ntk.num_pis() + index] =  value ? '1' : '0';
  }

  void on_po( uint32_t index, bool value )
  {
    assert( index < in.size() );
    out[index] = value ? '1' : '0';
  }

  void on_time_frame_end( uint32_t time_frame )
  {
    (void)time_frame;
    os << in << ' ' << out << std::endl;
  }

protected:
  Ntk const& ntk;
  std::ostream& os;

  std::string in, out;
}; /* simulation_value_printer */

std::vector<std::vector<bool>> load_stimuli( std::string const& filename )
{
  std::vector<std::vector<bool>> stimuli;

  std::ifstream ifs( filename );
  std::string line;
  while ( std::getline( ifs, line ) )
  {
    std::vector<bool> input_assignment( line.size() );
    for ( auto i = 0u; i < line.size(); ++i )
    {
      input_assignment[i] = line.at( i ) == '1' ? true : false;
    }
    stimuli.emplace_back( input_assignment );
  }
  ifs.close();

  return stimuli;
}

int main( int argc, char* argv[] )
{
  (void)argc;
  (void)argv;

  std::string const design_filename = "LBDR.aig";
  std::string const inputs_filename = "inputs.txt";

  /* read design from AIG */
  std::cout << fmt::format( "[i] read AIG from file `{}`\n", design_filename );

  mockturtle::aig_network aig;
  auto const result = lorina::read_aiger( design_filename, mockturtle::aiger_reader( aig ) );
  if ( result != lorina::return_code::success )
  {
    std::cout << fmt::format( "[e] parsing file `{}` failed\n", design_filename );
    return -1;
  }

  /* print some statistics about the design */
  std::cout << fmt::format( "[i] AIG: i={} / o={} / r={} / g={}\n", aig.num_pis(), aig.num_pos(), aig.num_registers(), aig.num_gates() );

  /* read input assignments from file */
  auto const stimuli = load_stimuli( inputs_filename );

  /* print some statistics about the design */
  std::cout << fmt::format( "[i] number of input assignments: {}\n", stimuli.size() );

  /* test the length of the input assignments */
  for ( const auto& input_assignment : stimuli )
  {
    if ( input_assignment.size() != aig.num_pis() )
    {
      std::cout << "[e] the length of the input assignments differs from the number of pis\n";
      return -1;
    }
  }

  /* simulate */
  copycat::stimuli_simulator sim( aig, stimuli );
  simulation_value_printer printer( aig, std::cout );
  simulate( aig, sim, stimuli.size(), printer );

  return 0;
}
