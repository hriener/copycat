#include <copycat/algorithms/ltl_evaluator.hpp>
#include <copycat/algorithms/ltl_evaluator.hpp>
#include <copycat/algorithms/sequential_simulation.hpp>
#include <copycat/generators/waveform_generator.hpp>
#include <copycat/io/ltl.hpp>
#include <copycat/io/ltl_formula_reader.hpp>
#include <copycat/print.hpp>
#include <copycat/trace.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <lorina/aiger.hpp>
#include <fmt/format.h>

template<typename Ntk>
class trace_generator
{
public:
  explicit trace_generator( Ntk const& ntk, copycat::trace& tr )
    : ntk( ntk )
    , tr( tr )
  {
  }

  void on_time_frame_start( uint32_t time_frame )
  {
    (void)time_frame;
    data.clear();
  }

  void on_pi( uint32_t index, bool value )
  {
    assert( index < ntk.num_cis() + ntk.num_cos() );
    int32_t const id = index + 1;
    // data.emplace_back( value ? id : -id );
    if ( value )
      data.emplace_back( id );
  }

  void on_ri( uint32_t index, bool value )
  {
    assert( ntk.num_pis() + index < ntk.num_cis() + ntk.num_cos() );
    int32_t const id = ntk.num_pis() + index + 1;
    // data.emplace_back( value ? id : -id );
    if ( value )
      data.emplace_back( id );
  }

  void on_ro( uint32_t index, bool value )
  {
    assert( ntk.num_cis() + index < ntk.num_cis() + ntk.num_cos() );
    int32_t const id = ntk.num_cis() + index + 1;
    // data.emplace_back( value ? id : -id );
    if ( value )
      data.emplace_back( id );
  }

  void on_po( uint32_t index, bool value )
  {
    assert( ntk.num_cis() + ntk.num_registers() + index < ntk.num_cis() + ntk.num_cos() );
    int32_t const id = ntk.num_cis() + ntk.num_registers() + index + 1;
    // data.emplace_back( value ? id : -id );
    if ( value )
      data.emplace_back( id );
  }

  void on_time_frame_end( uint32_t time_frame )
  {
    (void)time_frame;
    tr.emplace_prefix( data );
  }

protected:
  Ntk const& ntk;
  copycat::trace& tr;

  std::vector<int32_t> data;
}; /* trace_generator */

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

  /* read LTL formulas */
  copycat::ltl_formula_store ltl;

  /* create the atomic propositions */
  std::map<std::string, copycat::ltl_formula_store::ltl_formula> names;
  names.emplace( "i0_rst", ltl.create_variable() ); // 1
  names.emplace( "i1_empty", ltl.create_variable() );  // 2
  names.emplace( "i2_rxy_rst<0>", ltl.create_variable() ); // 3
  names.emplace( "i3_rxy_rst<1>", ltl.create_variable() ); // 4
  names.emplace( "i4_rxy_rst<2>", ltl.create_variable() ); // 5
  names.emplace( "i5_rxy_rst<3>", ltl.create_variable() ); // 6
  names.emplace( "i6_rxy_rst<4>", ltl.create_variable() ); // 7
  names.emplace( "i7_rxy_rst<5>", ltl.create_variable() ); // 8
  names.emplace( "i8_rxy_rst<6>", ltl.create_variable() ); // 9
  names.emplace( "i9_rxy_rst<7>", ltl.create_variable() ); // 10
  names.emplace( "i10_Cx_rst<0>", ltl.create_variable() ); // 11
  names.emplace( "i11_Cx_rst<1>", ltl.create_variable() ); // 12
  names.emplace( "i12_Cx_rst<2>", ltl.create_variable() ); // 13
  names.emplace( "i13_Cx_rst<3>", ltl.create_variable() ); // 14
  names.emplace( "i14_flit_id<0>", ltl.create_variable() ); // 15
  names.emplace( "i15_flit_id<1>", ltl.create_variable() ); // 16
  names.emplace( "i16_flit_id<2>", ltl.create_variable() ); // 17
  names.emplace( "i17_dst_addr<0>", ltl.create_variable() ); // 18
  names.emplace( "i18_dst_addr<1>", ltl.create_variable() ); // 19
  names.emplace( "i19_dst_addr<2>", ltl.create_variable() ); // 20
  names.emplace( "i20_dst_addr<3>", ltl.create_variable() ); // 21
  names.emplace( "i21_dst_addr<3>", ltl.create_variable() ); // 22
  names.emplace( "i22_cur_addr_rst<0>", ltl.create_variable() ); // 23
  names.emplace( "i23_cur_addr_rst<1>", ltl.create_variable() ); // 24
  names.emplace( "i24_cur_addr_rst<2>", ltl.create_variable() ); // 25
  names.emplace( "i25_cur_addr_rst<3>", ltl.create_variable() ); // 26
  names.emplace( "li0_Nport", ltl.create_variable() ); // 27
  names.emplace( "li1_Wport", ltl.create_variable() ); // 28
  names.emplace( "li2_Eport", ltl.create_variable() ); // 29
  names.emplace( "li3_Sport", ltl.create_variable() ); // 30
  names.emplace( "li4_Lport", ltl.create_variable() ); // 31
  names.emplace( "li5_rxy<0>", ltl.create_variable() ); // 32
  names.emplace( "li6_rxy<1>", ltl.create_variable() ); // 33
  names.emplace( "li7_rxy<2>", ltl.create_variable() ); // 34
  names.emplace( "li8_rxy<3>", ltl.create_variable() ); // 35
  names.emplace( "li9_rxy<4>", ltl.create_variable() ); // 36
  names.emplace( "li10_rxy<5>", ltl.create_variable() ); // 37
  names.emplace( "li11_rxy<6>", ltl.create_variable() ); // 38
  names.emplace( "li12_rxy<7>", ltl.create_variable() ); // 39
  names.emplace( "li13_Cx<0>", ltl.create_variable() ); // 40
  names.emplace( "li14_Cx<1>", ltl.create_variable() ); // 41
  names.emplace( "li15_Cx<2>", ltl.create_variable() ); // 42
  names.emplace( "li16_Cx<3>", ltl.create_variable() ); // 43
  names.emplace( "li17_cur_addr<0>", ltl.create_variable() ); // 44
  names.emplace( "li18_cur_addr<1>", ltl.create_variable() ); // 45
  names.emplace( "li19_cur_addr<2>", ltl.create_variable() ); // 46
  names.emplace( "li20_cur_addr<3>", ltl.create_variable() ); // 47
  names.emplace( "lo0_Nport", ltl.create_variable() ); // 48
  names.emplace( "lo1_Wport", ltl.create_variable() ); // 49
  names.emplace( "lo2_Eport", ltl.create_variable() ); // 50
  names.emplace( "lo3_Sport", ltl.create_variable() ); // 51
  names.emplace( "lo4_Lport", ltl.create_variable() ); // 52
  names.emplace( "lo5_rxy<0>", ltl.create_variable() ); // 53
  names.emplace( "lo6_rxy<1>", ltl.create_variable() ); // 54
  names.emplace( "lo7_rxy<2>", ltl.create_variable() ); // 55
  names.emplace( "lo8_rxy<3>", ltl.create_variable() ); // 56
  names.emplace( "lo9_rxy<4>", ltl.create_variable() ); // 57
  names.emplace( "lo10_rxy<5>", ltl.create_variable() ); // 58
  names.emplace( "lo11_rxy<6>", ltl.create_variable() ); // 59
  names.emplace( "lo12_rxy<7>", ltl.create_variable() ); // 60
  names.emplace( "lo13_Cx<0>", ltl.create_variable() ); // 61
  names.emplace( "lo14_Cx<1>", ltl.create_variable() ); // 62
  names.emplace( "lo15_Cx<2>", ltl.create_variable() ); // 63
  names.emplace( "lo16_Cx<3>", ltl.create_variable() ); // 64
  names.emplace( "lo17_cur_addr<0>", ltl.create_variable() ); // 65
  names.emplace( "lo18_cur_addr<1>", ltl.create_variable() ); // 66
  names.emplace( "lo19_cur_addr<2>", ltl.create_variable() ); // 67
  names.emplace( "lo20_cur_addr<3>", ltl.create_variable() ); // 68
  names.emplace( "o0_Nport", ltl.create_variable() ); // 69
  names.emplace( "o1_Eport", ltl.create_variable() ); // 70
  names.emplace( "o2_Wport", ltl.create_variable() ); // 71
  names.emplace( "o3_Sport", ltl.create_variable() ); // 72
  names.emplace( "o4_Lport", ltl.create_variable() ); // 73

  copycat::ltl_formula_reader reader( ltl, names );
  if ( copycat::read_ltl( "LBDR.ltl", reader ) != copycat::return_code::success )
  {
    std::cout << "[e] could not parse LTL formulas\n";
    return -1;
  }

  std::cout << "#formulas = " << ltl.num_formulas() << std::endl;

  /* simulate */
  copycat::stimuli_simulator sim( aig, stimuli );
  copycat::trace tr;
  trace_generator printer( aig, tr );
  simulate( aig, sim, stimuli.size(), printer );
  // tr.print();

  ltl.foreach_formula( [&]( copycat::ltl_formula_store::ltl_formula const& f ) -> bool {
      /* print the formula */
      copycat::print( std::cout, ltl, f );
      // std::cout << std::endl;

      /* evaluate the formula */
      copycat::ltl_finite_trace_evaluator eval( ltl );
      std::cout << " evaluates to " << copycat::evaluate<copycat::ltl_finite_trace_evaluator>( f, tr, eval ).to_string() << std::endl;

      return true;
    });

  return 0;
}
