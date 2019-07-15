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
  \file ltl_synthesis_spec_reader.hpp
  \brief Reader for LTL synthesis specifications

  \author Heinz Riener
*/

#pragma once

#include <copycat/io/traces.hpp>
#include <copycat/trace.hpp>
#include <string>
#include <vector>
#include <iostream>

namespace copycat
{

struct ltl_synthesis_spec
{
  std::string name;
  std::vector<trace> good_traces;
  std::vector<trace> bad_traces;
  std::vector<operator_opcode> operators;
  std::vector<std::string> parameters;
  std::vector<std::string> formulas;
  uint32_t num_propositions = 0u;
}; /* ltl_synthesis_spec */

class ltl_synthesis_spec_reader : public trace_reader
{
public:
  explicit ltl_synthesis_spec_reader( ltl_synthesis_spec& spec )
    : _spec( spec )
  {
  }

  ~ltl_synthesis_spec_reader()
  {
    /* if no operators have been specified, then enable everything */
    if ( _spec.operators.size() == 0u )
    {
      _spec.operators.emplace_back( copycat::operator_opcode::not_ );
      _spec.operators.emplace_back( copycat::operator_opcode::next_ );
      _spec.operators.emplace_back( copycat::operator_opcode::and_ );
      _spec.operators.emplace_back( copycat::operator_opcode::or_ );
      _spec.operators.emplace_back( copycat::operator_opcode::implies_ );
      _spec.operators.emplace_back( copycat::operator_opcode::until_ );
      _spec.operators.emplace_back( copycat::operator_opcode::eventually_ );
      _spec.operators.emplace_back( copycat::operator_opcode::globally_ );
    }
  }

  virtual void set_num_propositions( uint32_t num_propositions ) const override
  {
    _spec.num_propositions = num_propositions;
  }

  void on_good_trace( std::vector<std::vector<int>> const& prefix,
                      std::vector<std::vector<int>> const& suffix ) const override
  {
    trace t;
    for ( const auto& p : prefix )
      t.emplace_prefix( p );
    for ( const auto& s : suffix )
      t.emplace_suffix( s );
    _spec.good_traces.emplace_back( t );
  }

  void on_bad_trace( std::vector<std::vector<int>> const& prefix,
                     std::vector<std::vector<int>> const& suffix ) const override
  {
    trace t;
    for ( const auto& p : prefix )
      t.emplace_prefix( p );
    for ( const auto& s : suffix )
      t.emplace_suffix( s );
    _spec.bad_traces.emplace_back( t );
  }

  void on_operator( std::string const& op ) const override
  {
    if ( op == "!" )
      _spec.operators.emplace_back( operator_opcode::not_ );
    else if ( op == "&" )
      _spec.operators.emplace_back( operator_opcode::and_ );
    else if ( op == "|" )
      _spec.operators.emplace_back( operator_opcode::or_ );
    else if ( op == "->" )
      _spec.operators.emplace_back( operator_opcode::implies_ );
    else if ( op == "X" )
      _spec.operators.emplace_back( operator_opcode::next_ );
    else if ( op == "U" )
      _spec.operators.emplace_back( operator_opcode::until_ );
    else if ( op == "F" )
      _spec.operators.emplace_back( operator_opcode::eventually_ );
    else if ( op == "G" )
      _spec.operators.emplace_back( operator_opcode::globally_ );
    else
      std::cout << fmt::format( "[w] unsupported operator `{}'\n", op );
  }

  void on_parameter( std::string const& parameter ) const override
  {
    _spec.parameters.emplace_back( parameter );
  }

  void on_formula( std::string const& formula ) const override
  {
    _spec.formulas.emplace_back( formula );
  }

private:
  ltl_synthesis_spec& _spec;
}; /* ltl_synthesis_spec_reader */

/*! \brief Read LTL synthesis spec from an input stream */
bool read_ltl_synthesis_spec( std::istream& is, ltl_synthesis_spec& spec )
{
  return read_traces( is, ltl_synthesis_spec_reader( spec ) );
}

/*! \brief Read LTL synthesis spec from a file */
bool read_ltl_synthesis_spec( std::string const& filename, ltl_synthesis_spec& spec )
{
  return read_traces( filename, ltl_synthesis_spec_reader( spec ) );
}

} /* copycat */
