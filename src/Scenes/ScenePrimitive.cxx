
/*LICENSE_START*/
/*
 * Copyright 2012 Washington University,
 * All rights reserved.
 *
 * Connectome DB and Connectome Workbench are part of the integrated Connectome 
 * Informatics Platform.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of Washington University nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR  
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*LICENSE_END*/

#define __SCENE_PRIMITIVE_DECLARE__
#include "ScenePrimitive.h"
#undef __SCENE_PRIMITIVE_DECLARE__

#include "XmlAttributes.h"
#include "XmlWriter.h"

using namespace caret;


    
/**
 * \class caret::ScenePrimitive 
 * \brief Abstract class for 'primitive' data types for scenes.
 *
 * Design loosely based upon Java.lang.Number but also including
 * boolean and string values.
 */

/**
 * Constructor.
 * @param name
 *    Name of primitive.
 * @param dataType
 *    Data type of the primitive.
 */
ScenePrimitive::ScenePrimitive(const QString& name,
                               const SceneObjectDataTypeEnum::Enum dataType)
: SceneObject(name, dataType)
{
    
}

/**
 * Destructor.
 */
ScenePrimitive::~ScenePrimitive()
{
    
}

/**
 * Write the Scene Primitive to XML.
 * 
 * @param xmlWriter
 *    Writer that generates XML.
 * @throws
 *    XmlException if there is an error.
 */
void 
ScenePrimitive::writeAsXML(XmlWriter& xmlWriter) const throw (XmlException)
{
    XmlAttributes atts;
    atts.addAttribute(XML_ATTRIBUTE_NAME, this->getName());
    
    const AString elementName = SceneObjectDataTypeEnum::toName(this->getDataType());
    const AString elementText = this->stringValue();
    
    if (this->getDataType() == SceneObjectDataTypeEnum::SCENE_STRING) {
        xmlWriter.writeElementCData(elementName, 
                                    atts, 
                                    elementText);
    }
    else {
        xmlWriter.writeElementCharacters(elementName, 
                                         atts, 
                                         elementText);
    }
}
