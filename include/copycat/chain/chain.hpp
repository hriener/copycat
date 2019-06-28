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
  \file chain.hpp
  \brief Chain

  \author Heinz Riener
*/

#pragma once

#include <vector>

namespace copycat
{

namespace detail
{

template<class Fn, class ElementType, class ReturnType>
inline constexpr bool is_callable_with_index_v = std::is_invocable_r_v<ReturnType, Fn, ElementType, uint32_t>;

template<class Fn, class ElementType, class ReturnType>
inline constexpr bool is_callable_without_index_v = std::is_invocable_r_v<ReturnType, Fn, ElementType>;

} /* detail */

/*! \brief Container to represent generalized chains.
 *
 * A chain is a straight-line program.  This container allows to
 * provide the label_type as an external template parameter.
 */
template<typename LabelType, typename StepType>
class chain
{
public:
  using label_type = LabelType;
  using step_type  = StepType;

public:
  /*! \brief Construct a chain without inputs (e.g., because the number of inputs is unknown) */
  explicit chain( uint32_t size = 0u )
    : _steps( size )
    , _labels( size )
  {
  }

  /*! \brief Construct a chain with a specified number of inputs */
  explicit chain( uint32_t num_inputs, uint32_t size = 0 )
    : _num_inputs( num_inputs )
    , _steps( size )
    , _labels( size )
  {
  }

  /*! \brief Iterate over inputs */
  template<typename Fn>
  void foreach_input( Fn&& fn ) const
  {
    for ( auto i = 0u; i < _num_inputs; ++i )
      fn( i );
  }

  /*! \brief Iterate over steps */
  template<typename Fn>
  void foreach_step( Fn&& fn ) const
  {
    static_assert( detail::is_callable_with_index_v<Fn, step_type, void> ||
                   detail::is_callable_without_index_v<Fn, step_type, void> );

    if constexpr ( detail::is_callable_without_index_v<Fn, step_type, void> )
    {
      for ( auto i = 0u; i < _steps.size(); ++i )
        fn( _steps.at( i ) );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, step_type, void> )
    {
      for ( auto i = 0u; i < _steps.size(); ++i )
        fn( _steps.at( i ), i );
    }
  }

  /*! \brief Iterate over label */
  template<typename Fn>
  void foreach_label( Fn&& fn ) const
  {
    static_assert( detail::is_callable_with_index_v<Fn, label_type, void> ||
                   detail::is_callable_without_index_v<Fn, label_type, void> );

    if constexpr ( detail::is_callable_without_index_v<Fn, label_type, void> )
    {
      for ( auto i = 0u; i < _labels.size(); ++i )
        fn( _labels.at( i ) );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, label_type, void> )
    {
      for ( auto i = 0u; i < _labels.size(); ++i )
        fn( _labels.at( i ), i );
    }
  }

  /*! \brief Returns the number of inputs */
  uint32_t num_inputs() const
  {
    return _num_inputs;
  }

  /*! \brief Returns the number of steps */
  uint32_t num_steps() const
  {
    assert( _labels.size() == _steps.size() );
    return _steps.size();
  }

  /*! \brief Returns the i-th step */
  step_type step_at( uint32_t index ) const
  {
    assert( index < _steps.size() );
    return _steps.at( index );
  }

  /*! \brief Returns the i-th label */
  label_type label_at( uint32_t index ) const
  {
    assert( index < _labels.size() );
    return _labels.at( index );
  }

  /*! \brief Set the number of inputs */
  void set_inputs( uint32_t num_inputs )
  {
    _num_inputs = num_inputs;
  }

  /*! \brief Add a step to the chain */
  int32_t add_step( label_type const& label, step_type const& step )
  {
    assert( _steps.size() == _labels.size() );

    auto const index = _steps.size();
    _labels.emplace_back( label );
    _steps.emplace_back( step );
    return _num_inputs + int32_t( index );
  }

  /*! \brief Set a step in the chain */
  void set_step( uint32_t index, label_type const& label, step_type const& step )
  {
    assert( index < _labels.size() );
    assert( index < _steps.size() );

    _labels[index] = label;
    _steps[index] = step;
  }

  /*! \brief Incomplete check to ensure correctness of the data structure */
  bool okay() const
  {
    return ( _steps.size() == _labels.size() );
  }

private:
  /*! \brief Number of inputs */
  uint32_t _num_inputs = 0u;

  /*! \brief Steps of the chain */
  std::vector<step_type> _steps;

  /*! \brief Labels of the chain */
  std::vector<label_type> _labels;

  /*! \brief Outputs of the chain */
  // std::vector<int> _outputs;
}; /* chain */

} /* copycat */
