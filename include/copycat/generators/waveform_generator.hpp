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
  \brief Waveform generator
  \author Heinz Riener
*/

#pragma once

#include "../waveform.hpp"

namespace copycat
{

template<typename Ntk>
class waveform_generator
{
public:
  explicit waveform_generator( Ntk const& ntk, waveform& wf )
    : ntk( ntk )
    , wf( wf )
  {
  }

  void on_time_frame_start( uint32_t time_frame )
  {
    current_time_frame = time_frame;
  }

  void on_ro( uint32_t index, bool value )
  {
    wf.set_value( index + ntk.num_pis(), current_time_frame, value );
  }

  void on_pi( uint32_t index, bool value )
  {
    wf.set_value( index, current_time_frame, value );
  }

  void on_po( uint32_t index, bool value )
  {
    wf.set_value( index + ntk.num_cis(), current_time_frame, value );
  }

  void on_ri( uint32_t index, bool value )
  {
    (void)index;
    (void)value;
  }

  void on_time_frame_end( uint32_t time_frame )
  {
    (void)time_frame;
  }

protected:
  Ntk const& ntk;
  waveform& wf;
  uint32_t current_time_frame = 0;
}; /* waveform_generator */

} /* copycat */
