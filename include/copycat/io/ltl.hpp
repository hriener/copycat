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
  \brief LTL reader

  \author Heinz Riener
*/

#pragma once

#include <copycat/ltl.hpp>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace copycat
{

namespace detail
{

template<typename Data>
struct ast_node
{
public:
  bool operator==( ast_node const& other ) const
  {
    if ( data != other.data )
      return false;

    if ( children.size() != other.children.size() )
      return false;

    for ( auto i = 0u; i < children.size(); ++i )
    {
      if ( children.at( i ) != other.children.at( i ) )
        return false;
    }
    return true;
  }

public:
  Data data;
  std::vector<uint32_t> children;
}; /* ast_node */

template<typename Data>
class ast
{
public:
  explicit ast() = default;

  uint32_t create_node( Data const& data, std::vector<uint32_t> const& children )
  {
    ast_node<Data> node{data,children};

    auto const it = hash.find( node );
    if ( it != hash.end() )
      return it->second;

    auto const id = nodes.size();
    nodes.emplace_back( node );
    hash.emplace( node, id );
    return id;
  }

  uint32_t num_children( uint32_t node ) const
  {
    return nodes.at( node ).children.size();
  }

  uint32_t get_child( uint32_t node, uint32_t index ) const
  {
    return nodes.at( node ).children.at( index );
  }

  Data get_data( uint32_t node ) const
  {
    return nodes.at( node ).data;
  }

protected:
  std::vector<ast_node<Data>> nodes;
  std::unordered_map<ast_node<Data>, uint32_t> hash;
};

} /* namespace detail */

} /* namespace copycat */

namespace std
{

template<typename Data>
struct hash<copycat::detail::ast_node<Data>>
{
  uint64_t operator()( copycat::detail::ast_node<Data> const &node ) const noexcept
  {
    uint64_t seed = std::hash<Data>()( node.data );
    for ( const auto c : node.children )
    {
      seed += std::hash<uint32_t>()( c );
    }
    return seed;
  }
};

} /* namespace std */

namespace copycat
{

enum class return_code
{
  success = 0,
  parse_error
}; /* return_code */

enum class token_kind
{
  eof = 0,
  name = 1,
  op = 2,
  lparan = 3, // (
  rparan = 4, // )
  lexem_error = 5,
}; /* token_kind */

struct token
{
public:
  bool operator==( token const& other ) const
  {
    return lexem == other.lexem && kind == other.kind;
  }

  bool operator!=( token const& other ) const
  {
    return !this->operator==( other );
  }

public:
  std::string lexem;
  token_kind kind;
}; /* token */

} /* namespace copycat */

namespace std
{

template<>
struct hash<copycat::token>
{
  uint64_t operator()( copycat::token const &t ) const noexcept
  {
    uint64_t seed = std::hash<std::string>()( t.lexem );
    seed += std::hash<uint32_t>()( uint32_t(t.kind) );
    return seed;
  }
};

} /* namespace std */

namespace copycat
{

/*
 * note that the lexems provided in this map are required not share
 * any common prefixes
 */
inline std::map<std::string, token_kind> lexem_to_token_kind_map =
  {{std::string{"("},  token_kind::lparan},
   {std::string{")"},  token_kind::rparan},
   {std::string{"~"},  token_kind::op},
   {std::string{"*"},  token_kind::op},
   {std::string{"+"},  token_kind::op},
   {std::string{"!"},  token_kind::op},
   {std::string{"&"},  token_kind::op},
   {std::string{"|"},  token_kind::op},
   {std::string{"->"}, token_kind::op},
   {std::string{"X"},  token_kind::op},
   {std::string{"G"},  token_kind::op},
   {std::string{"F"},  token_kind::op},
   {std::string{"U"},  token_kind::op},
   {std::string{"R"},  token_kind::op}};

/*! \brief Lexicographical analyzer for LTL
 *
 * Diassembles an LTL formula read from an istream into tokens.
 *
 */
class ltl_lexer
{
public:
  explicit ltl_lexer( std::istream& in )
    : _in( in )
  {}

  token operator()()
  {
    uint8_t comment_mode = 0u;
    uint8_t op_mode = 0u;
    std::string lexem;

    bool okay;
    char c;
    while ( ( okay = bool( _in.get( c ) ) ) )
    {
      /* begin parse comments */
      if ( ( c == '/' && comment_mode == 0 ) ||
           ( c == '*' && comment_mode == 1 ) )
      {
        ++comment_mode;
        lexem.push_back( c );
        continue;
      }
      else if ( comment_mode == 1 )
      {
        comment_mode = 0;
      }

      if ( c == '*' && comment_mode == 2 )
      {
        ++comment_mode;
        lexem.push_back( c );
        continue;
      }
      else if ( c == '/' && comment_mode == 3 )
      {
        lexem.push_back( c );
        // std::cout << lexem << std::endl;
        comment_mode = 0;
        lexem = "";
        continue;
      }
      /* end parsing */

      /* ignore all whitespaces (at the beginning of a lexem) */
      if ( !lexem.empty() && op_mode == 0 && is_whitespace( c ) )
        break;

      bool next = false;
      for ( const auto& tok : lexem_to_token_kind_map )
      {
        if ( std::string{c} == tok.first )
        {
          if ( lexem.empty() )
          {
            return token{tok.first,tok.second};
          }
          else
          {
            _in.putback( c );
            return token{lexem,token_kind::name};
          }
        }
        else if ( c == tok.first[0] )
        {
          if ( lexem.empty() )
          {
            lexem.push_back( c );
            next = true;
            break;
          }
          else
          {
            _in.putback( c );
            return token{lexem,token_kind::name};
          }
        }
        else if ( lexem == tok.first )
        {
          _in.putback( c );
          return token{lexem,token_kind::name};
        }
      }

      if ( next )
        continue;

      if ( !is_whitespace( c ) )
        lexem.push_back( c );
    }

    if ( lexem.empty() )
    {
      return token{"", token_kind::eof};
    }
    else
    {
      auto const l = lexem.at( 0 );
      if ( ( l >= 'a' && l <= 'z' ) || ( l >= 'A' && l <= 'Z' ) || l == '_' )
      {
        return token{lexem, token_kind::name};
      }
      else
      {
        return token{lexem, token_kind::lexem_error};
      }
    }
  }

  bool is_whitespace( char c ) const
  {
    return ( c == ' ' || c == '\n' || c == '\\' );
  }

protected:
  std::istream& _in;
}; /* ltl_lexer */

/*! \brief Parser for LTL
 *
 * Translates an LTL formula into an abstract syntax tree.
 *
 */
template<class Lexer>
class ltl_parser
{
public:
  using ast = detail::ast<token>;
  using ast_id = uint32_t;

public:
  ltl_parser( ast& ast_store, Lexer& lexer )
    : _ast_store( ast_store )
    , _lexer( lexer )
  {}

  token get_token( uint32_t pos = 0 )
  {
    if ( _tokens.size() > pos )
    {
      return _tokens.at( pos );
    }
    while ( _tokens.size() <= pos )
    {
      token tok = _lexer();
      _tokens.push_back( tok );
    }
    return _tokens.at( pos );
  }

  void pop_token()
  {
    _tokens.pop_front();
  }

  ast_id parse_formula()
  {
    return parse_formula_recur();
  }

  ast_id parse_formula_recur()
  {
    ast_id left;

    token tok0 = get_token();
    if ( tok0.lexem == "(" )
    {
      pop_token();

      ast_id child = parse_formula_recur();

      token tok1 = get_token();
      if ( tok1.lexem != ")" )
      {
        // std::cerr << "[e] expected closing )" << std::endl;
        error = true;
      }
      pop_token();

      left = _ast_store.create_node( tok0, { child } );
    }
    else if ( tok0.lexem == "G" ||
              tok0.lexem == "F" ||
              tok0.lexem == "X" ||
              tok0.lexem == "~" ||
              tok0.lexem == "!" )
    {
      pop_token();

      ast_id child = parse_formula_recur();
      left = _ast_store.create_node( tok0, { child } );
    }
    else if ( tok0.kind == token_kind::name )
    {
      left = _ast_store.create_node( tok0, {} );
      pop_token();
    }
    else
    {
      // std::cerr << "[e] unexpected token " << tok0.lexem << std::endl;
      error = true;
    }

    token tok1 = get_token();
    if ( tok1.lexem == "U" || tok1.lexem == "R" ||
         tok1.lexem == "*" || tok1.lexem == "+" ||
         tok1.lexem == "&" || tok1.lexem == "|" ||
         tok1.lexem == "->" )
    {
      pop_token();
      ast_id right = parse_formula_recur();
      return _ast_store.create_node( tok1, { left, right } );
    }
    else
    {
      return left;
    }
  }

  bool successful()
  {
    return !error && get_token().kind == token_kind::eof;
  }

protected:
  ast& _ast_store;
  Lexer _lexer;
  std::deque<token> _tokens;
  bool error = false;
}; /* ltl_parser */

/*! \brief LTL reader
 *
 * Reader visitor for LTL formulas.
 *
 */
class ltl_reader
{
public:
  explicit ltl_reader() = default;

  virtual void on_proposition( uint32_t id, std::string const& name ) const
  {
    (void)id;
    (void)name;
  }

  virtual void on_unary_op( uint32_t id, std::string const& op, uint32_t child_id ) const
  {
    (void)id;
    (void)op;
    (void)child_id;
  }

  virtual void on_binary_op( uint32_t id, std::string const& op, uint32_t child0_id, uint32_t child1_id ) const
  {
    (void)id;
    (void)op;
    (void)child0_id;
    (void)child1_id;
  }

  virtual void on_formula( uint32_t id ) const
  {
    (void)id;
  }
}; /* ltl_reader */

class ltl_pretty_printer : public copycat::ltl_reader
{
public:
  explicit ltl_pretty_printer() = default;

  void on_proposition( uint32_t id, std::string const& op ) const override
  {
    std::cout << id << " := " << op << std::endl;
  }

  void on_unary_op( uint32_t id, std::string const& op, uint32_t child_id ) const override
  {
    std::cout << id << " := " << op << ' ' << child_id << std::endl;
  }

  void on_binary_op( uint32_t id, std::string const& op, uint32_t child0_id, uint32_t child1_id ) const override
  {
    std::cout << id << " := " << op << ' ' << child0_id << ' ' << child1_id << std::endl;
  }

  void on_formula( uint32_t id ) const override
  {
    std::cout << "FORMULA( " << id << " )" << std::endl;
  }
}; /* ltl_pretty_printer */

/*! \brief Apply LTL reader to abstract syntax tree
 *
 * Applies an LTL reader to an abstract syntax tree generated by LTL parser
 *
 */
class apply_ltl_reader
{
public:
  using ast = detail::ast<token>;
  using ast_node = detail::ast_node<token>;
  using ast_id = uint32_t;

public:
  explicit apply_ltl_reader( ast const& ast_store, ltl_reader const& reader )
    : _ast_store( ast_store )
    , _reader( reader )
  {}

  void operator()( ast_id const& n )
  {
    apply_recursive( n );
    _reader.on_formula( n );
  }

  void apply_recursive( ast_id const& n )
  {
    auto const num_children = _ast_store.num_children( n );
    switch ( num_children )
    {
    case 0u:
      _reader.on_proposition( n, _ast_store.get_data( n ).lexem );
      break;
    case 1u:
      apply_recursive( _ast_store.get_child( n, 0u ) );
      _reader.on_unary_op( n, _ast_store.get_data( n ).lexem, _ast_store.get_child( n, 0u ) );
      break;
    case 2u:
      apply_recursive( _ast_store.get_child( n, 0u ) );
      apply_recursive( _ast_store.get_child( n, 1u ) );
      _reader.on_binary_op( n, _ast_store.get_data( n ).lexem, _ast_store.get_child( n, 0u ), _ast_store.get_child( n, 1u ) );
      break;
    default:
      assert( false );
    }
  }

protected:
  ast const& _ast_store;
  ltl_reader const& _reader;
}; /* apply_ltl_reader */

inline return_code read_ltl( std::istream& in, ltl_reader const& reader )
{
  std::string line;
  while ( std::getline( in, line ) )
  {
    std::stringstream ss( line );
    detail::ast<token> ast_store;
    ltl_lexer lexer( ss );
    ltl_parser parser( ast_store, lexer );

    auto const f = parser.parse_formula();
    if ( parser.successful() )
    {
      apply_ltl_reader( ast_store, reader )( f );
    }
    else
    {
      return return_code::parse_error;
    }
  }
  return return_code::success;
}

inline return_code read_ltl( std::string const& filename, const ltl_reader& reader )
{
  std::ifstream in( filename, std::ifstream::in );
  return read_ltl( in, reader );
}

} // namespace copycat
