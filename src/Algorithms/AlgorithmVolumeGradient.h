#ifndef __ALGORITHM_VOLUME_GRADIENT_H__
#define __ALGORITHM_VOLUME_GRADIENT_H__

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

#include "AbstractAlgorithm.h"

namespace caret {
    
    class AlgorithmVolumeGradient : public AbstractAlgorithm
    {
        AlgorithmVolumeGradient();
    protected:
        static float getSubAlgorithmWeight();
        static float getAlgorithmInternalWeight();
    public:
        AlgorithmVolumeGradient(ProgressObject* myProgObj, const VolumeFile* volIn, VolumeFile* volOut, const float& presmooth = -1.0f,
                                    const VolumeFile* myRoi = NULL, VolumeFile* vectorsOut = NULL, const int& subvolNum = -1);
        static OperationParameters* getParameters();
        static void useParameters(OperationParameters* myParams, ProgressObject* myProgObj);
        static AString getCommandSwitch();
        static AString getShortDescription();
    };

    typedef TemplateAutoOperation<AlgorithmVolumeGradient> AutoAlgorithmVolumeGradient;

}

#endif //__ALGORITHM_VOLUME_GRADIENT_H__
