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
  \file read_json.hpp
  \brief Utility functions to read JSON files

  \author Heinz Riener
*/

#pragma once

#include <json/json.hpp>
#include <fstream>

namespace copycat
{

/*! \brief Read JSON from an input stream */
bool read_json( std::ifstream& is, nlohmann::json& json )
{
  try
  {
    is >> json;
  }
  catch (...)
  {
    return false;
  }
  return true;
}

/*! \brief Read JSON from a file */
bool read_json( std::string const& filename, nlohmann::json& json )
{
  std::ifstream ifs( filename, std::ios::in );
  auto const result = read_json( ifs, json );
  ifs.close();
  return result;
}

} /* namespace copycat */
