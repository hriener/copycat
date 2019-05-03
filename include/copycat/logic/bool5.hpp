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
  \file bool5.hpp
  \brief Five-valued Boolean

  \author Heinz Riener
*/

#pragma once

#include <string>

namespace copycat
{

class bool5
{
public:
  struct presumably_false_t {};
  struct inconclusive_t {};
  struct presumably_true_t {};

public:
  bool5()
    : value( false_value )
  {
  }

  bool5( bool b )
    : value( b ? true_value : false_value )
  {
  }

  explicit bool5( presumably_false_t const& )
    : value( presumably_false_value )
  {
  }

  explicit bool5( inconclusive_t const& )
    : value( inconclusive_value )
  {
  }

  explicit bool5( presumably_true_t const& )
    : value( presumably_true_value )
  {
  }
  
  bool operator==( bool5 const& other ) const
  {
    return value == other.value;
  }

  bool operator!=( bool5 const& other ) const
  {
    return !this->operator==( other );
  }

  bool operator<( bool5 const& other ) const
  {
    return value < other.value;
  }

  bool operator>( bool5 const& other ) const
  {
    return value > other.value;
  }

  bool operator<=( bool5 const& other ) const
  {
    return !operator>( other );
  }

  bool operator>=( bool5 const& other ) const
  {
    return !operator<( other );
  }

  bool5 operator!() const
  {
    switch ( value )
    {
    default:
    case false_value:
      return true;
    case presumably_false_value:
      return bool5(presumably_true_t());      
    case inconclusive_value:
      return bool5(inconclusive_t());
    case presumably_true_value:
      return bool5(presumably_false_t());
    case true_value:
      return false;
    }
  }

  bool5 operator&( bool5 const& other ) const
  {
    return std::min( *this, other );
  }

  bool5 operator&&( bool5 const& other ) const
  {
    if ( is_false() )
    {
      return false;
    }
    else if ( is_presumably_false() )
    {
      return bool5( presumably_false_t() );
    }
    return ( *this & other );
  }

  bool5 operator|( bool5 const& other ) const
  {
    return std::max( *this, other );
  }

  bool5 operator||( bool5 const& other ) const
  {
    if ( is_true() )
    {
      return true;
    }
    else if ( is_presumably_true() )
    {
      return bool5( presumably_true_t() );
    }
    return ( *this | other );
  }

  bool is_false() const
  {
    return ( value == false_value );
  }

  bool is_presumably_false() const
  {
    return ( value == presumably_false_value );
  }

  bool is_inconclusive() const
  {
    return ( value == inconclusive_value );
  }

  bool is_presumably_true() const
  {
    return ( value == presumably_true_value );
  }
  
  bool is_true() const
  {
    return ( value == true_value );
  }

  std::string to_string() const
  {
    switch ( value )
    {
    default:
    case inconclusive_value:
      return "?";
    case true_value:
      return "H";
    case presumably_true_value:
      return "h";
    case presumably_false_value:
      return "l";
    case false_value:
      return "L";
    }
  }

protected:
  enum value_t {
    false_value = 0,
    presumably_false_value = 1,
    inconclusive_value = 2,
    presumably_true_value = 3,
    true_value = 4,
  } value;
}; /* bool5 */

inline bool5 presumably_true{bool5::presumably_true_t()};
inline bool5 inconclusive5{bool5::inconclusive_t()};
inline bool5 presumably_false{bool5::presumably_false_t()};

} /* namespace copycat */
