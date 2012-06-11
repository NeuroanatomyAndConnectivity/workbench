#ifndef __SCENE_ATTRIBUTES__H_
#define __SCENE_ATTRIBUTES__H_

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


#include "CaretObject.h"
#include "SceneTypeEnum.h"
#include "XmlException.h"

namespace caret {
    class XmlWriter;
    
    class SceneAttributes : public CaretObject {
        
    public:
        SceneAttributes(const SceneTypeEnum::Enum sceneType);
        
        virtual ~SceneAttributes();
        
        SceneTypeEnum::Enum getSceneType() const;
        
        virtual void writeAsXML(XmlWriter& xmlWriter) const throw (XmlException);
        
        /** XML Tag for scene */
        static const AString XML_TAG_SCENE_ATTRIBUTES;
        
        /** XML Tag for scene name */
        static const AString XML_TAG_SCENE_TYPE;
        
    private:
        SceneAttributes(const SceneAttributes&);

        SceneAttributes& operator=(const SceneAttributes&);
        
    public:

        // ADD_NEW_METHODS_HERE

    private:

        const SceneTypeEnum::Enum m_sceneType;
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __SCENE_ATTRIBUTES_DECLARE__
    const AString SceneAttributes::XML_TAG_SCENE_ATTRIBUTES = "SceneAttributes";
    const AString SceneAttributes::XML_TAG_SCENE_TYPE = "SceneType";
#endif // __SCENE_ATTRIBUTES_DECLARE__

} // namespace
#endif  //__SCENE_ATTRIBUTES__H_