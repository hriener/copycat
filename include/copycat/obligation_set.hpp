/* copycat
 * Copyright (C) 2018  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file obligation_set.hpp
  \brief Obligation set

  \author Heinz Riener
*/

#pragma once

#include <copycat/ltl.hpp>
#include <algorithm>
#include <vector>

namespace copycat
{

class obligation_set
{
public:
  using formula = ltl_formula_store::ltl_formula;

public:
  explicit obligation_set() = default;

  bool operator==( obligation_set const& other ) const
  {
    if ( _obligations.size() != other._obligations.size() )
      return false;

    for ( auto i = 0u; i < _obligations.size(); ++i )
    {
      if ( _obligations.at( i ) != other._obligations.at( i ) )
        return false;
    }

    return true;
  }

  bool operator!=( obligation_set const& other ) const
  {
    return !operator==( other );
  }

  /*! \brief Adds an LTL formula to the obligation set */
  void add_formula( formula const& f )
  {
    for ( const auto& o : _obligations )
      if ( f == o )
        return;

    _obligations.emplace_back( f );
    std::sort( std::begin( _obligations ), std::end( _obligations ) );
  }

  void set_union( obligation_set const& other )
  {
    for ( const auto& o : other._obligations )
    {
      add_formula( o );
    }
  }

  template<typename Fn>
  void foreach_element( Fn&& fn )
  {
    auto index = 0u;
    for ( auto& o : _obligations )
    {
      fn( o, index++ );
    }
  }

  template<typename Fn>
  void foreach_element( Fn&& fn ) const
  {
    auto index = 0u;
    for ( const auto& o : _obligations )
    {
      fn( o, index++ );
    }
  }

protected:
  std::vector<formula> _obligations;
}; /* obligation_set */

obligation_set cart_product( ltl_formula_store& ltl, obligation_set const& as, obligation_set const& bs )
{
  obligation_set new_set;
  as.foreach_element( [&]( const auto& a, auto ){
      bs.foreach_element( [&]( const auto& b, auto ){
          new_set.add_formula( ltl.create_and( a, b ) );
        });
    });
  return new_set;
}

obligation_set compute_obligations( ltl_formula_store& ltl, ltl_formula_store::ltl_formula const& f )
{
  auto const node = ltl.get_node( f );

  /* constant */
  if ( ltl.is_constant( node ) )
  {
    if ( ltl.is_complemented( f ) )
    {
      /* true */
      return obligation_set{};
    }
    else
    {
      /* false */
      obligation_set olg;
      olg.add_formula( ltl.get_constant( false ) );
      return olg;
    }
  }

  /* literal */
  else if ( ltl.is_variable( node ) )
  {
    obligation_set olg;
    olg.add_formula( f );
    return olg;
  }

  /* next */
  else if ( ltl.is_next( node ) )
  {
    std::vector<ltl_formula_store::ltl_formula> fanins;
    ltl.foreach_fanin( node, [&]( const auto& f, auto ){
        fanins.emplace_back( f );
      } );
    assert( fanins.size() == 1u );
    return compute_obligations( ltl, fanins[0u] );
  }

  /* OR */
  else if ( ltl.is_or( node ) )
  {
    std::vector<ltl_formula_store::ltl_formula> fanins;
    ltl.foreach_fanin( node, [&]( const auto& f, auto ){
        fanins.emplace_back( f );
      } );
    assert( fanins.size() == 2u );

    obligation_set olg0 = compute_obligations( ltl, fanins[0u] );
    obligation_set const olg1 = compute_obligations( ltl, fanins[1u] );
    olg0.set_union( olg1 );
    return olg0;
  }

  /* AND */
  else if ( ltl.is_and( node ) )
  {
    std::vector<ltl_formula_store::ltl_formula> fanins;
    ltl.foreach_fanin( node, [&]( const auto& f, auto ){
        fanins.emplace_back( f );
      } );
    assert( fanins.size() == 2u );

    obligation_set const olg0 = compute_obligations( ltl, fanins[0u] );
    obligation_set const olg1 = compute_obligations( ltl, fanins[1u] );
    return cart_product( ltl, olg0, olg1 );
  }

  /* UNTIL or RELEASES */
  else if ( ltl.is_until( node ) || ltl.is_releases( node ) )
  {
    std::vector<ltl_formula_store::ltl_formula> fanins;
    ltl.foreach_fanin( node, [&]( const auto& f, auto ){
        fanins.emplace_back( f );
      } );
    assert( fanins.size() == 2u );

    return compute_obligations( ltl, fanins[1u] );
  }

  assert( false && "unsupported node type" );
  return obligation_set{};
}

} /* namespace copycat */
