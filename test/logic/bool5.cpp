#include <catch.hpp>
#include <copycat/logic/bool5.hpp>

using namespace copycat;

TEST_CASE ( "bool5", "[bool5]" )
{
  bool5 b = false;
  CHECK(  b.is_false() );
  CHECK( !b.is_presumably_false() );
  CHECK( !b.is_inconclusive() );
  CHECK( !b.is_presumably_true() );
  CHECK( !b.is_true() );

  b = presumably_false;
  CHECK( !b.is_false() );
  CHECK(  b.is_presumably_false() );
  CHECK( !b.is_inconclusive() );
  CHECK( !b.is_presumably_true() );
  CHECK( !b.is_true() );

  b = inconclusive5;
  CHECK( !b.is_false() );
  CHECK( !b.is_presumably_false() );
  CHECK(  b.is_inconclusive() );
  CHECK( !b.is_presumably_true() );
  CHECK( !b.is_true() );

  b = presumably_true;
  CHECK( !b.is_false() );
  CHECK( !b.is_presumably_false() );
  CHECK( !b.is_inconclusive() );
  CHECK(  b.is_presumably_true() );
  CHECK( !b.is_true() );

  b = true;
  CHECK( !b.is_false() );
  CHECK( !b.is_presumably_false() );
  CHECK( !b.is_inconclusive() );
  CHECK( !b.is_presumably_true() );
  CHECK(  b.is_true() );
}

TEST_CASE( "Order two bool5", "[bool5]" )
{
  CHECK( bool5( false ) < bool5( presumably_false ) );
  CHECK( bool5( presumably_false ) < bool5( inconclusive5 ) );
  CHECK( bool5( inconclusive5 ) < bool5 ( presumably_true ) );
  CHECK( bool5( presumably_true ) < bool5( true ) );
}

TEST_CASE( "Negate a bool5", "[bool5]" )
{
  CHECK( !bool5( false ) == bool5( true ) );
  CHECK( !bool5( presumably_false ) == bool5( presumably_true ) );
  CHECK( !bool5( inconclusive5 ) == bool5( inconclusive5 ) );
  CHECK( !bool5( presumably_true ) == bool5( presumably_false ) );
  CHECK( !bool5( true ) == bool5( false ) );
}

TEST_CASE( "And of two bool5", "[bool5]" )
{
  CHECK( bool5( true ) == ( bool5( true ) & bool5( true ) ) );
  CHECK( bool5( presumably_true ) == ( bool5( true ) & bool5( presumably_true ) ) );
  CHECK( bool5( inconclusive5 ) == ( bool5( true ) & bool5( inconclusive5 ) ) );
  CHECK( bool5( presumably_false ) == ( bool5( true ) & bool5( presumably_false ) ) );
  CHECK( bool5( false ) == ( bool5( true ) & bool5( false ) ) );

  CHECK( bool5( presumably_true ) == ( bool5( presumably_true ) & bool5( true ) ) );
  CHECK( bool5( presumably_true ) == ( bool5( presumably_true ) & bool5( presumably_true ) ) );
  CHECK( bool5( inconclusive5 ) == ( bool5( presumably_true ) & bool5( inconclusive5 ) ) );
  CHECK( bool5( presumably_false ) == ( bool5( presumably_true ) & bool5( presumably_false ) ) );
  CHECK( bool5( false ) == ( bool5( presumably_true ) & bool5( false ) ) );

  CHECK( bool5( inconclusive5 ) == ( bool5( inconclusive5 ) & bool5( true ) ) );
  CHECK( bool5( inconclusive5 ) == ( bool5( inconclusive5 ) & bool5( presumably_true ) ) );
  CHECK( bool5( inconclusive5 ) == ( bool5( inconclusive5 ) & bool5( inconclusive5 ) ) );
  CHECK( bool5( presumably_false ) == ( bool5( inconclusive5 ) & bool5( presumably_false ) ) );
  CHECK( bool5( false ) == ( bool5( inconclusive5 ) & bool5( false ) ) );

  CHECK( bool5( presumably_false ) == ( bool5( presumably_false ) & bool5( true ) ) );
  CHECK( bool5( presumably_false ) == ( bool5( presumably_false ) & bool5( presumably_true ) ) );
  CHECK( bool5( presumably_false ) == ( bool5( presumably_false ) & bool5( inconclusive5 ) ) );
  CHECK( bool5( presumably_false ) == ( bool5( presumably_false ) & bool5( presumably_false ) ) );
  CHECK( bool5( false ) == ( bool5( presumably_false ) & bool5( false ) ) );

  CHECK( bool5( false ) == ( bool5( false ) & bool5( true ) ) );
  CHECK( bool5( false ) == ( bool5( false ) & bool5( presumably_true ) ) );
  CHECK( bool5( false ) == ( bool5( false ) & bool5( inconclusive5 ) ) );
  CHECK( bool5( false ) == ( bool5( false ) & bool5( presumably_false ) ) );
  CHECK( bool5( false ) == ( bool5( false ) & bool5( false ) ) );
}

TEST_CASE( "Or of two bool5", "[bool5]" )
{
  CHECK( bool5( true ) == ( bool5( true ) | bool5( true ) ) );
  CHECK( bool5( true ) == ( bool5( true ) | bool5( presumably_true ) ) );
  CHECK( bool5( true ) == ( bool5( true ) | bool5( inconclusive5 ) ) );
  CHECK( bool5( true ) == ( bool5( true ) | bool5( presumably_false ) ) );
  CHECK( bool5( true ) == ( bool5( true ) | bool5( false ) ) );

  CHECK( bool5( true ) == ( bool5( presumably_true ) | bool5( true ) ) );
  CHECK( bool5( presumably_true ) == ( bool5( presumably_true ) | bool5( presumably_true ) ) );
  CHECK( bool5( presumably_true ) == ( bool5( presumably_true ) | bool5( inconclusive5 ) ) );
  CHECK( bool5( presumably_true ) == ( bool5( presumably_true ) | bool5( presumably_false ) ) );
  CHECK( bool5( presumably_true ) == ( bool5( presumably_true ) | bool5( false ) ) );

  CHECK( bool5( true ) == ( bool5( inconclusive5 ) | bool5( true ) ) );
  CHECK( bool5( presumably_true ) == ( bool5( inconclusive5 ) | bool5( presumably_true ) ) );
  CHECK( bool5( inconclusive5 ) == ( bool5( inconclusive5 ) | bool5( inconclusive5 ) ) );
  CHECK( bool5( inconclusive5 ) == ( bool5( inconclusive5 ) | bool5( presumably_false ) ) );
  CHECK( bool5( inconclusive5 ) == ( bool5( inconclusive5 ) | bool5( false ) ) );

  CHECK( bool5( true ) == ( bool5( presumably_false ) | bool5( true ) ) );
  CHECK( bool5( presumably_true ) == ( bool5( presumably_false ) | bool5( presumably_true ) ) );
  CHECK( bool5( inconclusive5 ) == ( bool5( presumably_false ) | bool5( inconclusive5 ) ) );
  CHECK( bool5( presumably_false ) == ( bool5( presumably_false ) | bool5( presumably_false ) ) );
  CHECK( bool5( presumably_false ) == ( bool5( presumably_false ) | bool5( false ) ) );

  CHECK( bool5( true ) == ( bool5( false ) | bool5( true ) ) );
  CHECK( bool5( presumably_true ) == ( bool5( false ) | bool5( presumably_true ) ) );
  CHECK( bool5( inconclusive5 ) == ( bool5( false ) | bool5( inconclusive5 ) ) );
  CHECK( bool5( presumably_false ) == ( bool5( false ) | bool5( presumably_false ) ) );
  CHECK( bool5( false ) == ( bool5( false ) | bool5( false ) ) );
}
