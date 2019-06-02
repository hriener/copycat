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
  \file print.hpp
  \brief Print LTL formulas

  \author Heinz Riener
*/

#pragma once

#include "ltl.hpp"
#include <fmt/format.h>
#include <ostream>

namespace copycat
{

void print( std::ostream& os, ltl_formula_store const& ltl, ltl_formula_store::ltl_formula const& f, std::unordered_map<uint32_t, std::string> const& names )
{
  auto const node = ltl.get_node( f );

  /* constant */
  if ( ltl.is_constant( node ) )
  {
    os << ( ltl.is_complemented( f ) ? "true" : "false" );
  }

  /* literal */
  else if ( ltl.is_variable( node ) )
  {
    auto const it = names.find( node );
    std::string const name = ( it != names.end() ) ? it->second : fmt::format( "var{}", node );

    os << ( ltl.is_complemented( f ) ? "~" : "" ) << name;
  }

  /* next */
  else if ( ltl.is_next( node ) )
  {
    std::vector<ltl_formula_store::ltl_formula> fanins;
    ltl.foreach_fanin( node, [&]( const auto& f, auto ){
        fanins.emplace_back( f );
      } );
    assert( fanins.size() == 2u );

    os << ( ltl.is_complemented( f ) ? "~((" : "((" );
    os << "X(";
    print( os, ltl, fanins[0u], names );
    os << ")";
  }

  /* eventually */
  else if ( ltl.is_eventually( node ) )
  {
    std::vector<ltl_formula_store::ltl_formula> fanins;
    ltl.foreach_fanin( node, [&]( const auto& f, auto ){
        fanins.emplace_back( f );
      } );
    assert( fanins.size() == 2u );

    os << ( ltl.is_complemented( f ) ? "~((" : "((" );
    os << "F(";
    print( os, ltl, fanins[0u], names );
    os << ")";
  }
  
  /* or */
  else if ( ltl.is_or( node ) )
  {
    std::vector<ltl_formula_store::ltl_formula> fanins;
    ltl.foreach_fanin( node, [&]( const auto& f, auto ){
        fanins.emplace_back( f );
      } );
    assert( fanins.size() == 2u );

    os << ( ltl.is_complemented( f ) ? "~((" : "((" );
    print( os, ltl, fanins[0u], names );
    os << ")|(";
    print( os, ltl, fanins[1u], names );
    os << "))";
  }

  /* and */
  else if ( ltl.is_and( node ) )
  {
    std::vector<ltl_formula_store::ltl_formula> fanins;
    ltl.foreach_fanin( node, [&]( const auto& f, auto ){
        fanins.emplace_back( f );
      } );
    assert( fanins.size() == 2u );

    os << ( ltl.is_complemented( f ) ? "~((" : "((" );
    print( os, ltl, fanins[0u], names );
    os << ")&(";
    print( os, ltl, fanins[1u], names );
    os << "))";    
  }
  
  /* until */
  else if ( ltl.is_until( node ) )
  {
    std::vector<ltl_formula_store::ltl_formula> fanins;
    ltl.foreach_fanin( node, [&]( const auto& f, auto ){
        fanins.emplace_back( f );
      } );
    assert( fanins.size() == 2u );

    os << ( ltl.is_complemented( f ) ? "~((" : "((" );
    print( os, ltl, fanins[0u], names );
    os << ")U(";
    print( os, ltl, fanins[1u], names );
    os << "))";    
  }

  /* releases */
  else if ( ltl.is_releases( node ) )
  {
    std::vector<ltl_formula_store::ltl_formula> fanins;
    ltl.foreach_fanin( node, [&]( const auto& f, auto ){
        fanins.emplace_back( f );
      } );
    assert( fanins.size() == 2u );

    os << ( ltl.is_complemented( f ) ? "~((" : "((" );
    print( os, ltl, fanins[0u], names );
    os << ")R(";
    print( os, ltl, fanins[1u], names );
    os << "))";    
  }

  /* all other nodes */
  else
  {
    assert( false && "unsupported node type" );
  }
}

void print( std::ostream& os, ltl_formula_store const& ltl, ltl_formula_store::ltl_formula const& f )
{
  std::unordered_map<uint32_t, std::string> names;
  print( os, ltl, f, names );
}

} /* namespace copycat */
