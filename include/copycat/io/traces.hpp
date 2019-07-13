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
  \file read_traces.hpp
  \brief Parser for trace files

  \author Heinz Riener
*/

#pragma once

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <cassert>

namespace copycat
{

namespace trace_regex
{
  static std::regex separator_line( R"(^-+$)" );
} /* namespace trace_regex */

class trace_reader
{
public:
  trace_reader() = default;

  virtual void on_good_trace( std::vector<std::vector<int>> const& prefix,
                              std::vector<std::vector<int>> const& suffix ) const
  {
    (void)prefix;
    (void)suffix;
  }

  virtual void on_bad_trace( std::vector<std::vector<int>> const& prefix,
                             std::vector<std::vector<int>> const& suffix ) const
  {
    (void)prefix;
    (void)suffix;
  }

  virtual void on_operator( std::string const& op ) const
  {
    (void)op;
  }

  virtual void on_parameter( std::string const& parameter ) const
  {
    (void)parameter;
  }

  virtual void on_formula( std::string const& formula ) const
  {
    (void) formula;
  }
}; /* trace_reader */


class trace_parser
{
public:
  struct section
  {
    std::string name;
    uint32_t next;
  }; /* section */

public:
  explicit trace_parser() = default;

  uint32_t create_section( std::string name, uint32_t next )
  {
    auto const index = sections.size();
    sections.emplace_back( section{name,next} );
    return index;
  }

  std::string section_name( uint32_t section_index ) const
  {
    assert( section_index < sections.size() );
    return sections.at( section_index ).name;
  }

  uint32_t next_section( uint32_t section_index ) const
  {
    assert( section_index < sections.size() );
    return sections.at( section_index ).next;
  }

private:
  std::vector<section> sections;
};

namespace detail
{
  std::vector<std::string> split_string( std::string const& input_string, std::string const& delimiter )
  {
    uint64_t curr, prev = 0;
    curr = input_string.find( delimiter );

    std::vector<std::string> result;
    while ( curr != std::string::npos )
    {
      result.emplace_back( input_string.substr( prev, curr - prev ) );
      prev = curr + delimiter.size();
      curr = input_string.find( delimiter, prev );
    }
    result.emplace_back( input_string.substr( prev, curr - prev ) );

    return result;
  }

  std::string trim( std::string const& input_string, std::string const& whitespace = " \t" )
  {
    auto const beg = input_string.find_first_not_of( whitespace );
    if ( beg == std::string::npos )
      return std::string();

    auto const end = input_string.find_last_not_of( whitespace );
    auto const range = end - beg + 1u;
    return input_string.substr( beg, range );
  }

  std::vector<std::vector<int>> parse_trace( std::string const& trace_string )
  {
    std::vector<std::vector<int>> trace_data;
    auto const time_steps = split_string( trace_string, ";" );
    for ( const auto& time_step : time_steps )
    {
      auto const signals = split_string( time_step, "," );
      std::vector<int> signal_data;
      for ( auto i = 0u; i < signals.size(); ++i )
      {
        if ( signals.at( i ) == "1" )
        {
          signal_data.emplace_back( i + 1 );
        }
      }
      trace_data.emplace_back( signal_data );
    }
    return trace_data;
  }
} /* detail */

bool read_traces( std::istream& is, trace_reader const& reader )
{
  using trace_t = std::vector<std::vector<int>>;

  trace_parser parser;
  auto const good_section = parser.create_section( "Good traces", 1u );
  auto const bad_section = parser.create_section( "Bad traces", 2u );
  auto const operator_section = parser.create_section( "Operators", 3u );
  auto const parameter_section = parser.create_section( "Parameter", 4u );
  auto const verify_section = parser.create_section( "Verification", 5u );

  uint32_t curr_section = good_section;
  // std::cout << "===--- " << parser.section_name( curr_section ) << " ---===" << std::endl;

  std::string line;
  while ( std::getline( is, line ) )
  {
    if ( std::regex_match( line, trace_regex::separator_line ) )
    {
      curr_section = parser.next_section( curr_section );
      // std::cout << "===--- " << parser.section_name( curr_section ) << " ---===" << std::endl;
    }
    else
    {
      if ( curr_section == good_section )
      {
        /* split line by :: into prefix and suffix*/
        auto const affixes = detail::split_string( line, "::" );
        assert( affixes.size() <= 2u );

        auto const lasso_start = affixes.size() >= 2u ? std::atoi( affixes[1u].c_str() ) : 0u;
        trace_t const t = detail::parse_trace( affixes[0u] );
        assert( lasso_start < t.size() );

        if ( lasso_start > 0u )
        {
          trace_t prefix( t.begin(), t.begin() + lasso_start );
          trace_t suffix( t.begin() + lasso_start, t.end() );
          reader.on_good_trace( prefix, suffix );
        }
        else
        {
          reader.on_good_trace( {}, t );
        }
      }
      else if ( curr_section == bad_section )
      {
        /* split line by :: into prefix and suffix*/
        auto const affixes = detail::split_string( line, "::" );
        assert( affixes.size() <= 2u );

        auto const lasso_start = affixes.size() >= 2u ? std::atoi( affixes[1u].c_str() ) : 0u;
        trace_t const t = detail::parse_trace( affixes[0u] );
        assert( lasso_start < t.size() );

        if ( lasso_start > 0u )
        {
          trace_t prefix( t.begin(), t.begin() + lasso_start );
          trace_t suffix( t.begin() + lasso_start, t.end() );
          reader.on_bad_trace( prefix, suffix );
        }
        else
        {
          reader.on_bad_trace( {}, t );
        }
      }
      else if ( curr_section == operator_section )
      {
        auto operators = detail::split_string( line, "," );
        for ( const auto& op : operators )
          reader.on_operator( detail::trim( op ) );
      }
      else if ( curr_section == parameter_section )
      {
        reader.on_parameter( detail::trim( line ) );
      }
      else if ( curr_section == verify_section )
      {
        reader.on_formula( detail::trim( line ) );
      }
      else
      {
        std::cerr << "[e] unknown section: " << line << std::endl;
        return false;
      }
    }
  }
  return true;
}

bool read_traces( std::string const& filename, trace_reader const& reader )
{
  std::ifstream ifs( filename, std::ios::in );
  auto const result = read_traces( ifs, reader );
  ifs.close();
  return result;
}

} /* namespace copycat */
