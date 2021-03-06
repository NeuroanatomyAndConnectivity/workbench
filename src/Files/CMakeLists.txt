#
# Name of project
#
PROJECT (Files)

#
# Need XML from Qt
#
SET(QT_DONT_USE_QTGUI)
SET(QT_USE_QTNETWORK TRUE)

#
# Add QT for includes
#
INCLUDE (${QT_USE_FILE})

#
# Files Library
#
ADD_LIBRARY(Files
AffineFile.h
BinaryFileReader.h
Border.h
BorderException.h
BorderFile.h
BorderFileSaxReader.h
CaretDataFile.h
CaretDataFileHelper.h
CaretMappableDataFile.h
CaretSparseFile.h
CaretVolumeExtension.h
ChartableBrainordinateInterface.h
ChartableInterface.h
ChartableMatrixInterface.h
CiftiBrainordinateDataSeriesFile.h
CiftiBrainordinateLabelFile.h
CiftiBrainordinateScalarFile.h
CiftiConnectivityMatrixDenseFile.h
CiftiConnectivityMatrixDenseParcelFile.h
CiftiConnectivityMatrixParcelFile.h
CiftiConnectivityMatrixParcelDenseFile.h
CiftiFacade.h
CiftiFiberOrientationFile.h
CiftiFiberTrajectoryFile.h
CiftiMappableDataFile.h
CiftiMappableConnectivityMatrixDataFile.h
CiftiParcelColoringModeEnum.h
CiftiParcelSeriesFile.h
CiftiParcelScalarFile.h
ConnectivityDataLoaded.h
DataFileTypeEnum.h
EventGetDisplayedDataFiles.h
EventPaletteGetByName.h
EventSurfaceColoringInvalidate.h
Fiber.h
FiberOrientation.h
FiberOrientationColoringTypeEnum.h
FiberOrientationTrajectory.h
FiberTrajectoryColorModel.h
FiberTrajectoryMapProperties.h
FiberTrajectoryDisplayModeEnum.h
FociFile.h
FociFileSaxReader.h
Focus.h
GeodesicHelper.h
GiftiTypeFile.h
GroupAndNameCheckStateEnum.h
GroupAndNameHierarchyGroup.h
GroupAndNameHierarchyItem.h
GroupAndNameHierarchyModel.h
GroupAndNameHierarchyName.h
ImageDimensionsModel.h
ImageFile.h
ImagePixelsPerSpatialUnitsEnum.h
ImageSpatialUnitsEnum.h
LabelFile.h
MetricFile.h
MetricSmoothingObject.h
NodeAndVoxelColoring.h
OxfordSparseThreeFile.h
PaletteFile.h
RgbaFile.h
SceneFile.h
SceneFileSaxReader.h
SignedDistanceHelper.h
SparseVolumeIndexer.h
SpecFile.h
SpecFileDataFileTypeGroup.h
SpecFileDataFile.h
SpecFileSaxReader.h
StudyMetaDataLink.h
StudyMetaDataLinkSet.h
StudyMetaDataLinkSetSaxReader.h
SurfaceFile.h
SurfaceProjectedItem.h
SurfaceProjectedItemSaxReader.h
SurfaceProjection.h
SurfaceProjectionBarycentric.h
SurfaceProjectionVanEssen.h
SurfaceProjector.h
SurfaceProjectorException.h
SurfaceResamplingHelper.h
SurfaceResamplingMethodEnum.h
SurfaceTypeEnum.h
TextFile.h
TopologyHelper.h
VolumeFile.h
VolumeFileVoxelColorizer.h
VolumePaddingHelper.h
VolumeSpline.h
VtkFileExporter.h
WarpfieldFile.h

AffineFile.cxx
BinaryFileReader.cxx
Border.cxx
BorderException.cxx
BorderFile.cxx
BorderFileSaxReader.cxx
CaretDataFile.cxx
CaretDataFileHelper.cxx
CaretMappableDataFile.cxx
CaretSparseFile.cxx
CaretVolumeExtension.cxx
ChartableInterface.cxx
CiftiBrainordinateDataSeriesFile.cxx
CiftiBrainordinateLabelFile.cxx
CiftiBrainordinateScalarFile.cxx
CiftiConnectivityMatrixDenseFile.cxx
CiftiConnectivityMatrixDenseParcelFile.cxx
CiftiConnectivityMatrixParcelFile.cxx
CiftiConnectivityMatrixParcelDenseFile.cxx
CiftiFacade.cxx
CiftiFiberOrientationFile.cxx
CiftiFiberTrajectoryFile.cxx
CiftiMappableDataFile.cxx
CiftiMappableConnectivityMatrixDataFile.cxx
CiftiParcelColoringModeEnum.cxx
CiftiParcelSeriesFile.cxx
CiftiParcelScalarFile.cxx
ConnectivityDataLoaded.cxx
DataFileTypeEnum.cxx
EventGetDisplayedDataFiles.cxx
EventPaletteGetByName.cxx
EventSurfaceColoringInvalidate.cxx
Fiber.cxx
FiberOrientation.cxx
FiberOrientationColoringTypeEnum.cxx
FiberOrientationTrajectory.cxx
FiberTrajectoryColorModel.cxx
FiberTrajectoryDisplayModeEnum.cxx
FiberTrajectoryMapProperties.cxx
FociFile.cxx
FociFileSaxReader.cxx
Focus.cxx
GeodesicHelper.cxx
GiftiTypeFile.cxx
GroupAndNameCheckStateEnum.cxx
GroupAndNameHierarchyGroup.cxx
GroupAndNameHierarchyItem.cxx
GroupAndNameHierarchyModel.cxx
GroupAndNameHierarchyName.cxx
ImageDimensionsModel.cxx
ImageFile.cxx
ImagePixelsPerSpatialUnitsEnum.cxx
ImageSpatialUnitsEnum.cxx
LabelFile.cxx
MetricFile.cxx
MetricSmoothingObject.cxx
NodeAndVoxelColoring.cxx
OxfordSparseThreeFile.cxx
PaletteFile.cxx
RgbaFile.cxx
SceneFile.cxx
SceneFileSaxReader.cxx
SignedDistanceHelper.cxx
SparseVolumeIndexer.cxx
SpecFile.cxx
SpecFileDataFileTypeGroup.cxx
SpecFileDataFile.cxx
SpecFileSaxReader.cxx
StudyMetaDataLink.cxx
StudyMetaDataLinkSet.cxx
StudyMetaDataLinkSetSaxReader.cxx
SurfaceFile.cxx
SurfaceProjectedItem.cxx
SurfaceProjectedItemSaxReader.cxx
SurfaceProjection.cxx
SurfaceProjectionBarycentric.cxx
SurfaceProjectionVanEssen.cxx
SurfaceProjector.cxx
SurfaceProjectorException.cxx
SurfaceResamplingHelper.cxx
SurfaceResamplingMethodEnum.cxx
SurfaceTypeEnum.cxx
TextFile.cxx
TopologyHelper.cxx
VolumeFile.cxx
VolumeFileVoxelColorizer.cxx
VolumePaddingHelper.cxx
VolumeSpline.cxx
VtkFileExporter.cxx
WarpfieldFile.cxx
)


#
# Find Headers
#
INCLUDE_DIRECTORIES(
${CMAKE_SOURCE_DIR}/Charting
${CMAKE_SOURCE_DIR}/Files
${CMAKE_SOURCE_DIR}/FilesBase
${CMAKE_SOURCE_DIR}/Cifti
${CMAKE_SOURCE_DIR}/Palette
${CMAKE_SOURCE_DIR}/Gifti
${CMAKE_SOURCE_DIR}/Nifti
${CMAKE_SOURCE_DIR}/Scenes
${CMAKE_SOURCE_DIR}/Xml
${CMAKE_SOURCE_DIR}/Common
)
