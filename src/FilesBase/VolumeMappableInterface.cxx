
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

#define __VOLUME_MAPPABLE_INTERFACE_DECLARE__
#include "VolumeMappableInterface.h"
#undef __VOLUME_MAPPABLE_INTERFACE_DECLARE__

#include "CaretAssert.h"
using namespace caret;


    
/**
 * Get the voxel spacing for each of the spatial dimensions.
 *
 * @param spacingOut1
 *    Spacing for the first dimension (typically X).
 * @param spacingOut2
 *    Spacing for the first dimension (typically Y).
 * @param spacingOut3
 *    Spacing for the first dimension (typically Z).
 */
void
VolumeMappableInterface::getVoxelSpacing(float& spacingOut1,
                                         float& spacingOut2,
                                         float& spacingOut3) const
{
    float originX, originY, originZ;
    float x1, y1, z1;
    indexToSpace(0, 0, 0, originX, originY, originZ);
    indexToSpace(1, 1, 1, x1, y1, z1);
    spacingOut1 = x1 - originX;
    spacingOut2 = y1 - originY;
    spacingOut3 = z1 - originZ;
}

