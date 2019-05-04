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
#include "../utils/extended_inttype.hpp"
#include <iostream>

namespace copycat
{

/*! \brief Abstract class for default_ltl_evaluator. */
class default_ltl_evaluator
{
public:
  using result_type = void;

public:
  explicit default_ltl_evaluator() = delete;
}; /* default_ltl_evaluator */

/*! \brief LTL evaluator for finite traces using three-valued logic */
class ltl_finite_trace_evaluator
{
public:
  using formula = ltl_formula_store::ltl_formula;
  using node = ltl_formula_store::node;

  using result_type = bool3;

public:
  explicit ltl_finite_trace_evaluator( ltl_formula_store& ltl )
    : ltl( ltl )
  {
  }

  bool3 evaluate_formula( formula const& f, trace const& t, uint32_t pos ) const
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

protected:
  bool3 evaluate_constant( node const& n ) const
  {
    assert( ltl.is_constant( n ) && n == 0 );
    return false;
  }

  bool3 evaluate_variable( node const& n, trace const& t, uint32_t pos ) const
  {
    if ( pos >= t.length() )
      return inconclusive3;

    return t.has( pos, n );
  }

  bool3 evaluate_or( node const& n, trace const& t, uint32_t pos ) const
  {
    std::array<formula,2> subformulas;
    ltl.foreach_fanin( n, [&]( const auto& formula, uint32_t index ) {
        subformulas[index] = formula;
      });

    return evaluate_formula( subformulas[0], t, pos ) || evaluate_formula( subformulas[1], t, pos );
  }

  bool3 evaluate_next( node const& n, trace const& t, uint32_t pos ) const
  {
    if ( pos+1 >= t.length() )
      return inconclusive3;

    std::array<formula,2> subformulas;
    ltl.foreach_fanin( n, [&]( const auto& formula, uint32_t index ) {
        subformulas[index] = formula;
      });

    return evaluate_formula( subformulas[0], t, pos+1 );
  }

  bool3 evaluate_until( node const& n, trace const& t, uint32_t pos ) const
  {
    if ( pos+1 >= t.length() )
      return inconclusive3;

    std::array<formula,2> subformulas;
    ltl.foreach_fanin( n, [&]( const auto& formula, uint32_t index ) {
        subformulas[index] = formula;
      });

    return evaluate_formula( subformulas[1], t, pos ) ||
      ( evaluate_formula( subformulas[0], t, pos ) && evaluate_until( n, t, pos+1 ) );
  }

protected:
  ltl_formula_store& ltl;
}; /* evaluator */

template<class Evaluator = default_ltl_evaluator>
typename Evaluator::result_type evaluate( ltl_formula_store::ltl_formula const& f, trace const& t, Evaluator const& eval = Evaluator() )
{
  return eval.evaluate_formula( f, t, 0 );
}

} /* namespace copycat */
