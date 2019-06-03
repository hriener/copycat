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
  \file ltl_operators.hpp
  \brief LTL syntactic sugar for creation of new nodes

  \author Gianluca Martino
*/

#pragma once

#include <copycat/ltl.hpp>

namespace copycat
{

class ltl_formula_store::ltl_operators
{
public:
  explicit ltl_operators( ltl_formula_store& ltl )
    : _ltl( ltl )
  {
  }

  ltl_formula make_and( ltl_formula const& a, ltl_formula const& b )
  {
    return !_ltl.create_or( !a, !b );
  }

  ltl_formula eventually( ltl_formula const& a )
  {
    /* F(a) = (true)U(a) */
    return _ltl.create_until( _ltl.get_constant( true ), a );
  }

  ltl_formula globally( ltl_formula const& a )
  {
    /* G(a) = !F(!(a)) */
    return !eventually( !a );
  }

  ltl_formula weak_until( ltl_formula const& a, ltl_formula const& b )
  {
    /* (a)W(b) = ((a)U(b))|G(a) */
    const auto until = _ltl.create_until( a, b );
    const auto globally = _ltl.create_globally( a );
    return _ltl.create_or( until, globally );
  }

  ltl_formula releases( ltl_formula const& a, ltl_formula const& b )
  {
    /* (a)R(b) = !((!a)U(!b)) */
    return !_ltl.create_until( !a, !b );
  }

  ltl_formula_store& _ltl;
}; /* ltl_formula_store::ltl_operatorss */

}
