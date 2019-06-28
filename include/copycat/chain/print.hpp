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
  \brief Printing functions for chains

  \author Heinz Riener
*/

#pragma once

#include "chain.hpp"

#include <fstream>
#include <iostream>
#include <fmt/format.h>

namespace copycat
{

namespace detail
{
  template<typename label_type>
  std::string label_to_string( label_type const& label );
}

template<typename L, typename S>
void write_chain( chain<L,S> const& c, std::ostream& os = std::cout )
{
  assert( c.okay() );
  for ( uint32_t i = 0; i < c.num_steps(); ++i )
  {
    auto const& step = c.step_at( i );
    if ( step.size() == 0u )
    {
      /* step has no arguments */
      os << fmt::format( "{} := {}\n", c.num_inputs() + i , detail::label_to_string( c.label_at( i ) ) );
    }
    else
    {
      /* print step with arguments */
      std::string operand_string;
      for ( uint32_t j = 0; j < step.size(); ++j )
      {
        operand_string += fmt::format( "{}", step.at( j ) );
        if ( j+1 < step.size() )
          operand_string += ',';
      }
      os << fmt::format( "{} := {}( {} )\n", c.num_inputs() + i, detail::label_to_string( c.label_at( i ) ), operand_string );
    }
  }
}

template<typename L, typename S>
void write_chain( chain<L,S> const& c, std::string const& filename )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  print_chain( c, os );
  os.close();
}

template<typename L, typename S>
void write_dot( chain<L,S> const& c, std::ostream& os = std::cout )
{
  assert( c.okay() );

  os << "graph{\n";
  os << "rankdir = BT;\n";
  c.foreach_input( [&os]( int index ) {
      auto const dot_index = index + 1;
      os << fmt::format( "x{} [shape=none,label=<x<sub>{}</sub>>];\n", dot_index, dot_index );
    });

  c.foreach_step( [&os,&c]( auto const& step, int index ){
      auto const dot_index = c.num_inputs() + index + 1u;
      os << fmt::format( "x{} [label=<x<sub>{}</sub>: {}>];\n", dot_index, dot_index, detail::label_to_string( c.label_at( index ) ) );

      for ( const auto& s : step )
      {
        auto const dot_child_index = abs( s ) + 1;
        if ( s < 0 )
        {
          os << fmt::format( "x{} -- x{} [style=dashed];\n", dot_child_index, dot_index );
        }
        else
        {
          os << fmt::format( "x{} -- x{};\n", dot_child_index, dot_index );
        }
      }
    });

  /* Group inputs on the same level */
  os << "{rank = same; ";
  for ( auto i = 0u; i < c.num_inputs(); ++i )
  {
    os << fmt::format( "x{}; ", ( i + 1 ) );
  }
  os << "}\n";

  /* Add invisible edges between PIs to enforce order */
  os << "edge[style=invisible];\n";
  for ( auto i = c.num_inputs(); i > 1; --i )
  {
    os << fmt::format( "x{} -- x{};\n", ( i - 1 ), i );
  }

  os << "}" << std::endl;
}

template<typename L, typename S>
void write_dot( chain<L,S> const& c, std::string const& filename )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_dot( c, os );
  os.close();
}

} /* copycat */
