/* percy
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
  \file dag.hpp
  \brief DAG

  \author Winston Haaswijk
  \author Heinz Riener
*/

#include <array>
#include <vector>

namespace percy
{

namespace detail
{

template<class Fn, class ElementType, class ReturnType>
inline constexpr bool is_callable_with_index_v = std::is_invocable_r_v<ReturnType, Fn, ElementType, uint32_t>;

template<class Fn, class ElementType, class ReturnType>
inline constexpr bool is_callable_without_index_v = std::is_invocable_r_v<ReturnType, Fn, ElementType>;

} /* detail */

template<int FI>
class dag
{
public:
  using fanin = int;
  using vertex = std::array<fanin, FI>;

  static const int num_fanins = FI;

public:
  explicit dag() = default;

  explicit dag( int num_inputs )
  {
    reset( num_inputs, 0 );
  }

  explicit dag( int num_inputs, int num_vertices )
  {
    reset( num_inputs, num_vertices );
  }

  explicit dag( dag&& other )
    : _num_inputs( other._num_inputs )
    , _num_vertices( other._num_vertices )
    , _vertices( std::move( other._vertices ) )
  {
    /* invalidate other */
    other._num_inputs = -1;
    other._num_vertices = -1;
  }

  explicit dag( dag const& other )
  {
    copy_dag( other );
  }

  dag& operator=( dag const& other )
  {
    copy_dag( other );
    return *this;
  }

  bool operator==( dag const& other ) const
  {
    if ( other._num_vertices != _num_vertices ||
         other._num_inputs != _num_inputs )
      return false;

    for ( int i = 0; i < _num_vertices; ++i )
      for ( int j = 0; j < FI; ++j )
        if ( _vertices.at( i ).at( j )!= other._vertices.at( i ).at( j ) )
          return false;

    return true;
  }

  bool operator!=( dag const& other ) const
  {
    return !( *this == other );
  }

  /*! \brief Returns the number of inputs */
  int num_inputs() const
  {
    return _num_inputs;
  }

  /*! \brief Returns the number of vertices */
  int num_vertices() const
  {
    return _num_vertices;
  }

  /*! \brief Iterate over inputs */
  template<typename Fn>
  void foreach_input( Fn&& fn ) const
  {
    for ( int i = 0; i < _num_inputs; ++i )
      fn( i );
  }

  /*! \brief Iterate over vertices */
  template<typename Fn>
  void foreach_vertex( Fn&& fn ) const
  {
    static_assert( detail::is_callable_with_index_v<Fn, vertex, void> ||
                   detail::is_callable_without_index_v<Fn, vertex, void> );

    if constexpr ( detail::is_callable_without_index_v<Fn, vertex, void> )
    {
      for ( int i = 0; i < _num_vertices; ++i )
        fn( _vertices.at( i ) );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, vertex, void> )
    {
      for ( int i = 0; i < _num_vertices; ++i )
        fn( _vertices.at( i ), i );
    }
  }

  /*! \brief Iterate over fanins */
  template<typename Fn>
  void foreach_fanin( vertex const& v, Fn&& fn ) const
  {
    static_assert( detail::is_callable_with_index_v<Fn, fanin, void> ||
                   detail::is_callable_without_index_v<Fn, fanin, void> );

    if constexpr ( detail::is_callable_without_index_v<Fn, fanin, void> )
    {
      for ( auto i = 0; i < FI; ++i )
        fn( v.at( i ) );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, fanin, void> )
    {
      for ( auto i = 0; i < FI; ++i )
        fn( v.at( i ), i );
    }
  }

  /*! \brief Set fanins of vertex at index */
  void set_vertex( int index, std::array<fanin, FI> const& fs )
  {
    assert( index < _num_vertices && "Index out of bounds" );
    for ( auto i = 0; i < FI; ++i )
      _vertices[index][i] = fs.at( i );
  }

  /*! \brief Set fanins of vertex at index */
  void set_vertex( int index, std::vector<fanin> const& fs )
  {
    assert( index < _num_vertices && "Index out of bounds" );
    for ( auto i = 0u; i < fs.size(); ++i )
      _vertices[index][i] = fs.at( i );
  }

  /*! \brief Add vertex to DAG */
  int add_vertex( std::vector<fanin> const& fs )
  {
    assert( fs.size() >= FI && "Static limit on the number of fanins is exceeded" );

    vertex new_v;
    for ( int i = 0; i < FI; ++i )
      new_v = fs.at( i );

    _vertices.emplace_back( new_v );

    auto const id = _num_vertices;
    ++_num_vertices;

    return id;
  }

  /*! \brief Get vertex at index from DAG */
  vertex const& vertex_at( int index ) const
  {
    assert( index < _num_vertices && "Index out of bounds" );
    return _vertices.at( index );
  }

private:
  /*! \brief Reset (and resize) the DAG */
  inline void reset( int num_inputs, int num_vertices )
  {
    _num_inputs = num_inputs;
    _num_vertices = num_vertices;
    _vertices.resize( _num_vertices );

    /* invalidate all vertices */
    for ( int i = 0; i < _num_vertices; ++i )
      for ( int j = 0; j < FI; ++j )
        _vertices[i][j] = -1;
  }

  /* \brief Copies a DAG */
  inline void copy_dag( dag const& other )
  {
    reset( other._num_inputs, other._num_vertices );
    _vertices.resize( other._num_vertices );

    for ( int i = 0; i < _num_vertices; ++i )
      for ( int j = 0; j < FI; ++j )
        _vertices[i][j] = other._vertices.at( i ).at( j );
  }

protected:
  int _num_inputs;
  int _num_vertices;
  std::vector<std::array<fanin, FI>> _vertices;
}; /* dag */

template<>
class dag<2>
{
public:
  using fanin = int;
  using vertex = std::pair<int,int>;

  explicit dag() = default;

  explicit dag( int num_inputs )
  {
    reset( num_inputs, 0 );
  }

  explicit dag( int num_inputs, int num_vertices )
  {
    reset( num_inputs, num_vertices );
  }

  explicit dag( dag&& other )
    : _num_inputs( other._num_inputs )
    , _num_vertices( other._num_vertices )
    , _js( other._js )
    , _ks( other._ks )
  {
    /* invalidate other */
    other._num_inputs = -1;
    other._num_vertices = -1;
    other._js = nullptr;
    other._ks = nullptr;
  }

  explicit dag( dag const& other )
  {
    copy_dag( other );
  }

  ~dag()
  {
    if ( _js != nullptr )
      delete[] _js;
    if ( _ks != nullptr )
      delete[] _ks;
  }

  dag& operator=( dag const& other )
  {
    copy_dag( other );
    return *this;
  }

  bool operator==( dag const& other ) const
  {
    if ( _num_inputs != other._num_inputs ||
         _num_vertices != other._num_vertices )
      return false;

    for ( int i = 0; i < _num_vertices; ++i )
      if ( _js[i] != other._js[i] ||
           _ks[i] != other._ks[i] )
        return false;

    return true;
  }

  bool operator!=( dag const& other ) const
  {
    return !( *this == other );
  }

  /*! \brief Returns the number of inputs */
  int num_inputs() const
  {
    return _num_inputs;
  }

  /*! \brief Returns the number of vertices */
  int num_vertices() const
  {
    return _num_vertices;
  }

  /*! \brief Set fanins of vertex at index */
  void set_vertex( int index, fanin fi0, fanin fi1 )
  {
    assert( index < _num_vertices && "Index out of bounds" );
    _js[index] = fi0;
    _ks[index] = fi1;
  }

  /*! \brief Set fanins of vertex at index */
  void set_vertex( int index, std::array<fanin,2> const& fs )
  {
    assert( index < _num_vertices && "Index out of bounds" );
    _js[index] = fs.at( 0 );
    _ks[index] = fs.at( 1 );
  }

  /*! \brief Set fanins of vertex at index */
  void set_vertex( int index, std::vector<fanin> const& fs )
  {
    assert( index < _num_vertices && "Index out of bounds" );
    _js[index] = fs.at( 0 );
    _ks[index] = fs.at( 1 );
  }

  /*! \brief Add vertex to DAG */
  int add_vertex( fanin fi0, fanin fi1 )
  {
    auto new_js = new fanin[_num_vertices + 1u];
    auto new_ks = new fanin[_num_vertices + 1u];

    for ( int i = 0; i < _num_vertices; ++i )
    {
      new_js[i] = _js[i];
      new_ks[i] = _ks[i];
    }

    new_js[_num_vertices] = fi0;
    new_ks[_num_vertices] = fi1;

    if ( _js != nullptr )
      delete[] _js;
    _js = new_js;

    if ( _ks != nullptr )
      delete[] _ks;
    _ks = new_ks;

    auto const id = _num_vertices;
    ++_num_vertices;
    return _num_vertices;
  }

  /*! \brief Add vertex to DAG */
  int add_vertex( std::array<fanin,2> const& fs )
  {
    auto new_js = new fanin[_num_vertices + 1u];
    auto new_ks = new fanin[_num_vertices + 1u];

    for ( int i = 0; i < _num_vertices; ++i )
    {
      new_js[i] = _js[i];
      new_ks[i] = _ks[i];
    }

    new_js[_num_vertices] = fs.at( 0 );
    new_ks[_num_vertices] = fs.at( 1 );

    if ( _js != nullptr )
      delete[] _js;
    _js = new_js;

    if ( _ks != nullptr )
      delete[] _ks;
    _ks = new_ks;

    auto const id = _num_vertices;
    ++_num_vertices;
    return id;
  }

  /*! \brief Get vertex at index from DAG */
  vertex const vertex_at( int index ) const
  {
    assert( index < _num_vertices && "Index out of bounds" );
    return std::make_pair( _js[index], _ks[index] );
  }

  /*! \brief Iterate over inputs */
  template<typename Fn>
  void foreach_input( Fn&& fn ) const
  {
    for ( int i = 0; i < _num_inputs; ++i )
      fn( i );
  }

  /*! \brief Iterate over vertices */
  template<typename Fn>
  void foreach_vertex( Fn&& fn ) const
  {
    static_assert( detail::is_callable_with_index_v<Fn, vertex, void> ||
                   detail::is_callable_without_index_v<Fn, vertex, void> );

    if constexpr ( detail::is_callable_without_index_v<Fn, vertex, void> )
    {
      for ( int i = 0; i < _num_vertices; ++i )
        fn( std::make_pair( _js[i], _ks[i] ) );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, vertex, void> )
    {
      for ( int i = 0; i < _num_vertices; ++i )
        fn( std::make_pair( _js[i], _ks[i] ), i );
    }
  }

  /*! \brief Iterate over fanins */
  template<typename Fn>
  void foreach_fanin( vertex const& v, Fn&& fn ) const
  {
    static_assert( detail::is_callable_with_index_v<Fn, fanin, void> ||
                   detail::is_callable_without_index_v<Fn, fanin, void> );

    if constexpr ( detail::is_callable_without_index_v<Fn, fanin, void> )
    {
      fn( v.first );
      fn( v.second );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, fanin, void> )
    {
      fn( v.first, 0 );
      fn( v.second, 1 );
    }
  }

private:
  /*! \brief Reset (and resize) the DAG */
  void reset( int num_inputs, int num_vertices )
  {
    _num_inputs = num_inputs;
    _num_vertices = num_vertices;

    if ( _js != nullptr )
      delete[] _js;
    _js = new int[_num_vertices]();

    if ( _ks != nullptr )
      delete[] _ks;
    _ks = new int[_num_vertices]();
  }

  /* \brief Copies a DAG */
  void copy_dag( dag const& other )
  {
    reset( other._num_inputs, other._num_vertices );
    for ( int i = 0; i < _num_vertices; ++i )
    {
      _js[i] = other._js[i];
      _ks[i] = other._ks[i];
    }
  }

private:
  int _num_inputs;
  int _num_vertices;
  int *_js{nullptr};
  int *_ks{nullptr};
}; /* dag<2> */

template<typename Dag>
using fanin = typename Dag::fanin;

template<typename Dag>
using vertex = typename Dag::vertex;

using binary_dag = dag<2>;
using ternary_dag = dag<3>;

} /* namespace percy */
