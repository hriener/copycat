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
  \file exact_ltl_pdag_encoder.hpp
  \brief An encoder for Exact Synthesis of LTL formulas from traces
         that uses partial DAGs to guide the search

  \author Heinz Riener
*/

#pragma once

#include <copycat/trace.hpp>
#include <copycat/algorithms/exact_ltl_traits.hpp>
#include <percy/partial_dag.hpp>
#include <copycat/chain/chain.hpp>
#include <bill/sat/types.hpp>
#include <fmt/format.h>
#include <unordered_map>
#include <vector>

namespace copycat
{

template<uint32_t step_size>
class partial_dag_generator;

template<>
class partial_dag_generator<2u>
{
public:
  explicit partial_dag_generator( uint32_t num_vertices )
  {
    reset( num_vertices );
  }

  void reset( uint32_t num_vertices )
  {
    assert( num_vertices > 0u );

    _num_vertices = num_vertices;

    std::fill( std::begin( _covered_steps ), std::end( _covered_steps ), 0 );

    for ( uint32_t i = 0u; i < 18; ++i )
      for ( uint32_t j = 0u; j < 18; ++j )
        for ( uint32_t k = 0u; k < 18; ++k )
          _disabled_matrix[i][j][k] = 0u;

    /* first vertex can only point to PIs */
    _as[0u] = _bs[0u] = 0u;

    _num_solutions = 0u;
    _level = 0u;
    _stop_level = -1;
  }


  /*! \brief Returns number of vertices */
  uint32_t num_vertices() const
  {
    return _num_vertices;
  }

  /*! \brief Set callback function */
  void set_callback( std::function<void(partial_dag_generator<2u>*)> const &f )
  {
    _callback = f;
  }

  /*! \brief Set callback function */
  void set_callback(std::function<void(partial_dag_generator<2u>*)>&& f)
  {
    _callback = std::move( f );
  }

  /*! \brief clear callback function */
  void clear_callback()
  {
    _callback = 0;
  }

  auto count_dags_noreapply()
  {
    _num_solutions = 0;
    _level = 1;
    _stop_level = -1;

    search_noreapply_dags();

    return _num_solutions;
  }

  void noreapply_backtrack()
  {
    --_level;
    auto const a = _as[_level];
    auto const b = _bs[_level];
    if ( int32_t( _level ) > _stop_level )
    {
      --_covered_steps[a];
      --_covered_steps[b];

      for ( uint32_t ip = _level + 1u; ip < _num_vertices; ++ip )
      {
        --_disabled_matrix[ip][a][_level];
        --_disabled_matrix[ip][b][_level];
      }
    }
  }

  void search_noreapply_dags()
  {
    if ( _level == _num_vertices )
    {
      for ( uint32_t i = 1u; i < _num_vertices - 1u; ++i )
      {
        if ( _covered_steps[i] == 0 )
        {
          /* There is some uncovered internal step, so the graph
             cannot be connected. */
          noreapply_backtrack();
          return;
        }
      }

      ++_num_solutions;

      if ( _callback )
        _callback( this );

      noreapply_backtrack();
    }
    else
    {
      /* Check if this step can have pure PI fanins */
      _as[_level] = 0u;
      _bs[_level] = 0u;
      ++_level;
      search_noreapply_dags();

      /* We are only interested in DAGs that are in co-lexicographical
         order.  Look at the previous step on the stack.  The current
         step should be greater or equal to it. */
      uint32_t const start_a = _as[_level - 1u];
      uint32_t start_b = _bs[_level - 1u];

      /* Previous input has two PI inputs */
      if ( start_a == start_b )
        ++start_b;

      _bs[_level] = start_b;
      for ( uint32_t a = start_a; a < start_b; ++a )
      {
        if ( _disabled_matrix[_level][a][start_b])
          continue;

        /* If we choose fanin (a,b) record that a and b are
           covered. */
        ++_covered_steps[a];
        ++_covered_steps[start_b];

        /* We are adding step (i, a, b).  This means that we don't
           have to consider steps (i', a, i) or (i', b, i) for i < i' <=
           n+r.  This avoids reapplying an operand. */
        for ( uint32_t ip = _level + 1; ip < _num_vertices; ++ip )
        {
          ++_disabled_matrix[ip][a][_level];
          ++_disabled_matrix[ip][start_b][_level];
        }

        _as[_level] = a;
        ++_level;
        search_noreapply_dags();
      }
      for ( uint32_t b = start_b + 1u; b <= _level; ++b )
      {
        for ( uint32_t a = 0u; a < b; ++a )
        {
          if ( _disabled_matrix[_level][a][b] )
            continue;

          ++_covered_steps[a];
          ++_covered_steps[b];

          for ( uint32_t ip = _level + 1u; ip < _num_vertices; ++ip )
          {
            ++_disabled_matrix[ip][a][_level];
            ++_disabled_matrix[ip][b][_level];
          }

          _as[_level] = a;
          _bs[_level] = b;
          ++_level;

          search_noreapply_dags();
        }
      }

      noreapply_backtrack();
    }
  }

  auto count_dags()
  {
    return count_dags_noreapply();
  }

private:
  uint32_t _num_vertices = 0u;
  uint32_t _num_solutions = 0u;
  uint32_t _level = 0u;
  int32_t  _stop_level = -1;

  /* Array indicating which steps have been covered (and how many times.) */
  std::array<uint32_t, 18> _covered_steps;

  /* Array indicating which steps are ``disabled'', meaning that
     selecting them will not result in a valid DAG. */
  std::array<std::array<std::array<uint32_t, 18>, 18>, 18> _disabled_matrix;

  /* Function called when a solution is found */
  std::function<void(partial_dag_generator<2u>*)> _callback;

public:
  std::array<int32_t, 18> _as, _bs;
}; /* partial_dag_generator */

inline std::vector<percy::partial_dag> pd_generate( uint32_t num_vertices )
{
  partial_dag_generator<2u> gen( num_vertices );
  percy::partial_dag g;
  std::vector<percy::partial_dag> dags;

  gen.set_callback([&g,&dags](partial_dag_generator<2u>* gen){
      for ( uint32_t i = 0u; i < gen->num_vertices(); ++i )
        g.set_vertex( i, gen->_bs[i], gen->_as[i] );

      dags.emplace_back( g );
    });

  g.reset( 2u, num_vertices );
  gen.reset( num_vertices );
  gen.count_dags();

  return dags;
}

inline std::vector<percy::partial_dag> pd_generate_filtered( uint32_t num_vertices, uint32_t num_pis )
{
  partial_dag_generator<2u> gen( num_vertices );
  percy::partial_dag g;
  std::vector<percy::partial_dag> dags;

  gen.set_callback([&g,&dags,&num_pis](partial_dag_generator<2u>* gen){
      for ( uint32_t i = 0u; i < gen->num_vertices(); ++i )
        g.set_vertex( i, gen->_bs[i], gen->_as[i] );

      if ( uint32_t( g.nr_pi_fanins() ) >= num_pis )
        dags.emplace_back( g );
    });

  g.reset( 2u, num_vertices );
  gen.reset( num_vertices );
  gen.count_dags();

  return dags;
}

} /* copycat */

namespace copycat
{

struct exact_ltl_pdag_encoder_parameter
{
  /* partial dag */
  percy::partial_dag pd;

  /* number of atomic propositions */
  uint32_t num_propositions = 0u;

  /* allowed operators */
  std::vector<operator_opcode> ops;

  /* traces */
  std::vector<std::pair<trace, bool>> traces;

  /* be verbose? */
  bool verbose = true;
}; /* exact_ltl_pdag_encoder_paramter */

template<typename Solver>
class exact_ltl_pdag_encoder
{
public:
  struct lit_vector_hash
  {
    std::size_t operator()( std::vector<bill::lit_type> const& vs ) const
    {
      uint64_t seed = 0u;
      for ( auto const& v : vs )
      {
        detail::hash_combine( seed, uint32_t( v ) );
      }
      return seed;
    }
  }; /* compute_table_hash */

  struct lit_array2_hash
  {
    std::size_t operator()( std::array<bill::lit_type, 2u> const& vs ) const
    {
      uint64_t seed = 0u;
      detail::hash_combine( seed, uint32_t( vs.at( 0u ) ) );
      detail::hash_combine( seed, uint32_t( vs.at( 1u ) ) );
      return seed;
    }
  }; /* compute_table_hash */

public:
  explicit exact_ltl_pdag_encoder( Solver& solver )
    : _solver( solver )
  {
  }

  void encode( exact_ltl_pdag_encoder_parameter const& ps )
  {
    /* update internal parameters */
    _ps = ps;
    _num_nodes = ps.pd.nr_vertices();
    _num_vertices = _ps.pd.nr_pi_fanins() + _num_nodes;

    if ( _ps.verbose )
      std::cout << "[i] exact_ltl_pdag_encoder::encoder" << std::endl;

    /* some input checks */
    assert( uint32_t( _ps.pd.nr_vertices() ) > 0u && "PD parameter required" );
    assert( _ps.num_propositions > 0u && "No atomic propositions specified" );

    // print_partial_dag(); /* debug */
    allocate_variables();
    // print_variables(); /* debug */
    create_clauses();
  }

  chain<std::string, std::vector<int>> extract_chain()
  {
    /* get model */
    auto const model = _solver.get_model().model();

    /* chain */
    chain<std::string, std::vector<int>> c;

    std::unordered_map<uint32_t, uint32_t> pi_to_step;
    std::unordered_map<uint32_t, uint32_t> node_to_step;

    for ( auto i = 0; i < _ps.pd.nr_vertices(); ++i )
    {
      auto const pd_vertex = _ps.pd.get_vertex( i );

      /* map pd_vertex to vertex id in synthesis problem */
      auto const vertex_index = i + _ps.pd.nr_pi_fanins();

      /* get the label for this vertex_index */
      std::string s = "";
      for ( auto label_index = 0u; label_index < num_labels( vertex_index ); ++label_index )
      {
        assert( get_vertex_type( vertex_index ) == vertex_type::mixed || get_vertex_type( vertex_index ) == vertex_type::binary );               
        if ( model.at( label( vertex_index, label_index ).variable() ) == bill::lbool_type::true_ )
        {
          if ( get_vertex_type( vertex_index ) == vertex_type::mixed )
          {
            s = operator_opcode_to_string( mixed_operators.at( label_index ) );
            break;
          }
          else if ( get_vertex_type( vertex_index ) == vertex_type::binary )
          {
            s = operator_opcode_to_string( binary_operators.at( label_index ) );
            break;
          }
        }
      }

      std::vector<int> children;

      /* left child */
      if ( pd_vertex[0u] == 0u )
      {
        /* map pi to vertex id in synthesis problem */
        auto const vertex_index = zeroes[i];

        /* get the label for this pi */
        auto pi_index = 0u;
        for ( auto label_index = 0u; label_index < num_labels( vertex_index ); ++label_index )
        {
          if ( model.at( label( vertex_index, label_index ).variable() ) == bill::lbool_type::true_ )
          {
            pi_index = label_index;
            break;
          }
        }

        /* pi_index has already a step then use it */
        auto const it = pi_to_step.find( pi_index );
        if ( it != pi_to_step.end() )
        {
          children.emplace_back( it->second );
        }
        else
        {
          /* otherwise create a new step for the pi */
          auto const step_index = c.add_step( fmt::format( "x{}", pi_index ), {} );
          children.emplace_back( step_index );
          pi_to_step.emplace( pi_index, step_index );
        }
      }
      else
      {
        assert( node_to_step.find( pd_vertex[0u] - 1u ) != node_to_step.end() );
        children.emplace_back( node_to_step.at( pd_vertex[0u] - 1u ) );
      }

      /* right child */
      if ( s == "|" || s == "&" || s == "U" || s == "->" )
      {
        if ( pd_vertex[1u] == 0u )
        {
          /* map pi to vertex id in synthesis problem */
          auto const vertex_index = zeroes[i] + ( pd_vertex[0u] == 0 );

          /* get the label for this pi */
          auto pi_index = 0u;
          for ( auto label_index = 0u; label_index < num_labels( vertex_index ); ++label_index )
          {
            if ( model.at( label( vertex_index, label_index ).variable() ) == bill::lbool_type::true_ )
            {
              pi_index = label_index;
              break;
            }
          }

          /* pi_index has already a step then use it */
          auto const it = pi_to_step.find( pi_index );
          if ( it != pi_to_step.end() )
          {
            children.emplace_back( it->second );
          }
          else
          {
            /* otherwise create a new step for the pi */
            auto const step_index = c.add_step( fmt::format( "x{}", pi_index ), {} );
            children.emplace_back( step_index );
            pi_to_step.emplace( pi_index, step_index );
          }
        }
        else
        {
          assert( node_to_step.find( pd_vertex[1u] - 1u ) != node_to_step.end() );
          children.emplace_back( node_to_step.at( pd_vertex[1u] - 1u ) );
        }
      }

      auto const step_index = c.add_step( s, children );
      node_to_step.emplace( i, step_index );
    }
    return c;
  }

private:
  /*! \brief Allocate variables */
  void allocate_variables()
  {
    zeroes.clear();

    zeroes.resize( _ps.pd.nr_vertices() );
    auto counter = 0u;
    for ( auto i = 0u; i < uint32_t( _ps.pd.nr_vertices() ); ++i )
    {
      zeroes[i] = counter;
      for ( const auto& v : _ps.pd.get_vertex( i ) )
        if ( v == 0u )
          ++counter;
    }

    /* Count number of mixed and binary operators.  A mixed operator
     * could be either an unary or binary operator.
     */
    mixed_operators.clear();
    binary_operators.clear();

    for ( auto i = 0u; i < _ps.ops.size(); ++i )
    {
      auto const arity = operator_opcode_arity( _ps.ops.at( i ) );
      if ( arity == 1u )
      {
        mixed_operators.emplace_back( _ps.ops.at( i ) );
      }
      else if ( arity == 2u )
      {
        binary_operators.emplace_back( _ps.ops.at( i ) );
        mixed_operators.emplace_back( _ps.ops.at( i ) );
      }
    }

    /* Compute label offsets for each node */
    label_offset.resize( _num_vertices + 1u );

    uint32_t offset = 0u;
    for ( uint32_t vertex_index = 0u; vertex_index < _num_vertices; ++vertex_index )
    {
      label_offset[vertex_index] = offset;
      offset += num_labels( vertex_index );
    }
    label_offset[_num_vertices] = offset;

    /* Compute trace offsets for each trace */
    trace_offset.resize( _ps.traces.size() + 1u );
    trace_vars_begin = label_offset[label_offset.size() - 1u];
    offset = 0u;
    for ( uint32_t trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
    {
      trace_offset[trace_index] = offset;
      offset += _ps.traces.at( trace_index ).first.length();
    }
    trace_offset[_ps.traces.size()] = offset;

    tseytin_vars_begin = trace( _num_vertices-1u, _ps.traces.size()-1u, _ps.traces.at( _ps.traces.size()-1u ).first.length()-1u ).variable() + 1u;
    std::cout << "tseytin_vars_begin = " << tseytin_vars_begin << std::endl;

    /* pre-allocate variables in solver */
    _solver.add_variables( tseytin_vars_begin );
  }

  /*! \brief Print variable layout (for debugging purpose only) */
  void print_variables() const
  {
    /* label variables */
    for ( uint32_t vertex_index = 0u; vertex_index < _num_vertices; ++vertex_index )
    {
      std::cout << fmt::format( "vertex #{}\n", vertex_index );
      for ( uint32_t label_index = 0u; label_index < num_labels( vertex_index ); ++label_index )
      {
        std::cout << fmt::format( "label({},{}) = {}\n", vertex_index, label_index, label( vertex_index, label_index ).variable() );        
      }
    }

    /* trace variables */
    for ( uint32_t vertex_index = 0u; vertex_index < _num_vertices; ++vertex_index )
    {
      std::cout << fmt::format( "vertex #{}\n", vertex_index );
      for ( uint32_t trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( uint32_t time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length(); ++time_index )
        {
          std::cout << fmt::format( "trace({},{},{}) = {}\n",
                                    vertex_index, trace_index, time_index,
                                    trace( vertex_index, trace_index, time_index ).variable() );        
        }
      }
    }
  }

  std::vector<uint32_t> positions_between( uint32_t time_index, uint32_t another_time_index, uint32_t prefix_length, uint32_t trace_length )
  {
    std::vector<uint32_t> pos;
    if ( time_index < another_time_index )
    {
      for ( auto i = time_index; i < another_time_index; ++i )
      {
        pos.emplace_back( i );
      }
    }
    else if ( time_index == another_time_index )
    {
      return {};
    }
    else
    {
      for ( auto i = prefix_length; i < another_time_index; ++i )
      {
        pos.emplace_back( i );
      }
      for ( auto i = time_index; i < trace_length; ++i )
      {
        pos.emplace_back( i );
      }
    }

    return pos;
  }

  /*! \brief Create clauses */
  void create_clauses()
  {
    /* each node has to be labeled with exactly one label */
    for ( uint32_t vertex_index = 0u; vertex_index < _num_vertices; ++vertex_index )
    {
      std::vector<bill::lit_type> cl;
      for ( uint32_t label_index = 0u; label_index < num_labels( vertex_index ); ++label_index )
        cl.emplace_back( label( vertex_index, label_index ) );
      add_clause( cl );
    }

    for ( uint32_t vertex_index = 0u; vertex_index < _num_vertices; ++vertex_index )
    {
      for ( auto label_index1 = 0u; label_index1 < num_labels( vertex_index ); ++label_index1 )
      {
        for ( auto label_index2 = label_index1 + 1u; label_index2 < num_labels( vertex_index ); ++label_index2 )
        {
          std::vector<bill::lit_type> cl;
          cl.emplace_back( ~( label( vertex_index, label_index1 ) ) );
          cl.emplace_back( ~( label( vertex_index, label_index2 ) ) );
          add_clause( cl );
        }
      }
    }

    /* propositions */
    for ( auto trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
    {
      for ( auto vertex_index = 0u; vertex_index < uint32_t(  _ps.pd.nr_pi_fanins() ); ++vertex_index )
      {
        for ( auto prop_index = 0u; prop_index < _ps.num_propositions; ++prop_index )
        {
          std::vector<bill::lit_type> cb;
          for ( auto time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length(); ++time_index )
          {
            if ( _ps.traces.at( trace_index ).first.is_true( time_index, prop_index + 1u ) )
            {
              cb.emplace_back(  trace( vertex_index, trace_index, time_index ) );
            }
            else
            {
              cb.emplace_back( ~trace( vertex_index, trace_index, time_index ) );
            }
          }

          if ( cb.size() == 1u )
          {
            add_clause( { ~label( vertex_index, prop_index ), cb.at( 0u ) } );              
          }
          else
          {
            auto const t = add_tseytin_and( cb );
            add_clause( { ~label( vertex_index, prop_index ), t } );
          }
        }
      }
    }

    /* Boolean operators: NOT, AND, OR, IMPLIES */
    if ( std::find( std::begin( _ps.ops ), std::end( _ps.ops ), operator_opcode::not_ ) != std::end( _ps.ops ) )
    {
      for ( uint32_t trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( uint32_t vertex_index = uint32_t( _ps.pd.nr_pi_fanins() ); vertex_index < _num_vertices; ++vertex_index )
        {
          if ( get_vertex_type( vertex_index ) != vertex_type::mixed ) continue;

          /* find label index */
          auto label_index = 0;
          for ( const auto& m : mixed_operators )
          {
            if ( m == operator_opcode::not_ )
              break;
            else
              label_index++;
          }

          auto const left_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[0u];
          auto const child_index =
            left_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) : uint32_t( _ps.pd.nr_pi_fanins() ) + left_dag_index - 1u;
          assert( child_index < vertex_index );

          auto const t = label( vertex_index, label_index );

          std::vector<bill::lit_type> cb;
          for ( auto time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length(); ++time_index )
          {
            auto const t_eq = add_tseytin_equals( trace( vertex_index, trace_index, time_index ), ~trace( child_index, trace_index, time_index ) );
            cb.emplace_back( t_eq );
          }
          auto const t_and = add_tseytin_and( cb );
          add_clause( { ~t, t_and } );
        }
      }
    }

    if ( std::find( std::begin( _ps.ops ), std::end( _ps.ops ), operator_opcode::or_ ) != std::end( _ps.ops ) )
    {
      for ( uint32_t trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( uint32_t vertex_index = uint32_t( _ps.pd.nr_pi_fanins() ); vertex_index < _num_vertices; ++vertex_index )
        {
          if ( get_vertex_type( vertex_index ) != vertex_type::mixed && get_vertex_type( vertex_index ) != vertex_type::binary )
            continue;

          auto label_index = 0;

          /* find label index */
          if ( get_vertex_type( vertex_index ) == vertex_type::mixed )
          {
            for ( const auto& m : mixed_operators )
            {
              if ( m == operator_opcode::or_ )
                break;
              else
                label_index++;
            }
          }
          else if ( get_vertex_type( vertex_index ) == vertex_type::binary )
          {
            for ( const auto& m : binary_operators )
            {
              if ( m == operator_opcode::or_ )
                break;
              else
                label_index++;
            }
          }

          auto const left_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[0u];
          auto const right_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[1u];

          auto const child_index0 =
            left_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) : uint32_t( _ps.pd.nr_pi_fanins() ) + left_dag_index - 1u;
          auto const child_index1 =
            right_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) + ( left_dag_index == 0u ? 1u : 0u ) : uint32_t( _ps.pd.nr_pi_fanins() ) + right_dag_index - 1u;
          assert( vertex_index > child_index0 );
          assert( vertex_index > child_index1 );

          auto const t = label( vertex_index, label_index );

          std::vector<bill::lit_type> equals;
          for ( auto time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length(); ++time_index )
          {
            auto const t_or = add_tseytin_or( trace( child_index0, trace_index, time_index ), trace( child_index1, trace_index, time_index ) );
            auto const t_eq = add_tseytin_equals( trace( vertex_index, trace_index, time_index ), t_or );
            equals.emplace_back( t_eq );
          }
          auto const t_and = add_tseytin_and( equals );
          add_clause( { ~t, t_and } );
        }
      }
    }

    if ( std::find( std::begin( _ps.ops ), std::end( _ps.ops ), operator_opcode::and_ ) != std::end( _ps.ops ) )
    {
      for ( uint32_t trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( uint32_t vertex_index = uint32_t( _ps.pd.nr_pi_fanins() ); vertex_index < _num_vertices; ++vertex_index )
        {
          if ( get_vertex_type( vertex_index ) != vertex_type::mixed && get_vertex_type( vertex_index ) != vertex_type::binary )
            continue;

          auto label_index = 0;

          /* find label index */
          if ( get_vertex_type( vertex_index ) == vertex_type::mixed )
          {
            for ( const auto& m : mixed_operators )
            {
              if ( m == operator_opcode::and_ )
                break;
              else
                label_index++;
            }
          }
          else if ( get_vertex_type( vertex_index ) == vertex_type::binary )
          {
            for ( const auto& m : binary_operators )
            {
              if ( m == operator_opcode::and_ )
                break;
              else
                label_index++;
            }
          }

          auto const left_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[0u];
          auto const right_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[1u];

          auto const child_index0 =
            left_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) : uint32_t( _ps.pd.nr_pi_fanins() ) + left_dag_index - 1u;
          auto const child_index1 =
            right_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) + ( left_dag_index == 0u ? 1u : 0u ) : uint32_t( _ps.pd.nr_pi_fanins() ) + right_dag_index - 1u;
          assert( vertex_index > child_index0 );
          assert( vertex_index > child_index1 );

          auto const t = label( vertex_index, label_index );

          std::vector<bill::lit_type> equals;
          for ( auto time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length(); ++time_index )
          {
            auto const t_and = add_tseytin_and( trace( child_index0, trace_index, time_index ), trace( child_index1, trace_index, time_index ) );
            auto const t_eq = add_tseytin_equals( trace( vertex_index, trace_index, time_index ), t_and );
            equals.emplace_back( t_eq );
          }
          auto const t_and = add_tseytin_and( equals );
          add_clause( { ~t, t_and } );
        }
      }
    }

    if ( std::find( std::begin( _ps.ops ), std::end( _ps.ops ), operator_opcode::implies_ ) != std::end( _ps.ops ) )
    {
      for ( uint32_t trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( uint32_t vertex_index = uint32_t( _ps.pd.nr_pi_fanins() ); vertex_index < _num_vertices; ++vertex_index )
        {
          if ( get_vertex_type( vertex_index ) != vertex_type::mixed && get_vertex_type( vertex_index ) != vertex_type::binary )
            continue;

          auto label_index = 0;

          /* find label index */
          if ( get_vertex_type( vertex_index ) == vertex_type::mixed )
          {
            for ( const auto& m : mixed_operators )
            {
              if ( m == operator_opcode::implies_ )
                break;
              else
                label_index++;
            }
          }
          else if ( get_vertex_type( vertex_index ) == vertex_type::binary )
          {
            for ( const auto& m : binary_operators )
            {
              if ( m == operator_opcode::implies_ )
                break;
              else
                label_index++;
            }
          }

          auto const left_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[0u];
          auto const right_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[1u];

          auto const child_index0 =
            left_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) : uint32_t( _ps.pd.nr_pi_fanins() ) + left_dag_index - 1u;
          auto const child_index1 =
            right_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) + ( left_dag_index == 0u ? 1u : 0u ) : uint32_t( _ps.pd.nr_pi_fanins() ) + right_dag_index - 1u;
          assert( vertex_index > child_index0 );
          assert( vertex_index > child_index1 );

          auto const t = label( vertex_index, label_index );

          std::vector<bill::lit_type> equals;
          for ( auto time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length(); ++time_index )
          {
            auto const t_implies = add_tseytin_or( ~trace( child_index0, trace_index, time_index ), trace( child_index1, trace_index, time_index ) );
            auto const t_eq = add_tseytin_equals( trace( vertex_index, trace_index, time_index ), t_implies );
            equals.emplace_back( t_eq );
          }
          auto const t_and = add_tseytin_and( equals );
          add_clause( { ~t, t_and } );
        }
      }
    }

    /* temporal operators: X, U, G, F */
    if ( std::find( std::begin( _ps.ops ), std::end( _ps.ops ), operator_opcode::next_ ) != std::end( _ps.ops ) )
    {
      for ( uint32_t trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( uint32_t vertex_index = uint32_t( _ps.pd.nr_pi_fanins() ); vertex_index < _num_vertices; ++vertex_index )
        {
          if ( get_vertex_type( vertex_index ) != vertex_type::mixed ) continue;

          /* find label index */
          auto label_index = 0;
          for ( const auto& m : mixed_operators )
          {
            if ( m == operator_opcode::next_ )
              break;
            else
              label_index++;
          }

          auto const left_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[0u];
          auto const child_index =
            left_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) : uint32_t( _ps.pd.nr_pi_fanins() ) + left_dag_index - 1u;
          assert( child_index < vertex_index );

          auto const t = label( vertex_index, label_index );

          std::vector<bill::lit_type> equals;
          auto const trace_length = _ps.traces.at( trace_index ).first.length();
          for ( auto time_index = 0u; time_index < trace_length - 1u; ++time_index )
          {
            auto const t_eq = add_tseytin_equals( trace( vertex_index, trace_index, time_index ), trace( child_index, trace_index, time_index + 1u ) );
            equals.emplace_back( t_eq );
          }

          auto const prefix_length = _ps.traces.at( trace_index ).first.prefix_length();
          auto const t_eq = add_tseytin_equals(
              trace( vertex_index, trace_index, trace_length - 1u ),
              prefix_length > 0u ? trace( child_index, trace_index, prefix_length ) : trace( child_index, trace_index, 0u ) );
            equals.emplace_back( t_eq );

          auto const t_and = add_tseytin_and( equals );
          add_clause( { ~t, t_and } );
        }
      }
    }

    /* eventually */
    if ( std::find( std::begin( _ps.ops ), std::end( _ps.ops ), operator_opcode::eventually_ ) != std::end( _ps.ops ) )
    {
      for ( uint32_t trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( uint32_t vertex_index = uint32_t( _ps.pd.nr_pi_fanins() ); vertex_index < _num_vertices; ++vertex_index )
        {
          if ( get_vertex_type( vertex_index ) != vertex_type::mixed ) continue;

          /* find label index */
          auto label_index = 0;
          for ( const auto& m : mixed_operators )
          {
            if ( m == operator_opcode::eventually_ )
              break;
            else
              label_index++;
          }

          auto const left_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[0u];
          auto const child_index =
            left_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) : uint32_t( _ps.pd.nr_pi_fanins() ) + left_dag_index - 1u;
          assert( child_index < vertex_index );

          auto const t = label( vertex_index, label_index );

          auto const prefix_length = _ps.traces.at( trace_index ).first.prefix_length();
          auto const trace_length = _ps.traces.at( trace_index ).first.length();

          std::vector<bill::lit_type> bs;
          for ( auto time_index = 0u; time_index < trace_length; ++time_index )
          {
            std::vector<bill::lit_type> as;
            for ( auto another_time_index = time_index; another_time_index < trace_length; ++another_time_index )
              as.emplace_back( trace( child_index, trace_index, another_time_index ) );
            bs.emplace_back( add_tseytin_equals( trace( vertex_index, trace_index, time_index ), add_tseytin_or( as ) ) );
          }
          auto const prefix_part = add_tseytin_and( bs );

          std::vector<bill::lit_type> cs;
          for ( auto time_index = prefix_length; time_index < trace_length; ++time_index )
          {
            std::vector<bill::lit_type> as;
            for ( auto another_time_index = prefix_length; another_time_index < trace_length; ++another_time_index )
              as.emplace_back( trace( child_index, trace_index, another_time_index ) );
            cs.emplace_back( add_tseytin_equals( trace( vertex_index, trace_index, time_index ), add_tseytin_or( as ) ) );
          }
          auto const postfix_part = add_tseytin_and( cs );

          add_clause( { ~t, add_tseytin_and( prefix_part, postfix_part ) } );
        }
      }
    }

    /* globally */
    if ( std::find( std::begin( _ps.ops ), std::end( _ps.ops ), operator_opcode::globally_ ) != std::end( _ps.ops ) )
    {
      for ( uint32_t trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( uint32_t vertex_index = uint32_t( _ps.pd.nr_pi_fanins() ); vertex_index < _num_vertices; ++vertex_index )
        {
          if ( get_vertex_type( vertex_index ) != vertex_type::mixed ) continue;

          /* find label index */
          auto label_index = 0;
          for ( const auto& m : mixed_operators )
          {
            if ( m == operator_opcode::globally_ )
              break;
            else
              label_index++;
          }

          auto const left_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[0u];
          auto const child_index =
            left_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) : uint32_t( _ps.pd.nr_pi_fanins() ) + left_dag_index - 1u;
          assert( child_index < vertex_index );

          auto const t = label( vertex_index, label_index );

          auto const prefix_length = _ps.traces.at( trace_index ).first.prefix_length();
          auto const trace_length = _ps.traces.at( trace_index ).first.length();

          std::vector<bill::lit_type> bs;
          for ( auto time_index = 0u; time_index < trace_length; ++time_index )
          {
            std::vector<bill::lit_type> as;
            for ( auto another_time_index = time_index; another_time_index < trace_length; ++another_time_index )
              as.emplace_back( trace( child_index, trace_index, another_time_index ) );
            bs.emplace_back( add_tseytin_equals( trace( vertex_index, trace_index, time_index ), add_tseytin_and( as ) ) );
          }
          auto const prefix_part = add_tseytin_and( bs );

          std::vector<bill::lit_type> cs;
          for ( auto time_index = prefix_length; time_index < trace_length; ++time_index )
          {
            std::vector<bill::lit_type> as;
            for ( auto another_time_index = prefix_length; another_time_index < trace_length; ++another_time_index )
              as.emplace_back( trace( child_index, trace_index, time_index ) );
            cs.emplace_back( add_tseytin_equals( trace( vertex_index, trace_index, time_index ), add_tseytin_and( as ) ) );
          }
          auto const postfix_part = add_tseytin_and( cs );

          add_clause( { ~t, add_tseytin_and( prefix_part, postfix_part ) } );
        }
      }
    }

    /* operator: until */
    if ( std::find( std::begin( _ps.ops ), std::end( _ps.ops ), operator_opcode::until_ ) != std::end( _ps.ops ) )
    {
      for ( uint32_t trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( uint32_t vertex_index = uint32_t( _ps.pd.nr_pi_fanins() ); vertex_index < _num_vertices; ++vertex_index )
        {
          if ( get_vertex_type( vertex_index ) != vertex_type::mixed && get_vertex_type( vertex_index ) != vertex_type::binary )
            continue;

          auto label_index = 0;

          /* find label index */
          if ( get_vertex_type( vertex_index ) == vertex_type::mixed )
          {
            for ( const auto& m : mixed_operators )
            {
              if ( m == operator_opcode::until_ )
                break;
              else
                label_index++;
            }
          }
          else if ( get_vertex_type( vertex_index ) == vertex_type::binary )
          {
            for ( const auto& m : binary_operators )
            {
              if ( m == operator_opcode::until_ )
                break;
              else
                label_index++;
            }
          }

          auto const left_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[0u];
          auto const right_dag_index = _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[1u];

          auto const child_index0 =
            left_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) : uint32_t( _ps.pd.nr_pi_fanins() ) + left_dag_index - 1u;
          auto const child_index1 =
            right_dag_index == 0u ? zeroes.at( vertex_index - _ps.pd.nr_pi_fanins() ) + ( left_dag_index == 0u ? 1u : 0u ) : uint32_t( _ps.pd.nr_pi_fanins() ) + right_dag_index - 1u;
          assert( vertex_index > child_index0 );
          assert( vertex_index > child_index1 );

          auto const t = label( vertex_index, label_index );

          auto const prefix_length = _ps.traces.at( trace_index ).first.prefix_length();
          auto const trace_length = _ps.traces.at( trace_index ).first.length();

          std::vector<bill::lit_type> as;
          for ( auto time_index = 0u; time_index < prefix_length; ++time_index )
          {
            std::vector<bill::lit_type> bs;
            for ( auto another_time_index = time_index; another_time_index < trace_length; ++another_time_index )
            {
              std::vector<bill::lit_type> cs;
              for ( auto one_more_time_index = time_index; one_more_time_index < another_time_index; ++one_more_time_index )
                cs.emplace_back( trace( child_index0, trace_index, one_more_time_index ) );
              cs.emplace_back( trace( child_index1, trace_index, another_time_index ) );
              bs.emplace_back( add_tseytin_and( cs ) );
            }
            as.emplace_back( add_tseytin_equals( trace( vertex_index, trace_index, time_index ), add_tseytin_or( bs ) ) );
          }

          for ( auto time_index = prefix_length; time_index < trace_length; ++time_index )
          {
            std::vector<bill::lit_type> bs;
            for ( auto another_time_index = prefix_length; another_time_index < trace_length; ++another_time_index )
            {
              std::vector<bill::lit_type> cs;
              for ( auto const& one_more_time_index : positions_between( time_index, another_time_index, prefix_length, trace_length ) )
                cs.emplace_back( trace( child_index0, trace_index, one_more_time_index ) );
              cs.emplace_back( trace( child_index1, trace_index, another_time_index ) );
              bs.emplace_back( add_tseytin_and( cs ) );
            }
            as.emplace_back( add_tseytin_equals( trace( vertex_index, trace_index, time_index ), add_tseytin_or( bs ) ) );
          }
          add_clause( { ~t, add_tseytin_and( as ) } );
        }
      }
    }

    /* traces */
    for ( auto trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
    {
      if ( _ps.traces.at( trace_index ).second )
      {
        add_clause( { trace( _num_vertices-1u, trace_index, 0u ) } );
      }
      else
      {
        add_clause( { ~trace( _num_vertices-1u, trace_index, 0u ) } );
      }
    }
  }

  /*! \brief Print the partial DAG (for debugging purpose only) */
  void print_partial_dag() const
  {
    for ( const auto& v : _ps.pd.get_vertices() )
    {
      std::cout << "( ";
      for ( const auto& i : v )
        std::cout << i << ' ';
      std::cout << ")";
    }
    std::cout << std::endl;
  }

  /* \brief Label variable */
  bill::lit_type label( uint32_t vertex_index, uint32_t label_index ) const
  {
    assert( vertex_index < label_offset.size() );
    return bill::lit_type( bill::var_type( label_offset.at( vertex_index ) + label_index ), bill::lit_type::polarities::positive );
  }

  /* \brief Trace variable */
  bill::lit_type trace( uint32_t vertex_index, uint32_t trace_index, uint32_t time_index ) const
  {
    return bill::lit_type( bill::var_type( trace_vars_begin + trace_offset.at( trace_index ) + time_index + ( trace_offset.at( trace_offset.size() - 1u ) * vertex_index ) ), bill::lit_type::polarities::positive );
  }

private:
  enum class vertex_type
  {
    pi = 0,
    mixed = 1,
    binary = 2
  }; /* vertex_type */

  vertex_type get_vertex_type( uint32_t vertex_index ) const
  {
    if ( vertex_index < uint32_t( _ps.pd.nr_pi_fanins() ) )
      return vertex_type::pi;

    assert( vertex_index - _ps.pd.nr_pi_fanins() < uint32_t( _ps.pd.nr_vertices() ) );
    assert( _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() ).size() == 2u );
    if ( _ps.pd.get_vertex( vertex_index - _ps.pd.nr_pi_fanins() )[1u] == 0u )
      return vertex_type::mixed;

    return vertex_type::binary;
  }

  uint32_t num_labels( uint32_t vertex_index ) const
  {
    switch ( get_vertex_type( vertex_index ) )
    {
    case vertex_type::pi:
      return _ps.num_propositions;
    case vertex_type::mixed:
      return mixed_operators.size();
    case vertex_type::binary:
      return binary_operators.size();
    default:
      assert( false );
    }
    return -1;
  }

  bill::lit_type add_tseytin_and(bill::lit_type const& a, bill::lit_type const& b)
  {
    auto const r = add_variable();
    add_clause(std::vector{~a, ~b, r});
    add_clause(std::vector{a, ~r});
    add_clause(std::vector{b, ~r});
    return r;
  }

  bill::lit_type add_tseytin_and( std::vector<bill::lit_type> const& ls )
  {
    assert( ls.size() > 0u );

    if ( ls.size() == 1u )
      return ls[0u];

    if ( ls.size() == 2u )
      return add_tseytin_and( ls[0u], ls[1u] );

    /* lookup in compute table */
    auto const it = and_compute_table.find( ls );
    if ( it != and_compute_table.end() )
      return it->second;

    auto const r = add_variable();

    std::vector<bill::lit_type> cls;
    for ( const auto& l : ls )
      cls.emplace_back(~l);
    cls.emplace_back( r );
    add_clause(cls);
    for ( const auto& l : ls )
      add_clause(std::vector{l, ~r});

    /* insert into compute table */
    and_compute_table.emplace( ls, r );

    return r;
  }

  bill::lit_type add_tseytin_or( bill::lit_type const& a, bill::lit_type const& b )
  {
    auto const r = add_variable();
    add_clause(std::vector{a, b, ~r});
    add_clause(std::vector{~a, r});
    add_clause(std::vector{~b, r});
    return r;
  }

  bill::lit_type add_tseytin_or( std::vector<bill::lit_type> const& ls )
  {
    assert( ls.size() > 0u );

    if ( ls.size() == 1u )
      return ls[0u];

    if ( ls.size() == 2u )
      return add_tseytin_or( ls[0u], ls[1u] );

    /* lookup in compute table */
    auto const it = or_compute_table.find( ls );
    if ( it != or_compute_table.end() )
      return it->second;

    auto const r = add_variable();

    std::vector<bill::lit_type> cls(ls);
    cls.emplace_back(~r);
    add_clause(cls);
    for ( const auto& l : ls )
      add_clause(std::vector{~l, r});

    /* insert into compute table */
    or_compute_table.emplace( ls, r );

    return r;
  }

  bill::lit_type add_tseytin_equals( bill::lit_type const& a, bill::lit_type const& b )
  {
    /* lookup in compute table */
    std::array<bill::lit_type, 2u> key{a,b};
    auto const it = equals_compute_table.find( key );
    if ( it != equals_compute_table.end() )
      return it->second;

    auto const r = add_variable();

    add_clause(std::vector{~a, ~b, r});
    add_clause(std::vector{~a, b, ~r});
    add_clause(std::vector{a, ~b, ~r});
    add_clause(std::vector{a, b, r});

    /* insert into compute table */
    equals_compute_table.emplace( key, r );

    return r;
  }

  bill::lit_type add_variable( bill::lit_type::polarities pol = bill::lit_type::polarities::positive )
  {
    return bill::lit_type( _solver.add_variable(), pol );
  }

  void add_clause( std::vector<bill::lit_type> const& cl )
  {
    // for ( const auto& l : cl )
    //   std::cout << fmt::format("{}{} ", l.is_complemented() ? "~" : "", l.variable() );
    // std::cout << std::endl;

    _solver.add_clause( cl );
  }

private:
  Solver& _solver;

  exact_ltl_pdag_encoder_parameter _ps;
  uint32_t _num_nodes;
  uint32_t _num_vertices;

  /* compute tables */
  std::unordered_map<std::vector<bill::lit_type>, bill::lit_type, lit_vector_hash> and_compute_table;
  std::unordered_map<std::vector<bill::lit_type>, bill::lit_type, lit_vector_hash> or_compute_table;
  std::unordered_map<std::array<bill::lit_type, 2u>, bill::lit_type, lit_array2_hash> equals_compute_table;

  std::vector<operator_opcode> mixed_operators;
  std::vector<operator_opcode> binary_operators;
  std::vector<uint32_t> zeroes;
  std::vector<uint32_t> label_offset;
  std::vector<uint32_t> trace_offset;
  uint32_t trace_vars_begin = 0u;
  uint32_t tseytin_vars_begin = 0u;
}; /* exact_ltl_pdag_encoder */

} /* namespace copycat */
