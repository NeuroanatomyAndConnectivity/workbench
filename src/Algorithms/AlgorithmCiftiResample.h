#ifndef __ALGORITHM_CIFTI_RESAMPLE_H__
#define __ALGORITHM_CIFTI_RESAMPLE_H__

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
#include "FloatMatrix.h"
#include "StructureEnum.h"
#include "SurfaceResamplingMethodEnum.h"
#include "VolumeFile.h"

namespace caret {
    
    class AlgorithmCiftiResample : public AbstractAlgorithm
    {
        AlgorithmCiftiResample();
        void processSurfaceComponent(const CiftiFile* myCiftiIn, const int& direction, const StructureEnum::Enum& myStruct, const SurfaceResamplingMethodEnum::Enum& mySurfMethod,
                                     CiftiFile* myCiftiOut, const bool& surfLargest, const float& surfdilatemm, const SurfaceFile* curSphere, const SurfaceFile* newSphere,
                                     const SurfaceFile* curArea, const SurfaceFile* newArea);
        void processVolumeWarpfield(const CiftiFile* myCiftiIn, const int& direction, const StructureEnum::Enum& myStruct, const VolumeFile::InterpType& myVolMethod,
                                    CiftiFile* myCiftiOut, const float& voldilatemm, const VolumeFile* warpfield);
        void processVolumeAffine(const CiftiFile* myCiftiIn, const int& direction, const StructureEnum::Enum& myStruct, const VolumeFile::InterpType& myVolMethod,
                                 CiftiFile* myCiftiOut, const float& voldilatemm, const FloatMatrix& affine);
    protected:
        static float getSubAlgorithmWeight();
        static float getAlgorithmInternalWeight();
    public:
        AlgorithmCiftiResample(ProgressObject* myProgObj, const CiftiFile* myCiftiIn, const int& direction, const CiftiFile* myTemplate, const int& templateDir,
                               const SurfaceResamplingMethodEnum::Enum& mySurfMethod, const VolumeFile::InterpType& myVolMethod, CiftiFile* myCiftiOut,
                               const bool& surfLargest, const float& voldilatemm, const float& surfdilatemm,
                               const VolumeFile* warpfield,
                               const SurfaceFile* curLeftSphere, const SurfaceFile* newLeftSphere, const SurfaceFile* curLeftAreaSurf, const SurfaceFile* newLeftAreaSurf,
                               const SurfaceFile* curRightSphere, const SurfaceFile* newRightSphere, const SurfaceFile* curRightAreaSurf, const SurfaceFile* newRightAreaSurf,
                               const SurfaceFile* curCerebSphere, const SurfaceFile* newCerebSphere, const SurfaceFile* curCerebAreaSurf, const SurfaceFile* newCerebAreaSurf);
        
        AlgorithmCiftiResample(ProgressObject* myProgObj, const CiftiFile* myCiftiIn, const int& direction, const CiftiFile* myTemplate, const int& templateDir,
                               const SurfaceResamplingMethodEnum::Enum& mySurfMethod, const VolumeFile::InterpType& myVolMethod, CiftiFile* myCiftiOut,
                               const bool& surfLargest, const float& voldilatemm, const float& surfdilatemm,
                               const FloatMatrix& affine,
                               const SurfaceFile* curLeftSphere, const SurfaceFile* newLeftSphere, const SurfaceFile* curLeftAreaSurf, const SurfaceFile* newLeftAreaSurf,
                               const SurfaceFile* curRightSphere, const SurfaceFile* newRightSphere, const SurfaceFile* curRightAreaSurf, const SurfaceFile* newRightAreaSurf,
                               const SurfaceFile* curCerebSphere, const SurfaceFile* newCerebSphere, const SurfaceFile* curCerebAreaSurf, const SurfaceFile* newCerebAreaSurf);
        
        static OperationParameters* getParameters();
        static void useParameters(OperationParameters* myParams, ProgressObject* myProgObj);
        static AString getCommandSwitch();
        static AString getShortDescription();
    };

    typedef TemplateAutoOperation<AlgorithmCiftiResample> AutoAlgorithmCiftiResample;

}

#endif //__ALGORITHM_CIFTI_RESAMPLE_H__
