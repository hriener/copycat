#include <catch.hpp>
#include <copycat/algorithms/ltl_evaluator.hpp>

using namespace copycat;

TEST_CASE( "Evaluate LTL", "[ltl_evaluator]" )
{
  ltl_formula_store store;
  auto ops = ltl_formula_store::ltl_operators( store );

  auto const request = store.create_variable();
  auto const grant   = store.create_variable();

  trace t0;
  t0.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( request ) } );
  t0.prefix.emplace_back( std::vector<uint32_t>{} );
  t0.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( grant ) } );
  t0.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( request ) } );
  t0.prefix.emplace_back( std::vector<uint32_t>{} );
  t0.prefix.emplace_back( std::vector<uint32_t>{} );
  t0.prefix.emplace_back( std::vector<uint32_t>{} );

  trace t1;
  t1.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( request ) } );
  t1.prefix.emplace_back( std::vector<uint32_t>{} );
  t1.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( grant ) } );
  t1.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( request ) } );
  t1.prefix.emplace_back( std::vector<uint32_t>{} );
  t1.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( grant ) } );
  t1.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( request ) } );

  /* G( request --> F( grant ) ) */
  auto const property = ops.globally(  store.create_or( !request, ops.eventually( grant ) ) );

  ltl_finite_trace_evaluator eval( store );
  CHECK( evaluate<ltl_finite_trace_evaluator>( property, t0, eval ).is_inconclusive() );
  CHECK( evaluate<ltl_finite_trace_evaluator>( property, t1, eval ).is_inconclusive() );
}
