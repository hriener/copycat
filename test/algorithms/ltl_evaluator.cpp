#include <catch.hpp>
#include <copycat/algorithms/ltl_evaluator.hpp>
#include <copycat/logic/bool5.hpp>

using namespace copycat;

TEST_CASE( "Evaluate LTL", "[ltl_evaluator]" )
{
  ltl_formula_store store;

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
  auto const property =
    store.globally(  store.create_or( !request, store.eventually( grant ) ) );

  ltl_finite_trace_evaluator eval( store );
  CHECK( evaluate<ltl_finite_trace_evaluator>( property, t0, eval ).is_inconclusive() );
  CHECK( evaluate<ltl_finite_trace_evaluator>( property, t1, eval ).is_inconclusive() );
}

TEST_CASE( "Extended_inttype_pair", "[ltl_evaluator]" )
{
  using euint32_pair = extended_inttype_pair<uint32_t>;
  euint32_t impossible = euint32_t::impossible();
  euint32_t infinite = euint32_t::infinite();

  CHECK( euint32_pair( 0, 0 ).swap() == euint32_pair( 0, 0 ) );
  CHECK( euint32_pair( 0, 0 ).increment() == euint32_pair( 1, 1 ) );
  CHECK( euint32_pair( 0, 0 ).minmax( euint32_pair( impossible, 1 ) ) == euint32_pair( 0, 1 ) );
  CHECK( euint32_pair( 0, 0 ).maxmin( euint32_pair( impossible, 1 ) ) == euint32_pair( impossible, 0 ) );

  CHECK( euint32_pair( infinite, 1 ).swap() == euint32_pair( 1, infinite ) );
  CHECK( euint32_pair( infinite, 1 ).increment() == euint32_pair( infinite, 2 ) );
  CHECK( euint32_pair( infinite, 1 ).minmax( euint32_pair( 7, impossible ) ) == euint32_pair( 7, impossible ) );
  CHECK( euint32_pair( infinite, 1 ).maxmin( euint32_pair( 7, impossible ) ) == euint32_pair( infinite, 1 ) );
}

TEST_CASE( "Counting semantics", "[ltl_evaluator]" )
{
  ltl_formula_store store;

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

  ltl_finite_trace_evaluator_with_counting_semantics eval( store );
  CHECK( eval.evaluate_formula(  request, t1, 0 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula(  request, t1, 1 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula(  request, t1, 2 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula(  request, t1, 3 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula(  request, t1, 4 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula(  request, t1, 5 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula(  request, t1, 6 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula(  request, t1, 7 ) == euint32_pair( 0, 0 ) );

  CHECK( eval.evaluate_formula(    grant, t1, 0 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula(    grant, t1, 1 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula(    grant, t1, 2 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula(    grant, t1, 3 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula(    grant, t1, 4 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula(    grant, t1, 5 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula(    grant, t1, 6 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula(    grant, t1, 7 ) == euint32_pair( 0, 0 ) );

  CHECK( eval.evaluate_formula( !request, t1, 0 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula( !request, t1, 1 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( !request, t1, 2 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( !request, t1, 3 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula( !request, t1, 4 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( !request, t1, 5 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( !request, t1, 6 ) == euint32_pair( euint32_t::impossible(), 0 ) );
  CHECK( eval.evaluate_formula( !request, t1, 7 ) == euint32_pair( 0, 0 ) );
  
  CHECK( eval.evaluate_formula( store.create_eventually( grant ), t1, 0 ) == euint32_pair( 2, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_eventually( grant ), t1, 1 ) == euint32_pair( 1, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_eventually( grant ), t1, 2 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_eventually( grant ), t1, 3 ) == euint32_pair( 2, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_eventually( grant ), t1, 4 ) == euint32_pair( 1, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_eventually( grant ), t1, 5 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_eventually( grant ), t1, 6 ) == euint32_pair( 1, euint32_t::infinite() ) );
  CHECK( eval.evaluate_formula( store.create_eventually( grant ), t1, 7 ) == euint32_pair( 0, euint32_t::infinite() ) );

  CHECK( eval.evaluate_formula( store.create_or( !request, store.create_eventually( grant ) ), t1, 0 ) == euint32_pair( 2, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_or( !request, store.create_eventually( grant ) ), t1, 1 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_or( !request, store.create_eventually( grant ) ), t1, 2 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_or( !request, store.create_eventually( grant ) ), t1, 3 ) == euint32_pair( 2, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_or( !request, store.create_eventually( grant ) ), t1, 4 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_or( !request, store.create_eventually( grant ) ), t1, 5 ) == euint32_pair( 0, euint32_t::impossible() ) );
  CHECK( eval.evaluate_formula( store.create_or( !request, store.create_eventually( grant ) ), t1, 6 ) == euint32_pair( 1, euint32_t::infinite() ) );
  CHECK( eval.evaluate_formula( store.create_or( !request, store.create_eventually( grant ) ), t1, 7 ) == euint32_pair( 0, euint32_t::infinite() ) );

  /* G( request --> F( grant ) ) */
  auto const property =
    store.create_globally( store.create_or( !request, store.create_eventually( grant ) ) );
  
  CHECK( eval.evaluate_formula( property, t1, 0 ) == euint32_pair( euint32_t::infinite(), euint32_t::infinite() ) );
  CHECK( eval.evaluate_formula( property, t1, 1 ) == euint32_pair( euint32_t::infinite(), euint32_t::infinite() ) );
  CHECK( eval.evaluate_formula( property, t1, 2 ) == euint32_pair( euint32_t::infinite(), euint32_t::infinite() ) );
  CHECK( eval.evaluate_formula( property, t1, 3 ) == euint32_pair( euint32_t::infinite(), euint32_t::infinite() ) );
  CHECK( eval.evaluate_formula( property, t1, 4 ) == euint32_pair( euint32_t::infinite(), euint32_t::infinite() ) );
  CHECK( eval.evaluate_formula( property, t1, 5 ) == euint32_pair( euint32_t::infinite(), euint32_t::infinite() ) );
  CHECK( eval.evaluate_formula( property, t1, 6 ) == euint32_pair( euint32_t::infinite(), euint32_t::infinite() ) );
  CHECK( eval.evaluate_formula( property, t1, 7 ) == euint32_pair( euint32_t::infinite(), euint32_t::infinite() ) );
}












/*! \brief LTL predictive evaluator */
class ltl_predictive_evaluator
{
public:
  using formula = ltl_formula_store::ltl_formula;
  using node = ltl_formula_store::node;

  using result_type = bool5;

public:
  explicit ltl_predictive_evaluator( ltl_formula_store& ltl )
    : ltl( ltl )
    , eval( ltl )
  {
  }

  result_type evaluate_formula( formula const& f, trace const& t, uint32_t pos ) const
  {
    return predictive_evaluation( f, t, pos );
  }

  result_type predictive_evaluation( formula const& f, trace const& t, uint32_t pos ) const
  {    
    auto const d = eval.evaluate_formula( f, t, pos );
    auto const first = d.s;
    auto const second = d.f;

    if ( first.is_normal() && second.is_impossible() )
    {
      return true;
    }
    else if ( first.is_normal() && second.is_normal() )
    {
      assert( false );
    }
    else if ( first.is_normal() && second.is_infinite() )
    {
      assert( false );
    }
    else if ( first.is_infinite() && second.is_normal() )
    {
      assert( false );
    }
    else if ( first.is_infinite() && second.is_infinite() )
    {
      assert( false );
    }
    else if ( first.is_impossible() && second.is_normal() )
    {
      return false;
    }

    assert( false );
    
#if 0
  if ( ss.is_normal() && ff.is_impossible() && ss < t.length() - pos )
  {
    std::cout << "PREDICT TRUE" << std::endl;
    return true;
  }
  else if ( ss.is_normal() && ff.is_normal() && ss >= t.length() - pos && ff >= t.length() - pos )
  {
    assert( false && "split with predicate #1" );
  }
  else if ( ss.is_normal() && ff.is_infinite() && ss >= t.length() - pos && ff >= t.length() - pos )
  {
    assert( false && "split with predicate #2" );
  }
  else if ( ss.is_infinite() && ff.is_normal() && ff >= t.length() - pos )
  {
    return predictive_evaluation( store, !f, t, pos );
  }
  else if ( ss.is_infinite() && ff.is_infinite() )
  {
    assert( false && "invoke helper function" );
  }
  else if ( ss.is_impossible() && ff.is_normal() && ff < t.length() - pos )
  {
    std::cout << "PREDICT FALSE" << std::endl;
    return false;
  }
#endif    
    
    assert( false );
  }

  result_type auxiliar_evaluation( formula const& f, trace const& t, uint32_t pos ) const
  {
    assert( false );
  }
  
#if 0
  result_type evaluate_formula( formula const& f, trace const& t, uint32_t pos ) const
  {
    assert( t.is_finite() && "finite trace evaluator only looks at the prefix of the trace" );

    /* negation */
    if ( ltl.is_complemented( f ) )
    {
      return !evaluate_formula( !f, t, pos );
    }

    assert( !ltl.is_complemented( f ) );
    auto const n = ltl.get_node( f );

    /* constant */
    if ( ltl.is_constant( n ) )
    {
      return evaluate_constant( n );
    }
    /* variable */
    else if ( ltl.is_variable( n ) )
    {
      return evaluate_variable( n, t, pos );
    }
    /* or */
    else if ( ltl.is_or( n ) )
    {
      return evaluate_or( n, t, pos );
    }
    /* next */
    else if ( ltl.is_next( n ) )
    {
      return evaluate_next( n, t, pos );
    }
    /* eventually */
    else if ( ltl.is_eventually( n ) )
    {
      assert( false && "not implemented" );
    }
    /* until */
    else if ( ltl.is_until( n ) )
    {
      return evaluate_until( n, t, pos );
    }
    else
    {
      std::cerr << "[e] unknown operator" << std::endl;
      return inconclusive3;
    }
  }
#endif
  
protected:
  result_type evaluate_constant( node const& n ) const
  {
    assert( false );
  }

  result_type evaluate_variable( node const& n, trace const& t, uint32_t pos ) const
  {
    assert( false );
  }

  result_type evaluate_or( node const& n, trace const& t, uint32_t pos ) const
  {
    assert( false );
  }

  result_type evaluate_next( node const& n, trace const& t, uint32_t pos ) const
  {
    assert( false );
  }

  result_type evaluate_until( node const& n, trace const& t, uint32_t pos ) const
  {
    assert( false );
  }

protected:
  ltl_formula_store& ltl;
  ltl_finite_trace_evaluator_with_counting_semantics eval;  
}; /* ltl_finite_trace_evaluator */

















bool3 predictive_predicate( ltl_formula_store& store, ltl_formula_store::ltl_formula const& f, trace const& t, uint32_t pos )
{
  ltl_finite_trace_evaluator_with_counting_semantics eval( store );

  std::vector<euint32_pair> ds( pos+1, euint32_pair( 0, 0 ) );
  for ( auto i = 0u; i <= pos; ++i )
  {
    ds[i] = eval.evaluate_formula( f, t, i );
  }

  /* true */
  for ( auto i = 0u; i < pos; ++i )
  {
    if ( ds.at( i ).s.is_normal() && ds.at( i ).f.is_impossible() && ds.at( i ).s < ds.at( pos ).s )
      return true;
  }

  /* inconclusive */
  bool found = false;
  for ( auto i = 0u; i < pos; ++i )
  {
    if ( ds.at( i ).s.is_normal() && ds.at( i ).f.is_impossible() )
      found = true;
  }
  if ( !found )
    return inconclusive3;

  /* false */
  auto max = ds.at( 0 ).s;
  for ( auto i = 0u; i < pos; ++i )
  {
    if ( ds.at( i ).s.is_normal() && ds.at( i ).f.is_impossible() && ds.at( i ).s > max )
      max = ds.at( i ).s;
  }

  for ( auto i = 0u; i < pos; ++i )
  {
    if ( ds.at( i ).s.is_normal() && ds.at( i ).f.is_impossible() && ds.at( pos ).s > max )
      return false;      
  }
  
  assert( false );
}

bool5 predictive_evaluation( ltl_formula_store& store, ltl_formula_store::ltl_formula const& f, trace const& t, uint32_t pos )
{
  ltl_finite_trace_evaluator_with_counting_semantics eval( store );
  
  auto const d = eval.evaluate_formula( f, t, pos );
  auto const ss = d.s;
  auto const ff = d.f;

  std::cout << ss.value << ' ' << ss.extension << ' ' << ff.value << ' ' << ff.extension << std::endl;
  if ( ss.is_normal() && ff.is_impossible() && ss < t.length() - pos )
  {
    std::cout << "PREDICT TRUE" << std::endl;
    return true;
  }
  else if ( ss.is_normal() && ff.is_normal() && ss >= t.length() - pos && ff >= t.length() - pos )
  {
    assert( false && "split with predicate #1" );
  }
  else if ( ss.is_normal() && ff.is_infinite() && ss >= t.length() - pos && ff >= t.length() - pos )
  {
    assert( false && "split with predicate #2" );
  }
  else if ( ss.is_infinite() && ff.is_normal() && ff >= t.length() - pos )
  {
    return predictive_evaluation( store, !f, t, pos );
  }
  else if ( ss.is_infinite() && ff.is_infinite() )
  {
    assert( false && "invoke helper function" );
  }
  else if ( ss.is_impossible() && ff.is_normal() && ff < t.length() - pos )
  {
    std::cout << "PREDICT FALSE" << std::endl;
    return false;
  }
  std::cout << "UNKNOWN" << std::endl;
  return inconclusive5;
}

TEST_CASE( "Predictive predicate", "[ltl_evaluator]" )
{
  ltl_formula_store store;

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

#if 0
  std::cout << ( predictive_predicate( store, request, t1, 0 ).to_string() ) << std::endl;
  std::cout << ( predictive_predicate( store, request, t1, 1 ).to_string() ) << std::endl;
  std::cout << ( predictive_predicate( store, request, t1, 2 ).to_string() ) << std::endl;
  // std::cout << ( predictive_predicate( store, request, t1, 3 ).to_string() ) << std::endl;
  std::cout << ( predictive_predicate( store, request, t1, 4 ).to_string() ) << std::endl;
  std::cout << ( predictive_predicate( store, request, t1, 5 ).to_string() ) << std::endl;
  // std::cout << ( predictive_predicate( store, request, t1, 6 ).to_string() ) << std::endl;
  // std::cout << ( predictive_predicate( store, request, t1, 7 ).to_string() ) << std::endl;
  // std::cout << ( predictive_predicate( store, request, t1, 8 ).to_string() ) << std::endl;
#endif
  
  predictive_evaluation( store, request, t1, 0 );
  predictive_evaluation( store, request, t1, 1 );
  predictive_evaluation( store, request, t1, 2 );
  predictive_evaluation( store, request, t1, 3 );
  predictive_evaluation( store, request, t1, 4 );
  predictive_evaluation( store, request, t1, 5 );
  predictive_evaluation( store, request, t1, 6 );
  predictive_evaluation( store, request, t1, 7 );

}
