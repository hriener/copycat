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
  \file sequential_simulation.hpp
  \brief Sequential simulation
  \author Heinz Riener
*/

#pragma once

#include <mockturtle/algorithms/simulation.hpp>
#include <inttypes.h>
#include <cassert>
#include <vector>

namespace copycat
{

template<typename Ntk, typename RandomGenerator>
class random_simulator
{
public:
  explicit random_simulator( Ntk& ntk, RandomGenerator& gen )
    : ntk( ntk )
    , gen( gen )
  {
  }

  bool initialize_pi( uint32_t index )
  {
    (void)index;
    return gen();
  }

  bool initialize_ro( uint32_t index )
  {
    (void)index;
    return 0u;
  }

  bool compute_pi( uint32_t index, uint32_t time_frame )
  {
    (void)index;
    assert( time_frame > 0u );
    return gen();
  }

protected:
  Ntk const& ntk;
  RandomGenerator& gen;
}; /* random_simulator */

template<typename Ntk>
class stimuli_simulator
{
public:
  explicit stimuli_simulator( Ntk const& ntk, std::vector<std::vector<bool>> const& stimuli )
    : ntk( ntk )
    , stimuli( stimuli )
  {}

  bool initialize_pi( uint32_t index )
  {
    return stimuli.at( 0 ).at( index );
  }

  bool initialize_ro( uint32_t index )
  {
    (void)index;
    return false;
  }

  bool compute_pi( uint32_t index, uint32_t time_frame )
  {
    assert( time_frame > 0u );
    return stimuli.at( time_frame ).at( index );
  }

protected:
  Ntk const& ntk;
  std::vector<std::vector<bool>> const& stimuli;
}; /* stimuli_simulator */

template<typename Ntk, typename Simulator, typename Callback>
void simulate( Ntk const& ntk, Simulator& sim, uint32_t num_time_steps, Callback& callback )
{
  using namespace mockturtle;

  /* initialize simulator */
  std::vector<bool> assignments( ntk.num_cis() );
  for ( auto i = 0u; i < ntk.num_pis(); ++i )
    assignments[i] = sim.initialize_pi( i );
  for ( auto i = ntk.num_pis(); i < ntk.num_cis(); ++i )
    assignments[i] = sim.initialize_ro( i );

  using combinational_simulator_t = default_simulator<bool>;
  combinational_simulator_t comb_sim( assignments );

  uint32_t index;
  for ( auto k = 0u; k < num_time_steps; ++k )
  {
    callback.on_time_frame_start( k );

    /* simulate nodes */
    auto const v = simulate_nodes<bool, Ntk, combinational_simulator_t>( ntk, comb_sim );

    /* invoke callback */
    ntk.foreach_ro( [&]( const auto& node, auto index ){
        callback.on_ro( index, v[node] );
      });
    ntk.foreach_pi( [&]( const auto& node, auto index ){
        callback.on_pi( index, v[node] );
      });
    ntk.foreach_po( [&]( const auto& f, auto index ){
        bool const value = ntk.is_complemented( f ) ? !v[ ntk.get_node( f )] : v[ ntk.get_node( f )];
        callback.on_po( index, value );
      });
    ntk.foreach_ri( [&]( const auto& f, auto index ){
        bool const value = ntk.is_complemented( f ) ? !v[ ntk.get_node( f )] : v[ ntk.get_node( f )];
        callback.on_ri( index, value );
      });

    /* prepare inputs for next iteration */
    index = 0;
    ntk.foreach_pi( [&]( const auto& node ){
        (void)node;
        assignments[index] = sim.compute_pi( index, k );
        index++;
      });
    ntk.foreach_ri( [&]( const auto& f ){
        bool const value = v[ ntk.get_node( f )];
        assignments[index] = ntk.is_complemented( f ) ? !value : value;
        index++;
      });

    callback.on_time_frame_end( k );
  }
}

} /* copycat */
