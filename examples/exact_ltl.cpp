#include <copycat/io/traces.hpp>
#include <copycat/trace.hpp>
#include <copycat/utils/read_json.hpp>
#include <copycat/utils/string_utils.hpp>
#include <copycat/io/ltl_synthesis_spec_reader.hpp>
#include <copycat/algorithms/ltl_learner.hpp>
#include <copycat/chain/print.hpp>
#include <bill/sat/solver.hpp>
#include <fmt/format.h>
#include <iostream>
#include <iomanip>

namespace copycat::detail
{
  template<>
  std::string label_to_string( std::string const& label )
  {
    return label;
  }
}

struct exact_ltl_parameters
{
  bool generate_constraints = true;
  bool solve_constraints = true;

  /* name of log file */
  std::string filename = "exact_ltl.log";

  /* write statistics about the generated constraints into the log file */
  bool log_constraint_stats = false;

  /* write statistics about the solving process into the log file */
  bool log_solving_stats = false;
}; /* exact_ltl_parameters */

class exact_ltl_engine
{
public:
  explicit exact_ltl_engine( exact_ltl_parameters const& ps, nlohmann::json& log )
    : _ps( ps )
    , _log( log )
  {
  }

  void run( copycat::ltl_synthesis_spec const& spec )
  {
    auto entry = nlohmann::json( {} );
    entry["command"] = fmt::format( "exact_ltl {}", spec.name );

    auto const parts = copycat::split_path( spec.name, { '/' } );
    entry["file"] = parts.at( parts.size() - 1u );

    entry["#good_traces"] = spec.good_traces.size();
    entry["#bad_traces"] = spec.bad_traces.size();
    entry["has_op_params"] = spec.operators.size() > 0u ? true : false;
    entry["has_cost_params"] = spec.parameters.size() > 0u ? true : false;
    entry["has_verify_params"] = spec.formulas.size() > 0u ? true : false;

    /* determine number of propositions */
    assert( spec.good_traces.size() + spec.bad_traces.size() > 0u );
    auto const num_props =
      spec.bad_traces.size() > 0u ? spec.bad_traces.at( 0u ).count_propositions() : spec.good_traces.at( 0u ).count_propositions();

    /* bounded synthesis loop */
    auto instances = nlohmann::json::array();
    for ( uint32_t num_nodes = 1u; num_nodes <= 10u; ++num_nodes )
    {
      auto instance = nlohmann::json( {} );

      /* clear the progress bar */
      std::cout << "                                                                           \r";

      std::cout << "[i] bounded synthesis with " << num_nodes << std::endl;

      using solver_t = bill::solver<bill::solvers::glucose_41>;
      solver_t solver;
      copycat::ltl_encoder enc( solver );

      copycat::ltl_encoder_parameter ps;
      ps.verbose = false;
      ps.num_propositions = num_props;

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

      instance["#variables"] = solver.num_variables();
      instance["#clauses"] = solver.num_clauses();
      instance["#nodes"] = num_nodes;

      auto const result = solver.solve();
      if ( result == bill::result::states::satisfiable )
      {
        // std::cout << "SAT" << std::endl;

        instance["result"] = "sat";

        std::stringstream chain_as_string;
        auto const& c = enc.extract_chain();
        copycat::write_chain( c, chain_as_string );
        instance["chain"] = chain_as_string.str();

        instances.emplace_back( instance );
        break;
      }
      else
      {
        instance["result"] = "unsat";
        instances.emplace_back( instance );
      }
    }

    entry["instances"] = instances;
    _log.emplace_back( entry );
  }

protected:
  exact_ltl_parameters const& _ps;
  nlohmann::json& _log;
}; /* exact_ltl_engine */

int main( int argc, char* argv[] )
{
  if ( argc != 2 )
  {
    std::cout << fmt::format( "[i] usage: {} <JSON config file>\n", argv[0u] );
    return -1;
  }

  nlohmann::json config;
  std::string const filename = argv[1u];
  if ( !copycat::read_json( argv[1u], config ) )
  {
    std::cout << fmt::format( "[e] could not open or read configuration file `{}`\n", filename );
    return -1;
  }

  /* overwrite default parameters with configuration */
  exact_ltl_parameters ps;
  if ( config.count( "generate_constraints" ) )
    ps.generate_constraints = config["generate_constraints"].get<bool>();
  if ( config.count( "solve_constraints" ) )
    ps.solve_constraints = config["solve_constraints"].get<bool>();
  if ( config.count( "log_constraint_stats" ) )
    ps.log_constraint_stats = config["log_constraint_stats"].get<bool>();
  if ( config.count( "log_solving_stats" ) )
    ps.log_solving_stats = config["log_solving_stats"].get<bool>();

  if ( config.count( "benchmarks" ) )
  {
    auto const benchmarks = config["benchmarks"];

    nlohmann::json log = nlohmann::json::array();

    auto count_benchmarks = 0u;
    auto count = 0u;
    for ( const auto& value : benchmarks )
    {
      /* print progress */
      std::cout << fmt::format( "[i] benchmarks = {} / {} ({:6.2f}%)\r",
                                count_benchmarks, benchmarks.size(),
                                ( 100.00*count_benchmarks )/double(benchmarks.size()) );
      ++count_benchmarks;
      std::cout.flush();

      exact_ltl_engine engine( ps, log );

      copycat::ltl_synthesis_spec spec;
      if ( read_ltl_synthesis_spec( value["file"].get<std::string>(), spec ) )
      {
        spec.name = value["file"].get<std::string>();

        /* let's focus on simple benchmarks only */
        if ( spec.good_traces.size() == 0u && spec.bad_traces.size() == 0u )
          continue;

        if ( spec.good_traces.size() > 5u || spec.bad_traces.size() > 5u ) // 105
          continue;

        ++count;
        engine.run( spec );
      }

      std::ofstream ofs( ps.filename );
      ofs << std::setw( 4 ) << log << std::endl;
      ofs.close();
    }

    std::cout << "                                                                           \r";
    std::cout << count << std::endl;
  }

  return 0;
}
