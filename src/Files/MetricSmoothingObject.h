#ifndef __METRIC_SMOOTHING_OBJECT_H__
#define __METRIC_SMOOTHING_OBJECT_H__

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

//NOTE: this is largely for special-purpose smoothing by providing greater efficiency and flexibility, since it can be reused with different rois or on different metric objects
//      after being constructed once (constructor takes a while).  If you just want to smooth one metric object with one (or no) ROI, you probably want AlgorithmMetricSmoothing.
//
//NOTE: this object contains no mutable members, multiple threads can call the same function on the same instance and expect consistent behavior, while running concurrently,
//      as long as they don't call it with output arguments that overlap (same instance, same row, or one row plus full metric, etc)
//
//NOTE: for a static ROI, it is (sometimes much) more efficient to use it in the constructor, and provide no ROI (NULL) to the functions, using both an ROI in constructor and in method
//      will result in the effective ROI being the logical AND of the two (intersection).

#include "stdint.h"
#include "stddef.h"
#include <vector>

namespace caret {
    
    class SurfaceFile;
    class MetricFile;
    
    class MetricSmoothingObject
    {
    public:
        enum Method
        {
            GEO_GAUSS_AREA,
            GEO_GAUSS
        };
        MetricSmoothingObject(const SurfaceFile* mySurf, const float& kernel, const MetricFile* myRoi = NULL, Method myMethod = GEO_GAUSS_AREA);
        void smoothColumn(const MetricFile* metricIn, const int& whichColumn, MetricFile* columnOut, const MetricFile* roi = NULL, const bool& fixZeros = false);
        void smoothMetric(const MetricFile* metricIn, MetricFile* metricOut, const MetricFile* roi = NULL, const bool& fixZeros = false);
    private:
        struct WeightList
        {
            std::vector<int32_t> m_nodes;
            std::vector<float> m_weights;
            float m_weightSum;
        };
        std::vector<WeightList> m_weightLists;
        void smoothColumnInternal(float* scratch, const MetricFile* metricIn, const int& whichColumn, MetricFile* metricOut, const int& whichOutColumn, const bool& fixZeros);
        void smoothColumnInternal(float* scratch, const MetricFile* metricIn, const int& whichColumn, MetricFile* metricOut, const int& whichOutColumn, const MetricFile* roi, const bool& fixZeros);
        void precomputeWeights(const SurfaceFile* mySurf, float myKernel, const MetricFile* theRoi, Method myMethod);
        void precomputeWeightsGeoGauss(const SurfaceFile* mySurf, float myKernel);
        void precomputeWeightsROIGeoGauss(const SurfaceFile* mySurf, float myKernel, const MetricFile* theRoi);
        void precomputeWeightsGeoGaussArea(const SurfaceFile* mySurf, float myKernel);
        void precomputeWeightsROIGeoGaussArea(const SurfaceFile* mySurf, float myKernel, const MetricFile* theRoi);
        MetricSmoothingObject();
    };
    
}

#endif //__METRIC_SMOOTHING_OBJECT_H__