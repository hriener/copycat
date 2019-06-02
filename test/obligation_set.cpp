#include <catch.hpp>
#include <copycat/obligation_set.hpp>
#include <copycat/print.hpp>
#include <iostream>

using namespace copycat;

TEST_CASE( "Equal", "[obligation_set]" )
{
  ltl_formula_store ltl;
  auto const a = ltl.create_variable();
  auto const b = ltl.create_variable();

  obligation_set olg1;
  olg1.add_formula( a );
  olg1.add_formula( ltl.create_or( a, b ) );

  obligation_set olg2;
  olg2.add_formula( b );
  olg2.add_formula( ltl.create_or( a, b ) );

  obligation_set olg3;
  olg3.add_formula( ltl.create_or( a, b ) );
  olg3.add_formula( b );

  CHECK( olg1 != olg2 );
  CHECK( olg1 != olg3 );
  CHECK( olg2 == olg3 );
}

TEST_CASE( "Union", "[obligation_set]" )
{
  ltl_formula_store ltl;
  auto const a = ltl.create_variable();
  auto const b = ltl.create_variable();

  obligation_set olg;
  olg.add_formula( a );
  olg.add_formula( b );
  olg.add_formula( ltl.create_or( a, b ) );

  obligation_set olg1;
  olg1.add_formula( a );
  olg1.add_formula( ltl.create_or( a, b ) );

  obligation_set olg2;
  olg2.add_formula( b );
  olg2.add_formula( ltl.create_or( a, b ) );

  CHECK( olg1 != olg );
  CHECK( olg2 != olg );

  olg1.set_union( olg2 );
  CHECK( olg1 == olg );
}

TEST_CASE( "Compute obligation set 1", "[obligation_set]" )
{
  ltl_formula_store ltl;
  auto const a = ltl.create_variable();
  auto const b = ltl.create_variable();
  auto const c = ltl.create_variable();
  auto const d = ltl.create_variable();

  auto const f = !ltl.create_or( !ltl.create_until( a, b ), !ltl.create_until( c, d ) );
  auto const olg_f = compute_obligations( ltl, f );

  obligation_set olg;
  olg.add_formula( b );
  olg.add_formula( d );

  CHECK( olg == olg_f );
}

TEST_CASE( "Compute obligation set 2", "[obligation_set]" )
{
  ltl_formula_store ltl;
  auto const a = ltl.create_variable();
  auto const b = ltl.create_variable();

  {
    /* formula is not in negation normal form: a and !a is stored a obligations to satisfy, the obligation set is inconsistent */
    auto const f = !ltl.create_or( !ltl.create_releases( ltl.get_constant( false ), !a ), !ltl.create_until( ltl.get_constant( true ), a ) );
    CHECK( f != ltl.get_constant( false ) );

    obligation_set olg_f;
    olg_f.add_formula(  a );
    olg_f.add_formula( !a );

    CHECK( compute_obligations( ltl, f ) == olg_f );
  }

  {
    /* formula is in negation normal form: a & !a is simplified to false, the obligation set is inconsistent */
    auto const g = ltl.create_and( ltl.create_releases( ltl.get_constant( false ), !a ), ltl.create_until( ltl.get_constant( true ), a ) );
    CHECK( g != ltl.get_constant( false ) );

    obligation_set olg_g;
    olg_g.add_formula( ltl.get_constant( false ) );

    CHECK( compute_obligations( ltl, g ) == olg_g );
  }

  {
    /* formula is not in negation normal form, but simplifies to false, such that there is only one false obligation, the obligation set is inconsistent */
    auto const h = !ltl.create_or( ltl.create_until( ltl.get_constant( true ), a ), !ltl.create_until( ltl.get_constant( true ), a ) );
    CHECK( h == ltl.get_constant( false ) );

    obligation_set olg_h;
    olg_h.add_formula( ltl.get_constant( false ) );

    CHECK( compute_obligations( ltl, h) == olg_h );
  }

  {
    /* formula is not in negation normal form, but simplifies to false, such that there is only one false obligation, the obligation set is inconsistent */
    auto const i = ltl.create_and( ltl.create_until( ltl.get_constant( true ), a ), !ltl.create_until( ltl.get_constant( true ), a ) );
    CHECK( i == ltl.get_constant( false ) );

    obligation_set olg_i;
    olg_i.add_formula( ltl.get_constant( false ) );

    CHECK( compute_obligations( ltl, i ) == olg_i );
  }
}
