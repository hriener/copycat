#include <catch.hpp>
#include <copycat/chain/print.hpp>
#include <copycat/algorithms/ltl_learner.hpp>
#include <bill/sat/solver.hpp>

#include <iostream>
#include <sstream>

using namespace copycat;

TEST_CASE( "Learn one proposition", "[ltl_learner]" )
{
  using solver_t = bill::solver<bill::solvers::glucose_41>;
  solver_t solver;
  ltl_encoder enc( solver );

  trace t0;
  t0.emplace_prefix( { 1 } );

  ltl_encoder_parameter ps;
  ps.verbose = false;
  ps.num_propositions = 1u;
  ps.ops = {};
  ps.num_nodes = 1;
  ps.traces.push_back( std::make_pair( t0, true ) );

  enc.encode( ps );
  enc.allocate_variables();
  // enc.print_allocated_variables();
  enc.check_allocated_variables();
  enc.create_clauses();

  /* synthesize */
  auto const result = solver.solve();
  CHECK( result == bill::result::states::satisfiable );

  std::stringstream chain_as_string;
  auto const& c = enc.extract_chain();
  write_chain( c, chain_as_string );
  CHECK( chain_as_string.str() == "1 := x0\n" );
}

TEST_CASE( "Un-learnable problem", "[ltl_learner]" )
{
  using solver_t = bill::solver<bill::solvers::glucose_41>;
  solver_t solver;
  ltl_encoder enc( solver );

  trace t0;
  t0.emplace_prefix( { -1 } );

  /***
   * It does not mapper how many nodes are available. The problem
   * remains un-learnable because operators NOT is not provided.
   */
  ltl_encoder_parameter ps;
  ps.num_propositions = 1u;
  ps.ops = {};
  ps.num_nodes = 3;
  ps.traces.push_back( std::make_pair( t0, true ) );

  enc.encode( ps );
  enc.allocate_variables();
  // enc.print_allocated_variables();
  enc.check_allocated_variables();
  enc.create_clauses();

  /* synthesize */
  auto const result = solver.solve();
  CHECK( result == bill::result::states::unsatisfiable );
}

TEST_CASE( "Learn negated proposition", "[ltl_learner]" )
{
  using solver_t = bill::solver<bill::solvers::glucose_41>;
  solver_t solver;
  ltl_encoder enc( solver );

  trace t0;
  t0.emplace_prefix( { -1 } );

  ltl_encoder_parameter ps;
  ps.verbose = false;
  ps.num_propositions = 1u;
  ps.ops = { operator_opcode::not_ };
  ps.num_nodes = 2;
  ps.traces.push_back( std::make_pair( t0, true ) );

  enc.encode( ps );
  enc.allocate_variables();
  // enc.print_allocated_variables();
  enc.check_allocated_variables();
  enc.create_clauses();

  /* synthesize */
  auto const result = solver.solve();
  CHECK( result == bill::result::states::satisfiable );

  std::stringstream chain_as_string;
  auto const& c = enc.extract_chain();
  write_chain( c, chain_as_string );
  CHECK( chain_as_string.str() == "1 := x0\n2 := ~( 1 )\n" );
}

TEST_CASE( "Learn two proposition", "[ltl_learner]" )
{
  using solver_t = bill::solver<bill::solvers::glucose_41>;
  solver_t solver;
  ltl_encoder enc( solver );

  /* prepare specification */
  trace t0;
  t0.emplace_prefix( { 1 } );

  trace t1;
  t1.emplace_prefix( { 2 } );

  ltl_encoder_parameter ps;
  ps.num_propositions = 2u;
  ps.ops = { operator_opcode::or_ };
  ps.num_nodes = 3u;
  ps.traces.push_back( std::make_pair( t0, true ) );
  ps.traces.push_back( std::make_pair( t1, true ) );

  enc.encode( ps );
  enc.allocate_variables();
  // enc.print_allocated_variables();
  enc.check_allocated_variables();
  enc.create_clauses();

  /* synthesize */
  auto const result = solver.solve();
  CHECK( result == bill::result::states::satisfiable );
  std::stringstream chain_as_string;
  auto const& c = enc.extract_chain();
  write_chain( c, chain_as_string );
  CHECK( chain_as_string.str() == "1 := x1\n2 := x0\n3 := |( 1,2 )\n" );
}

TEST_CASE( "Learn one proposition, negation, and or", "[ltl_learner]" )
{
  using solver_t = bill::solver<bill::solvers::glucose_41>;
  solver_t solver;
  ltl_encoder enc( solver );

  trace t0;
  t0.emplace_prefix( { 1 } );

  trace t1;
  t1.emplace_prefix( { -1 } );

  ltl_encoder_parameter ps;
  ps.num_propositions = 1u;
  ps.ops = { operator_opcode::not_, operator_opcode::or_ };
  ps.num_nodes = 3u;
  ps.traces.push_back( std::make_pair( t0, true ) );
  ps.traces.push_back( std::make_pair( t1, true ) );

  enc.encode( ps );
  enc.allocate_variables();
  // enc.print_allocated_variables();
  enc.check_allocated_variables();
  enc.create_clauses();

  /* synthesize */
  auto const result = solver.solve();
  CHECK( result == bill::result::states::satisfiable );
  std::stringstream chain_as_string;
  auto const& c = enc.extract_chain();
  write_chain( c, chain_as_string );
  CHECK( chain_as_string.str() == "1 := x0\n2 := ~( 1 )\n3 := |( 2,1 )\n" );
}

TEST_CASE( "Learn next", "[ltl_learner]" )
{
  using solver_t = bill::solver<bill::solvers::glucose_41>;
  solver_t solver;
  ltl_encoder enc( solver );

  /* prepare specification */
  trace t0;
  t0.emplace_prefix( { 2 } );
  t0.emplace_prefix( { 1 } );

  trace t1;
  t1.emplace_prefix( { 2 } );

  ltl_encoder_parameter ps;
  ps.num_propositions = 2u;
  ps.ops = { operator_opcode::next_ };
  ps.num_nodes = 2u;
  ps.traces.push_back( std::make_pair( t0, true ) );
  ps.traces.push_back( std::make_pair( t1, false ) );

  enc.encode( ps );
  enc.allocate_variables();
  // enc.print_allocated_variables();
  enc.check_allocated_variables();
  enc.create_clauses();

  /* synthesize */
  auto const result = solver.solve();
  CHECK( result == bill::result::states::satisfiable );
  std::stringstream chain_as_string;
  auto const& c = enc.extract_chain();
  write_chain( c, chain_as_string );
  CHECK( chain_as_string.str() == "1 := x0\n2 := X( 1 )\n" );
}
