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
  \file bool3.hpp
  \brief Three-valued Boolean

  \author Heinz Riener
*/

#pragma once

#include <string>

namespace copycat
{

class bool3
{
public:
  struct inconclusive_t {};

public:
  bool3()
    : value( false_value )
  {
  }

  bool3( bool b )
    : value( b ? true_value : false_value )
  {
  }

  explicit bool3( inconclusive_t const& )
    : value( inconclusive_value )
  {
  }

  bool operator==( bool3 const& other ) const
  {
    return value == other.value;
  }

  bool operator!=( bool3 const& other ) const
  {
    return !this->operator==( other );
  }

  bool operator<( bool3 const& other ) const
  {
    return value < other.value;
  }

  bool operator>( bool3 const& other ) const
  {
    return value > other.value;
  }

  bool operator<=( bool3 const& other ) const
  {
    return !operator>( other );
  }

  bool operator>=( bool3 const& other ) const
  {
    return !operator<( other );
  }

  bool3 operator!() const
  {
    switch ( value )
    {
    default:
    case inconclusive_value:
      return bool3(inconclusive_t());
    case false_value:
      return true;
    case true_value:
      return false;
    }
  }

  bool3 operator&( bool3 const& other ) const
  {
    return std::min( *this, other );
  }

  bool3 operator&&( bool3 const& other ) const
  {
    if ( is_false() )
      return false;

    return ( *this & other );
  }

  bool3 operator|( bool3 const& other ) const
  {
    return std::max( *this, other );
  }

  bool3 operator||( bool3 const& other ) const
  {
    if ( is_true() )
      return true;

    return ( *this | other );
  }

  bool is_false() const
  {
    return ( value == false_value );
  }

  bool is_true() const
  {
    return ( value == true_value );
  }

  bool is_inconclusive() const
  {
    return ( value == inconclusive_value );
  }

  std::string to_string() const
  {
    switch ( value )
    {
    default:
    case inconclusive_value:
      return "?";
    case false_value:
      return "0";
    case true_value:
      return "1";
    }
  }

protected:
  enum value_t {
    false_value = -1,
    inconclusive_value = 0,
    true_value = 1,
  } value;
}; /* bool3 */

inline bool3 inconclusive3{bool3::inconclusive_t()};

} /* namespace copycat */
