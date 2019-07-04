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
  \file ltl_pdag_learner.hpp
  \brief LTL partial DAG learner

  \author Heinz Riener
*/

#pragma once

#include <copycat/algorithms/ltl_learner.hpp>
#include <percy/partial_dag.hpp>

namespace copycat
{

struct ltl_pdag_encoder_parameter
{
  bool verbose = false;
  uint32_t num_propositions = 0u;
  std::vector<operator_opcode> ops;
  uint32_t num_nodes = 0u;
  std::vector<std::pair<trace, bool>> traces;
  percy::partial_dag pdag;
}; /* ltl_encoder_parameter */

struct label
{
  label( std::string symbol, uint32_t arity )
    : symbol( symbol )
    , arity( arity )
  {
  }
  
  std::string symbol;
  uint32_t arity;
};

enum node_type : uint32_t
{
  primary_input_ = 0u,
  boundary_node_ = 1u,
  inner_node_ = 2u,
}; /* node_type */
    

template<typename Solver>
class ltl_pdag_encoder
{
public:
  ltl_pdag_encoder( Solver& solver )
    : _solver( solver )
  {
  }

  /* \brief Encode the parameters to clauses */
  void encode( ltl_pdag_encoder_parameter const& ps )
  {
    _ps = ps;

    prepare_internal_datastructures();
    allocate_variables();
    print_allocated_variables(); /* debug */
    check_allocated_variables(); /* debug */
    create_clauses();
  }

  chain<std::string, std::vector<int>> extract_chain()
  {
    /* get a model */
    auto const model = _solver.get_model().model();

    /* extract the labeling from the model */    
    std::vector<uint32_t> labeling;
    for ( auto node_index = 1u; node_index <= _ps.num_nodes; ++node_index )
    {
      for ( auto label_index = 0u; label_index < num_node_labels( node_index ); ++label_index )
      {
        if ( model.at( label_lit( node_index, label_index ).variable() ) == bill::lbool_type::true_ )
        {
          auto const type = get_node_type( node_index );
          labeling.emplace_back( _possible_node_labels.at( type ).at( label_index ) );
        }
      }
    }

    /* convert the pdag with the labeling into a chain */
    chain<std::string, std::vector<int>> c;
    
    for ( auto const& l : labeling )
    {
      auto const label = _labels.at( l ).symbol;
      if ( label.at( 0u ) == 'x' )
        c.add_step( label, {} );
    }
    
    auto const vertices = _ps.pdag.get_vertices();
    uint32_t count_pi_fanins = 0u;
    for ( auto i = 0u; i < vertices.size(); ++i )
    {
      auto const label = _labels.at( labeling.at( _ps.pdag.nr_pi_fanins() ) ).symbol;
      auto const vs = vertices.at( i );

      assert( vs.size() == 2u );
      std::vector<int32_t> children;
      if ( label == "~" || label == "X" )
      {
        auto const child_label_index = vs.at( 0u ) == 0u ? count_pi_fanins++ : vs.at( 0u );
        children.emplace_back( labeling.at( child_label_index ) );
      }
      else
      {
        auto const child_label_index0 = vs.at( 0u ) == 0u ? count_pi_fanins++ : vs.at( 0u );
        auto const child_label_index1 = vs.at( 1u ) == 0u ? count_pi_fanins++ : vs.at( 1u );
        children.emplace_back( labeling.at( child_label_index0 ) );
        children.emplace_back( labeling.at( child_label_index1 ) );
      }

      c.add_step( label, children );
    }

    // write_chain( c );    
    c.remove_unused_steps();
    // write_chain( c );    

    return c;
  }
  
private:
  void create_clauses()
  {
    if ( _ps.verbose )
      std::cout << "[i] create clauses" << std::endl;

    /* each node has to be labeled with at least one label */
    for ( auto node_index = 1u; node_index <= _ps.num_nodes; ++node_index )
    {
      std::vector<bill::lit_type> clause;
      for ( auto label_index = 0u; label_index < num_node_labels( node_index ); ++label_index )
      {
        clause.emplace_back( label_lit( node_index, label_index ) );
      }
      add_clause( clause );
    }

    /* each node has to be labeled with at most one operator */
    for ( auto node_index = 1u; node_index <= _ps.num_nodes; ++node_index )
    {
      for ( auto one_label_index = 0u; one_label_index < num_node_labels( node_index ); ++one_label_index )
      {
        for ( auto another_label_index = one_label_index + 1u; another_label_index < num_node_labels( node_index ); ++another_label_index )
        {
          std::vector<bill::lit_type> clause;
          clause.emplace_back( ~( label_lit( node_index, one_label_index ) ) );
          clause.emplace_back( ~( label_lit( node_index, another_label_index ) ) );
          add_clause( clause );
        }
      }
    }

    /* proposition semantics */
    if ( _possible_node_labels.at( node_type::primary_input_ ).size() > 0u )
    {
      for ( auto trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( auto node_index = 1u; node_index <= _ps.num_nodes; ++node_index )
        {
          if ( get_node_type( node_index ) != node_type::primary_input_ ) continue;

          for ( auto prop_index = 0u; prop_index < _possible_node_labels.at( node_type::primary_input_ ).size(); ++prop_index )
          {
            std::vector<bill::lit_type> cube;
            for ( auto time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length(); ++time_index )
            {
              if ( _ps.traces.at( trace_index ).first.is_true( time_index, prop_index + 1u ) )
              {
                cube.emplace_back( trace_lit( trace_index, node_index, time_index ) );
              }
              else
              {
                cube.emplace_back( ~trace_lit( trace_index, node_index, time_index ) );
              }
            }

            if ( cube.size() == 1u )
            {
              add_clause( { ~label_lit( node_index, prop_index ), cube.at( 0u ) } );
            }
            else
            {
              auto const t = add_tseytin_and( cube );
              add_clause( { ~label_lit( node_index, prop_index ), t } );
            }
          }
        }
      }
    }

    /* operator semantics of Boolean negation */
    if ( std::find( std::begin( _ps.ops ), std::end( _ps.ops ), operator_opcode::not_ ) != std::end( _ps.ops ) )
    {
      for ( auto trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( auto root_index = 2u; root_index <= _ps.num_nodes; ++root_index )
        {
          if ( get_node_type( root_index ) != node_type::boundary_node_ ) continue;
          
          /* infer child_index from structure */
          auto const& vs = _ps.pdag.get_vertices().at( root_index - _ps.pdag.nr_pi_fanins() - 1u );

          uint32_t child_index;
          if ( vs.at( 0u ) == 0u )
          {
            child_index = zero_count.at( root_index - _ps.pdag.nr_pi_fanins() - 1u ) + 1u;
          }
          else
          {
            child_index = _ps.num_propositions + vs.at( 0u );
          }
          assert( child_index >= 1 );
          
          /* find not */
          auto const not_label_index = get_operator_label_for_node_type( operator_opcode::not_, node_type::boundary_node_ );

          std::vector<bill::lit_type> cube;
          for ( auto time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length(); ++time_index )
          {
            auto const t_eq = add_tseytin_equals( trace_lit( trace_index, root_index, time_index ), ~trace_lit( trace_index, child_index, time_index ) );
            cube.emplace_back( t_eq );
          }
          auto const t_and = add_tseytin_and( cube );
          add_clause( { ~label_lit( root_index, not_label_index ), t_and } );
        }        
      }      
    }

    /* operator semantics for temporal operator X */
    if ( std::find( std::begin( _ps.ops ), std::end( _ps.ops ), operator_opcode::next_ ) != std::end( _ps.ops ) )
    {
      for ( auto trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
      {
        for ( auto root_index = _ps.num_propositions + 1u; root_index <= _ps.num_nodes; ++root_index )
        {
          if ( get_node_type( root_index ) != node_type::boundary_node_ ) continue;

          /* infer child_index from structure */
          auto const& vs = _ps.pdag.get_vertices().at( root_index - _ps.pdag.nr_pi_fanins() - 1u );

          uint32_t child_index;
          if ( vs.at( 0u ) == 0u )
          {
            child_index = zero_count.at( root_index - _ps.pdag.nr_pi_fanins() - 1u ) + 1u;
          }
          else
          {
            child_index = _ps.num_propositions + vs.at( 0u );
          }
          assert( child_index >= 1 );
          
          std::vector<bill::lit_type> equals;
          for ( auto time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length() - 1u; ++time_index )
          {
            auto const t_eq = add_tseytin_equals( trace_lit( trace_index, root_index, time_index ), trace_lit( trace_index, child_index, time_index + 1 ) );
            equals.emplace_back( t_eq );
          }
          auto const t_eq = add_tseytin_equals( trace_lit( trace_index, root_index, _ps.traces.at( trace_index ).first.length()-1u ), trace_lit( trace_index, child_index, _ps.traces.at( trace_index ).first.prefix_length() - 1u ) );
          equals.emplace_back( t_eq );
          
          auto const t_and = add_tseytin_and( equals );

          /* find X */
          auto const next_label_index = get_operator_label_for_node_type( operator_opcode::next_, node_type::boundary_node_ );
          add_clause( { ~label_lit( root_index, next_label_index ), t_and } );
        }
      }
    }
    
    /* trace semantics */
    for ( auto trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
    {
      if ( _ps.traces.at( trace_index ).second )
      {
        add_clause( { trace_lit( trace_index, _ps.num_nodes, 0u ) } );
      }
      else
      {
        add_clause( { ~trace_lit( trace_index, _ps.num_nodes, 0u ) } );
      }
    }    
  }
  
  void prepare_internal_datastructures()
  {
    if ( _ps.verbose )
      std::cout << "[i] prepare internal data-structures" << std::endl;    

    /* populate labels and possible_node_labels */
    std::vector<uint32_t> proposition_labels;
    std::vector<uint32_t> unary_operator_labels;
    std::vector<uint32_t> binary_operator_labels;

    for ( auto i = 0u; i < _ps.num_propositions; ++i )
    {
      auto const index = _labels.size();
      _labels.emplace_back( fmt::format( "x{}", i ), 0u );
      proposition_labels.emplace_back( index );
    }
    
    for ( auto const& op : _ps.ops )
    {
      auto const index = _labels.size();
      auto const arity = operator_opcode_arity( op );
      _labels.emplace_back( operator_opcode_to_string( op ), arity );
      switch ( arity )
      {
      case 0u:
        unary_operator_labels.emplace_back( index );
        break;
      case 1u:
        binary_operator_labels.emplace_back( index );
        break;
      default:
        std::cerr << "[e] unsupported operator arity" << std::endl;
        assert( false && "unsupported operator arity" );
        break;
      }
    }

    std::vector<uint32_t> unary_or_binary_operator_labels;
    std::set_union( std::begin( unary_operator_labels ), std::end( unary_operator_labels ),
                    std::begin( binary_operator_labels ), std::end( binary_operator_labels ),
                    std::back_inserter( unary_or_binary_operator_labels ) );
    _possible_node_labels.emplace( node_type::primary_input_, proposition_labels );
    _possible_node_labels.emplace( node_type::boundary_node_, unary_or_binary_operator_labels );
    _possible_node_labels.emplace( node_type::inner_node_, binary_operator_labels );

    /* prepare zero_count */
    auto zero_counter = 0u;
    for ( auto i = 0u; i < _ps.pdag.get_vertices().size(); ++i )
    {
      zero_count.emplace_back( zero_counter );
      
      auto vs = _ps.pdag.get_vertices().at( i );
      for ( auto j = 0u; j < vs.size(); ++j )
        ++zero_counter;
    }
  }

  void allocate_variables()
  {
    if ( _ps.verbose )
      std::cout << "[i] allocate variables" << std::endl;

    /* allocate variables for labels */
    _label_var_begin = 0u;
    _label_var_end = _label_var_begin;
    for ( auto i = 1u; i <= _ps.num_nodes; ++i )
      _label_var_end += _possible_node_labels.at( get_node_type( i ) ).size();     

    --_label_var_end;
    
    /* allocate variables for traces */
    _trace_var_begin = _label_var_end + 1u;
    _trace_var_end = _trace_var_begin - 1u;
    for ( auto i = 0u; i < _ps.traces.size(); ++i )
      _trace_var_end += _ps.traces.at( i ).first.length() * _ps.num_nodes;

    _tseytin_var_begin = _tseytin_var_end = _trace_var_end + 1u;
    
    /* actually allocate the variables in the solver */
    if ( _ps.verbose )
      std::cout << fmt::format( "[i] add {} Boolean variables to SAT solver\n",
                                _tseytin_var_begin - _label_var_begin );
    _solver.add_variables( _tseytin_var_begin - _label_var_begin );
  }

  void print_allocated_variables() const
  {
    std::cout << fmt::format( "label variables: {}..{}\n", _label_var_begin, _label_var_end );
    for ( auto node_index = 1u; node_index <= _ps.num_nodes; ++node_index )
    {
      for ( auto label_index = 0u; label_index < num_node_labels( node_index ); ++label_index )
      {
        std::cout << fmt::format( "  label_lit(node={}, label={}): {}\n",
                                  node_index,
                                  _possible_node_labels.at( get_node_type( node_index ) ).at( label_index ),
                                  label_lit( node_index, label_index ).variable() );
      }
    }
    
    std::cout << fmt::format( "trace variables: {}..{}\n", _trace_var_begin, _trace_var_end );
    for ( auto trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
    {
      for ( auto node_index = 1u; node_index <= _ps.num_nodes; ++node_index )
      {
        for ( auto time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length(); ++time_index )
        {
          std::cout << fmt::format( "  trace_lit(trace={}, node={}, time={}): {}\n",
                                    trace_index, node_index, time_index,
                                    trace_lit( trace_index, node_index, time_index ).variable() );
        }
      }
    }

    /*** print the structure of the pdag ***/
    for ( auto node_index = 1u; node_index <= _ps.num_nodes; ++node_index )
    {
      if ( node_index <= _ps.num_propositions )
      {
        std::cout << fmt::format( "node={} corresponds to PDAG-fanin {}\n", node_index, node_index );
      }
      else
      {
        std::cout << fmt::format( "node={} corresponds to inner node {}\n", node_index, node_index - _ps.pdag.nr_pi_fanins() );

        std::cout << "  with children { ";
        auto local_zero_count = 0;
        for ( const auto& v : _ps.pdag.get_vertices().at( node_index - _ps.pdag.nr_pi_fanins() - 1u ) )
        {
          if ( v == 0 )
          {
            std::cout << "PDAG-fanin " << ( zero_count.at( node_index - _ps.pdag.nr_pi_fanins() - 1u ) + ++local_zero_count ) << " ";
          }
          else
          {
            std::cout << "inner node " << v << " ";
          }
        }
        std::cout << "}" << std::endl;
      }
    }
  }

  void check_allocated_variables() const
  {
    if ( _ps.verbose )
      std::cout << "[i] check allocated variables" << std::endl;

    /* check if the variable layout has holes */
    assert( _label_var_end + 1u == _trace_var_begin );
    assert( _trace_var_end + 1u == _tseytin_var_begin );

    /* check if the layout overlaps */
    std::vector<int32_t> vars;
    for ( auto node_index = 1u; node_index <= _ps.num_nodes; ++node_index )
    {
      for ( auto label_index = 0u; label_index < num_node_labels( node_index ); ++label_index )
      {
        auto const var = label_lit( node_index, label_index ).variable();
        assert( std::find( std::begin( vars ), std::end( vars ), var ) == vars.end() );
        vars.emplace_back( var );
      }
    }

    /* check if each variables is used only once */
    for ( auto trace_index = 0u; trace_index < _ps.traces.size(); ++trace_index )
    {
      for ( auto node_index = 1u; node_index <= _ps.num_nodes; ++node_index )
      {
        for ( auto time_index = 0u; time_index < _ps.traces.at( trace_index ).first.length(); ++time_index )
        {
          auto const trace_var = trace_lit( trace_index, node_index, time_index ).variable();
          assert( std::find( std::begin( vars ), std::end( vars ), trace_var ) == vars.end() );
          vars.emplace_back( trace_var );
        }
      }
    }

    /* check if the variables follow each other */
    auto it = std::begin( vars );
    auto it2 = std::begin( vars ) + 1;
    for ( auto ie = std::end( vars ); it != ie && it2 != ie; ++it, ++it2 )
    {
      assert( *it + 1 == *it2 );
    }
  }

private:
  uint32_t get_operator_label_for_node_type( operator_opcode const& opcode, node_type const& type )
  {
    for ( const auto& label_index : _possible_node_labels.at( type ) )
    {
      if ( _labels.at( label_index ).symbol == operator_opcode_to_string( opcode ) )
      {
        return label_index;
      }
    }
    assert( false && "opcode not found for this node type" );
    return -1;
  }

  node_type get_node_type( uint32_t node_index ) const
  {
    assert( node_index > 0u );
    
    if ( node_index <= uint32_t( _ps.pdag.nr_pi_fanins() ) )
      return node_type::primary_input_;

    assert( node_index <= uint32_t( _ps.pdag.nr_pi_fanins() + _ps.pdag.nr_vertices() ) );
    auto const& v = _ps.pdag.get_vertex( node_index - _ps.pdag.nr_pi_fanins() - 1u );
    for ( const auto& vv : v )
    {
      if ( vv == 0 )
        return node_type::boundary_node_;
    }

    return node_type::inner_node_;
  }

  uint32_t num_node_labels( uint32_t node_index ) const
  {
    assert( node_index > 0u );
    return _possible_node_labels.at( get_node_type( node_index ) ).size();
  }
  
  
  bill::lit_type label_lit( uint32_t node_index, uint32_t label_index ) const
  {
    auto offset = 0u;
    for ( auto i = 1u; i < node_index; ++i )
      offset += num_node_labels( i );

    return bill::lit_type( _label_var_begin + offset + label_index,
                           bill::lit_type::polarities::positive );
  }

  bill::lit_type trace_lit( uint32_t trace_index, uint32_t node_index, uint32_t time_index ) const
  {
    auto offset = 0u;
    for ( auto i = 0u; i < trace_index; ++i )
      offset += _ps.traces.at( i ).first.length() * _ps.num_nodes;

    return bill::lit_type( _trace_var_begin + offset + ( node_index - 1 ) * _ps.traces.at( trace_index ).first.length() + time_index,
                           bill::lit_type::polarities::positive );
  }

  bill::lit_type add_tseytin_and(bill::lit_type const& a, bill::lit_type const& b)
  {
    auto const r = add_variable();
    add_clause(std::vector{~a, ~b, r});
    add_clause(std::vector{a, ~r});
    add_clause(std::vector{b, ~r});
    return r;
  }

  bill::lit_type add_tseytin_and(std::vector<bill::lit_type> const& ls)
  {
    auto const r = add_variable();
    std::vector<bill::lit_type> cls;
    for ( const auto& l : ls )
      cls.emplace_back(~l);
    cls.emplace_back( r );
    add_clause(cls);
    for ( const auto& l : ls )
      add_clause(std::vector{l, ~r});
    return r;
  }

  bill::lit_type add_tseytin_or(bill::lit_type const& a, bill::lit_type const& b)
  {
    auto const r = add_variable();
    add_clause(std::vector{a, b, ~r});
    add_clause(std::vector{~a, r});
    add_clause(std::vector{~b, r});
    return r;
  }

  bill::lit_type add_tseytin_or(std::vector<bill::lit_type> const& ls)
  {
    auto const r = add_variable();
    std::vector<bill::lit_type> cls(ls);
    cls.emplace_back(~r);
    add_clause(cls);
    for ( const auto& l : ls )
      add_clause(std::vector{~l, r});
    return r;
  }

  bill::lit_type add_tseytin_equals(bill::lit_type const& a, bill::lit_type const& b)
  {
    auto const r = add_variable();
    add_clause(std::vector{~a, ~b, r});
    add_clause(std::vector{~a, b, ~r});
    add_clause(std::vector{a, ~b, ~r});
    add_clause(std::vector{a, b, r});
    return r;
  }
  
  bill::lit_type add_variable( bill::lit_type::polarities pol = bill::lit_type::polarities::positive, std::string const& note = "" )
  {
    auto const r = _solver.add_variable();
    if ( _ps.verbose )
      std::cout << fmt::format( "[i] add_variable {}: {}\n", note, uint32_t( r ) );
    return bill::lit_type( r, pol );
  }

  void add_clause( std::vector<bill::lit_type> const& clause, std::string const& note = "")
  {
    if ( _ps.verbose )
    {
      std::cout << fmt::format( "[i] add_clause {}: ", note );
      for ( auto const& lit : clause )
      {
        if ( lit.is_complemented() )
        {
          std::cout << '~';
        }
        std::cout << uint32_t( lit.variable() ) << ' ';
      }
      std::cout << "\n";
    }

    _solver.add_clause( clause );
  }

private:
  /*! \brief Backend solver that takes the encoded constraints */
  Solver& _solver;

  /*! \brief Node labels */
  std::vector<label> _labels;

  /*! \brief Maps a partial DAG node_type to a set of possible label indices */
  std::unordered_map<node_type, std::vector<uint32_t>> _possible_node_labels;

  /*! \brief Count the zeros in the pdag before the actual position */
  std::vector<uint32_t> zero_count;

  /*! Parameters for the synthesis problem */
  ltl_pdag_encoder_parameter _ps;

  uint32_t _label_var_begin;
  uint32_t _label_var_end;
  uint32_t _trace_var_begin;
  uint32_t _trace_var_end;
  uint32_t _tseytin_var_begin;
  uint32_t _tseytin_var_end;

}; /* ltl_pdag_encoder */

} /* copycat */
