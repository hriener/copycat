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
  \file protocol_extractor.hpp
  \brief A protocol extraction engine
  \author Heinz Riener
*/

#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <unordered_map>

namespace std
{

template<>
struct hash<std::tuple<std::string,std::string>>
{
  using argument_type = std::tuple<std::string,std::string>;
  using result_type = std::size_t;

  result_type operator()( argument_type const& g ) const noexcept {
    result_type const h1 ( std::hash<std::string>{}( std::get<0>( g ) ) );
    result_type const h2 ( std::hash<std::string>{}( std::get<1>( g ) ) );
    return h1 ^ ( h2 << 1 );
  }
}; /* std::hash<vertex> */

} /* namespace std */

namespace copycat
{

/* ***** Definition of protocol graph ***** */
struct vertex
{
  bool operator==( vertex const &other ) const
  {
    return input == other.input && output == other.output;
  }

  std::string input;
  std::string output;

  std::vector<uint32_t> parents;
  std::vector<uint32_t> children;
}; /* vertex */

class protocol_graph
{
public:
  using index_t = uint32_t;
  using hash_key_t = std::tuple<std::string, std::string>;
  
  explicit protocol_graph()
  {
  }

  index_t get_root()
  {
    auto index = add_node( "root", "root" );
    assert( index == 0u );
    return index;
  }

  index_t add_node( std::string const &input, std::string const &output )
  {
    auto const key = hash_key_t{input,output};
    auto const it = node_map.find( key );
    if ( it == node_map.end() )
    {
      auto const index = nodes.size();
      nodes.emplace_back( vertex{input,output,{},{}} );
      node_map.emplace( key, index );
      return index;
    }
    else
    {
      return it->second;
    }
  }

  void add_edge( index_t source, index_t target )
  {
    nodes[source].children.emplace_back( target );
    nodes[target].parents.emplace_back( source );
  }

  void write_dot()
  {
    assert( false && "yet not implemented" );
  }

  uint32_t size() const
  {
    return nodes.size();
  }

protected:
  std::vector<vertex> nodes;
  std::unordered_map<hash_key_t,index_t> node_map;
}; /* protocol_graph */


} /* namespace copycat */

