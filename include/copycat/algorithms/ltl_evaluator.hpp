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
  \file ltl_evaluator.hpp
  \brief LTL evaluator

  \author Heinz Riener
*/

#pragma once

#include "../ltl.hpp"
#include "../trace.hpp"
#include "../logic/bool3.hpp"
#include <iostream>

namespace copycat
{

class ltl_evaluator
{
public:
  using formula = ltl_formula_store::ltl_formula;

public:
  explicit ltl_evaluator( ltl_formula_store const& store )
    : store( store )
  {
  }

  bool3 eval( formula const& f, trace const& t, uint32_t pos = 0 )
  {
    if ( store.is_complemented( f ) )
    {
      return !( eval( !f, t, pos ) );
    }

    assert( !store.is_complemented( f ) );

    auto const n = store.get_node( f );
    if ( store.is_constant( n ) )
    {
      return eval_constant( n );
    }
    else if ( store.is_variable( n ) )
    {
      return eval_variable( n, t, pos );
    }
    else if ( store.is_or( n ) )
    {
      std::array<formula,2> subformulas;
      store.foreach_fanin( n, [&]( const auto& formula, uint32_t index ) {
          subformulas[index] = formula;
        });
      return eval_or( subformulas[0], subformulas[1], t, pos );
    }
    else if ( store.is_next( n ) )
    {
      return eval_next( n, t, pos );
    }
    else if ( store.is_until( n ) )
    {
      std::array<formula,2> subformulas;
      store.foreach_fanin( n, [&]( const auto& formula, uint32_t index ) {
          subformulas[index] = formula;
        });
      return eval_until( subformulas[0], subformulas[1], t, pos );
    }
    else
    {
      std::cerr << "[e] unknown operator" << std::endl;
      return indeterminate;
    }
  }

  bool3 eval_constant( ltl_formula_store::node const& n )
  {
    assert( store.is_constant( n ) && n == 0 );
    return false;
  }

  bool3 eval_variable( ltl_formula_store::node const& n, trace const& t, uint32_t pos )
  {
    if ( pos >= t.length() )
      return indeterminate;

    return t.has( pos, n );
  }

  bool3 eval_or( formula const& a, formula const& b, trace const& t, uint32_t pos )
  {
    bool3 result_a = eval( a, t, pos );
    if ( result_a.is_true() )
      return true;

    bool3 result_b = eval( b, t, pos );
    return ( result_a | result_b );
  }

  bool3 eval_next( ltl_formula_store::node const& n, trace const& t, uint32_t pos )
  {
    if ( pos+1 >= t.length() )
      return indeterminate;

    std::array<formula,2> subformulas;
    store.foreach_fanin( n, [&]( const auto& formula, uint32_t index ) {
        subformulas[index] = formula;
      });

    return eval( subformulas[0], t, pos+1 );
  }

  bool3 eval_until( formula const& a, formula const& b, trace const& t, uint32_t pos )
  {
    if ( pos+1 >= t.length() )
      return indeterminate;

    bool3 const result_b = eval( b, t, pos );
    if ( result_b.is_true() )
      return true;

    bool3 const result_a = eval( a, t, pos );
    if ( result_a.is_false() )
      return false;

    bool3 const result_next = eval_until( a, b, t, pos+1 );
    return ( result_b | ( result_a & result_next ) );
  }

protected:
  ltl_formula_store const& store;
}; /* ltl_evaluator */

} /* namespace copycat */
