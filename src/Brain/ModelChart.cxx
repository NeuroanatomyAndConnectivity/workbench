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

#include <algorithm>
#include <cmath>

#include "Brain.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "CaretLogger.h"
#include "CaretMappableDataFile.h"
#include "ChartAxis.h"
#include "ChartableInterface.h"
#include "ChartData.h"
#include "ChartDataCartesian.h"
#include "ChartDataSource.h"
#include "ChartModelDataSeries.h"
#include "EventBrowserTabGetAll.h"
#include "EventManager.h"
#include "EventNodeIdentificationColorsGetFromCharts.h"
#include "ModelChart.h"
#include "OverlaySet.h"
#include "OverlaySetArray.h"
#include "PlainTextStringBuilder.h"
#include "SceneClass.h"
#include "SceneClassArray.h"
#include "SceneClassAssistant.h"
#include "SurfaceFile.h"

using namespace caret;

/**
 * Constructor.
 *
 */
ModelChart::ModelChart(Brain* brain)
: Model(ModelTypeEnum::MODEL_TYPE_CHART,
                         brain)
{
    std::vector<StructureEnum::Enum> overlaySurfaceStructures;
    m_overlaySetArray = new OverlaySetArray(overlaySurfaceStructures,
                                            Overlay::INCLUDE_VOLUME_FILES_YES,
                                            "Chart View");

    initializeCharts();
    
    EventManager::get()->addEventListener(this,
                                          EventTypeEnum::EVENT_NODE_IDENTIFICATION_COLORS_GET_FROM_CHARTS);
}

/**
 * Destructor
 */
ModelChart::~ModelChart()
{
    delete m_overlaySetArray;
    EventManager::get()->removeAllEventsFromListener(this);
    
    removeAllCharts();    
}

void
ModelChart::initializeCharts()
{
    for (int32_t i = 0; i < BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS; i++) {
        m_selectedChartDataType[i] = ChartDataTypeEnum::CHART_DATA_TYPE_TIME_SERIES;
        
        m_chartModelDataSeries[i] =
        new ChartModelDataSeries(ChartDataTypeEnum::CHART_DATA_TYPE_DATA_SERIES,
                                 ChartAxisUnitsEnum::CHART_AXIS_UNITS_NONE,
                                 ChartAxisUnitsEnum::CHART_AXIS_UNITS_NONE);
        m_chartModelDataSeries[i]->getLeftAxis()->setText("Value");
        m_chartModelDataSeries[i]->getBottomAxis()->setText("Map Index");
        
        m_chartModelTimeSeries[i] =
        new ChartModelDataSeries(ChartDataTypeEnum::CHART_DATA_TYPE_TIME_SERIES,
                                 ChartAxisUnitsEnum::CHART_AXIS_UNITS_TIME_SECONDS,
                                 ChartAxisUnitsEnum::CHART_AXIS_UNITS_NONE);
        m_chartModelTimeSeries[i]->getLeftAxis()->setText("Activity");
        m_chartModelTimeSeries[i]->getBottomAxis()->setText("Time");
    }    
}

/**
 * Reset this model.
 */
void
ModelChart::reset()
{
    removeAllCharts();
    
    initializeCharts();
}

/**
 * Remove all of the charts.
 */
void
ModelChart::removeAllCharts()
{
    for (int32_t i = 0; i < BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS; i++) {
        if (m_chartModelDataSeries[i] != NULL) {
            delete m_chartModelDataSeries[i];
            m_chartModelDataSeries[i] = NULL;
        }
        
        if (m_chartModelTimeSeries[i] != NULL) {
            delete m_chartModelTimeSeries[i];
            m_chartModelTimeSeries[i] = NULL;
        }
    }
    
    m_dataSeriesChartData.clear();
    m_timeSeriesChartData.clear();
}

/**
 * Load chart data for an average of surface nodes.
 *
 * @param structure
 *     The surface structure
 * @param surfaceNumberOfNodes
 *     Number of nodes in surface.
 * @param nodeIndices
 *     Indices of node.
 * @throws
 *     DataFileException if there is an error loading data.
 */
void
ModelChart::loadAverageChartDataForSurfaceNodes(const StructureEnum::Enum structure,
                                         const int32_t surfaceNumberOfNodes,
                                         const std::vector<int32_t>& nodeIndices) throw (DataFileException)
{
    std::map<ChartableInterface*, std::vector<int32_t> > chartFileEnabledTabs;
    getTabsAndChartFilesForChartLoading(chartFileEnabledTabs);
    
    for (std::map<ChartableInterface*, std::vector<int32_t> >::iterator fileTabIter = chartFileEnabledTabs.begin();
         fileTabIter != chartFileEnabledTabs.end();
         fileTabIter++) {
        ChartableInterface* chartFile = fileTabIter->first;
        const std::vector<int32_t>  tabIndices = fileTabIter->second;
        
        CaretAssert(chartFile);
        ChartData* chartData = chartFile->loadAverageChartDataForSurfaceNodes(structure,
                                                                              nodeIndices);
        if (chartData != NULL) {
            ChartDataSource* dataSource = chartData->getChartDataSource();
            dataSource->setSurfaceNodeAverage(chartFile->getCaretMappableDataFile()->getFileName(),
                                              StructureEnum::toName(structure),
                                              surfaceNumberOfNodes, nodeIndices);
            
            addChartToChartModels(tabIndices,
                                  chartData);
        }
    }    
}

/**
 * Load chart data for voxel at the given coordinate.
 *
 * @param xyz
 *     Coordinate of voxel.
 * @throws
 *     DataFileException if there is an error loading data.
 */
void
ModelChart::loadChartDataForVoxelAtCoordinate(const float xyz[3]) throw (DataFileException)
{
    std::map<ChartableInterface*, std::vector<int32_t> > chartFileEnabledTabs;
    getTabsAndChartFilesForChartLoading(chartFileEnabledTabs);
    
    for (std::map<ChartableInterface*, std::vector<int32_t> >::iterator fileTabIter = chartFileEnabledTabs.begin();
         fileTabIter != chartFileEnabledTabs.end();
         fileTabIter++) {
        ChartableInterface* chartFile = fileTabIter->first;
        const std::vector<int32_t>  tabIndices = fileTabIter->second;
        
        CaretAssert(chartFile);
        ChartData* chartData = chartFile->loadChartDataForVoxelAtCoordinate(xyz);
        if (chartData != NULL) {
            ChartDataSource* dataSource = chartData->getChartDataSource();
            dataSource->setVolumeVoxel(chartFile->getCaretMappableDataFile()->getFileName(),
                                       xyz);
            
            addChartToChartModels(tabIndices,
                                  chartData);
        }
    }
}

/**
 * Add the chart to the given tabs.
 *
 * @param tabIndices
 *    Indices of tabs for chart data
 * @param chartData
 *    Chart data that is added.
 */
void
ModelChart::addChartToChartModels(const std::vector<int32_t>& tabIndices,
                                  ChartData* chartData)
{
    CaretAssert(chartData);
    
    const ChartDataTypeEnum::Enum chartDataDataType = chartData->getChartDataType();
    
    switch (chartDataDataType) {
        case ChartDataTypeEnum::CHART_DATA_TYPE_INVALID:
            CaretAssert(0);
            break;
        case ChartDataTypeEnum::CHART_DATA_TYPE_MATRIX:
            CaretAssert(0);
            break;
        case ChartDataTypeEnum::CHART_DATA_TYPE_DATA_SERIES:
        {
            ChartDataCartesian* cdc = dynamic_cast<ChartDataCartesian*>(chartData);
            CaretAssert(cdc);
            QSharedPointer<ChartDataCartesian> cdcPtr(cdc);
            for (std::vector<int32_t>::const_iterator iter = tabIndices.begin();
                 iter != tabIndices.end();
                 iter++) {
                const int32_t tabIndex = *iter;
                m_chartModelDataSeries[tabIndex]->addChartData(cdcPtr);
            }
            m_dataSeriesChartData.push_front(cdcPtr.toWeakRef());
        }
            break;
        case ChartDataTypeEnum::CHART_DATA_TYPE_TIME_SERIES:
        {
            ChartDataCartesian* cdc = dynamic_cast<ChartDataCartesian*>(chartData);
            CaretAssert(cdc);
            QSharedPointer<ChartDataCartesian> cdcPtr(cdc);
            for (std::vector<int32_t>::const_iterator iter = tabIndices.begin();
                 iter != tabIndices.end();
                 iter++) {
                const int32_t tabIndex = *iter;
                m_chartModelTimeSeries[tabIndex]->addChartData(cdcPtr);
            }
            m_timeSeriesChartData.push_front(cdcPtr.toWeakRef());
        }
            break;
    }
}

/**
 * Get tabs and chart files for loading chart data.
 *
 * @param chartFileEnabledTabsOut
 *    Map with first being a chartable file and the second being
 *    tabs for which that chartable file is enabled.
 */
void
ModelChart::getTabsAndChartFilesForChartLoading(std::map<ChartableInterface*, std::vector<int32_t> >& chartFileEnabledTabsOut) const
{
    chartFileEnabledTabsOut.clear();
    
    EventBrowserTabGetAll allTabsEvent;
    EventManager::get()->sendEvent(allTabsEvent.getPointer());
    std::vector<int32_t> validTabIndices = allTabsEvent.getBrowserTabIndices();
    
    std::vector<ChartableInterface*> chartFiles;
    m_brain->getAllChartableDataFilesWithChartingEnabled(chartFiles);
    
    for (std::vector<ChartableInterface*>::iterator iter = chartFiles.begin();
         iter != chartFiles.end();
         iter++) {
        ChartableInterface* cf = *iter;
        std::vector<int32_t> chartFileTabIndices;
        
        for (std::vector<int32_t>::iterator tabIter = validTabIndices.begin();
             tabIter != validTabIndices.end();
             tabIter++) {
            const int32_t tabIndex = *tabIter;
            if (cf->isChartingEnabled(tabIndex)) {
                chartFileTabIndices.push_back(tabIndex);
            }
        }
        
        if ( ! chartFileTabIndices.empty()) {
            chartFileEnabledTabsOut.insert(std::make_pair(cf, chartFileTabIndices));
        }
    }
}

/**
 * Load chart data for a surface node.
 *
 * @param structure
 *     The surface structure
 * @param surfaceNumberOfNodes
 *     Number of nodes in surface.
 * @param nodeIndex
 *     Index of node.
 * @throws
 *     DataFileException if there is an error loading data.
 */
void
ModelChart::loadChartDataForSurfaceNode(const StructureEnum::Enum structure,
                                        const int32_t surfaceNumberOfNodes,
                                        const int32_t nodeIndex) throw (DataFileException)
{
    std::map<ChartableInterface*, std::vector<int32_t> > chartFileEnabledTabs;
    getTabsAndChartFilesForChartLoading(chartFileEnabledTabs);
    
    for (std::map<ChartableInterface*, std::vector<int32_t> >::iterator fileTabIter = chartFileEnabledTabs.begin();
         fileTabIter != chartFileEnabledTabs.end();
         fileTabIter++) {
        ChartableInterface* chartFile = fileTabIter->first;
        const std::vector<int32_t>  tabIndices = fileTabIter->second;

        CaretAssert(chartFile);
        ChartData* chartData = chartFile->loadChartDataForSurfaceNode(structure,
                                               nodeIndex);
        if (chartData != NULL) {
            ChartDataSource* dataSource = chartData->getChartDataSource();
            dataSource->setSurfaceNode(chartFile->getCaretMappableDataFile()->getFileName(),
                                       StructureEnum::toName(structure),
                                       surfaceNumberOfNodes,
                                       nodeIndex);
            
            addChartToChartModels(tabIndices,
                                  chartData);
        }
    }
}

/**
 * Receive an event.
 * 
 * @param event
 *     The event that the receive can respond to.
 */
void 
ModelChart::receiveEvent(Event* event)
{
    if (event->getEventType() == EventTypeEnum::EVENT_NODE_IDENTIFICATION_COLORS_GET_FROM_CHARTS) {
        EventNodeIdentificationColorsGetFromCharts* nodeChartID =
           dynamic_cast<EventNodeIdentificationColorsGetFromCharts*>(event);
        CaretAssert(nodeChartID);
        
        EventBrowserTabGetAll allTabsEvent;
        EventManager::get()->sendEvent(allTabsEvent.getPointer());
        std::vector<int32_t> validTabIndices = allTabsEvent.getBrowserTabIndices();
        
        
        const AString structureName = nodeChartID->getStructureName();
        
        std::vector<ChartDataCartesian*> cartesianChartData;
        
        for (std::list<QWeakPointer<ChartDataCartesian> >::iterator dsIter = m_dataSeriesChartData.begin();
             dsIter != m_dataSeriesChartData.end();
             dsIter++) {
            QSharedPointer<ChartDataCartesian> spCart = dsIter->toStrongRef();
            if ( ! spCart.isNull()) {
                cartesianChartData.push_back(spCart.data());
            }
        }
        for (std::list<QWeakPointer<ChartDataCartesian> >::iterator tsIter = m_timeSeriesChartData.begin();
             tsIter != m_timeSeriesChartData.end();
             tsIter++) {
            QSharedPointer<ChartDataCartesian> spCart = tsIter->toStrongRef();
            if ( ! spCart.isNull()) {
                cartesianChartData.push_back(spCart.data());
            }
        }
        
        
        /*
         * Iterate over node indices for which colors are desired.
         */
        const std::vector<int32_t> nodeIndices = nodeChartID->getNodeIndices();
        for (std::vector<int32_t>::const_iterator nodeIter = nodeIndices.begin();
             nodeIter != nodeIndices.end();
             nodeIter++) {
            const int32_t nodeIndex = *nodeIter;
            
            /*
             * Iterate over the data in the cartesian chart
             */
            bool foundNodeFlag = false;
            for (std::vector<ChartDataCartesian*>::iterator cdIter = cartesianChartData.begin();
                 cdIter != cartesianChartData.end();
                 cdIter++) {
                const ChartDataCartesian* cdc = *cdIter;
                const ChartDataSource* cds = cdc->getChartDataSource();
                if (cds->isSurfaceNodeSourceOfData(structureName, nodeIndex)) {
                    /*
                     * Found node index so add its color to the event
                     */
                    foundNodeFlag = true;
                    const CaretColorEnum::Enum color = cdc->getColor();
                    const float* rgb = CaretColorEnum::toRGB(color);
                    nodeChartID->addNode(nodeIndex,
                                         rgb);
                    break;
                }
            }
        }

        nodeChartID->setEventProcessed();
    }
}

/**
 * Get the name for use in a GUI.
 *
 * @param includeStructureFlag - Prefix label with structure to which
 *      this structure model belongs.
 * @return   Name for use in a GUI.
 *
 */
AString
ModelChart::getNameForGUI(const bool /*includeStructureFlag*/) const
{
    AString name = "Chart";
    return name;
}

/**
 * @return The name that should be displayed in the tab
 * displaying this model.
 */
AString 
ModelChart::getNameForBrowserTab() const
{
    AString name = "Chart";
    return name;
}

/**
 * Get the overlay set for the given tab.
 * @param tabIndex
 *   Index of tab.
 * @return
 *   Overlay set at the given tab index.
 */
OverlaySet* 
ModelChart::getOverlaySet(const int tabIndex)
{
    CaretAssertArrayIndex(m_overlaySetArray,
                          BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS,
                          tabIndex);
    return m_overlaySetArray->getOverlaySet(tabIndex);
}

/**
 * Get the overlay set for the given tab.
 * @param tabIndex
 *   Index of tab.
 * @return
 *   Overlay set at the given tab index.
 */
const OverlaySet* 
ModelChart::getOverlaySet(const int tabIndex) const
{
    CaretAssertArrayIndex(m_overlaySetArray,
                          BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS,
                          tabIndex);
    return m_overlaySetArray->getOverlaySet(tabIndex);
}

/**
 * Initilize the overlays for this model.
 */
void 
ModelChart::initializeOverlays()
{
    m_overlaySetArray->initializeOverlaySelections();
}

/**
 * Save information specific to this type of model to the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    saving the scene.
 *
 * @param sceneClass
 *    SceneClass to which model specific information is added.
 */
void 
ModelChart::saveModelSpecificInformationToScene(const SceneAttributes* sceneAttributes,
                                                      SceneClass* sceneClass)
{
    std::vector<int32_t> tabIndices = sceneAttributes->getIndicesOfTabsForSavingToScene();
    
    std::set<AString> validChartDataIDs;
    saveChartModelsToScene(sceneAttributes,
                           sceneClass,
                           tabIndices,
                           validChartDataIDs);
    
    sceneClass->addEnumeratedTypeArrayForTabIndices<ChartDataTypeEnum, ChartDataTypeEnum::Enum>("m_selectedChartDataType",
                                                                                                m_selectedChartDataType,
                                                                                                tabIndices);
}

/**
 * Restore information specific to the type of model from the scene.
 * 
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass from which model specific information is obtained.
 */
void 
ModelChart::restoreModelSpecificInformationFromScene(const SceneAttributes* sceneAttributes,
                                                           const SceneClass* sceneClass)
{
    reset();
    
    /*
     * Restore the chart models
     */
    restoreChartModelsFromScene(sceneAttributes,
                                sceneClass);
    
    sceneClass->getEnumerateTypeArrayForTabIndices<ChartDataTypeEnum, ChartDataTypeEnum::Enum>("m_selectedChartDataType",
                                                                                               m_selectedChartDataType);
}

/**
 * Save chart models to the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    saving the scene.
 *
 * @param sceneClass
 *    SceneClass to which model specific information is added.
 */
void
ModelChart::saveChartModelsToScene(const SceneAttributes* sceneAttributes,
                            SceneClass* sceneClass,
                            const std::vector<int32_t>& tabIndices,
                            std::set<AString>& validChartDataIDsOut)
{
    validChartDataIDsOut.clear();
    
    std::set<ChartData*> chartDataForSavingToSceneSet;
    
    /*
     * Save chart models to scene.
     */
    std::vector<SceneClass*> chartModelVector;
    for (std::vector<int32_t>::const_iterator tabIter = tabIndices.begin();
         tabIter != tabIndices.end();
         tabIter++) {
        const int32_t tabIndex = *tabIter;
        
        ChartModel* chartModel = getSelectedChartModel(tabIndex);
        SceneClass* chartModelClass = chartModel->saveToScene(sceneAttributes,
                                                              "chartModel");
        if (chartModelClass == NULL) {
            continue;
        }
        
        SceneClass* chartClassContainer = new SceneClass("chartClassContainer",
                                                "ChartClassContainer",
                                                1);
        chartClassContainer->addInteger("tabIndex", tabIndex);
        chartClassContainer->addEnumeratedType<ChartDataTypeEnum,ChartDataTypeEnum::Enum>("chartDataType",
                                                                                          chartModel->getChartDataType());
        chartClassContainer->addClass(chartModelClass);
        
        chartModelVector.push_back(chartClassContainer);
        
        /*
         * Add chart data that is in models saved to scene.
         * 
         */
        std::vector<ChartData*> chartDatasInModel = chartModel->getAllChartDatas();
        chartDataForSavingToSceneSet.insert(chartDatasInModel.begin(),
                                            chartDatasInModel.end());
    }

    if ( ! chartModelVector.empty()) {
        SceneClassArray* modelArray = new SceneClassArray("chartModelArray",
                                                      chartModelVector);
        sceneClass->addChild(modelArray);
    }

    if ( ! chartDataForSavingToSceneSet.empty()) {
        std::vector<SceneClass*> chartDataClassVector;
        for (std::set<ChartData*>::iterator cdIter = chartDataForSavingToSceneSet.begin();
             cdIter != chartDataForSavingToSceneSet.end();
             cdIter++) {
            ChartData* chartData = *cdIter;
            SceneClass* chartDataClass = chartData->saveToScene(sceneAttributes,
                                                                "chartData");
            
            SceneClass* chartDataContainer = new SceneClass("chartDataContainer",
                                                            "ChartDataContainer",
                                                            1);
            chartDataContainer->addEnumeratedType<ChartDataTypeEnum, ChartDataTypeEnum::Enum>("chartDataType",
                                                                                              chartData->getChartDataType());
            chartDataContainer->addClass(chartDataClass);
            
            chartDataClassVector.push_back(chartDataContainer);
        }
        
        if ( ! chartDataClassVector.empty()) {
            SceneClassArray* dataArray = new SceneClassArray("chartDataArray",
                                                             chartDataClassVector);
            sceneClass->addChild(dataArray);
        }
    }
}

/**
 * Restore the chart models from the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass from which model specific information is obtained.
 */
void
ModelChart::restoreChartModelsFromScene(const SceneAttributes* sceneAttributes,
                                 const SceneClass* sceneClass)
{
    /*
     * Restore the chart models
     */
    const SceneClassArray* chartModelArray = sceneClass->getClassArray("chartModelArray");
    if (chartModelArray != NULL) {
        const int numElements = chartModelArray->getNumberOfArrayElements();
        for (int32_t i = 0; i < numElements; i++) {
            const SceneClass* chartClassContainer = chartModelArray->getClassAtIndex(i);
            if (chartClassContainer != NULL) {
                const int32_t tabIndex = chartClassContainer->getIntegerValue("tabIndex", -1);
                const ChartDataTypeEnum::Enum chartDataType =  chartClassContainer->getEnumeratedTypeValue<ChartDataTypeEnum, ChartDataTypeEnum::Enum>("chartDataType",
                                                                                                        ChartDataTypeEnum::CHART_DATA_TYPE_INVALID);
                const SceneClass* chartModelClass = chartClassContainer->getClass("chartModel");
                
                if ((tabIndex >= 0)
                    && (chartDataType != ChartDataTypeEnum::CHART_DATA_TYPE_INVALID)
                    && (chartModelClass != NULL)) {
                    CaretAssertArrayIndex(m_chartModelDataSeries,
                                          BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS,
                                          tabIndex);
                    
                    switch (chartDataType) {
                        case ChartDataTypeEnum::CHART_DATA_TYPE_INVALID:
                            break;
                        case ChartDataTypeEnum::CHART_DATA_TYPE_MATRIX:
                            CaretAssert(0);
                            break;
                        case ChartDataTypeEnum::CHART_DATA_TYPE_DATA_SERIES:
                            m_chartModelDataSeries[tabIndex]->restoreFromScene(sceneAttributes,
                                                                               chartModelClass);
                            break;
                        case ChartDataTypeEnum::CHART_DATA_TYPE_TIME_SERIES:
                            m_chartModelTimeSeries[tabIndex]->restoreFromScene(sceneAttributes,
                                                                               chartModelClass);
                            break;
                    }
                }
            }
        }
    }
    
    /*
     * Restore the chart data
     */
    std::vector<QSharedPointer<ChartData> > restoredChartData;
    const SceneClassArray* chartDataArray = sceneClass->getClassArray("chartDataArray");
    if (chartDataArray != NULL) {
        const int numElements = chartDataArray->getNumberOfArrayElements();
        for (int32_t i = 0; i < numElements; i++) {
            const SceneClass* chartDataContainer = chartDataArray->getClassAtIndex(i);
            if (chartDataContainer != NULL) {
                const ChartDataTypeEnum::Enum chartDataType = chartDataContainer->getEnumeratedTypeValue<ChartDataTypeEnum, ChartDataTypeEnum::Enum>("chartDataType",
                                                                                                                                                       ChartDataTypeEnum::CHART_DATA_TYPE_INVALID);
                const SceneClass* chartDataClass = chartDataContainer->getClass("chartData");
                if ((chartDataType != ChartDataTypeEnum::CHART_DATA_TYPE_INVALID)
                    && (chartDataClass != NULL)) {
                    ChartData* chartData = ChartData::newChartDataForChartDataType(chartDataType);
                    chartData->restoreFromScene(sceneAttributes, chartDataClass);
                    
                    restoredChartData.push_back(QSharedPointer<ChartData>(chartData));
                }
                
            }
        }
    }
    
    /*
     * Have chart models restore pointers to chart data
     */
    for (int32_t i = 0; i < BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS; i++) {
        m_chartModelDataSeries[i]->restoreChartDataFromScene(restoredChartData);
    }
    for (int32_t i = 0; i < BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS; i++) {
        m_chartModelTimeSeries[i]->restoreChartDataFromScene(restoredChartData);
    }
    
    
    for (std::vector<QSharedPointer<ChartData> >::iterator rcdIter = restoredChartData.begin();
         rcdIter != restoredChartData.end();
         rcdIter++) {
        QSharedPointer<ChartData> chartPointer = *rcdIter;
        
        switch (chartPointer->getChartDataType()) {
            case ChartDataTypeEnum::CHART_DATA_TYPE_INVALID:
                CaretAssert(0);
                break;
            case ChartDataTypeEnum::CHART_DATA_TYPE_DATA_SERIES:
            {
                QSharedPointer<ChartDataCartesian> cartChartPointer = chartPointer.dynamicCast<ChartDataCartesian>();
                CaretAssert( ! cartChartPointer.isNull());
                m_dataSeriesChartData.push_back(cartChartPointer);
            }
                break;
            case ChartDataTypeEnum::CHART_DATA_TYPE_MATRIX:
                CaretAssert(0);
                break;
            case ChartDataTypeEnum::CHART_DATA_TYPE_TIME_SERIES:
            {
                QSharedPointer<ChartDataCartesian> cartChartPointer = chartPointer.dynamicCast<ChartDataCartesian>();
                CaretAssert( ! cartChartPointer.isNull());
                m_timeSeriesChartData.push_back(cartChartPointer);
            }
                break;
        }
    }
}


/**
 * Get a text description of the window's content.
 *
 * @param tabIndex
 *    Index of the tab for content description.
 * @param descriptionOut
 *    Description of the window's content.
 */
void
ModelChart::getDescriptionOfContent(const int32_t tabIndex,
                                    PlainTextStringBuilder& descriptionOut) const
{
    const ChartModel* chartModel = getSelectedChartModel(tabIndex);
    if (chartModel != NULL) {
        descriptionOut.addLine("Chart Type: "
                               + ChartDataTypeEnum::toGuiName(chartModel->getChartDataType()));

        descriptionOut.pushIndentation();
        
        const std::vector<const ChartData*> cdVec = chartModel->getAllChartDatas();
        for (std::vector<const ChartData*>::const_iterator iter = cdVec.begin();
             iter != cdVec.end();
             iter++) {
            const ChartData* cd = *iter;
            if (cd->isSelected(tabIndex)) {
                descriptionOut.addLine(cd->getChartDataSource()->getDescription());
            }
        }
        
        if (chartModel->isAverageChartDisplaySupported()) {
            if (chartModel->isAverageChartDisplaySelected()) {
                descriptionOut.addLine("Average Chart Displayed");
            }
        }
        
        descriptionOut.popIndentation();
    }
    else {
        descriptionOut.addLine("No charts to display");
    }
}

/**
 * Copy the tab content from the source tab index to the
 * destination tab index.
 *
 * @param sourceTabIndex
 *    Source from which tab content is copied.
 * @param destinationTabIndex
 *    Destination to which tab content is copied.
 */
void
ModelChart::copyTabContent(const int32_t sourceTabIndex,
                      const int32_t destinationTabIndex)
{
    Model::copyTabContent(sourceTabIndex,
                          destinationTabIndex);
    
    m_overlaySetArray->copyOverlaySet(sourceTabIndex,
                                      destinationTabIndex);
    
    m_selectedChartDataType[destinationTabIndex] = m_selectedChartDataType[sourceTabIndex];
    *m_chartModelDataSeries[destinationTabIndex] = *m_chartModelDataSeries[sourceTabIndex];
    *m_chartModelTimeSeries[destinationTabIndex] = *m_chartModelTimeSeries[sourceTabIndex];
}

/**
 * Set the type of chart selected in the given tab.
 *
 * @param tabIndex
 *    Index of tab.
 * @param dataType
 *    Type of data for chart.
 */
void
ModelChart::setSelectedChartDataType(const int32_t tabIndex,
                              const ChartDataTypeEnum::Enum dataType)
{
    CaretAssertArrayIndex(m_selectedChartDataType,
                          BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS,
                          tabIndex);
    m_selectedChartDataType[tabIndex] = dataType;
}

/**
 * Get the type of chart selected in the given tab.
 *
 * @param tabIndex
 *    Index of tab.
 * @return
 *    Chart type in the given tab.
 */
ChartDataTypeEnum::Enum
ModelChart::getSelectedChartDataType(const int32_t tabIndex) const
{
    CaretAssertArrayIndex(m_selectedChartDataType,
                          BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS,
                          tabIndex);
    return m_selectedChartDataType[tabIndex];
}

/**
 * Get the chart model selected in the given tab.
 *
 * @param tabIndex
 *    Index of tab.
 * @return
 *    Chart model in the given tab or none if not valid.
 */
ChartModel*
ModelChart::getSelectedChartModel(const int32_t tabIndex)
{
    const ChartModel* model = getSelectedChartModelHelper(tabIndex);
    if (model == NULL) {
        return NULL;
    }
    ChartModel* nonConstModel = const_cast<ChartModel*>(model);
    return nonConstModel;
}

/**
 * Get the chart model selected in the given tab.
 *
 * @param tabIndex
 *    Index of tab.
 * @return
 *    Chart model in the given tab or none if not valid.
 */
const ChartModel*
ModelChart::getSelectedChartModel(const int32_t tabIndex) const
{
    return getSelectedChartModelHelper(tabIndex);
}

/**
 * Get the chart model selected in the given tab.
 *
 * @param tabIndex
 *    Index of tab.
 * @return
 *    Chart model in the given tab or none if not valid.
 */
const ChartModel*
ModelChart::getSelectedChartModelHelper(const int32_t tabIndex) const
{
    const ChartDataTypeEnum::Enum chartType = getSelectedChartDataType(tabIndex);
    
    ChartModel* model = NULL;
    
    switch (chartType) {
        case ChartDataTypeEnum::CHART_DATA_TYPE_INVALID:
            break;
        case ChartDataTypeEnum::CHART_DATA_TYPE_MATRIX:
            break;
        case ChartDataTypeEnum::CHART_DATA_TYPE_DATA_SERIES:
            model = m_chartModelDataSeries[tabIndex];
            break;
        case ChartDataTypeEnum::CHART_DATA_TYPE_TIME_SERIES:
            model = m_chartModelTimeSeries[tabIndex];
            break;
    }
    
    return model;
    
}
