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
  \file trace.hpp
  \brief Trace

  \author Heinz Riener
*/

#pragma once

#include <vector>

namespace copycat
{

struct trace
{
public:
  explicit trace() = default;

  std::vector<uint32_t> at( uint32_t index ) const
  {
    if ( index < prefix.size() )
    {
      return prefix.at( index );
    }
    else
    {
      return suffix.at( index - prefix.size() );
    }
  }

  bool has( uint32_t index, uint32_t value ) const
  {
    auto const values = at( index );
    return ( std::find( std::begin( values ), std::end( values ), value ) != std::end( values ) );
  }

  uint64_t length() const
  {
    return prefix.size() + suffix.size();
  }

  uint64_t prefix_length() const
  {
    return prefix.size();
  }

  uint64_t suffix_length() const
  {
    return suffix.size();
  }

  bool is_finite() const
  {
    return suffix.size() == 0u;
  }

public:
  std::vector<std::vector<uint32_t>> prefix;
  std::vector<std::vector<uint32_t>> suffix;
}; /* trace */

} /* namespace copycat */

