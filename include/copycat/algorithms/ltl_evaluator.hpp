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
#include <iostream>

namespace copycat
{

struct indeterminate_t {};
struct presumably_true_t {};
struct presumably_false_t {};

class bool3
{
public:
  bool3()
    : value( false_value )
  {
  }

  bool3( bool b )
    : value( b ? true_value : false_value )
  {
  }

  explicit bool3( indeterminate_t const& )
    : value( indeterminate_value )
  {
  }

  bool operator==( bool3 const& other ) const
  {
    return value == other.value;
  }

  bool operator!=( bool3 const& other ) const
  {
    return !this->operator==( other );
  }

  bool operator<( bool3 const& other ) const
  {
    return value < other.value;
  }

  bool operator>( bool3 const& other ) const
  {
    return value > other.value;
  }

  bool operator<=( bool3 const& other ) const
  {
    return !operator>( other );
  }

  bool operator>=( bool3 const& other ) const
  {
    return !operator<( other );
  }

  bool3 operator!() const
  {
    switch ( value )
    {
    default:
      std::cerr << "[e] unsupported value" << std::endl;
    case indeterminate_value:
      return bool3(indeterminate_t());
    case false_value:
      return true;
    case true_value:
      return false;
    }
  }

  bool is_false() const
  {
    return ( value == false_value );
  }

  bool is_true() const
  {
    return ( value == true_value );
  }

  bool is_indeterminate() const
  {
    return ( value == indeterminate_value );
  }

  std::string to_string() const
  {
    switch ( value )
    {
    default:
      std::cerr << "[e] unsupported value" << std::endl;
    case indeterminate_value:
      return "?";
    case false_value:
      return "0";
    case true_value:
      return "1";
    }
  }

protected:
  enum value_t {
    false_value = -1,
    indeterminate_value = 0,
    true_value = 1,
  } value;
}; /* bool3 */

inline bool3 indeterminate{indeterminate_t()};

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
      assert( !store.is_complemented( f ) );
      std::array<formula,2> subformulas;
      store.foreach_fanin( n, [&]( const auto& formula, uint32_t index ) {
          subformulas[index] = formula;
        });
      return eval_or( subformulas[0], subformulas[1], t, pos );
    }
    else if ( store.is_next( n ) )
    {
      assert( !store.is_complemented( f ) );
      return eval_next( n, t, pos );
    }
    else if ( store.is_until( n ) )
    {
      assert( !store.is_complemented( f ) );
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
    {
      return indeterminate;
    }
    return t.has( pos, n );
  }

  bool3 eval_or( formula const& a, formula const& b, trace const& t, uint32_t pos )
  {
    bool3 result_a = eval( a, t, pos );
    if ( result_a.is_true() )
      return true;

    bool3 result_b = eval( b, t, pos );
    return std::max( result_a, result_b );
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
    return std::max( result_b, std::min( result_a, result_next ) );
  }

protected:
  ltl_formula_store const& store;
}; /* ltl_evaluator */

} /* namespace copycat */
