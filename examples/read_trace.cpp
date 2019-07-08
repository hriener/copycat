#include <copycat/io/traces.hpp>
#include <copycat/trace.hpp>
#include <copycat/algorithms/ltl_learner.hpp>
#include <copycat/chain/print.hpp>
#include <bill/sat/solver.hpp>

struct ltl_synthesis_spec
{
  std::vector<copycat::trace> good_traces;
  std::vector<copycat::trace> bad_traces;
  std::vector<std::string> operators;
  std::vector<std::string> parameters;
  std::vector<std::string> formulas;
}; /* ltl_synthesis_spec */

class ltl_synthesis_spec_reader : public copycat::trace_reader
{
public:
  explicit ltl_synthesis_spec_reader( ltl_synthesis_spec& spec )
    : _spec( spec )
  {
  }

  void on_good_trace( std::vector<std::vector<int>> const& prefix,
                      std::vector<std::vector<int>> const& suffix ) const override
  {
    copycat::trace t;
    for ( const auto& p : prefix )
      t.emplace_prefix( p );
    for ( const auto& s : suffix )
      t.emplace_suffix( s );
    _spec.good_traces.emplace_back( t );
  }

  void on_bad_trace( std::vector<std::vector<int>> const& prefix,
                     std::vector<std::vector<int>> const& suffix ) const override
  {
    copycat::trace t;
    for ( const auto& p : prefix )
      t.emplace_prefix( p );
    for ( const auto& s : suffix )
      t.emplace_suffix( s );
    _spec.bad_traces.emplace_back( t );
  }

  void on_operator( std::string const& op ) const override
  {
    _spec.operators.emplace_back( op );
  }

  void on_parameter( std::string const& parameter ) const override
  {
    _spec.parameters.emplace_back( parameter );
  }

  void on_formula( std::string const& formula ) const override
  {
    _spec.formulas.emplace_back( formula );
  }

private:
  ltl_synthesis_spec& _spec;
}; /* ltl_synthesis_spec_reader */

bool read_ltl_synthesis_spec( std::istream& is, ltl_synthesis_spec& spec )
{
  return copycat::read_traces( is, ltl_synthesis_spec_reader( spec ) );
}

bool read_ltl_synthesis_spec( std::string const& filename, ltl_synthesis_spec& spec )
{
  return copycat::read_traces( filename, ltl_synthesis_spec_reader( spec ) );
}

namespace copycat::detail
{
  template<>
  std::string label_to_string( std::string const& label )
  {
    return label;
  }
}

int main( int argc, char* argv[] )
{
  assert( argc == 2u );

  std::string const filename{argv[1]};

  ltl_synthesis_spec spec;
  if ( read_ltl_synthesis_spec( filename, spec ) )
  {
    std::cout << filename << " success" << std::endl;
    // for ( const auto& t : spec.good_traces )
    // {
    //   std::cout << "[+] "; t.print();
    // }
    // for ( const auto& t : spec.bad_traces )
    // {
    //   std::cout << "[-] "; t.print();
    // }
    std::cout << "[i] #good traces: " << spec.good_traces.size() << std::endl;
    std::cout << "[i] #bad traces: "  << spec.bad_traces.size() << std::endl;
  }
  else
  {
    std::cout << filename << " failure" << std::endl;
    return -1;
  }

  /* bound synthesis loop */
  for ( uint32_t num_nodes = 1u; num_nodes <= 10u; ++num_nodes )
  {

    std::cout << "[i] bounded synthesis with " << num_nodes << std::endl;
    using solver_t = bill::solver<bill::solvers::glucose_41>;
    solver_t solver;
    copycat::ltl_encoder enc( solver );

    copycat::ltl_encoder_parameter ps;
    ps.verbose = false;
    ps.num_propositions = 2u;

    std::vector<copycat::operator_opcode> ops;
    for ( const auto& o : spec.operators )
    {
      if ( o == "!" )
        ops.emplace_back( copycat::operator_opcode::not_ );
      else if ( o == "&" )
        ops.emplace_back( copycat::operator_opcode::and_ );
      else if ( o == "|" )
        ops.emplace_back( copycat::operator_opcode::or_ );
      else if ( o == "->" )
        ops.emplace_back( copycat::operator_opcode::implies_ );
      else if ( o == "X" )
        ops.emplace_back( copycat::operator_opcode::next_ );
      else if ( o == "U" )
        ops.emplace_back( copycat::operator_opcode::until_ );
      else if ( o == "F" )
        ops.emplace_back( copycat::operator_opcode::eventually_ );
      else if ( o == "G" )
        ops.emplace_back( copycat::operator_opcode::globally_ );
      else
      {
        std::cout << fmt::format( "[w] unsupported operator `{}'\n", o );
      }
    }

    /* default operator support */
    if ( ops.size() == 0u )
    {
      ops.emplace_back( copycat::operator_opcode::not_ );
      ops.emplace_back( copycat::operator_opcode::next_ );
      ops.emplace_back( copycat::operator_opcode::and_ );
      ops.emplace_back( copycat::operator_opcode::or_ );
      ops.emplace_back( copycat::operator_opcode::implies_ );
      ops.emplace_back( copycat::operator_opcode::until_ );
      ops.emplace_back( copycat::operator_opcode::eventually_ );
      ops.emplace_back( copycat::operator_opcode::globally_ );
    }

    ps.ops = ops;
    ps.num_nodes = num_nodes;

    for ( const auto& t : spec.good_traces )
      ps.traces.emplace_back( std::make_pair( t, true ) );
    for ( const auto& t : spec.bad_traces )
      ps.traces.emplace_back( std::make_pair( t, false ) );

    enc.encode( ps );
    enc.allocate_variables();
    // enc.print_allocated_variables();
    enc.check_allocated_variables();
    enc.create_clauses();

    auto const result = solver.solve();
    if ( result == bill::result::states::satisfiable )
    {
      // std::cout << "SAT" << std::endl;

      std::stringstream chain_as_string;
      auto const& c = enc.extract_chain();
      copycat::write_chain( c );
      return 1;
    }
    else
    {
      // std::cout << "UNSAT" << std::endl;
    }
  }

  return 0;
}
