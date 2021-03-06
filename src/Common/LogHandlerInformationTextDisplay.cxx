
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

#include <iostream>

#define __LOG_HANDLER_INFORMATION_TEXT_DISPLAY_DECLARE__
#include "LogHandlerInformationTextDisplay.h"
#undef __LOG_HANDLER_INFORMATION_TEXT_DISPLAY_DECLARE__

#include "EventManager.h"
#include "LogRecord.h"

using namespace caret;


/**
 * Constructor.
 */
LogHandlerInformationTextDisplay::LogHandlerInformationTextDisplay()
: LogHandler()
{
    
}

/**
 * Destructor.
 */
LogHandlerInformationTextDisplay::~LogHandlerInformationTextDisplay()
{
    
}

/**
 * Get a description of this object's content.
 * @return String describing this object's content.
 */
AString 
LogHandlerInformationTextDisplay::toString() const
{
    return "LogHandlerStandardError";
}

/**
 * close the handler and free resources.
 */
void 
LogHandlerInformationTextDisplay::close()
{
    // nothing to close
}

/**
 * Flush any buffered output.
 */
void 
LogHandlerInformationTextDisplay::flush()
{
    std::cerr.flush();
}

/**
 * Publish a log record.
 *
 * @param logRecord
 *    Logging record that is sent to standard error.
 */
void 
LogHandlerInformationTextDisplay::publish(const LogRecord& logRecord)
{
    AString msg =
     "Level: " + LogLevelEnum::toName(logRecord.getLevel()) + "\n";
    if (logRecord.getMethodName().isEmpty() == false) {
        msg += "Method: " + logRecord.getMethodName() + "\n";        
    }
    msg += ("File:   " + logRecord.getFilename()
            + "\nLine:   " + AString::number(logRecord.getLineNumber())
    + "\nText:   " + logRecord.getText()
     + "\n");
}


