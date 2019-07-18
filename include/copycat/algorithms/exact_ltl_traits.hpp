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
  \file exact_ltl_traits.hpp
  \brief Type traits for exact LTL encoder

  \author Heinz Riener
*/

#pragma once

namespace copycat
{

namespace detail
{

template <class T>
inline void hash_combine( uint64_t &seed, T const& v )
{
  std::hash<T> hasher;
  seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

} /* detail */
  
enum class operator_opcode : int32_t
{
  not_        = 0u,
  or_         = 1u,
  next_       = 2u,
  until_      = 3u,
  implies_    = 4u,
  and_        = 5u,
  eventually_ = 6u,
  globally_   = 7u,
}; /* operator_opcodes */

std::string operator_opcode_to_string( operator_opcode const& opcode )
{
  switch ( opcode )
  {
  case operator_opcode::not_:
    return "~";
  case operator_opcode::and_:
    return "&";
  case operator_opcode::or_:
    return "|";
  case operator_opcode::implies_:
    return "->";
  case operator_opcode::next_:
    return "X";
  case operator_opcode::until_:
    return "U";
  case operator_opcode::eventually_:
    return "F";
  case operator_opcode::globally_:
    return "G";
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
  case operator_opcode::eventually_:
  case operator_opcode::globally_:
    return 1;
  case operator_opcode::and_:
  case operator_opcode::or_:
  case operator_opcode::implies_:
  case operator_opcode::until_:
    return 2;
  default:
    break;
  }
  return 0;
}

} /* namespace copycat */
