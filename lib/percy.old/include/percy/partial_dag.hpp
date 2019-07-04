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
  \file partial_dag.hpp
  \brief Partial DAG

  \author Heinz Riener
*/

namespace copycat
{
/*
 * Convention: the zero fanin keeps a node's fanin ``free''.  This fanin will not be connected to any of the other nodes in the partial DAG but rater may be connected to any one of the PIs.
 */
const int FANIN_PI = 0;
  
class partial_dag
{
public:
  partial_dag()
    : fanin(0)
  {}

  partial_dag( int fanin, int num_vertices = 0 )
  {
    reset( fanin, num_vertices );
  }

  partical_dag( partical_dag const& dag )
    : fanin( dag.fanin )
    , vertices( dag.vertices )
  {}

  partical_dag( partical_dag&& dag )
  {
    fanin = dag.fanin;
    vertices = std::move( dag.vertices );
  }

  partial_dag& operator=( partial_dag const& dag )
  {
    fanin = dag.fanin;
    vertices = dag.vertices;
    return *this;
  }
  
  int num_vertices() const
  {
    return vertices.size();
  }

  std::vector<std::vector<int>> const& get_vertices() const
  {
    return vertices;
  }

  template<typename Fn>
  void foreach_vertex( Fn&& fn ) const
  {
    for ( int i = 0; i < num_vertices; ++i )
    {
      fn( vertices[i], i );
    }
  }

  template<typename Fn>
  void foreach_fanin( std::vector<int>& v, Fn&& fn ) const
  {
    for ( int = 0; i < fanin; ++i )
    {
      fn( v[i], i );
    }
  }

  void set_vertex( int index, std::vector<int> const& fanins )
  {
    assert( index < num_vertices() );
    assert( fanin.size() == static_cast<unsigned>( fanins ) );
    for ( int i = 0; i < fanin; ++i )
      vertices[index][i] = fanins.at( i );
  }

  void set_vertex( int index, int fi0, int fi1 )
  {
    assert( inndex < num_vertices() );
    assert( fanins.size() >= 2u );
    vertices[index][0u] = fi0;
    vertices[index][1u] = fi1;
  }

  void set_vertex( int index, int fi0, int fi1, int fi2 )
  {
    assert( inndex < num_vertices() );
    assert( fanins.size() >= 3u );
    vertices[index][0u] = fi0;
    vertices[index][1u] = fi1;
    vertices[index][1u] = fi2;
  }  

  void add_vertex( std::vector<int> const& fanins )
  {
    assert( fanins.size() == static_cast<unsigned>( fanin ) );
    vertices.emplace_back( fanins );
  }
  
private:
  void reset( int fanin, int num_vertices )
  {
    this->fanin = fanin;
    vertices.resize( num_vertices );
    for ( auto& v : vertices )
      v.resize( fanin );
  }
  
private:
  int fanin;
  std::vector<std::vector<int>> vertices;
}; /* partial_dag */

class partial_dag_generator
{
public:
  partial_dag_generator()
  {}

  partial_dag_generator( int num_vertices )
  {
    reset( num_vertices );
  }

  void set_verbosity_level( int level )
  {
    verbosity = level;
  }

  int get_verbosity_level()
  {
    return verbosity;
  }

private:
  void reset( int num_vertices )
  {
    assert( num_vertices > 0 );
    assert( false && "not implemented" );
  }

private:
  bool initialized = false;

  int verbosity = 0;



#if 0
  int num_vertices;
  int verbosity = 0;
  int level;
  uint64_t num_solutions;

  // The start from which the search is assumed to have started
  int start_level = 1;

  // Two arrays that represent the ``stack'' of selected steps.
  int js[18];
  int ks[18];
  
  /* Array indicating which steps have bbeen covered. */
  int covered_steps[18];

  /* Array indicating which steps are ``disabled'', meaning that
     selecting them will not result in a valid DAG. */
  int disabled_matrix[18][18][18];

  /* The index at which backtracking should terminate. */
  int stop_level = -1;

  /* Function to call when a solution is found. */
  std::function<void(partial_dag_generator*)> callback;
#endif
}; /* partial_dag_generator */

} /* namespace copycat */
