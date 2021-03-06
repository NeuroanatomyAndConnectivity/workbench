
/*LICENSE_START*/
/*
 *  Copyright (C) 2014  Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/

#include <limits>

#define __SCENE_FLOAT_DECLARE__
#include "SceneFloat.h"
#undef __SCENE_FLOAT_DECLARE__

using namespace caret;


    
/**
 * \class caret::SceneFloat 
 * \brief For storage of a float value in a scene.
 * \ingroup Scene
 *
 * See the documentation in the class Scene for how to use the Scene system.
 */

/**
 * Constructor.
 *
 * @param name
 *   Name of object.
 * @param value
 *   Value of object.
 */
SceneFloat::SceneFloat(const AString& name,
                       const float value)
: ScenePrimitive(name,
                 SceneObjectDataTypeEnum::SCENE_FLOAT)
{
    m_value = value;
}

/**
 * Destructor.
 */
SceneFloat::~SceneFloat()
{
    
}

/**
 * Set the value.
 * @param value
 *    The new value.
 */
void 
SceneFloat::setValue(const float value)
{
    m_value = value;
}

/**
 * @return The value as a boolean data type.
 */
bool 
SceneFloat::booleanValue() const
{
    const bool b = ((m_value != 0.0) ? true : false);
    return b;
}

/**
 * @return The value as a float data type.
 */
float
SceneFloat::floatValue() const
{
    return m_value;
}

/**
 * @return The value as a integer data type.
 */
int32_t 
SceneFloat::integerValue() const
{
    if (m_value > std::numeric_limits<int32_t>::max()) {
        return std::numeric_limits<int32_t>::max();
    }
    else if (m_value < std::numeric_limits<int32_t>::min()) {
        return std::numeric_limits<int32_t>::min();
    }
    
    const float i = static_cast<int32_t>(m_value);
    return i;
}

/**
 * @return The value as a string data type.
 */
AString 
SceneFloat::stringValue() const
{
    const AString s = AString::number(m_value);
    return s;
}

