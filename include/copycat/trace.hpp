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

  void emplace_prefix( std::vector<int> const& prop )
  {
    assert( _suffix_length == 0u );
    _data.emplace_back( prop );
    ++_prefix_length;
  }

  void emplace_suffix( std::vector<int> const& prop )
  {
    _data.emplace_back( prop );
    ++_suffix_length;
  }

  bool is_true( uint32_t time_index, int32_t prop_index ) const
  {
    assert( time_index < _data.size() );
    auto const& data = _data.at( time_index );
    return std::find( std::begin( data ), std::end( data ), prop_index ) != std::end( data );
  }

  std::vector<int32_t> at( uint32_t index ) const
  {
    assert( index < _data.size() );
    return _data.at( index );
  }

  bool has( uint32_t index, int32_t value ) const
  {
    assert( index < _data.size() );
    auto const values = at( index );
    return ( std::find( std::begin( values ), std::end( values ), value ) != std::end( values ) );
  }

  uint64_t length() const
  {
    assert( _prefix_length + _suffix_length == _data.size() );
    return _data.size();
  }

  uint64_t prefix_length() const
  {
    return _prefix_length;
  }

  uint64_t suffix_length() const
  {
    return _suffix_length;
  }

  bool is_finite() const
  {
    return _suffix_length == 0u;
  }

public:
  uint32_t _prefix_length = 0u;
  uint32_t _suffix_length = 0u;
  std::vector<std::vector<int32_t>> _data;
}; /* trace */

} /* namespace copycat */
