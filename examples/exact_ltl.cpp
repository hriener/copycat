#include <bill/sat/solver.hpp>
#include <copycat/algorithms/ltl_learner.hpp>
#include <copycat/chain/print.hpp>
#include <copycat/io/ltl_synthesis_spec_reader.hpp>
#include <copycat/io/traces.hpp>
#include <copycat/trace.hpp>
#include <copycat/utils/read_json.hpp>
#include <copycat/utils/stopwatch.hpp>
#include <copycat/utils/string_utils.hpp>
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

  /* maximum number of nodes to try bounded synthesis */
  uint32_t max_num_nodes = 10u;

  /* solver conflict limit */
  int32_t conflict_limit = -1;
}; /* exact_ltl_parameters */

class exact_ltl_engine
{
public:
  using solver_t = bill::solver<bill::solvers::glucose_41>;

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
    for ( uint32_t num_nodes = 1u; num_nodes <= _ps.max_num_nodes; ++num_nodes )
      if ( exact_synthesis( spec, num_nodes, num_props, spec.operators, instances ) )
        break;

    entry["instances"] = instances;
    _log.emplace_back( entry );
  }

  bool exact_synthesis( copycat::ltl_synthesis_spec const& spec, uint32_t num_nodes, uint32_t num_props, std::vector<copycat::operator_opcode> const& ops, nlohmann::json& json )
  {
    auto instance = nlohmann::json( {} );

    /* clear the progress bar */
    std::cout << "                                                                           \r";

    std::cout << "[i] bounded synthesis with " << num_nodes << std::endl;

    copycat::ltl_encoder enc( solver );

    copycat::ltl_encoder_parameter enc_ps;
    enc_ps.verbose = false;
    enc_ps.num_nodes = num_nodes;
    enc_ps.num_propositions = num_props;
    enc_ps.ops = ops;

    for ( const auto& t : spec.good_traces )
      enc_ps.traces.emplace_back( t, true );
    for ( const auto& t : spec.bad_traces )
      enc_ps.traces.emplace_back( t, false );

    /* restart the solver */
    solver.restart();

    enc.encode( enc_ps );
    enc.allocate_variables();
    // enc.print_allocated_variables();
    enc.check_allocated_variables();
    enc.create_clauses();

    instance["#variables"] = solver.num_variables();
    instance["#clauses"] = solver.num_clauses();
    instance["#nodes"] = num_nodes;

    bill::result::states result;
    copycat::stopwatch<>::duration time_solving;
    {
      copycat::stopwatch watch( time_solving );
      std::cout << "invoke solver" << std::endl;
      result = _ps.conflict_limit < 0 ? solver.solve() : solver.solve( /* no assumptions */{}, _ps.conflict_limit );
    }
    std::cout << fmt::format( "[i] solver time = {}s\n", copycat::to_seconds( time_solving ) );

    instance["time_solving"] = copycat::to_seconds( time_solving );

    if ( result == bill::result::states::satisfiable )
    {
      instance["result"] = "sat";

      std::stringstream chain_as_string;
      auto const& c = enc.extract_chain();
      copycat::write_chain( c, chain_as_string );
      instance["chain"] = chain_as_string.str();

      json.emplace_back( instance );
      return true; /* terminate loop */
    }
    else if ( result == bill::result::states::undefined )
    {
      instance["result"] = "undefined";
      json.emplace_back( instance );
      return false; /* keep going */
    }
    else
    {
      instance["result"] = "unsat";
      json.emplace_back( instance );
      return false; /* keep going */
    }
  }

protected:
  exact_ltl_parameters const& _ps;
  nlohmann::json& _log;

  /* solver */
  solver_t solver;
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
