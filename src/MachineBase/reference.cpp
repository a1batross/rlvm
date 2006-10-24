// This file is part of RLVM, a RealLive virutal machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006 El Riot
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  
// -----------------------------------------------------------------------

/**
 * @file   reference.cpp
 * @author Elliot Glaysher
 * @date   Sat Oct  7 11:15:37 2006
 * 
 * @brief  Defines the iterator interface to RLMachine's memory
 */

#include "MachineBase/reference.hpp"
#include "MachineBase/RLMachine.hpp"

/** 
 * Read from the memory location, and return the value.
 * 
 * @return The integer value of the memory location.
 */
IntAccessor::operator int() const
{
  return it->m_machine->getIntValue(it->m_type, it->m_location); 
}

// -----------------------------------------------------------------------

/** 
 * Assign a new value to the memory location.
 * 
 * @param newValue New value to set.
 * @return Self
 */
IntAccessor& IntAccessor::operator=(const int newValue) { 
  it->m_machine->setIntValue(it->m_type, it->m_location, newValue);
  return *this;
} 

// -----------------------------------------------------------------------

/** 
 * Assigns to this accessor from another IntAccessor. This allows us
 * to use the "*dest = *src" mechanic since normally, that would call
 * the default copy operator, which would copy rhs.it onto it.
 * 
 * @param rhs IntAccessor to read from
 * @return Self
 */
IntAccessor& IntAccessor::operator=(const IntAccessor& rhs)
{
  return operator=(rhs.operator int());
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

/** 
 * Read from the memory location, and return the value.
 * 
 * @return The string value of the memory location.
 */
StringAccessor::operator std::string() const
{
  return it->m_machine->getStringValue(it->m_type, it->m_location);
}

// -----------------------------------------------------------------------

/** 
 * Assign a new value to the memory location.
 * 
 * @param newValue New value to set.
 * @return Self
 */
StringAccessor& StringAccessor::operator=(const std::string& newValue) {
  it->m_machine->setStringValue(it->m_type, it->m_location, newValue);
  return *this;
}

// -----------------------------------------------------------------------

bool StringAccessor::operator==(const std::string& rhs) {
  return operator std::string() == rhs;
}

// -----------------------------------------------------------------------

/** 
 * Assigns to this accessor from another StringAccessor. This allows
 * us to use the "*dest = *src" mechanic since normally, that would
 * call the default copy operator, which would copy rhs.it onto it.
 * 
 * @param rhs StringAccessor to read from
 * @return Self
 */
StringAccessor& StringAccessor::operator=(const StringAccessor& rhs)
{
  return operator=(rhs.operator std::string());
}