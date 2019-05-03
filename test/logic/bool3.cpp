#include <catch.hpp>
#include <copycat/logic/bool3.hpp>

using namespace copycat;

TEST_CASE ( "bool3", "[bool3]" )
{
  bool3 b = false;
  CHECK(  b.is_false() );
  CHECK( !b.is_true() );
  CHECK( !b.is_indeterminate() );

  b = true;
  CHECK( !b.is_false() );
  CHECK(  b.is_true() );
  CHECK( !b.is_indeterminate() );

  b = indeterminate3;
  CHECK( !b.is_false() );
  CHECK( !b.is_true() );
  CHECK(  b.is_indeterminate() );
}

TEST_CASE( "Order two bool3", "[bool3]" )
{
  CHECK( bool3( false ) < bool3( indeterminate3 ) );
  CHECK( bool3( indeterminate3 ) < bool3 ( true ) );
}

TEST_CASE( "Negate a bool3", "[bool3]" )
{
  CHECK( !bool3( false ) == bool3( true ) );
  CHECK( !bool3( true ) == bool3( false ) );
  CHECK( !bool3( indeterminate3 ) == bool3( indeterminate3 ) );
}

TEST_CASE( "And of two bool3", "[bool3]" )
{
  CHECK( bool3( true ) == ( bool3( true ) & bool3( true ) ) );
  CHECK( bool3( false ) == ( bool3( true ) & bool3( false ) ) );
  CHECK( bool3( indeterminate3 ) == ( bool3( true ) & bool3( indeterminate3 ) ) );

  CHECK( bool3( false ) == ( bool3( false ) & bool3( true ) ) );
  CHECK( bool3( false ) == ( bool3( false ) & bool3( false ) ) );
  CHECK( bool3( false ) == ( bool3( false ) & bool3( indeterminate3 ) ) );

  CHECK( bool3( indeterminate3 ) == ( bool3( indeterminate3 ) & bool3( true ) ) );
  CHECK( bool3( false ) == ( bool3( indeterminate3 ) & bool3( false ) ) );
  CHECK( bool3( indeterminate3 ) == ( bool3( indeterminate3 ) & bool3( indeterminate3 ) ) );
}

TEST_CASE( "Or of two bool3", "[bool3]" )
{
  CHECK( bool3( true ) == ( bool3( true ) | bool3( true ) ) );
  CHECK( bool3( true ) == ( bool3( true ) | bool3( false ) ) );
  CHECK( bool3( true ) == ( bool3( true ) | bool3( indeterminate3 ) ) );

  CHECK( bool3( true ) == ( bool3( false ) | bool3( true ) ) );
  CHECK( bool3( false ) == ( bool3( false ) | bool3( false ) ) );
  CHECK( bool3( indeterminate3 ) == ( bool3( false ) | bool3( indeterminate3 ) ) );

  CHECK( bool3( true ) == ( bool3( indeterminate3 ) | bool3( true ) ) );
  CHECK( bool3( indeterminate3 ) == ( bool3( indeterminate3 ) | bool3( false ) ) );
  CHECK( bool3( indeterminate3 ) == ( bool3( indeterminate3 ) | bool3( indeterminate3 ) ) );
}
