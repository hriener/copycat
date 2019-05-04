#include <catch.hpp>
#include <copycat/ltl.hpp>

using namespace copycat;

TEST_CASE( "Constant", "[ltl]" )
{
  ltl_formula_store store;
  CHECK( store.get_constant( false ) !=  store.get_constant( true ) );
  CHECK( store.get_constant( false ) == !store.get_constant( true ) );
  CHECK( store.is_constant( store.get_node( store.get_constant( false ) ) ) );
  CHECK( store.is_constant( store.get_node( store.get_constant( true ) ) ) );
  CHECK( store.get_node( store.get_constant( false ) ) ==
         store.get_node( store.get_constant( true ) ) );
  CHECK( !store.is_complemented( store.get_constant( false ) ) );
  CHECK(  store.is_complemented( store.get_constant( true ) ) );
}

TEST_CASE( "Variable", "[ltl]" )
{
  ltl_formula_store store;
  auto const a = store.create_variable();
  auto const b = store.create_variable();
  CHECK( store.is_variable( store.get_node(  a ) ) );
  CHECK( store.is_variable( store.get_node(  b ) ) );
  CHECK( store.is_variable( store.get_node( !a ) ) );
  CHECK( store.is_variable( store.get_node( !b ) ) );
  CHECK( a != b );
  CHECK( !store.is_complemented( a ) );
  CHECK( !store.is_complemented( b ) );
  CHECK( store.get_node( a ) != store.get_node( b ) );
}

TEST_CASE( "Or", "[ltl]" )
{
  ltl_formula_store store;
  auto const a = store.create_variable();
  auto const b = store.create_variable();
  auto const c = store.create_variable();

  // trivial cases
  CHECK( store.create_or( a, store.get_constant( false ) ) == a );
  CHECK( store.create_or( a, store.get_constant( true ) ) == store.get_constant( true ) );

  // symmetry
  CHECK( store.create_or( a, b ) == store.create_or( b, a ) );

  // strashing
  CHECK( store.create_or( store.create_or( a, b ), c ) == store.create_or( c, store.create_or( a, b ) ) );
}

TEST_CASE( "Next", "[ltl]" )
{
  ltl_formula_store store;
  auto const a = store.create_variable();

  // trivial cases
  CHECK( store.create_next( store.get_constant( true ) ) == store.get_constant( true ) );
  CHECK( store.create_next( store.get_constant( false ) ) == store.get_constant( false ) );

  auto const Xa = store.create_next( a );
  CHECK( store.is_next( store.get_node( Xa ) ) );

  CHECK( Xa != a );
}

TEST_CASE( "Until", "[ltl]" )
{
  ltl_formula_store store;
  auto const a = store.create_variable();
  auto const b = store.create_variable();

  auto const aUb = store.create_until( a, b );
  auto const bUa = store.create_until( b, a );
  CHECK( store.is_until( store.get_node( aUb ) ) );
  CHECK( store.is_until( store.get_node( bUa ) ) );

  // anti-symmetry
  CHECK( aUb != bUa );
}
