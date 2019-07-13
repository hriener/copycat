#include <copycat/io/traces.hpp>
#include <copycat/trace.hpp>
#include <copycat/utils/read_json.hpp>
#include <copycat/utils/string_utils.hpp>
#include <copycat/io/ltl_synthesis_spec_reader.hpp>
#include <fmt/format.h>
#include <iostream>
#include <iomanip>

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

    /* ... and finially do absolutely nothing ... */

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
	engine.run( spec );
      }

      std::ofstream ofs( ps.filename );
      ofs << std::setw( 4 ) << log << std::endl;
      ofs.close();
    }
  }

  return 0;
}
