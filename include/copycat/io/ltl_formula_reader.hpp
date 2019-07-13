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
  \file ltl_formula_reader.hpp
  \brief LTL formula reader

  \author Heinz Riener
*/

#pragma once

#include <copycat/io/ltl.hpp>
#include <copycat/ltl.hpp>

namespace copycat
{

class ltl_formula_reader : public ltl_reader
{
public:
  explicit ltl_formula_reader( ltl_formula_store& ltl,
                               std::map<std::string, ltl_formula_store::ltl_formula>& names )
    : _ltl( ltl )
    , _names( names )
  {
  }

  void on_proposition( uint32_t id, std::string const& name ) const override
  {
    if ( formula.size() <= id )
      formula.resize( id+1 );

    auto const it = _names.find( name );
    if ( it == _names.end() )
    {
      auto const var = _ltl.create_variable();
      formula[id] = var;
      _names.emplace( name, var );
    }
    else
    {
      formula[id] = it->second;
    }
  }

  void on_unary_op( uint32_t id, std::string const& op, uint32_t child_id ) const override
  {
    if ( formula.size() <= id )
      formula.resize( id+1 );

    if ( op == "(" )
    {
      formula[id] = formula[child_id];
    }
    else if ( op == "!" )
    {
      formula[id] = !formula[child_id];
    }
    else if ( op == "X" )
    {
      formula[id] = _ltl.create_next( formula[child_id] );
    }
    else if ( op == "F" )
    {
      formula[id] = _ltl.create_eventually( formula[child_id] );
    }
    else if ( op == "G" )
    {
      formula[id] = _ltl.create_globally( formula[child_id] );
    }
    else
    {
      assert( false );
    }
  }

  void on_binary_op( uint32_t id, std::string const& op, uint32_t child0_id, uint32_t child1_id ) const override
  {
    if ( formula.size() <= id )
      formula.resize( id+1 );

    if ( op == "->" )
    {
      formula[id] = _ltl.create_or( !formula[child0_id], formula[child1_id] );
    }
    else if ( op == "*" )
    {
      formula[id] = !_ltl.create_or( !formula[child0_id], !formula[child1_id] );
    }
    else if ( op == "+" )
    {
      formula[id] = _ltl.create_or( formula[child0_id], formula[child1_id] );
    }
    else if ( op == "U" )
    {
      formula[id] = _ltl.create_until( formula[child0_id], formula[child1_id] );
    }
    else if ( op == "R" )
    {
      formula[id] = _ltl.create_releases( formula[child0_id], formula[child1_id] );
    }
    else
    {
      assert( false );
    }
  }

  void on_formula( uint32_t id ) const override
  {
    _ltl.create_formula( formula[id] );
  }

protected:
  ltl_formula_store& _ltl;
  std::map<std::string, ltl_formula_store::ltl_formula>& _names;
  mutable std::vector<ltl_formula_store::ltl_formula> formula;
}; /* ltl_formula_reader */

} /* copycat */
