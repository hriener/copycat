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
  \file extended_inttype.hpp
  \brief Integer type extended with infinite and impossible

  \author Heinz Riener
*/

#pragma once

namespace copycat
{

template<typename IntType>
class extended_inttype
{
public:
  struct infinite_t {};
  struct impossible_t {};

public:
  static extended_inttype<IntType> impossible()
  {
    return extended_inttype<IntType>( impossible_t() );
  }

  static extended_inttype<IntType> infinite()
  {
    return extended_inttype<IntType>( infinite_t() );
  }

public:
  extended_inttype( uint32_t value )
    : value( value )
    , extension( normal_value )
  {
  }

  explicit extended_inttype( infinite_t const& )
    : value( 0 )
    , extension( infinite_value )
  {
  }

  explicit extended_inttype( impossible_t const& )
    : value( 0 )
    , extension( impossible_value )
  {
  }

  bool operator==( extended_inttype<IntType> const& other ) const
  {
    return value == other.value && extension == other.extension;
  }

  bool operator!=( extended_inttype<IntType> const& other ) const
  {
    return !this->operator==( other );
  }

  bool operator<( extended_inttype<IntType> const& other ) const
  {
    /* compare values if both types are normal */
    if ( is_normal() && other.is_normal() )
    {
      return value < other.value;
    }
    /* compare extensions otherwise */
    else
    {
      return extension < other.extension;
    }
  }

  bool operator>( extended_inttype<IntType> const& other ) const
  {
    /* compare values if both types are normal */
    if ( is_normal() && other.is_normal() )
    {
      return value > other.value;
    }
    /* compare extensions otherwise */
    else
    {
      return extension > other.extension;
    }
  }

  bool operator<=( extended_inttype<IntType> const& other ) const
  {
    return !this->operator<( other );
  }

  bool operator>=( extended_inttype<IntType> const& other ) const
  {
    return !this->operator>( other );
  }

  bool is_normal() const
  {
    return extension == normal_value;
  }

  bool is_infinite() const
  {
    return extension == infinite_value;
  }

  bool is_impossible() const
  {
    return extension == impossible_value;
  }

  extended_inttype<IntType> operator+( extended_inttype<IntType> const& other ) const
  {
    if ( is_normal() && other.is_normal() )
    {
      return extended_inttype<IntType>( value + other.value );
    }

    switch ( std::max( extension, other.extension ) )
    {
    default:
    case impossible_value:
      return extended_inttype<IntType>( impossible_t() );
    case infinite_value:
      return extended_inttype<IntType>( infinite_t() );
    }
  }

public:
  IntType value;

  enum extension_t {
    normal_value = -1,
    infinite_value = 0,
    impossible_value = 1,
  } extension;
}; /* extended_inttype */

using euint32_t = extended_inttype<uint32_t>;

template<typename IntType>
class extended_inttype_pair
{
public:
  explicit extended_inttype_pair( extended_inttype<IntType> const& s, extended_inttype<IntType> const& f )
    : s( s )
    , f( f )
  {
  }

  bool operator==( extended_inttype_pair<IntType> const& other ) const
  {
    return s == other.s && f == other.f;
  }

  bool operator!=( extended_inttype_pair<IntType> const& other ) const
  {
    return !( this->operator==( other ) );
  }

  extended_inttype_pair<IntType> swap() const
  {
    return extended_inttype_pair( f, s );
  }

  extended_inttype_pair<IntType> increment() const
  {
    return extended_inttype_pair( s + 1, f + 1 );
  }

  extended_inttype_pair<IntType> minmax( extended_inttype_pair<IntType> const& other ) const
  {
    return extended_inttype_pair( std::min( s, other.s ), std::max( f, other.f ) );
  }

  extended_inttype_pair<IntType> maxmin( extended_inttype_pair<IntType> const& other ) const
  {
    return extended_inttype_pair( std::max( s, other.s ), std::min( f, other.f ) );
  }

public:
  extended_inttype<IntType> s;
  extended_inttype<IntType> f;
}; /* extended_inttype_pair */

using euint32_pair = extended_inttype_pair<uint32_t>;

} /* namespace copycat */
