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
  \file ltl.hpp
  \brief LTL formulae

  \author Heinz Riener
*/

#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace copycat
{

enum ltl_operator
{
  Constant = 0,
  Variable = 1,
  Or = 2,
  Next = 3,
  Eventually = 4,
  Until = 5,
  Releases = 6,
}; /* ltl_operator */

class ltl_node_pointer
{
public:
  using internal_type = uint32_t;

  ltl_node_pointer() = default;
  ltl_node_pointer( uint32_t index, uint32_t weight )
    : weight( weight )
    , index( index )
  {
  }

  union
  {
    struct
    {
      uint32_t weight : 1;
      uint32_t index  : 31;
    };
    uint32_t data;
  };

  bool operator==( ltl_node_pointer const& other ) const
  {
    return data == other.data;
  }
}; /* ltl_node_pointer */

class ltl_node
{
public:
  using pointer_type = ltl_node_pointer;

  std::array<pointer_type, 2u> children;
  std::array<uint32_t, 2u> data;

  bool operator==( ltl_node const &other ) const
  {
    return children == other.children;
  }
}; /* ltl_node */

} /* namespace copycat */

namespace std
{

template<>
struct hash<copycat::ltl_node>
{
  uint64_t operator()( copycat::ltl_node const &node ) const noexcept
  {
    uint64_t seed = -2011;
    seed += node.children[0u].index * 7937;
    seed += node.children[1u].index * 2971;
    seed += node.children[0u].weight * 911;
    seed += node.children[1u].weight * 353;
    seed += node.data[0u] * 911;
    seed += node.data[1u] * 353;
    return seed;
  }
};

} /* namespace std */

namespace copycat
{

class ltl_storage
{
public:
  ltl_storage()
  {
    nodes.reserve( 10000u );
    hash.reserve( 10000u );

    /* first node reserved for a constant */
    nodes.emplace_back();
    nodes[0].children[0].data = ltl_operator::Constant;
    nodes[0].children[1].data = 0;
  }

  using node_type = ltl_node;

  std::vector<node_type> nodes;
  std::vector<uint32_t> inputs;
  std::vector<typename node_type::pointer_type> outputs;

  std::unordered_map<node_type, uint32_t> hash;

  uint32_t num_pis{0};
}; /* ltl_storage */

} /* namespace copycat */

namespace copycat
{

class ltl_formula_store
{
public:
  using node = uint32_t;

  struct ltl_formula
  {
    ltl_formula() = default;

    ltl_formula( uint32_t index, uint32_t complement )
      : complement( complement )
      , index( index )
    {
    }

    explicit ltl_formula( uint32_t data )
      : data( data )
    {
    }

    ltl_formula( ltl_storage::node_type::pointer_type const& p )
      : complement( p.weight )
      , index( p.index )
    {
    }

    union {
      struct
      {
        uint32_t complement :  1;
        uint32_t index      : 31;
      };
      uint32_t data;
    };

    ltl_formula operator!() const
    {
      return ltl_formula( data ^ 1 );
    }

    ltl_formula operator+() const
    {
      return {index, 0};
    }

    ltl_formula operator-() const
    {
      return {index, 1};
    }

    ltl_formula operator^( bool complement ) const
    {
      return ltl_formula( data ^ ( complement ? 1 : 0 ) );
    }

    bool operator==( ltl_formula const& other ) const
    {
      return data == other.data;
    }

    bool operator!=( ltl_formula const& other ) const
    {
      return data != other.data;
    }

    bool operator<( ltl_formula const& other ) const
    {
      return data < other.data;
    }

    operator ltl_storage::node_type::pointer_type() const
    {
      return { index, complement };
    }
  }; /* ltl_formula */

public:
  ltl_formula_store()
    : storage( std::make_shared<ltl_storage>() )
  {
  }

  ltl_formula_store( std::shared_ptr<ltl_storage> const& storage )
    : storage( storage )
  {
  }

  uint32_t num_variables() const
  {
    return storage->inputs.size();
  }

  uint32_t num_formulas() const
  {
    return storage->outputs.size();
  }

  uint32_t num_nodes() const
  {
    return storage->nodes.size();
  }

  ltl_formula get_constant( bool value ) const
  {
    return {0, static_cast<uint32_t>( value ? 1 : 0 ) };
  }

  ltl_formula create_variable()
  {
    const auto index = node( storage->nodes.size() );
    auto& node = storage->nodes.emplace_back();
    node.data[0] = ltl_operator::Variable;
    node.data[1] = storage->inputs.size();

    storage->inputs.emplace_back( index );
    ++storage->num_pis;
    return {index,0};
  }

  void create_formula( ltl_formula const& a )
  {
    storage->outputs.push_back( a );
  }

  ltl_formula create_or( ltl_formula a, ltl_formula b )
  {
    /* order inputs */
    if ( a.index > b.index )
    {
      std::swap( a, b );
    }

    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? a : get_constant( true );
    }
    else if ( a.index == 0 )
    {
      return a.complement ? get_constant( true ) : b;
    }

    ltl_storage::node_type n;
    n.children[0] = a;
    n.children[1] = b;
    n.data[0] = ltl_operator::Or;
    n.data[1] = 0;

    const auto it = storage->hash.find( n );
    if ( it != std::end( storage->hash ) )
    {
      return {it->second, 0};
    }

    const auto index = node( storage->nodes.size() );
    if ( index >= .9 * storage->nodes.capacity() )
    {
      storage->nodes.reserve( uint32_t( 3.1415f * index ) );
      storage->hash.reserve( uint32_t( 3.1415f * index ) );
    }

    storage->nodes.push_back( n );
    storage->hash[n] = index;

    return {index,0};
  }

  ltl_formula create_next( ltl_formula const& a )
  {
    /* trivial cases */
    if ( a.index == 0 )
    {
      return a.complement ? get_constant( true ) : get_constant( false );
    }

    ltl_storage::node_type n;
    n.children[0] = a;
    n.children[1] = {0,0};
    n.data[0] = ltl_operator::Next;
    n.data[1] = 0;

    const auto it = storage->hash.find( n );
    if ( it != std::end( storage->hash ) )
    {
      return {it->second, 0};
    }

    const auto index = node( storage->nodes.size() );
    if ( index >= .9 * storage->nodes.capacity() )
    {
      storage->nodes.reserve( uint32_t( 3.1415f * index ) );
      storage->hash.reserve( uint32_t( 3.1415f * index ) );
    }

    storage->nodes.push_back( n );
    storage->hash[n] = index;

    return {index,0};
  }

  ltl_formula create_until( ltl_formula const& a, ltl_formula const& b )
  {
    ltl_storage::node_type n;
    n.children[0] = a;
    n.children[1] = b;
    n.data[0] = ltl_operator::Until;
    n.data[1] = 0;

    const auto it = storage->hash.find( n );
    if ( it != std::end( storage->hash ) )
    {
      return {it->second, 0};
    }

    const auto index = node( storage->nodes.size() );
    if ( index >= .9 * storage->nodes.capacity() )
    {
      storage->nodes.reserve( uint32_t( 3.1415f * index ) );
      storage->hash.reserve( uint32_t( 3.1415f * index ) );
    }

    storage->nodes.push_back( n );
    storage->hash[n] = index;

    return {index,0};
  }

  ltl_formula create_releases( ltl_formula const& a, ltl_formula const& b )
  {
    ltl_storage::node_type n;
    n.children[0] = a;
    n.children[1] = b;
    n.data[0] = ltl_operator::Releases;
    n.data[1] = 0;

    const auto it = storage->hash.find( n );
    if ( it != std::end( storage->hash ) )
    {
      return {it->second, 0};
    }

    const auto index = node( storage->nodes.size() );
    if ( index >= .9 * storage->nodes.capacity() )
    {
      storage->nodes.reserve( uint32_t( 3.1415f * index ) );
      storage->hash.reserve( uint32_t( 3.1415f * index ) );
    }

    storage->nodes.push_back( n );
    storage->hash[n] = index;

    return {index,0};
  }

  ltl_formula create_eventually( ltl_formula const &a )
  {
    /* trivial cases */
    if ( a.index == 0 )
    {
      return a.complement ? get_constant( true ) : get_constant( false );
    }

    ltl_storage::node_type n;
    n.children[0] = a;
    n.children[1] = {0,0};
    n.data[0] = ltl_operator::Eventually;
    n.data[1] = 0;

    const auto it = storage->hash.find( n );
    if ( it != std::end( storage->hash ) )
    {
      return {it->second, 0};
    }

    const auto index = node( storage->nodes.size() );
    if ( index >= .9 * storage->nodes.capacity() )
    {
      storage->nodes.reserve( uint32_t( 3.1415f * index ) );
      storage->hash.reserve( uint32_t( 3.1415f * index ) );
    }

    storage->nodes.push_back( n );
    storage->hash[n] = index;

    return {index,0};
  }

  ltl_formula eventually( ltl_formula const &a )
  {
    /* F(a) = (true)U(a) */
    return create_until( get_constant( true ), a );
  }

  ltl_formula create_globally( ltl_formula const& a )
  {
    /* G(a) = !F(!(a)) */
    return !create_eventually( !a );
  }

  ltl_formula globally( ltl_formula const& a )
  {
    /* G(a) = !F(!(a)) */
    return !eventually( !a );
  }

  bool is_constant( node const& n ) const
  {
    assert( storage->nodes[0].data[0] == ltl_operator::Constant );
    return n == 0;
  }

  bool is_variable( node const& n ) const
  {
    return storage->nodes[n].data[0] == ltl_operator::Variable &&
           storage->nodes[n].data[1] < static_cast<uint32_t>(storage->num_pis);
  }

  bool is_or( node const& n ) const
  {
    return storage->nodes[n].data[0] == ltl_operator::Or;
  }

  bool is_next( node const& n ) const
  {
    return storage->nodes[n].data[0] == ltl_operator::Next;
  }

  bool is_eventually( node const& n ) const
  {
    return storage->nodes[n].data[0] == ltl_operator::Eventually;
  }

  bool is_until( node const& n ) const
  {
    return storage->nodes[n].data[0] == ltl_operator::Until;
  }

  bool is_releases( node const& n ) const
  {
    return storage->nodes[n].data[0] == ltl_operator::Releases;
  }

  node get_node( ltl_formula const& f ) const
  {
    return f.index;
  }

  ltl_formula make_formula( node const& n ) const
  {
    return ltl_formula( n, 0 );
  }

  bool is_complemented( ltl_formula const& f ) const
  {
    return f.complement;
  }

  uint32_t node_to_index( node const& n ) const
  {
    return static_cast<uint32_t>( n );
  }

  node index_to_node( uint32_t index ) const
  {
    return index;
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    for ( auto i = 0u; i < 2u; ++i )
    {
      fn( ltl_formula{storage->nodes[n].children[i]}, i );
    }
  }

  template<typename Fn>
  void foreach_formula( Fn&& fn ) const
  {
    auto begin = storage->outputs.begin(), end = storage->outputs.end();
    while ( begin != end )
    {
      if ( !fn( *begin++ ) )
      {
        return;
      }
    }
  }

protected:
  std::shared_ptr<ltl_storage> storage;
}; /* ltl_formula_store */

} /* namespace copycat */

namespace std
{

template<>
struct hash<copycat::ltl_formula_store::ltl_formula>
{
  uint64_t operator()( copycat::ltl_formula_store::ltl_formula const &f ) const noexcept
  {
    uint64_t k = f.data;
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
  }
}; /* hash */

} /* namespace std */
