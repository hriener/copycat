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
  \file string_utils.hpp
  \brief Utility functions for string manipulation

  \author Heinz Riener
*/

#pragma once

#include <set>
#include <string>
#include <vector>

namespace copycat
{

/*! \brief Split string by delimiter */
std::vector<std::string> split_path( std::string const& s, std::set<char> const& delimiters )
{
  std::vector<std::string> result;

  char const* pch = s.c_str();
  char const* start = pch;
  for( ; *pch; ++pch )
  {
    if ( delimiters.find( *pch ) == delimiters.end() )
      continue;

    if ( start != pch )
    {
      std::string token( start, pch );
      result.emplace_back( token );
    }
    else
    {
      result.emplace_back( "" );
    }

    start = pch + 1;
  }
  result.emplace_back( start );
  return result;
}

} /* namespace copycat */
