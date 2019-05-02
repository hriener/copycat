#include <catch.hpp>
#include <copycat/algorithms/ltl_evaluator.hpp>

using namespace copycat;

TEST_CASE ( "bool3", "[ltl_evaluator]" )
{
  bool3 b = false;
  CHECK(  b.is_false() );
  CHECK( !b.is_true() );
  CHECK( !b.is_indeterminate() );

  b = true;
  CHECK( !b.is_false() );
  CHECK(  b.is_true() );
  CHECK( !b.is_indeterminate() );

  b = indeterminate;
  CHECK( !b.is_false() );
  CHECK( !b.is_true() );
  CHECK(  b.is_indeterminate() );

  CHECK( bool3( false ) < bool3( indeterminate ) );
  CHECK( bool3( indeterminate ) < bool3 ( true ) );

  CHECK( !bool3( false ) == bool3( true ) );
  CHECK( !bool3( true ) == bool3( false ) );
  CHECK( !bool3( indeterminate ) == bool3( indeterminate ) );
}

TEST_CASE( "Evaluate LTL", "[ltl_evaluator]" )
{
  ltl_formula_store store;

  auto const request = store.create_variable();
  auto const grant   = store.create_variable();

  ltl_evaluator evaluator( store );

  trace t0;
  t0.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( request ) } );
  t0.prefix.emplace_back( std::vector<uint32_t>{} );
  t0.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( grant ) } );
  t0.suffix.emplace_back( std::vector<uint32_t>{ store.get_node( request ) } );
  t0.prefix.emplace_back( std::vector<uint32_t>{} );
  t0.prefix.emplace_back( std::vector<uint32_t>{} );
  t0.prefix.emplace_back( std::vector<uint32_t>{} );

  trace t1;
  t1.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( request ) } );
  t1.prefix.emplace_back( std::vector<uint32_t>{} );
  t1.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( grant ) } );
  t1.suffix.emplace_back( std::vector<uint32_t>{ store.get_node( request ) } );
  t1.prefix.emplace_back( std::vector<uint32_t>{} );
  t1.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( grant ) } );
  t1.prefix.emplace_back( std::vector<uint32_t>{ store.get_node( request ) } );

  /* G( request --> F( grant ) ) */
  auto const property =
    store.create_globally(  store.create_or( !request, store.create_eventually( grant ) ) );

  CHECK( evaluator.eval( property, t0, 0 ).is_indeterminate() );
  CHECK( evaluator.eval( property, t1, 0 ).is_indeterminate() );
}
