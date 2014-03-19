
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

#include <algorithm>
#include <iostream>
#include <iterator>
#include <stack>

#include "AlgorithmNodesInsideBorder.h"

#include "AlgorithmException.h"
#include "Border.h"
#include "BorderFile.h"
#include "CaretAssert.h"
#include "CaretOMP.h"
#include "CaretLogger.h"
#include "GeodesicHelper.h"
#include "LabelFile.h"
#include "MetricFile.h"
#include "SurfaceFile.h"
#include "SurfaceProjectedItem.h"
#include "TopologyHelper.h"

using namespace caret;

AString AlgorithmNodesInsideBorder::getCommandSwitch()
{
    return "-border-to-rois";//maybe the command should be in a separate file, and the nodes inside border code should be a helper class?
}

AString AlgorithmNodesInsideBorder::getShortDescription()
{
    return "MAKE METRIC ROIS FROM BORDERS";
}

OperationParameters* AlgorithmNodesInsideBorder::getParameters()
{
    OperationParameters* ret = new OperationParameters();
    
    ret->addSurfaceParameter(1, "surface", "the surface the borders are drawn on");
    
    ret->addBorderParameter(2, "border-file", "the border file");
    
    ret->addMetricOutputParameter(3, "metric-out", "the output metric file");
    
    OptionalParameter* borderOpt = ret->createOptionalParameter(4, "-border", "create ROI for only one border");
    borderOpt->addStringParameter(1, "name", "the name of the border");
    
    ret->createOptionalParameter(5, "-inverse", "use inverse selection (outside border)");
    
    ret->setHelpText(
        AString("By default, draws ROIs inside all borders in the border file, as separate metric columns.")
    );
    return ret;
}

void AlgorithmNodesInsideBorder::useParameters(OperationParameters* myParams, ProgressObject* myProgObj)
{
    SurfaceFile* mySurf = myParams->getSurface(1);
    BorderFile* myBorderFile = myParams->getBorder(2);
    MetricFile* myMetricOut = myParams->getOutputMetric(3);
    OptionalParameter* borderOpt = myParams->getOptionalParameter(4);
    bool inverse = myParams->getOptionalParameter(5)->m_present;
    //TODO: check that border file is valid on this surface file
    if (borderOpt->m_present)
    {
        AString findName = borderOpt->getString(1);
        int numBorders = myBorderFile->getNumberOfBorders();
        int borderIndx = -1;
        for (int i = 0; i < numBorders; ++i)
        {
            if (myBorderFile->getBorder(i)->getName() == findName)
            {
                borderIndx = i;
                break;
            }
        }
        if (borderIndx == -1) throw AlgorithmException("border name not found");
        myMetricOut->setNumberOfNodesAndColumns(mySurf->getNumberOfNodes(), 1);
        myMetricOut->setStructure(mySurf->getStructure());
        myMetricOut->setColumnName(0, findName);
        AlgorithmNodesInsideBorder(myProgObj, mySurf, myBorderFile->getBorder(borderIndx), inverse, 0, 1.0f, myMetricOut);
    } else {
        int numBorders = myBorderFile->getNumberOfBorders();
        myMetricOut->setNumberOfNodesAndColumns(mySurf->getNumberOfNodes(), numBorders);
        myMetricOut->setStructure(mySurf->getStructure());
#pragma omp CARET_PARFOR
        for (int i = 0; i < numBorders; ++i)
        {
            myMetricOut->setColumnName(i, myBorderFile->getBorder(i)->getName());
            AlgorithmNodesInsideBorder(myProgObj, mySurf, myBorderFile->getBorder(i), inverse, i, 1.0f, myMetricOut);
        }
    }
}

/**
 * \class caret::AlgorithmNodesInsideBorder 
 * \brief Assign attribute values to nodes within a closed border.
 *
 * Identify nodes within a closed border and assign metric or
 * label values to those nodes.
 */

/**
 * Constructor.
 * 
 * @param myProgObj
 *    
 * @param surfaceFile
 *    Surface file for nodes inside border.
 * @param border
 *    Border for which nodes inside are found.
 * @param assignToMetricMapIndex
 *    Map index in metric file to which assigments are made for
 *    nodes inside the border.
 * @param assignMetricValue
 *    Metric value assigned to nodes within the border.
 * @param metricFileInOut
 *    Metric file that has map set with nodes inside border.
 */
AlgorithmNodesInsideBorder::AlgorithmNodesInsideBorder(ProgressObject* myProgObj, 
                                                       const SurfaceFile* surfaceFile,
                                                       const Border* border, 
                                                       const bool isInverseSelection,
                                                       const int32_t assignToMetricMapIndex,
                                                       const float assignMetricValue,
                                                       MetricFile* metricFileInOut)
: AbstractAlgorithm(myProgObj)
{
    CaretAssert(surfaceFile);
    CaretAssert(border);
    CaretAssert(metricFileInOut);
    if (surfaceFile->getNumberOfNodes() != metricFileInOut->getNumberOfNodes()) throw AlgorithmException("metric file must be initialized to same number of nodes");//a method that requires this really shouldn't be public
    
    this->isInverseSelection = isInverseSelection;
    
    std::vector<int32_t> nodesInsideBorder;
    this->findNodesInsideBorder(surfaceFile,
                                border,
                                nodesInsideBorder);
    
    const int32_t numberOfNodesInsideBorder = static_cast<int32_t>(nodesInsideBorder.size());
    for (int32_t i = 0; i < numberOfNodesInsideBorder; i++) {
        const int32_t nodeNumber = nodesInsideBorder[i];
        metricFileInOut->setValue(nodeNumber,
                                  assignToMetricMapIndex,
                                  assignMetricValue);
    }
}

AlgorithmNodesInsideBorder::AlgorithmNodesInsideBorder(ProgressObject* myProgObj, 
                                                       const SurfaceFile* surfaceFile,
                                                       const Border* border, 
                                                       const bool isInverseSelection,
                                                       const int32_t assignToLabelMapIndex,
                                                       const int32_t assignLabelKey,
                                                       LabelFile* labelFileInOut)
: AbstractAlgorithm(myProgObj)
{
    CaretAssert(surfaceFile);
    CaretAssert(border);
    CaretAssert(labelFileInOut);
    if (surfaceFile->getNumberOfNodes() != labelFileInOut->getNumberOfNodes()) throw AlgorithmException("metric file must be initialized to same number of nodes");//a method that requires this really shouldn't be public
    
    this->isInverseSelection = isInverseSelection;
    
    std::vector<int32_t> nodesInsideBorder;
    this->findNodesInsideBorder(surfaceFile,
                                border,
                                nodesInsideBorder);
    
    const int32_t numberOfNodesInsideBorder = static_cast<int32_t>(nodesInsideBorder.size());
    for (int32_t i = 0; i < numberOfNodesInsideBorder; i++) {
        const int32_t nodeNumber = nodesInsideBorder[i];
        labelFileInOut->setLabelKey(nodeNumber,
                                    assignToLabelMapIndex,
                                    assignLabelKey);
    }
}

/**
 * Find nodes inside the border.
 *
 * @param surfaceFile
 *    Surface file for nodes inside border.
 * @param border
 *    Border for which nodes inside are found.
 * @param nodesInsideBorderOut
 *    Vector into which nodes inside border are loaded.
 */
void 
AlgorithmNodesInsideBorder::findNodesInsideBorder(const SurfaceFile* surfaceFile,
                                                  const Border* border,
                                                  std::vector<int32_t>& nodesInsideBorderOut)
{
    CaretAssert(surfaceFile);
    CaretAssert(border);
    
    nodesInsideBorderOut.clear();
    
    /*
     * Move border points to the nearest nodes.
     */
    std::vector<int32_t> nodesAlongBorder;
    const int32_t originalNumberOfBorderPoints = border->getNumberOfPoints();
    for (int32_t i = 0; i < originalNumberOfBorderPoints; i++) {
        const SurfaceProjectedItem* spi = border->getPoint(i);
        float xyz[3];
        if (spi->getProjectedPosition(*surfaceFile, xyz, true)) {
            const int32_t nearestNode = surfaceFile->closestNode(xyz);
            if (nearestNode >= 0) {
                nodesAlongBorder.push_back(nearestNode);
            }
        }
    }
    
    /*
     * Make sure first and last nodes are not the same
     */
    int32_t numberOfPointsInBorder = static_cast<int32_t>(nodesAlongBorder.size());
    if (nodesAlongBorder.size() < 4) {
        throw AlgorithmException("Border is too small.  "
                                 "When moved to nearest vertices, border consists of four or fewer vertices.");
    }
    if (nodesAlongBorder[0] == nodesAlongBorder[numberOfPointsInBorder - 1]) {
        nodesAlongBorder.resize(numberOfPointsInBorder - 1);
    }
    numberOfPointsInBorder = static_cast<int32_t>(nodesAlongBorder.size());
    
    findNodesEnclosedByUnconnectedPath(surfaceFile,
                                       nodesAlongBorder,
                                       nodesInsideBorderOut);
}

/**
 * Given an path consisting of unconnected nodes, create a path
 * that connectects the nodes.
 *
 * @param surfaceFile
 *    Surface whose topology is used for finding the path.
 * @param unconnectedNodesPath
 *    Input consisting of the unconnected nodes path.
 * @param connectedNodesPathOut
 *    Output connected nodes path.
 */
void
AlgorithmNodesInsideBorder::createConnectedNodesPath(const SurfaceFile* surfaceFile,
                                                     const std::vector<int32_t>& unconnectedNodesPath,
                                                     std::vector<int32_t>& connectedNodesPathOut)
{
    connectedNodesPathOut.clear();
    
    /*
     * Geodesic helper for surface
     */
    CaretPointer<GeodesicHelper> geodesicHelper = surfaceFile->getGeodesicHelper();
    
    /*
     * Find connected path along node neighbors
     */
    const int32_t numberOfNodesInUnconnectedPath = static_cast<int32_t>(unconnectedNodesPath.size());
    for (int32_t i = 0; i < numberOfNodesInUnconnectedPath; i++) {
        const int node = unconnectedNodesPath[i];
        int nextNode = -1;
        const bool lastNodeFlag = (i >= (numberOfNodesInUnconnectedPath - 1));
        if (lastNodeFlag) {
            nextNode = unconnectedNodesPath[0];
        }
        else {
            nextNode = unconnectedNodesPath[i + 1];
        }
        
        /*
         * Find path from node to next node
         */
        connectedNodesPathOut.push_back(node);
        if (node != nextNode) {
            std::vector<float> distances;
            std::vector<int32_t> parentNodes;
            geodesicHelper->getGeoFromNode(node,
                                           distances,
                                           parentNodes,
                                           false);
            
            bool doneFlag = false;
            std::vector<int32_t> pathFromNextNodeToNode;
            int32_t pathNode = nextNode;
            while (doneFlag == false) {
                int32_t nextPathNode = parentNodes[pathNode];
                if (nextPathNode >= 0) {
                    if (nextPathNode == node) {
                        doneFlag = true;
                    }
                    else {
                        pathFromNextNodeToNode.push_back(nextPathNode);
                    }
                }
                else {
                    doneFlag = true;
                }
                
                if (doneFlag == false) {
                    pathNode = nextPathNode;
                }
            }
            
            const int32_t numberOfPathNodes = static_cast<int32_t>(pathFromNextNodeToNode.size());
            for (int32_t i = (numberOfPathNodes - 1); i >= 0; i--) {
                connectedNodesPathOut.push_back(pathFromNextNodeToNode[i]);
            }
        }
    }
    
    /*
     * Remove duplicates.
     */
    this->cleanConnectedNodesPath(connectedNodesPathOut);
    
    /*
     * Valid that the path nodes are connected.
     */
    this->validateConnectedNodesPath(surfaceFile,
                                     connectedNodesPathOut);
    
}

/**
 * Find the nodes inside the unconnected path on the surface.
 *
 * @param surfaceFile
 *    Surface file for nodes inside connected path.
 * @param unconnectedNodesPath
 *    Unconnected path for which nodes inside are found.
 * @param nodesEnclosedByPathOut
 *    Nodes enclosed by the path.
 */
void
AlgorithmNodesInsideBorder::findNodesEnclosedByUnconnectedPath(const SurfaceFile* surfaceFile,
                                                               const std::vector<int32_t>& unconnectedNodesPath,
                                                               std::vector<int32_t>& nodesEnclosedByPathOut)
{
    /*
     * Find the nodes enclosed by the unconnected path
     * assuming the path is oriented counter-clockwise
     */
    findNodesEnclosedByUnconnectedPathCCW(surfaceFile,
                                          unconnectedNodesPath,
                                          nodesEnclosedByPathOut);
    
    /*
     * If the number of nodes enclosed by the connected path is greater
     * than HALF the number of nodes in the surface, then the path is
     * likely clockwise so reverse the unconnected path and try again.
     */
    const int32_t halfNumberOfSurfaceNodes = surfaceFile->getNumberOfNodes() / 2;
    if (static_cast<int32_t>(nodesEnclosedByPathOut.size()) > halfNumberOfSurfaceNodes) {
        std::vector<int32_t> reversedUnconnectedNodesPath = unconnectedNodesPath;
        std::reverse(reversedUnconnectedNodesPath.begin(),
                     reversedUnconnectedNodesPath.end());
        
        findNodesEnclosedByUnconnectedPathCCW(surfaceFile,
                                              reversedUnconnectedNodesPath,
                                              nodesEnclosedByPathOut);
    }

    /*
     * User requested inverse (nodes outside path)
     */
    if (this->isInverseSelection) {
        /*
         * Get the topology helper for the surface with neighbors sorted.
         */
        CaretPointer<TopologyHelper> th = surfaceFile->getTopologyHelper(true);
        
        const int32_t numberOfNodes = surfaceFile->getNumberOfNodes();
        std::vector<bool> insideROI(numberOfNodes,
                                    true);

        for (std::vector<int32_t>::iterator iter = nodesEnclosedByPathOut.begin();
             iter != nodesEnclosedByPathOut.end();
             iter++) {
            const int32_t nodeIndex = *iter;
            CaretAssertVectorIndex(insideROI, nodeIndex);
            insideROI[nodeIndex] = false;
        }

        nodesEnclosedByPathOut.clear();
        for (int32_t i = 0; i < numberOfNodes; i++) {
            CaretAssertVectorIndex(insideROI, i);
            if (insideROI[i]) {
                if (th->getNodeHasNeighbors(i)) {
                    nodesEnclosedByPathOut.push_back(i);
                }
            }
        }
    }
}

/**
 * Find the nodes inside the unconnected path on the surface.
 *
 * @param surfaceFile
 *    Surface file for nodes inside connected path.
 * @param unconnectedNodesPath
 *    Unconnected path for which nodes inside are found.
 * @param nodesEnclosedByPathOut
 *    Nodes enclosed by the path.
 */
void
AlgorithmNodesInsideBorder::findNodesEnclosedByUnconnectedPathCCW(const SurfaceFile* surfaceFile,
                                                                  const std::vector<int32_t>& unconnectedNodesPath,
                                                                  std::vector<int32_t>& nodesEnclosedByPathOut)
{
    nodesEnclosedByPathOut.clear();
    
    /*
     * Convert the unconnected nodes path into a connected nodes path
     */
    std::vector<int32_t> connectedNodesPath;
    createConnectedNodesPath(surfaceFile,
                             unconnectedNodesPath,
                             connectedNodesPath);
    
    int32_t numberOfNodesInConnectedPath = static_cast<int32_t>(connectedNodesPath.size());
    if (numberOfNodesInConnectedPath < 4) {
        throw AlgorithmException("Connected path is too small "
                                 "as it consists of fewer than four vertices.");
    }
    
    /*
     * Determine the nodes inside the connected path
     */
    this->findNodesEnclosedByConnectedNodesPathCounterClockwise(surfaceFile,
                                                connectedNodesPath,
                                                nodesEnclosedByPathOut);
}

/**
 * Find the nodes inside the connected path on the surface assuming the
 * path is counter-clockwise around the region.
 *
 * @param surfaceFile
 *    Surface file for nodes inside connected path.
 * @param connectedNodesPath
 *    Connected path for which nodes inside are found.
 * @param nodesInsidePathOut
 *    Vector into which nodes inside connected path are loaded.
 */
void 
AlgorithmNodesInsideBorder::findNodesEnclosedByConnectedNodesPathCounterClockwise(const SurfaceFile* surfaceFile,
                                                          const std::vector<int32_t>& connectedNodesPath,
                                                          std::vector<int32_t>& nodesInsidePathOut)
{
    /*
     * Get the topology helper for the surface with neighbors sorted.
     */
    CaretPointer<TopologyHelper> th = surfaceFile->getTopologyHelper(true);
    
    /*
     * Using three nodes, find a node that is 'on the left'
     * assuming the path is oriented counter-clockwise.
     */
    int32_t startNode = -1;
    const int32_t numberOfNodesInConnectedPath = static_cast<int32_t>(connectedNodesPath.size());
    for (int32_t i = 1; i < (numberOfNodesInConnectedPath - 1); i++) {
        const int prevPathNode = connectedNodesPath[i - 1];
        const int pathNode     = connectedNodesPath[i];
        const int nextPathNode = connectedNodesPath[i + 1];
        
        int numNeighbors;
        const int* neighbors = th->getNodeNeighbors(pathNode, numNeighbors);
        if (numNeighbors > 2) {
            int32_t prevIndex = -1;
            int32_t nextIndex = -1;
            for (int32_t j = 0; j < numNeighbors; j++) {
                if (neighbors[j] == prevPathNode) {
                    prevIndex = j;
                }
                else if (neighbors[j] == nextPathNode) {
                    nextIndex = j;
                }
            }
            
            if ((nextIndex >= 0) && (prevIndex >= 0)) {
                if (nextIndex >= (numNeighbors - 1)) {
                    if (prevIndex > 0) {
                        startNode = neighbors[0];
                    }
                }
                else {
                    if (prevIndex != (nextIndex + 1)) {
                        startNode = neighbors[nextIndex + 1];
                    }
                }
            }
            
            if (startNode >= 0) {
                if (std::find(connectedNodesPath.begin(),
                              connectedNodesPath.end(),
                              startNode) != connectedNodesPath.end()) {
                    startNode = -1;
                }
                
                if (startNode >= 0) {
                    break;
                }
            }
        }
    }
    
    if (startNode < 0) {
        throw AlgorithmException("Failed to find vertex that is not inside of the connected path.");
    }
    
    /*
     * Track nodes that are found inside and/or have been visited.
     */
    const int32_t numberOfNodes = surfaceFile->getNumberOfNodes();
    std::vector<bool> visited(numberOfNodes, false);
    std::vector<bool> inside(numberOfNodes, false);
    
    /*
     * Mark all nodes in connected path as visited.
     */
    for (int32_t i = 0; i < numberOfNodesInConnectedPath; i++) {
        visited[connectedNodesPath[i]] = true;
    }
    
    /*
     * Mark the starting node as 'inside'.
     */
    inside[startNode] = true;
    
    /*
     * Use a stack to help with search.
     */
    std::stack<int32_t> stack;
    stack.push(startNode);
    
    /*
     * Search until no more to search.
     */
    while (stack.empty() == false) {
        const int32_t nodeNumber = stack.top();
        stack.pop();
        
        /*
         * Has node been visited?
         */
        if (visited[nodeNumber] == false){
            visited[nodeNumber] = true;
            
            /*
             * Set node as inside
             */
            inside[nodeNumber] = true;
            
            /*
             * Get neighbors of this node
             */
            int numNeighbors = 0;
            const int* neighbors = th->getNodeNeighbors(nodeNumber, numNeighbors);
            
            /*
             * add neighbors to search
             */
            for (int i = 0; i < numNeighbors; i++) {
                const int neighborNode = neighbors[i];
                if (visited[neighborNode] == false) {
                    stack.push(neighborNode);
                }
            }
        }
    }
    
    /*
     * Return nodes inside the path
     */
    for (int32_t i = 0; i < numberOfNodes; i++) {
        if (inside[i]) {
            nodesInsidePathOut.push_back(i);
        }
    }
}

/**
 * Clean the path by removing any consecutive nodes that are identical.
 * @param connectedNodesPath
 *    Path that is cleaned.
 */
void 
AlgorithmNodesInsideBorder::cleanConnectedNodesPath(std::vector<int32_t>& connectedNodesPath)
{
    std::vector<int32_t> path = connectedNodesPath;
    connectedNodesPath.clear();
    
    /*
     * Unique copy will remove consecutive identical elements
     */
    std::unique_copy(path.begin(),
                     path.end(),
                     back_inserter(connectedNodesPath));
}

/**
 * Verify that the connect nodes path is fully connected.
 *
 * @param surfaceFile
 *    Surface file for nodes inside connected path.
 * @param connectedNodesPath
 *    Path that is validated.
 */
void 
AlgorithmNodesInsideBorder::validateConnectedNodesPath(const SurfaceFile* surfaceFile,
                                                       const std::vector<int32_t>& connectedNodesPath)
{
    /*
     * Get the topology helper for the surface with neighbors sorted.
     */
    CaretPointer<TopologyHelper> th = surfaceFile->getTopologyHelper(false);
    
    /*
     * Check the path to see that each pair of nodes are connected.
     */
    const int32_t numberOfNodesInPath = static_cast<int32_t>(connectedNodesPath.size());
    for (int32_t i = 0; i < numberOfNodesInPath; i++) {
        int numNeighbors;
        const int node = connectedNodesPath[i];
        const int* neighbors = th->getNodeNeighbors(node, numNeighbors);
        
        int32_t nextNode = -1;
        if (i >= (numberOfNodesInPath - 1)) {
            nextNode = connectedNodesPath[0];
        }
        else {
            nextNode = connectedNodesPath[i + 1];
        }
        
        if (node != nextNode) {
            bool foundIt = false;
            for (int j = 0; j < numNeighbors; j++) {
                if (neighbors[j] == nextNode) {
                    foundIt = true;
                    break;
                }
            }
            
            if (foundIt == false) {
                throw AlgorithmException("Validation of vertex path along border failed.  Vertex "
                                         + AString::number(node)
                                         + " should be connected to "
                                         + AString::number(nextNode)
                                         + " but it is not.");
            }
        }
    }
}


