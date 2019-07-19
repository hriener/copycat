#include <bill/sat/solver.hpp>
#include <copycat/algorithms/exact_ltl_pdag_encoder.hpp>
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
  uint32_t max_num_nodes = 8u;

  /* solver conflict limit */
  int32_t conflict_limit = -1;

  /* be verbose? */
  bool verbose = false;
}; /* exact_ltl_parameters */

namespace copycat
{

class default_ltl_simulator
{
public:
  explicit default_ltl_simulator() = default;

  bool run( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace ) const
  {
    return eval_rec( chain, trace, chain.length(), 0u );
  }

  bool eval_rec( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace, uint32_t chain_node, uint32_t trace_pos ) const
  {
    auto const label = chain.label_at( chain_node );
    if ( label.size() >= 1u && label[0u] == 'x' )
      return eval_proposition( chain, trace, chain_node, trace_pos );
    else if ( label == "~" )
      return eval_negation( chain, trace, chain_node, trace_pos );
    else if ( label == "&" )
      return eval_conjunction( chain, trace, chain_node, trace_pos );
    else if ( label == "|" )
      return eval_disjunction( chain, trace, chain_node, trace_pos );
    else if ( label == "->" )
      return eval_implies( chain, trace, chain_node, trace_pos );
    else if ( label == "X" )
      return eval_next( chain, trace, chain_node, trace_pos );
    else if ( label == "G" )
      return eval_globally( chain, trace, chain_node, trace_pos );
    else if ( label == "F" )
      return eval_eventually( chain, trace, chain_node, trace_pos );
    else if ( label == "U" )
      return eval_until( chain, trace, chain_node, trace_pos );
    else
    {
      std::cout << "[e] unsupported label " << label << std::endl;
      assert( false );
    }

    return false;
  }

  bool eval_proposition( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace, uint32_t chain_node, uint32_t trace_pos ) const
  {
    // std::cout << "eval_proposition: " << chain_node << ' ' << trace_pos << std::endl;
    auto const label = chain.label_at( chain_node );
    auto const prop_id = std::atoi( label.substr( 1u ).c_str() );
    return trace.has( trace_pos, prop_id+1 );
  }

  bool eval_negation( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace, uint32_t chain_node, uint32_t trace_pos ) const
  {
    // std::cout << "eval_negation: " << chain_node << std::endl;
    auto const step = chain.step_at( chain_node );
    assert( step.size() == 1u );
    return !eval_rec( chain, trace, step[0u], trace_pos );
  }

  bool eval_conjunction( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace, uint32_t chain_node, uint32_t trace_pos ) const
  {
    // std::cout << "eval_conjunction: " << chain_node << std::endl;
    auto const step = chain.step_at( chain_node );
    assert( step.size() == 2u );
    return eval_rec( chain, trace, step[0u], trace_pos ) && eval_rec( chain, trace, step[1u], trace_pos );
  }

  bool eval_disjunction( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace, uint32_t chain_node, uint32_t trace_pos ) const
  {
    // std::cout << "eval_disjunction: " << chain_node << std::endl;
    auto const step = chain.step_at( chain_node );
    assert( step.size() == 2u );
    return eval_rec( chain, trace, step[0u], trace_pos ) || eval_rec( chain, trace, step[1u], trace_pos );
  }

  bool eval_implies( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace, uint32_t chain_node, uint32_t trace_pos ) const
  {
    // std::cout << "eval_implies: " << chain_node << std::endl;
    auto const step = chain.step_at( chain_node );
    assert( step.size() == 2u );
    return ( !eval_rec( chain, trace, step[0u], trace_pos ) ) || eval_rec( chain, trace, step[1u], trace_pos );
  }

  bool eval_globally( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace, uint32_t chain_node, uint32_t trace_pos ) const
  {
    // std::cout << "eval_globally: " << chain_node << std::endl;
    auto const step = chain.step_at( chain_node );
    assert( step.size() == 1u );

    uint32_t start_pos = trace_pos < trace.prefix_length() ? trace_pos : trace.prefix_length();
    for ( auto i = start_pos; i < trace.length(); ++i )
    {
      if ( !eval_rec( chain, trace, step[0u], i ) )
      {
        return false;
      }
    }

    return true;
  }

  bool eval_eventually( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace, uint32_t chain_node, uint32_t trace_pos ) const
  {
    // std::cout << "eval_eventually: " << chain_node << std::endl;
    auto const step = chain.step_at( chain_node );
    assert( step.size() == 1u );

    uint32_t start_pos = trace_pos < trace.prefix_length() ? trace_pos : trace.prefix_length();
    for ( auto i = start_pos; i < trace.length(); ++i )
    {
      if ( eval_rec( chain, trace, step[0u], i ) )
      {
        return true;
      }
    }

    return false;
  }

  bool eval_next( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace, uint32_t chain_node, uint32_t trace_pos ) const
  {
    // std::cout << "eval_next: " << chain_node << std::endl;
    auto const step = chain.step_at( chain_node );
    assert( step.size() == 1u );
    if ( trace_pos == trace.length() - 1u )
      return eval_rec( chain, trace, step[0u], trace.prefix_length() );
    else
      return eval_rec( chain, trace, step[0u], trace_pos + 1u );
  }

  bool eval_until( copycat::chain<std::string,std::vector<int>> const& chain, copycat::trace const& trace, uint32_t chain_node, uint32_t trace_pos ) const
  {
    // std::cout << "eval_until: " << chain_node << std::endl;
    auto const step = chain.step_at( chain_node );
    assert( step.size() == 2u );

    auto start_pos = trace_pos;
    int32_t pos;

    while ( start_pos < trace.length() )
    {
      pos = -1;

      /* find a position at which step[1u] is satisfied */
      for ( int32_t i = start_pos; i < int32_t( trace.length() ); ++i )
      {
        if ( eval_rec( chain, trace, step[1u], i ) )
        {
          pos = int32_t( i );
          break;
        }
      }

      if ( pos == -1 )
        break;

      /* check if step[0u] is true before */
      bool all_true = true;
      for ( int32_t j = trace_pos; j < pos; ++j )
      {
        all_true &= eval_rec( chain, trace, step[0u], j );
        if ( !all_true )
          break;
      }

      if ( all_true )
        return true;

      start_pos = pos + 1;
    }

    /* if we cannot find a positions and we are already looking in the suffix, then
       look in the suffix before the current position */
    if ( pos == -1 && trace_pos >= trace.prefix_length() )
    {
      for ( uint32_t i = trace.prefix_length(); i < trace_pos; ++i )
      {
        if ( eval_rec( chain, trace, step[1u], i ) )
        {
          pos = int32_t( i );
          break;
        }
      }
    }

    if ( pos == -1 )
      return false;

    /* check if step[0u] is true before */
    bool all_true = true;
    for ( uint32_t j = trace_pos; j < trace.length(); ++j )
    {
      all_true &= eval_rec( chain, trace, step[0u], j );
      if ( !all_true )
        return false;
    }
    for ( int32_t j = trace.prefix_length(); j < pos; ++j )
    {
      all_true &= eval_rec( chain, trace, step[0u], j );
      if ( !all_true )
        return false;
    }

    return true;

    // return eval_rec( chain, trace, step[1u], trace_pos ) || ( eval_rec( chain, trace, step[0u], trace_pos ) && eval_rec( chain, trace, chain_node, trace_pos + 1 ) );
  }
}; /* ltl_default_simulator */

template<class Simulator = default_ltl_simulator>
bool simulate( copycat::chain<std::string,std::vector<int>> const& c, copycat::trace const& trace, Simulator const& sim = Simulator() )
{
  return sim.run( c, trace );
}

bool simulate( copycat::chain<std::string,std::vector<int>> const& c, copycat::ltl_synthesis_spec const& spec )
{
  for ( const auto& g : spec.good_traces )
  {
    // std::cout << "good trace "; g.print();
    if ( !simulate( c, g ) )
    {
      return false;
    }
  }

  for ( const auto& b : spec.bad_traces )
  {
    // std::cout << "bad trace "; b.print();
    if ( simulate( c, b ) )
    {
      return false;
    }
  }

  return true;
}

} /* namespace copycat */

template<
  typename Solver = bill::solver<bill::solvers::glucose_41>,
  typename Encoder = copycat::exact_ltl_pdag_encoder<Solver>
>
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

    /* clear the progress bar */
    std::cout << "                                                                           \r";

    std::cout << "[i] problem instance: " << entry["file"] << std::endl;

    /* determine number of propositions */
    assert( spec.good_traces.size() + spec.bad_traces.size() > 0u );

    entry["#good_traces"] = spec.good_traces.size();
    entry["#bad_traces"] = spec.bad_traces.size();
    entry["has_op_params"] = spec.operators.size() > 0u ? true : false;
    entry["has_cost_params"] = spec.parameters.size() > 0u ? true : false;
    entry["has_verify_params"] = spec.formulas.size() > 0u ? true : false;
    entry["num_propositions"] = spec.num_propositions;

    /* bounded synthesis loop */
    copycat::stopwatch<>::duration time_total{0};
    total_pdags_explored = 0u;
    {
      copycat::stopwatch watch( time_total );

      auto instances = nlohmann::json::array();
      for ( uint32_t num_nodes = 1u; num_nodes <= _ps.max_num_nodes; ++num_nodes )
        if ( exact_synthesis( spec, num_nodes, instances ) )
          break;

      entry["instances"] = instances;
    }
    entry["#total_pdags_explored"] = total_pdags_explored;
    entry["total_time"] = fmt::format( "{:8.2f}", copycat::to_seconds( time_total ) );
    std::cout << fmt::format( "[i] total time: {:8.2f}s\n", copycat::to_seconds( time_total ) );

    _log.emplace_back( entry );
  }

  bool exact_synthesis( copycat::ltl_synthesis_spec const& spec, uint32_t num_nodes, nlohmann::json& json )
  {
    auto instance = nlohmann::json( {} );

    std::cout << "[i] bounded synthesis with " << num_nodes << " node" << std::endl;

    bool return_value = false; /* keep going with loop */
    if constexpr ( std::is_same<Encoder,copycat::ltl_encoder<Solver>>::value )
    {
      std::cout << "Encoder: exact_ltl_encoder" << std::endl;
      copycat::ltl_encoder_parameter enc_ps;
      enc_ps.verbose = _ps.verbose;
      enc_ps.num_nodes = num_nodes;
      enc_ps.num_propositions = spec.num_propositions;
      enc_ps.ops = spec.operators;

      for ( const auto& t : spec.good_traces )
        enc_ps.traces.emplace_back( t, true );
      for ( const auto& t : spec.bad_traces )
        enc_ps.traces.emplace_back( t, false );

      /* restart the solver */
      solver.restart();

      Encoder enc( solver );
      enc.encode( enc_ps );

      instance["#variables"] = solver.num_variables();
      instance["#clauses"] = solver.num_clauses();
      instance["#nodes"] = num_nodes;

      bill::result::states result;
      copycat::stopwatch<>::duration time_solving{0};
      {
        copycat::stopwatch watch( time_solving );
        result = _ps.conflict_limit < 0 ? solver.solve() : solver.solve( /* no assumptions */{}, _ps.conflict_limit );
      }
      std::cout << fmt::format( "[i] solver: {} in {:8.2f}s\n",
                                copycat::to_upper( bill::result::to_string( result ) ),
                                copycat::to_seconds( time_solving ) );

      instance["time_solving"] = fmt::format( "{:8.2f}", copycat::to_seconds( time_solving ) );
      instance["result"] = bill::result::to_string( result );

      if ( result == bill::result::states::satisfiable )
      {
        std::stringstream chain_as_string;
        auto const& c = enc.extract_chain();
        copycat::write_chain( c, chain_as_string );
        instance["chain"] = chain_as_string.str();

        copycat::write_chain( c );

        auto const sim_result = simulate( c, spec );
        std::cout << "[i] simulate: " << ( sim_result ? "verified" : "failed" ) << std::endl;
        instance["verified"] = sim_result;

        return_value = true; /* terminate loop */
      }
    }

    if constexpr ( std::is_same<Encoder,copycat::exact_ltl_pdag_encoder<Solver>>::value )
    {
      std::cout << "Encoder: exact_ltl_pdag_encoder" << std::endl;

      /* pre-compute partial DAGs to guide synthesis */
      auto const pdags = copycat::pd_generate_filtered( num_nodes, spec.num_propositions );
      std::cout << "[i] #pdags: " << pdags.size() << std::endl;

      copycat::exact_ltl_pdag_encoder_parameter enc_ps;
      enc_ps.verbose = _ps.verbose;
      enc_ps.num_propositions = spec.num_propositions;
      enc_ps.ops = spec.operators;

      for ( const auto& t : spec.good_traces )
        enc_ps.traces.emplace_back( t, true );
      for ( const auto& t : spec.bad_traces )
        enc_ps.traces.emplace_back( t, false );

      instance["#pdags"] = pdags.size();
      instance["#nodes"] = num_nodes;
      // instance["#nodes"] = num_nodes;

      uint32_t total_num_vars = 0u;
      uint32_t total_num_clauses = 0u;
      uint32_t num_considered_instances = 0u;

      copycat::stopwatch<>::duration time_solving{0};
      for ( auto i = 0u; i < pdags.size(); ++i )
      {
        ++total_pdags_explored;

        if ( pdags.at( i ).get_vertices().size() != num_nodes )
          continue;

        /* restart the solver */
        solver.restart();

        enc_ps.pd = pdags.at( i );

        Encoder enc( solver );
        enc.encode( enc_ps );

        ++num_considered_instances;
        total_num_vars += solver.num_variables();
        total_num_clauses += solver.num_clauses();

        instance["#variables"] = total_num_vars / num_considered_instances;
        instance["#clauses"] = total_num_clauses / num_considered_instances;
        instance["#pdags_explored"] = ( i + 1 );

        bill::result::states result;
        {
          copycat::stopwatch watch( time_solving );
          result = _ps.conflict_limit < 0 ? solver.solve() : solver.solve( /* no assumptions */{}, _ps.conflict_limit );
        }
        std::cout << fmt::format( "[i] solver (pdag #{}/{}): {} in {:8.2f}s\n",
                                  i, pdags.size(),
                                  copycat::to_upper( bill::result::to_string( result ) ),
                                  copycat::to_seconds( time_solving ) );

        instance["time_solving"] = fmt::format( "{:8.2f}", copycat::to_seconds( time_solving ) );

        if ( result == bill::result::states::satisfiable )
        {
          std::stringstream chain_as_string;
          auto const& c = enc.extract_chain();
          copycat::write_chain( c, chain_as_string );
          instance["chain"] = chain_as_string.str();

          copycat::write_chain( c );

          auto const sim_result = simulate( c, spec );
          std::cout << "[i] simulate: " << ( sim_result ? "verified" : "failed" ) << std::endl;
          instance["verified"] = sim_result;

          return_value = true; /* terminate loop */
          break;
        }
      }
    }

    json.emplace_back( instance );
    return return_value;
  }

protected:
  exact_ltl_parameters const& _ps;
  nlohmann::json& _log;

  /* solver */
  Solver solver;

  uint32_t total_pdags_explored = 0u;
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
  if ( config.count( "verbose" ) )
    ps.verbose = config["verbose"].get<bool>();
  if ( config.count( "max_num_nodes" ) )
    ps.max_num_nodes = config["max_num_nodes"].get<uint32_t>();

  if ( config.count( "benchmarks" ) )
  {
    auto const benchmarks = config["benchmarks"];

    nlohmann::json log = nlohmann::json::array();

    auto progress_counter = 0u;
    for ( const auto& value : benchmarks )
    {
      /* print progress */
      std::cout << fmt::format( "[i] benchmarks = {} / {} ({:6.2f}%)\r",
                                progress_counter, benchmarks.size(),
                                ( 100.00*progress_counter )/double(benchmarks.size()) );
      ++progress_counter;
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

        engine.run( spec );
      }

      std::ofstream ofs( ps.filename );
      ofs << std::setw( 4 ) << log << std::endl;
      ofs.close();
    }
  }

  return 0;
}
