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
  \file waveform.hpp
  \brief Waveform
  \author Heinz Riener
*/

#pragma once

#include <vector>
#include <map>
#include <iostream>

namespace copycat
{

class waveform
{
public:
  explicit waveform( uint32_t num_traces, uint32_t num_time_steps )
    : traces( num_traces, std::vector<bool>( num_time_steps ) )
  {
  }

  std::vector<bool> get_trace_by_index( uint32_t index ) const
  {
    return traces.at( index );
  }

  std::vector<bool> get_trace_by_name( std::string const& name ) const
  {
    return get_trace_by_index( name_to_index.at( name ) );
  }

  void print( uint32_t beg = 0, uint32_t end = 0 ) const
  {
    if ( traces.size() == 0 )
    {
      std::cout << "NO TRACES" << std::endl;
      return;
    }

    assert( end <= beg );
    if ( beg == end )
      end = traces[0u].size();

    for ( const auto& t : traces )
    {
      for ( uint32_t i = beg; i < end; ++i )
      {
        std::cout << t.at( i );
      }
      std::cout << std::endl;
    }
  }

  void set_value( uint32_t index, uint32_t time_frame, bool value )
  {
    traces[index][time_frame] = value;
  }

  bool get_value( uint32_t index, uint32_t time_frame ) const
  {
    return traces.at( index ).at( time_frame );
  }

  template<typename Fn>
  void foreach_value( Fn&& fn )
  {
    uint64_t const num_time_steps = traces.size() > 0u ? traces[0].size() : 0;
    std::vector<bool> values_in_current_time_step( traces.size() );
    for ( auto time_step = 0u; time_step < num_time_steps; ++time_step )
    {
      auto counter = 0u;
      for ( const auto& trace : traces )
        values_in_current_time_step[counter++] = trace[time_step];

      fn( values_in_current_time_step, time_step );
    }
  }

protected:
  std::map<std::string,uint32_t> name_to_index;
  std::vector<std::vector<bool>> traces;
}; /* waveform */

} /* copycat */
