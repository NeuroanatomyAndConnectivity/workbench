#ifndef __ABSTRACT_OPERATION_H__
#define __ABSTRACT_OPERATION_H__

/*LICENSE_START*/
/*
 *  Copyright 1995-2002 Washington University School of Medicine
 *
 *  http://brainmap.wustl.edu
 *
 *  This file is part of CARET.
 *
 *  CARET is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CARET is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CARET; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//make it easy to use these in an algorithm class, don't just forward declare them
#include "ProgressObject.h"
#include "CaretAssert.h"
#include "OperationParameters.h"

namespace caret {

    class AbstractOperation
    {
    protected:
        AbstractOperation();
        virtual ~AbstractOperation();
    public:

        ///override these to allow operation parsers to use your operation without writing an explicit command class
        static OperationParameters* getParameters() { CaretAssert(false); return NULL; }

        ///override these to allow operation parsers to use your operation without writing an explicit command class
        static void useParameters(OperationParameters*, ProgressObject*) { CaretAssert(false); }
        
        ///override this to set the command switch
        static AString getCommandSwitch() { CaretAssert(false); return ""; }
        
        ///override this to set the short description
        static AString getShortDescription() { CaretAssert(false); return ""; }
        
        ///override this if the operation doesn't take parameters
        static bool takesParameters() { return true; }
    };

    ///interface class for use by operation parsers
    struct AutoOperationInterface
    {
        virtual OperationParameters* getParameters() = 0;
        virtual void useParameters(OperationParameters* a, ProgressObject* b) = 0;
        virtual const AString& getCommandSwitch() = 0;
        virtual const AString& getShortDescription() = 0;
        virtual bool takesParameters() = 0;
        virtual ~AutoOperationInterface();
    };

    ///templated interface class to pass through to something that inherits from AbstractOperation (or implements equivalent functions)
    template<typename T>
    struct TemplateAutoOperation : public AutoOperationInterface
    {
        AString m_switch, m_shortInfo;
        TemplateAutoOperation() : m_switch(T::getCommandSwitch()), m_shortInfo(T::getShortDescription()) { }
        OperationParameters* getParameters() { return T::getParameters(); }
        void useParameters(OperationParameters* a, ProgressObject* b) { T::useParameters(a, b); }
        const AString& getCommandSwitch() { return m_switch; }
        const AString& getShortDescription() { return m_shortInfo; }
        bool takesParameters() { return T::takesParameters(); }
    };

    ///interface class for parsers to inherit from
    class OperationParserInterface
    {
        OperationParserInterface();//must take an interface object, for its vtable to the real algorithm, so deny default construction
    protected:
        AutoOperationInterface* m_autoOper;
    public:
        OperationParserInterface(AutoOperationInterface* myAutoOper) : m_autoOper(myAutoOper) { }
        virtual ~OperationParserInterface();
    };
}
#endif //__ABSTRACT_OPERATION_H__