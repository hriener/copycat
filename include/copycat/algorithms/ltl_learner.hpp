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
  \file ltl_learner.hpp
  \brief LTL learner

  \author Heinz Riener
*/

#pragma once

#include <copycat/chain/chain.hpp>
#include <copycat/trace.hpp>
#include <bill/sat/solver.hpp>
#include <bill/sat/tseytin.hpp>
#include <fmt/format.h>
#include <unordered_map>
#include <iostream>

namespace copycat
{

enum class operator_opcode : uint32_t
{
  not_   = 0u,
  or_    = 1u,
  next_  = 2u,
  until_ = 3u,
}; /* operator_opcodes */

std::string operator_opcode_to_string( operator_opcode const& opcode )
{
  switch ( opcode )
  {
  case operator_opcode::not_:
    return "~";
  case operator_opcode::or_:
    return "|";
  case operator_opcode::next_:
    return "X";
  case operator_opcode::until_:
    return "U";
  default:
    break;
  }
  return "?";
}

uint32_t operator_opcode_arity( operator_opcode const& opcode )
{
  switch ( opcode )
  {
  case operator_opcode::not_:
  case operator_opcode::next_:
    return 1;
  case operator_opcode::or_:
  case operator_opcode::until_:
    return 2;
  default:
    break;
  }
  return 0;
}

struct ltl_encoder_parameter
{
  bool verbose = false;
  uint32_t num_propositions = 0u;
  std::vector<operator_opcode> ops;
  uint32_t num_nodes = 0u;
  std::vector<std::pair<trace, bool>> traces;
}; /* ltl_encoder_parameter */

template<typename Solver>
class ltl_encoder
{
public:
  ltl_encoder( Solver& solver )
    : _solver( solver )
  {
  }

  /*! \brief Encode paramters */
  void encode( ltl_encoder_parameter const& ps )
  {
    verbose = ps.verbose;
    num_propositions = ps.num_propositions;
    ops = ps.ops;

    for ( auto i = 0u; i < ps.ops.size(); ++i )
      operator_to_label.emplace( ps.ops[i], ps.num_propositions + i );

    num_labels = num_propositions +  ops.size();
    num_nodes = ps.num_nodes;
    num_traces = ps.traces.size();
    traces = ps.traces;

    assert( num_nodes > 0u );
    assert( num_traces > 0u );
  }

  void allocate_variables()
  {
    if ( verbose )
      std::cout << "[i] allocate variables" << std::endl;

    /* allocate variables for labels */
    label_var_begin = 0u;
    label_var_end = label_var_begin + num_labels * num_nodes - 1u;

    /* allocate variables for structure */
    structural_var_begin = label_var_end + 1u;
    structural_var_end = structural_var_begin + num_nodes * ( num_nodes - 1 ) - 1u;

    /* allocate variables for traces */
    trace_var_begin = structural_var_end + 1u;
    trace_var_end = trace_var_begin - 1u;
    for ( auto i = 0u; i < traces.size(); ++i )
    {
      trace_var_end += traces.at( i ).first.length() * num_nodes;
    }

    tseytin_var_begin = tseytin_var_end = trace_var_end + 1u;

    /* actually allocate the variables in the solver */
    if ( verbose )
      std::cout << fmt::format( "[i] add {} Boolean variables to SAT solver\n",
                                tseytin_var_begin - label_var_begin + 1u );
    _solver.add_variables( tseytin_var_begin - label_var_begin );
  }

  void print_allocated_variables() const
  {
    std::cout << fmt::format( "label variables: {}..{}\n", label_var_begin, label_var_end );
    for ( auto node_index = 1u; node_index <= num_nodes; ++node_index )
    {
      for ( auto label_index = 0u; label_index < num_labels; ++label_index )
      {
        std::cout << fmt::format( "  label_lit(node={}, label={}): {}\n",
                                  node_index, label_index, label_lit( node_index, label_index ).variable() );
      }
    }

    std::cout << fmt::format( "structure variables: {}..{}\n", structural_var_begin, structural_var_end );
    for ( auto root_index = 1u; root_index <= num_nodes; ++root_index )
      {
      for ( auto child_index = 1u; child_index < root_index; ++child_index )
      {
        std::cout << fmt::format( "  left_lit(root={}, child={}): {}\n",
                                  root_index, child_index, left_lit( root_index, child_index ).variable() );
        std::cout << fmt::format( "  right_lit(root={}, child={}): {}\n",
                                  root_index, child_index, right_lit( root_index, child_index ).variable() );
      }
    }

    std::cout << fmt::format( "trace variables: {}..{}\n", trace_var_begin, trace_var_end );
    for ( auto trace_index = 0u; trace_index < traces.size(); ++trace_index )
    {
      for ( auto node_index = 1u; node_index <= num_nodes; ++node_index )
      {
        for ( auto time_index = 0u; time_index < traces.at( trace_index ).first.length(); ++time_index )
        {
          std::cout << fmt::format( "  trace_lit(trace={}, node={}, time={}): {}\n",
                                    trace_index, node_index, time_index,
                                    trace_lit( trace_index, node_index, time_index ).variable() );
        }
      }
    }
  }

  void check_allocated_variables() const
  {
    if ( verbose )
      std::cout << "[i] check allocated variables" << std::endl;

    /* check if the variable layout has holes */
    assert( label_var_end + 1u == structural_var_begin );
    assert( structural_var_end + 1u == trace_var_begin );
    assert( trace_var_end + 1u == tseytin_var_begin );

    /* check if the layout overlaps */
    std::vector<int32_t> vars;
    for ( auto node_index = 1u; node_index <= num_nodes; ++node_index )
    {
      for ( auto label_index = 0u; label_index < num_labels; ++label_index )
      {
        auto const var = label_lit( node_index, label_index ).variable();
        assert( std::find( std::begin( vars ), std::end( vars ), var ) == vars.end() );
        vars.emplace_back( var );
      }
    }

    for ( auto root_index = 1u; root_index <= num_nodes; ++root_index )
    {
      for ( auto child_index = 1u; child_index < root_index; ++child_index )
      {
        auto const left_var = left_lit( root_index, child_index ).variable();
        assert( std::find( std::begin( vars ), std::end( vars ), left_var ) == vars.end() );

        auto const right_var = right_lit( root_index, child_index ).variable();
        assert( std::find( std::begin( vars ), std::end( vars ), right_var ) == vars.end() );

        vars.emplace_back( left_var );
        vars.emplace_back( right_var );
      }
    }

    /* check if each variables is used only once */
    for ( auto trace_index = 0u; trace_index < traces.size(); ++trace_index )
    {
      for ( auto node_index = 1u; node_index <= num_nodes; ++node_index )
      {
        for ( auto time_index = 0u; time_index < traces.at( trace_index ).first.length(); ++time_index )
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

  void create_clauses()
  {
    if ( verbose )
      std::cout << "[i] create clauses" << std::endl;

    /* each node has to be labeled with at least one operator */
    for ( auto node_index = 1u; node_index <= num_nodes; ++node_index )
    {
      std::vector<bill::lit_type> clause;
      for ( auto label_index = 0u; label_index < num_labels; ++label_index )
      {
        clause.emplace_back( label_lit( node_index, label_index ) );
      }
      add_clause( clause );
    }

    /* each node has to be labeled with at most one operator */
    for ( auto node_index = 1u; node_index <= num_nodes; ++node_index )
    {
      for ( auto one_label_index = 0u; one_label_index < num_labels; ++one_label_index )
      {
        for ( auto another_label_index = one_label_index + 1u; another_label_index < num_labels; ++another_label_index )
        {
          std::vector<bill::lit_type> clause;
          clause.emplace_back( ~( label_lit( node_index, one_label_index ) ) );
          clause.emplace_back( ~( label_lit( node_index, another_label_index ) ) );
          add_clause( clause );
        }
      }
    }

    /* each node has a left child */
    for ( auto root_index = 2u; root_index <= num_nodes; ++root_index )
    {
      std::vector<bill::lit_type> clause;
      for ( auto child_index = 1u; child_index < root_index; ++child_index )
      {
        clause.emplace_back( left_lit( root_index, child_index ) );
      }
      add_clause( clause );
    }

    for ( auto root_index = 2u; root_index <= num_nodes; ++root_index )
    {
      for ( auto one_child_index = 1u; one_child_index < root_index; ++one_child_index )
      {
        for ( auto another_child_index = one_child_index + 1; another_child_index < root_index; ++another_child_index )
        {
          std::vector<bill::lit_type> clause;
          clause.emplace_back( ~( left_lit( root_index, one_child_index ) ) );
          clause.emplace_back( ~( left_lit( root_index, another_child_index ) ) );
          add_clause( clause );
        }
      }
    }

    /* each node has a right child */
    for ( auto root_index = 2u; root_index <= num_nodes; ++root_index )
    {
      std::vector<bill::lit_type> clause;
      for ( auto child_index = 1u; child_index < root_index; ++child_index )
      {
        clause.emplace_back( right_lit( root_index, child_index ) );
      }
      add_clause( clause );
    }

    for ( auto root_index = 2u; root_index <= num_nodes; ++root_index )
    {
      for ( auto one_child_index = 1u; one_child_index < root_index; ++one_child_index )
      {
        for ( auto another_child_index = one_child_index + 1; another_child_index < root_index; ++another_child_index )
        {
          std::vector<bill::lit_type> clause;
          clause.emplace_back( ~( right_lit( root_index, one_child_index ) ) );
          clause.emplace_back( ~( right_lit( root_index, another_child_index ) ) );
          add_clause( clause );
        }
      }
    }

    /* the first node must be labeled with a proposition */
    std::vector<bill::lit_type> clause;
    for ( auto prop_index = 0u; prop_index < num_propositions; ++prop_index )
    {
      clause.emplace_back( label_lit( 1u, prop_index ) );
    }
    add_clause( clause );

    /* proposition semantics */
    if ( num_propositions > 0u )
    {
      for ( auto trace_index = 0u; trace_index < traces.size(); ++trace_index )
      {
        for ( auto node_index = 1u; node_index <= num_nodes; ++node_index )
        {
          for ( auto prop_index = 0u; prop_index < num_propositions; ++prop_index )
          {
            std::vector<bill::lit_type> cube;
            for ( auto time_index = 0u; time_index < traces.at( trace_index ).first.length(); ++time_index )
            {
              if ( traces.at( trace_index ).first.is_true( time_index, prop_index + 1u ) )
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

    /* operator: not */
    if ( std::find( std::begin( ops ), std::end( ops ), operator_opcode::not_ ) != std::end( ops ) )
    {
      for ( auto trace_index = 0u; trace_index < traces.size(); ++trace_index )
      {
        for ( auto root_index = 2u; root_index <= num_nodes; ++root_index )
        {
          for ( auto child_index = 1u; child_index < root_index; ++child_index )
          {
            auto const t = add_tseytin_and(
              { label_lit( root_index, operator_to_label[ operator_opcode::not_ ] ), left_lit( root_index, child_index ) }
              );

            std::vector<bill::lit_type> cube;
            for ( auto time_index = 0u; time_index < traces.at( trace_index ).first.length(); ++time_index )
            {
              auto const t_eq = add_tseytin_equals( trace_lit( trace_index, root_index, time_index ), ~trace_lit( trace_index, child_index, time_index ) );
              cube.emplace_back( t_eq );
            }
            auto const t_and = add_tseytin_and( cube );
            add_clause( { ~t, t_and } );
          }
        }
      }
    }

    /* operator: or */
    if ( std::find( std::begin( ops ), std::end( ops ), operator_opcode::or_ ) != std::end( ops ) )
    {
      for ( auto trace_index = 0u; trace_index < traces.size(); ++trace_index )
      {
        for ( auto root_index = 2u; root_index <= num_nodes; ++root_index )
        {
          for ( auto one_child_index = 1u; one_child_index < root_index; ++one_child_index )
          {
            for ( auto another_child_index = 1u; another_child_index < root_index; ++another_child_index )
            {
              auto const t = add_tseytin_and(
                { label_lit( root_index, operator_to_label[operator_opcode::or_] ), left_lit( root_index, one_child_index ), right_lit( root_index, another_child_index ) }
                );

              std::vector<bill::lit_type> cube;
              for ( auto time_index = 0u; time_index < traces.at( trace_index ).first.length(); ++time_index )
              {
                auto const t_or = add_tseytin_or( trace_lit( trace_index, one_child_index, time_index ), trace_lit( trace_index, another_child_index, time_index ) );
                auto const t_eq = add_tseytin_equals( trace_lit( trace_index, root_index, time_index ), t_or );
                cube.emplace_back( t_eq );
              }
              auto const t_and = add_tseytin_and( cube );
              add_clause( { ~t, t_and } );
            }
          }
        }
      }
    }

    /* operator: next */
    if ( std::find( std::begin( ops ), std::end( ops ), operator_opcode::next_ ) != std::end( ops ) )
    {
      for ( auto trace_index = 0u; trace_index < traces.size(); ++trace_index )
      {
        for ( auto root_index = 2u; root_index <= num_nodes; ++root_index )
        {
          for ( auto child_index = 1u; child_index < root_index; ++child_index )
          {
            auto const t = add_tseytin_and(
              { label_lit( root_index, num_propositions + operator_to_label[operator_opcode::next_] ), left_lit( root_index, child_index ) } );

            std::vector<bill::lit_type> equals;
            for ( auto time_index = 0u; time_index < traces.at( trace_index ).first.length() - 1u; ++time_index )
            {
              auto const t_eq = add_tseytin_equals( trace_lit( trace_index, root_index, time_index ), trace_lit( trace_index, child_index, time_index + 1 ) );
              equals.emplace_back( t_eq );
            }

            auto const t_eq = add_tseytin_equals( trace_lit( trace_index, root_index, traces.at( trace_index ).first.length()-1u ), trace_lit( trace_index, child_index, traces.at( trace_index ).first.prefix_length() - 1u ) );
            equals.emplace_back( t_eq );

            auto const t_and = add_tseytin_and( equals );
            add_clause( { ~t, t_and } );
          }
        }
      }
    }

    /* operator: until */
    if ( std::find( std::begin( ops ), std::end( ops ), operator_opcode::until_ ) != std::end( ops ) )
    {
      for ( auto trace_index = 0u; trace_index < traces.size(); ++trace_index )
      {
        for ( auto root_index = 2u; root_index <+ num_nodes; ++root_index )
        {
          for ( auto one_child_index = 1u; one_child_index < root_index; ++one_child_index )
          {
            for ( auto another_child_index = 1u; another_child_index < root_index; ++another_child_index )
            {
              /* condition */
              auto const t = add_tseytin_and(
                { label_lit( root_index, operator_to_label[operator_opcode::until_] ), left_lit( root_index, one_child_index ), right_lit( root_index, another_child_index ) }
                );

              /* part 1 */
              std::vector<bill::lit_type> equals_part1;
              for ( auto time_index = 0u; time_index < traces.at( trace_index ).first.prefix_length(); ++time_index )
              {
                std::vector<bill::lit_type> ored;
                for ( auto another_time_index = time_index; another_time_index < traces.at( trace_index ).first.length() - 1u; ++another_time_index )
                {
                  std::vector<bill::lit_type> enclosed;
                  for ( auto one_more_time_index = time_index; one_more_time_index < another_time_index; ++one_more_time_index )
                    enclosed.emplace_back( trace_lit( trace_index, one_child_index, one_more_time_index ) );
                  enclosed.emplace_back( trace_lit( trace_index, another_child_index, another_time_index ) );
                  ored.emplace_back( add_tseytin_and( enclosed ) );
                }
                auto const rhs = add_tseytin_or( ored );
                auto const eq = add_tseytin_equals( trace_lit( trace_index, root_index, time_index ), rhs );
                equals_part1.emplace_back( eq );
              }
              auto const part1 = add_tseytin_and( equals_part1 );

              /* part 2 */
              std::vector<bill::lit_type> equals_part2;
              for ( auto time_index = traces.at( trace_index ).first.prefix_length(); time_index < traces.at( trace_index ).first.length(); ++time_index )
              {
                std::vector<bill::lit_type> ored;
                for ( auto another_time_index = traces.at( trace_index ).first.prefix_length(); another_time_index < traces.at( trace_index ).first.length(); ++another_time_index )
                {
                  std::vector<bill::lit_type> enclosed;
                  if ( time_index < another_time_index )
                  {
                    for ( auto one_more_time_index = time_index; one_more_time_index < another_time_index - 1; ++one_more_time_index )
                      enclosed.emplace_back( trace_lit( trace_index, one_child_index, one_more_time_index ) );
                    enclosed.emplace_back( trace_lit( trace_index, another_child_index, another_time_index ) );
                    ored.emplace_back( add_tseytin_and( enclosed ) );
                  }
                  else
                  {
                    for ( auto one_more_time_index = traces.at( trace_index ).first.prefix_length(); one_more_time_index < another_time_index - 1; ++one_more_time_index )
                    {
                      enclosed.emplace_back( trace_lit( trace_index, one_child_index, one_more_time_index ) );
                    }
                    for ( auto one_more_time_index = time_index; one_more_time_index < traces.at( trace_index ).first.length() - 1; ++one_more_time_index )
                    {
                      enclosed.emplace_back( trace_lit( trace_index, one_child_index, one_more_time_index ) );
                    }
                    enclosed.emplace_back( trace_lit( trace_index, another_child_index, another_time_index ) );
                    ored.emplace_back( add_tseytin_and( enclosed ) );
                  }
                }
                auto const rhs = add_tseytin_or( ored );
                auto const eq = add_tseytin_equals( trace_lit( trace_index, root_index, time_index ), rhs );
                equals_part2.emplace_back( eq );
              }
              auto const part2 = add_tseytin_and( equals_part2 );

              auto const part1_and_part2 = add_tseytin_and( part1, part2 );
              add_clause( { ~t, part1_and_part2 } );
            }
          }
        }
      }
    }

    /* trace semantics */
    for ( auto trace_index = 0u; trace_index < traces.size(); ++trace_index )
    {
      if ( traces.at( trace_index ).second )
      {
        add_clause( { trace_lit( trace_index, num_nodes, 0u ) } );
      }
      else
      {
        add_clause( { ~trace_lit( trace_index, num_nodes, 0u ) } );
      }
    }
  }

  chain<std::string, std::vector<int>> extract_chain()
  {
    auto const model = _solver.get_model().model();

    chain<std::string, std::vector<int>> c;
    for ( auto node_index = 1u; node_index <= num_nodes; ++node_index )
    {
      std::optional<std::string> label;
      std::optional<int32_t> left;
      std::optional<int32_t> right;

      /* read label of current node */
      for ( auto label_index = 0u; label_index < num_labels; ++label_index )
      {
        if ( model.at( label_lit( node_index, label_index ).variable() ) == bill::lbool_type::true_ )
        {
          if ( label_index < num_propositions )
          {
            if ( verbose )
              std::cout << fmt::format( "[i] node = {} is labelled with proposition {}\n",
                                        node_index, label_index );

            assert( !label );
            label = fmt::format( "x{}", label_index );
          }
          else
          {
            if ( verbose )
              std::cout << fmt::format( "[i] node = {} is labelled with operator {}\n",
                                        node_index, label_index - num_propositions );

            assert( !label );

            /* convert operator label to string */
            auto const op_label = label_index;
            assert( op_label >= 0u );
            if ( op_label == operator_to_label[operator_opcode::not_] )
            {
              label = "~";
            }
            else if ( op_label == operator_to_label[operator_opcode::or_] )
            {
              label = "|";
            }
            else if ( op_label == operator_to_label[operator_opcode::next_] )
            {
              label = "X";
            }
            else if ( op_label == operator_to_label[operator_opcode::until_] )
            {
              label = "U";
            }
            else
            {
              assert( false );
            }
          }
        }
      }

      for ( auto child_index = 1u; child_index < node_index; ++child_index )
      {
        /* propositions have no children */
        if ( label->substr( 0, 1 ) == "x" ) continue;

        if ( model.at( left_lit( node_index, child_index ).variable() ) == bill::lbool_type::true_ )
        {
          if ( verbose )
            std::cout << fmt::format( "[i] node = {} has {} as left child\n",
                                      node_index, child_index );

          assert( !left );
          left = child_index;
        }

        /* unary operators have no right child */
        if ( label->substr( 0, 1 ) == "~" || label->substr( 0, 1 ) == "X" ) continue;

        if ( model.at( right_lit( node_index, child_index ).variable() ) == bill::lbool_type::true_ )
        {
          if ( verbose )
            std::cout << fmt::format( "[i] node = {} has {} as right child\n",
                                      node_index, child_index );

          assert( !right );
          right = child_index;
        }
      }

      if ( left && right )
      {
        c.add_step( *label, { *left, *right } );
      }
      else if ( left )
      {
        c.add_step( *label, { *left } );
      }
      else
      {
        c.add_step( *label, {} );
      }
    }
    return c;
  }

private:
  bill::lit_type label_lit( uint32_t node_index, uint32_t label_index ) const
  {
    return bill::lit_type( label_var_begin + ( node_index - 1u ) * num_labels + label_index,
                           bill::lit_type::polarities::positive );
  }

  bill::lit_type left_lit( uint32_t root_index, uint32_t child_index ) const
  {
    return bill::lit_type( structural_var_begin + ( root_index - 1u) * ( root_index - 2u ) + 2u * ( child_index - 1u ), bill::lit_type::polarities::positive );
  }

  bill::lit_type right_lit( uint32_t root_index, uint32_t child_index ) const
  {
    return bill::lit_type( structural_var_begin + ( root_index - 1u) * ( root_index - 2u ) + 2u * child_index - 1u, bill::lit_type::polarities::positive );
  }

  bill::lit_type trace_lit( uint32_t trace_index, uint32_t node_index, uint32_t time_index ) const
  {
    auto offset = 0u;
    for ( auto i = 0u; i < trace_index; ++i )
    {
      offset += traces.at( i ).first.length() * num_nodes;
    }
    return bill::lit_type( trace_var_begin + offset + ( node_index - 1 ) * traces.at( trace_index ).first.length() + time_index, bill::lit_type::polarities::positive );
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
    if ( verbose )
      std::cout << fmt::format( "[i] add_variable {}: {}\n", note, uint32_t( r ) );
    return bill::lit_type( r, pol );
  }

  void add_clause( std::vector<bill::lit_type> const& clause, std::string const& note = "")
  {
    if ( verbose )
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
  Solver& _solver;

  bool verbose;
  uint32_t num_propositions;
  std::vector<operator_opcode> ops;
  uint32_t num_labels;
  uint32_t num_nodes;
  uint32_t num_traces;
  std::vector<std::pair<trace, bool>> traces;

  /* operator to label */
  std::unordered_map<operator_opcode, uint32_t> operator_to_label;

  uint32_t label_var_begin;
  uint32_t label_var_end;
  uint32_t structural_var_begin;
  uint32_t structural_var_end;
  uint32_t trace_var_begin;
  uint32_t trace_var_end;
  uint32_t tseytin_var_begin;
  uint32_t tseytin_var_end;
}; /* ltl_encoder */

} /* copycat */
