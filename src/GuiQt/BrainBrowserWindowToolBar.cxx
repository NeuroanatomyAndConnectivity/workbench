
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
#include <limits>

#include <QActionGroup>
#include <QApplication>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCleanlooksStyle>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabBar>
#include <QTextEdit>
#include <QToolButton>
#include <QtWebKit/QtWebKit>
#include <QtWebKit/QWebView>

#include "Brain.h"
#include "BrainBrowserWindow.h"
#include "BrainBrowserWindowScreenModeEnum.h"
#include "BrainBrowserWindowToolBar.h"
#include "BrainStructure.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "CaretFunctionName.h"
#include "CaretLogger.h"
#include "CaretPreferences.h"
#include "CursorDisplayScoped.h"
#include "DisplayPropertiesBorders.h"
#include "EventBrowserTabDelete.h"
#include "EventBrowserTabGet.h"
#include "EventBrowserTabGetAll.h"
#include "EventBrowserTabNew.h"
#include "EventBrowserWindowContentGet.h"
#include "EventBrowserWindowCreateTabs.h"
#include "EventBrowserWindowNew.h"
#include "EventGetOrSetUserInputModeProcessor.h"
#include "EventGraphicsUpdateOneWindow.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventUserInterfaceUpdate.h"
#include "EventManager.h"
#include "EventModelGetAll.h"
#include "EventSurfaceColoringInvalidate.h"
#include "EventUpdateYokedWindows.h"
#include "GuiManager.h"
#include "Model.h"
#include "ModelSurface.h"
#include "ModelSurfaceMontage.h"
#include "ModelSurfaceSelector.h"
#include "ModelTransform.h"
#include "ModelVolume.h"
#include "ModelWholeBrain.h"
#include "OverlaySet.h"
#include "SceneAttributes.h"
#include "SceneClass.h"
#include "SceneIntegerArray.h"
#include "ScenePrimitiveArray.h"
#include "SessionManager.h"
#include "Surface.h"
#include "SurfaceSelectionModel.h"
#include "SurfaceSelectionViewController.h"
#include "StructureSurfaceSelectionControl.h"
#include "UserInputReceiverInterface.h"
#include "VolumeFile.h"
#include "VolumeSliceViewModeEnum.h"
#include "VolumeSliceViewPlaneEnum.h"
#include "VolumeSurfaceOutlineSetModel.h"
#include "WuQDataEntryDialog.h"
#include "WuQFactory.h"
#include "WuQMessageBox.h"
#include "WuQWidgetObjectGroup.h"
#include "WuQtUtilities.h"


using namespace caret;

/**
 * Constructor.
 *
 * @param browserWindowIndex
 *    Index of the parent browser window.
 * @param initialBrowserTabContent
 *    Content of default tab (may be NULL in which cast
 *    new content is created).
 * @param toolBoxToolButtonAction
 *    Action for the Toolbox button in this toolbar.
 * @param parent
 *    Parent for this toolbar.
 */
BrainBrowserWindowToolBar::BrainBrowserWindowToolBar(const int32_t browserWindowIndex,
                                                     BrowserTabContent* initialBrowserTabContent,
                                                     QAction* overlayToolBoxAction,
                                                     QAction* layersToolBoxAction,
                                                     BrainBrowserWindow* parentBrainBrowserWindow)
: QToolBar(parentBrainBrowserWindow)
{
    this->browserWindowIndex = browserWindowIndex;
    this->updateCounter = 0;
    
    this->isContructorFinished = false;
    this->isDestructionInProgress = false;

    this->viewOrientationLeftIcon = NULL;
    this->viewOrientationRightIcon = NULL;
    this->viewOrientationAnteriorIcon = NULL;
    this->viewOrientationPosteriorIcon = NULL;
    this->viewOrientationDorsalIcon = NULL;
    this->viewOrientationVentralIcon = NULL;
    this->viewOrientationLeftLateralIcon = NULL;
    this->viewOrientationLeftMedialIcon = NULL;
    this->viewOrientationRightLateralIcon = NULL;
    this->viewOrientationRightMedialIcon = NULL;

    /*
     * Create tab bar that displays models.
     */
    this->tabBar = new QTabBar();
    if (WuQtUtilities::isSmallDisplay()) {
        this->tabBar->setStyleSheet("QTabBar::tab:selected {"
                                    "    font: bold;"
                                    "}  " 
                                    "QTabBar::tab {"
                                    "    font: italic"
                                    "}");
    }
    else {
        this->tabBar->setStyleSheet("QTabBar::tab:selected {"
                                    "    font: bold 14px;"
                                    "}  " 
                                    "QTabBar::tab {"
                                    "    font: italic"
                                    "}");
    }

    this->tabBar->setShape(QTabBar::RoundedNorth);
#ifdef Q_OS_MACX
    /*
     * Adding a parent to the style will result in it
     * being destroyed when this instance is destroyed.
     * The style must remain valid until the destruction
     * of this instance.  It cannot be declared statically.
     */
    QCleanlooksStyle* cleanLooksStyle = new QCleanlooksStyle();
    cleanLooksStyle->setParent(this);
    this->tabBar->setStyle(cleanLooksStyle);
#endif // Q_OS_MACX
    QObject::connect(this->tabBar, SIGNAL(currentChanged(int)),
                     this, SLOT(selectedTabChanged(int)));
    QObject::connect(this->tabBar, SIGNAL(tabCloseRequested(int)),
                     this, SLOT(tabClosed(int)));
    
    /*
     * Actions at right side of toolbar
     */
    QToolButton* informationDialogToolButton = new QToolButton();
    informationDialogToolButton->setDefaultAction(GuiManager::get()->getInformationDisplayDialogEnabledAction());
    
    QToolButton* sceneDialogToolButton = new QToolButton();
    sceneDialogToolButton->setDefaultAction(parentBrainBrowserWindow->m_showSceneDialogAction);
    
    /*
     * Toolbar action and tool button at right of the tab bar
     */
    QIcon toolBarIcon;
    const bool toolBarIconValid =
    WuQtUtilities::loadIcon(":/toolbar.png", 
                            toolBarIcon);
    
    this->toolBarToolButtonAction =
    WuQtUtilities::createAction("Toolbar", 
                                "Show or hide the toolbar",
                                this,
                                this,
                                SLOT(showHideToolBar(bool)));
    if (toolBarIconValid) {
        this->toolBarToolButtonAction->setIcon(toolBarIcon);
        this->toolBarToolButtonAction->setIconVisibleInMenu(false);
    }
    this->toolBarToolButtonAction->setIconVisibleInMenu(false);
    this->toolBarToolButtonAction->blockSignals(true);
    this->toolBarToolButtonAction->setCheckable(true);
    this->toolBarToolButtonAction->setChecked(true);
    this->showHideToolBar(this->toolBarToolButtonAction->isChecked());
    this->toolBarToolButtonAction->blockSignals(false);
    QToolButton* toolBarToolButton = new QToolButton();
    toolBarToolButton->setDefaultAction(this->toolBarToolButtonAction);
    
    /*
     * Toolbox control at right of the tab bar
     */
    QToolButton* overlayToolBoxToolButton = new QToolButton();
    overlayToolBoxToolButton->setDefaultAction(overlayToolBoxAction);
    
    QToolButton* layersToolBoxToolButton = new QToolButton();
    layersToolBoxToolButton->setDefaultAction(layersToolBoxAction);
    
    /*
     * Tab bar and controls at far right side of toolbar
     */
    this->tabBarWidget = new QWidget();
    QHBoxLayout* tabBarLayout = new QHBoxLayout(this->tabBarWidget);
    WuQtUtilities::setLayoutMargins(tabBarLayout, 2, 1);
    tabBarLayout->addWidget(this->tabBar, 100);
    tabBarLayout->addWidget(informationDialogToolButton);
    tabBarLayout->addWidget(sceneDialogToolButton);
    tabBarLayout->addWidget(toolBarToolButton);
    tabBarLayout->addWidget(overlayToolBoxToolButton);
    tabBarLayout->addWidget(layersToolBoxToolButton);
    
    /*
     * Create the toolbar's widgets.
     */
    this->viewWidget = this->createViewWidget();
    this->orientationWidget = this->createOrientationWidget();
    this->wholeBrainSurfaceOptionsWidget = this->createWholeBrainSurfaceOptionsWidget();
    this->volumeIndicesWidget = this->createVolumeIndicesWidget();
    this->modeWidget = this->createModeWidget();
    this->windowWidget = this->createWindowWidget();
    this->singleSurfaceSelectionWidget = this->createSingleSurfaceOptionsWidget();
    this->surfaceMontageSelectionWidget = this->createSurfaceMontageOptionsWidget();
    this->volumeMontageWidget = this->createVolumeMontageWidget();
    this->volumePlaneWidget = this->createVolumePlaneWidget();
    this->clippingWidget = this->createClippingWidget();
    this->chartWidget = this->createChartWidget();
    
    /*
     * Layout the toolbar's widgets.
     */
    m_toolbarWidget = new QWidget();
    this->toolbarWidgetLayout = new QHBoxLayout(m_toolbarWidget);
    WuQtUtilities::setLayoutMargins(this->toolbarWidgetLayout, 2, 1);
    
    this->toolbarWidgetLayout->addWidget(this->viewWidget, 0, Qt::AlignLeft);
    
    this->toolbarWidgetLayout->addWidget(this->orientationWidget, 0, Qt::AlignLeft);
    
    this->toolbarWidgetLayout->addWidget(this->wholeBrainSurfaceOptionsWidget, 0, Qt::AlignLeft);
    
    this->toolbarWidgetLayout->addWidget(this->singleSurfaceSelectionWidget, 0, Qt::AlignLeft);
    
    this->toolbarWidgetLayout->addWidget(this->surfaceMontageSelectionWidget, 0, Qt::AlignLeft);
    
    this->toolbarWidgetLayout->addWidget(this->volumePlaneWidget, 0, Qt::AlignLeft);
    
    this->toolbarWidgetLayout->addWidget(this->volumeIndicesWidget, 0, Qt::AlignLeft);
    
    this->toolbarWidgetLayout->addWidget(this->volumeMontageWidget, 0, Qt::AlignLeft);
    
    this->toolbarWidgetLayout->addWidget(this->modeWidget, 0, Qt::AlignLeft);
    
    this->toolbarWidgetLayout->addWidget(this->clippingWidget, 0, Qt::AlignLeft);
    
    this->toolbarWidgetLayout->addWidget(this->windowWidget, 0, Qt::AlignLeft);

   // this->toolbarWidgetLayout->addWidget(this->chartWidget, 0, Qt::AlignLeft);

    this->toolbarWidgetLayout->addStretch();

    /*
     * Widget below toolbar for user input mode mouse controls
     */
    this->userInputControlsWidgetLayout = new QHBoxLayout();
    this->userInputControlsWidgetLayout->addSpacing(5);
    WuQtUtilities::setLayoutMargins(this->userInputControlsWidgetLayout, 0, 0);
    this->userInputControlsWidget = new QWidget();
    QVBoxLayout* userInputLayout = new QVBoxLayout(this->userInputControlsWidget);
    WuQtUtilities::setLayoutMargins(userInputLayout, 2, 0);
    userInputLayout->addWidget(WuQtUtilities::createHorizontalLineWidget());
    userInputLayout->addLayout(this->userInputControlsWidgetLayout);
    userInputControlsWidgetActiveInputWidget = NULL;
    
    /*
     * Arrange the tabbar and the toolbar vertically.
     */
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    WuQtUtilities::setLayoutMargins(layout, 1, 0);
    layout->addWidget(this->tabBarWidget);
    layout->addWidget(m_toolbarWidget);
    layout->addWidget(this->userInputControlsWidget);
    
    this->addWidget(w);
    
    if (initialBrowserTabContent != NULL) {
        this->addNewTab(initialBrowserTabContent);
    }
    else {
        AString errorMessage;
        this->createNewTab(errorMessage);
    }
    
    this->updateToolBar();
    
    this->isContructorFinished = true;
    
    EventManager::get()->addEventListener(this, EventTypeEnum::EVENT_BROWSER_WINDOW_CONTENT_GET);
    EventManager::get()->addEventListener(this, EventTypeEnum::EVENT_BROWSER_WINDOW_CREATE_TABS);
    EventManager::get()->addEventListener(this, EventTypeEnum::EVENT_UPDATE_YOKED_WINDOWS);
    EventManager::get()->addEventListener(this, EventTypeEnum::EVENT_USER_INTERFACE_UPDATE);
}

/**
 * Destructor.
 */
BrainBrowserWindowToolBar::~BrainBrowserWindowToolBar()
{
    this->isDestructionInProgress = true;
    
    if (this->viewOrientationLeftIcon != NULL) {
        delete this->viewOrientationLeftIcon;
        this->viewOrientationLeftIcon = NULL;
    }
    if (this->viewOrientationRightIcon != NULL) {
        delete this->viewOrientationRightIcon;
        this->viewOrientationRightIcon = NULL;
    }

    if (this->viewOrientationAnteriorIcon != NULL) {
        delete this->viewOrientationAnteriorIcon;
        this->viewOrientationAnteriorIcon = NULL;
    }

    if (this->viewOrientationPosteriorIcon != NULL) {
        delete this->viewOrientationPosteriorIcon;
        this->viewOrientationPosteriorIcon = NULL;
    }

    if (this->viewOrientationDorsalIcon != NULL) {
        delete this->viewOrientationDorsalIcon;
        this->viewOrientationDorsalIcon = NULL;
    }

    if (this->viewOrientationVentralIcon != NULL) {
        delete this->viewOrientationVentralIcon;
        this->viewOrientationVentralIcon = NULL;
    }

    if (this->viewOrientationLeftLateralIcon != NULL) {
        delete this->viewOrientationLeftLateralIcon;
        this->viewOrientationLeftLateralIcon = NULL;
    }

    if (this->viewOrientationLeftMedialIcon != NULL) {
        delete this->viewOrientationLeftMedialIcon;
        this->viewOrientationLeftMedialIcon = NULL;
    }

    if (this->viewOrientationRightLateralIcon != NULL) {
        delete this->viewOrientationRightLateralIcon;
        this->viewOrientationRightLateralIcon = NULL;
    }

    if (this->viewOrientationRightMedialIcon != NULL) {
        delete this->viewOrientationRightMedialIcon;
        this->viewOrientationRightMedialIcon = NULL;
    }

    EventManager::get()->removeAllEventsFromListener(this);
    
    this->viewWidgetGroup->clear();
    this->orientationWidgetGroup->clear();
    this->wholeBrainSurfaceOptionsWidgetGroup->clear();
    this->volumeIndicesWidgetGroup->clear();
    this->modeWidgetGroup->clear();
    this->windowWidgetGroup->clear();
    this->singleSurfaceSelectionWidgetGroup->clear();
    this->surfaceMontageSelectionWidgetGroup->clear();
    this->volumeMontageWidgetGroup->clear();
    this->volumePlaneWidgetGroup->clear();
    this->clippingWidgetGroup->clear();
    
    for (int i = (this->tabBar->count() - 1); i >= 0; i--) {
        this->tabClosed(i);
    }

    this->isDestructionInProgress = false;
}

/**
 * Create a new tab.
 * @param errorMessage
 *     If fails to create new tab, it will contain a message
 *     describing the error.
 * @return 
 *     Pointer to content of new tab or NULL if unable to
 *     create the new tab.
 */
BrowserTabContent* 
BrainBrowserWindowToolBar::createNewTab(AString& errorMessage)
{
    errorMessage = "";
    
    EventBrowserTabNew newTabEvent;
    EventManager::get()->sendEvent(newTabEvent.getPointer());
    
    if (newTabEvent.isError()) {
        errorMessage = newTabEvent.getErrorMessage();
        return NULL;
    }
    
    BrowserTabContent* tabContent = newTabEvent.getBrowserTab();
    Brain* brain = GuiManager::get()->getBrain();
    tabContent->getVolumeSurfaceOutlineSet()->selectSurfacesAfterSpecFileLoaded(brain, 
                                                                                false);
    this->addNewTab(tabContent);
    
    return tabContent;
}


/**
 * Add a new tab and clone the content of the given tab.
 * @param browserTabContentToBeCloned
 *    Tab Content that is to be cloned into the new tab.
 */
void 
BrainBrowserWindowToolBar::addNewTabCloneContent(BrowserTabContent* browserTabContentToBeCloned)
{
    /*
     * Wait cursor
     */
    CursorDisplayScoped cursor;
    cursor.showWaitCursor();
    
    AString errorMessage;
    BrowserTabContent* tabContent = this->createNewTab(errorMessage);
    if (tabContent == NULL) {
        cursor.restoreCursor();
        QMessageBox::critical(this,
                              "",
                              errorMessage);
        return;
    }
    
    if (browserTabContentToBeCloned != NULL) {
        /*
         * New tab is clone of tab that was displayed when the new tab was created.
         */
        tabContent->cloneBrowserTabContent(browserTabContentToBeCloned);
    }
    
    this->updateToolBar();
    
    EventManager::get()->sendEvent(EventSurfaceColoringInvalidate().getPointer());
    EventManager::get()->sendEvent(EventUserInterfaceUpdate().setWindowIndex(this->browserWindowIndex).getPointer());
    EventManager::get()->sendEvent(EventGraphicsUpdateOneWindow(this->browserWindowIndex).getPointer());
}

/**
 * Add a new tab containing the given content.
 * @param tabContent
 *    Content for new tab.
 */
void 
BrainBrowserWindowToolBar::addNewTab(BrowserTabContent* tabContent)
{
    CaretAssert(tabContent);
    
    this->tabBar->blockSignals(true);
    
    const int32_t tabContentIndex = tabContent->getTabNumber();
    
    int32_t newTabIndex = -1;
    const int32_t numTabs = this->tabBar->count();
    if (numTabs <= 0) {
        newTabIndex = this->tabBar->addTab("NewTab");
    }
    else {
        int insertIndex = 0;
        for (int32_t i = 0; i < numTabs; i++) {
            if (tabContentIndex > this->getTabContentFromTab(i)->getTabNumber()) {
                insertIndex = i + 1;
            }
        }
        if (insertIndex >= numTabs) {
            newTabIndex = this->tabBar->addTab("NewTab");
        }
        else {
            this->tabBar->insertTab(insertIndex, "NewTab");
            newTabIndex = insertIndex;
        }
    }
    
    this->tabBar->setTabData(newTabIndex, qVariantFromValue((void*)tabContent));
    
    const int32_t numOpenTabs = this->tabBar->count();
    this->tabBar->setTabsClosable(numOpenTabs > 1);
    
    this->updateTabName(newTabIndex);
    
    this->tabBar->setCurrentIndex(newTabIndex);
    
    this->tabBar->blockSignals(false);
}

/**
 * Shows/hides the toolbar.
 */
void 
BrainBrowserWindowToolBar::showHideToolBar(bool showIt)
{
    if (this->isContructorFinished) {
        m_toolbarWidget->setVisible(showIt);
    }
    
    this->toolBarToolButtonAction->blockSignals(true);
    if (showIt) {
        this->toolBarToolButtonAction->setToolTip("Hide Toolbar");
        this->toolBarToolButtonAction->setChecked(true);
    }
    else {
        this->toolBarToolButtonAction->setToolTip("Show Toolbar");
        this->toolBarToolButtonAction->setChecked(false);
    }
    this->toolBarToolButtonAction->blockSignals(false);
}


/**
 * Add the default tabs after loading a spec file.
 */
void 
BrainBrowserWindowToolBar::addDefaultTabsAfterLoadingSpecFile()
{
    EventModelGetAll eventAllControllers;
    EventManager::get()->sendEvent(eventAllControllers.getPointer());
    
    const std::vector<Model*> allControllers =
       eventAllControllers.getModels();

    ModelSurface* leftSurfaceController = NULL;
    int32_t leftSurfaceTypeCode = 1000000;
    
    ModelSurface* rightSurfaceController = NULL;
    int32_t rightSurfaceTypeCode = 1000000;

    ModelSurface* cerebellumSurfaceController = NULL;
    int32_t cerebellumSurfaceTypeCode = 1000000;
    
    ModelSurfaceMontage* surfaceMontageController = NULL;
    ModelVolume* volumeController = NULL;
    ModelWholeBrain* wholeBrainController = NULL;
    
    for (std::vector<Model*>::const_iterator iter = allControllers.begin();
         iter != allControllers.end();
         iter++) {
        ModelSurface* surfaceController =
            dynamic_cast<ModelSurface*>(*iter);
        if (surfaceController != NULL) {
            Surface* surface = surfaceController->getSurface();
            StructureEnum::Enum structure = surface->getStructure();
            SurfaceTypeEnum::Enum surfaceType = surface->getSurfaceType();
            const int32_t surfaceTypeCode = SurfaceTypeEnum::toIntegerCode(surfaceType);
            
            switch (structure) {
                case StructureEnum::CEREBELLUM:
                    if (surfaceTypeCode < cerebellumSurfaceTypeCode) {
                        cerebellumSurfaceController = surfaceController;
                        cerebellumSurfaceTypeCode = surfaceTypeCode;
                    }
                    break;
                case StructureEnum::CORTEX_LEFT:
                    if (surfaceTypeCode < leftSurfaceTypeCode) {
                        leftSurfaceController = surfaceController;
                        leftSurfaceTypeCode = surfaceTypeCode;
                    }
                    break;
                case StructureEnum::CORTEX_RIGHT:
                    if (surfaceTypeCode < rightSurfaceTypeCode) {
                        rightSurfaceController = surfaceController;
                        rightSurfaceTypeCode = surfaceTypeCode;
                    }
                    break;
                default:
                    break;
            }
        }
        else if (dynamic_cast<ModelSurfaceMontage*>(*iter) != NULL) {
            surfaceMontageController = dynamic_cast<ModelSurfaceMontage*>(*iter);
        }
        else if (dynamic_cast<ModelVolume*>(*iter) != NULL) {
            volumeController = dynamic_cast<ModelVolume*>(*iter);
        }
        else if (dynamic_cast<ModelWholeBrain*>(*iter) != NULL) {
            wholeBrainController = dynamic_cast<ModelWholeBrain*>(*iter);
        }
        else {
            CaretAssertMessage(0, AString("Unknow controller type: ") + (*iter)->getNameForGUI(true));
        }
    }
    
    int32_t numberOfTabsNeeded = 0;
    if (leftSurfaceController != NULL) {
        numberOfTabsNeeded++;
    }
    if (rightSurfaceController != NULL) {
        numberOfTabsNeeded++;
    }
    if (cerebellumSurfaceController != NULL) {
        numberOfTabsNeeded++;
    }
    if (surfaceMontageController != NULL) {
        numberOfTabsNeeded++;
    }
    if (volumeController != NULL) {
        numberOfTabsNeeded++;
    }
    if (wholeBrainController != NULL) {
        numberOfTabsNeeded++;
    }
    
    const int32_t numberOfTabsToAdd = numberOfTabsNeeded - this->tabBar->count();
    for (int32_t i = 0; i < numberOfTabsToAdd; i++) {
        AString errorMessage;
        this->createNewTab(errorMessage);
    }
    
    int32_t tabIndex = 0;
    tabIndex = loadIntoTab(tabIndex,
                           leftSurfaceController);
    tabIndex = loadIntoTab(tabIndex,
                           rightSurfaceController);
    tabIndex = loadIntoTab(tabIndex,
                           cerebellumSurfaceController);
    tabIndex = loadIntoTab(tabIndex, 
                           surfaceMontageController);
    tabIndex = loadIntoTab(tabIndex,
                           volumeController);
    tabIndex = loadIntoTab(tabIndex,
                           wholeBrainController);
    
    const int numTabs = this->tabBar->count();
    if (numTabs > 0) {
        this->tabBar->setCurrentIndex(0);

        Brain* brain = GuiManager::get()->getBrain();
        for (int32_t i = 0; i < numTabs; i++) {
            BrowserTabContent* btc = this->getTabContentFromTab(i);
            if (btc != NULL) {
                btc->getVolumeSurfaceOutlineSet()->selectSurfacesAfterSpecFileLoaded(brain,
                                                                                     true);
            }
        }
        
        /*
         * Set the default tab to whole brain, if present
         */
        int32_t surfaceTabIndex = -1;
        int32_t montageTabIndex = -1;
        int32_t wholeBrainTabIndex = -1;
        int32_t volumeTabIndex = -1;
        for (int32_t i = 0; i < numTabs; i++) {
            BrowserTabContent* btc = getTabContentFromTab(i);
            if (btc != NULL) {
                switch (btc->getSelectedModelType()) {
                    case ModelTypeEnum::MODEL_TYPE_INVALID:
                        break;
                    case ModelTypeEnum::MODEL_TYPE_SURFACE:
                        if (surfaceTabIndex < 0) {
                            surfaceTabIndex = i;
                        }
                        break;
                    case ModelTypeEnum::MODEL_TYPE_SURFACE_MONTAGE:
                        if (montageTabIndex < 0) {
                            montageTabIndex = i;
                        }
                        break;
                    case ModelTypeEnum::MODEL_TYPE_VOLUME_SLICES:
                        if (volumeTabIndex < 0) {
                            volumeTabIndex = i;
                        }
                        break;
                    case ModelTypeEnum::MODEL_TYPE_WHOLE_BRAIN:
                        if (wholeBrainTabIndex < 0) {
                            wholeBrainTabIndex = i;
                        }
                        break;
                    case ModelTypeEnum::MODEL_TYPE_CHART:
                        break;
                }
            }
        }

        int32_t defaultTabIndex = 0;
        if (montageTabIndex >= 0) {
            defaultTabIndex = montageTabIndex;
        }
        else if (surfaceTabIndex >= 0) {
            defaultTabIndex = surfaceTabIndex;
        }
        else if (volumeTabIndex >= 0) {
            defaultTabIndex = volumeTabIndex;
        }
        else if (wholeBrainTabIndex >= 0) {
            defaultTabIndex = wholeBrainTabIndex;
        }
        
        this->tabBar->setCurrentIndex(defaultTabIndex);
    }
}

/**
 * Load a controller into the tab with the given index.
 * @param tabIndexIn
 *   Index of tab into which controller is loaded.  A
 *   new tab will be created, if needed.
 * @param controller
 *   Controller that is to be displayed in the tab.  If
 *   NULL, this method does nothing.
 * @return
 *   Index of next tab after controller is displayed.
 *   If the input controller was NULL, the returned 
 *   value is identical to the input tab index.
 */
int32_t 
BrainBrowserWindowToolBar::loadIntoTab(const int32_t tabIndexIn,
                                       Model* controller)
{
    if (tabIndexIn < 0) {
        return -1;
    }
    
    int32_t tabIndex = tabIndexIn;
    
    if (controller != NULL) {
        if (tabIndex >= this->tabBar->count()) {
            AString errorMessage;
            if (this->createNewTab(errorMessage) == NULL) {
                return -1;
            }
            tabIndex = this->tabBar->count() - 1;
        }
        
        void* p = this->tabBar->tabData(tabIndex).value<void*>();
        BrowserTabContent* btc = (BrowserTabContent*)p;
        btc->setSelectedModelType(controller->getControllerType());
        
        ModelSurface* surfaceController =
        dynamic_cast<ModelSurface*>(controller);
        if (surfaceController != NULL) {
            btc->getSurfaceModelSelector()->setSelectedStructure(surfaceController->getSurface()->getStructure());
            btc->getSurfaceModelSelector()->setSelectedSurfaceController(surfaceController);
            btc->setSelectedModelType(ModelTypeEnum::MODEL_TYPE_SURFACE);
        }
        this->updateTabName(tabIndex);
        
        tabIndex++;
    }
 
    return tabIndex;
}

/**
 * Move all but the current tab to new windows.
 */
void 
BrainBrowserWindowToolBar::moveTabsToNewWindows()
{
    int32_t numTabs = this->tabBar->count();
    if (numTabs > 1) {
        const int32_t currentIndex = this->tabBar->currentIndex();
        
        QWidget* lastParent = this->parentWidget();
        if (lastParent == NULL) {
            lastParent = this;
        }
        for (int32_t i = (numTabs - 1); i >= 0; i--) {
            if (i != currentIndex) {
                void* p = this->tabBar->tabData(i).value<void*>();
                BrowserTabContent* btc = (BrowserTabContent*)p;

                EventBrowserWindowNew eventNewWindow(lastParent, btc);
                EventManager::get()->sendEvent(eventNewWindow.getPointer());
                if (eventNewWindow.isError()) {
                    QMessageBox::critical(this,
                                          "",
                                          eventNewWindow.getErrorMessage());
                    break;
                }
                else {
                    lastParent = eventNewWindow.getBrowserWindowCreated();
                    this->tabBar->setTabData(i, qVariantFromValue((void*)NULL));
                    this->tabClosed(i);
                }
            }
        }
        
    }
    
    EventManager::get()->sendEvent(EventUserInterfaceUpdate().getPointer());
}

/**
 * Remove and return all tabs from this toolbar.
 * After this the window containing this toolbar 
 * will contain no tabs!
 *
 * @param allTabContent
 *    Will contain the content from the tabs upon return.
 */
void 
BrainBrowserWindowToolBar::removeAndReturnAllTabs(std::vector<BrowserTabContent*>& allTabContent)
{
    allTabContent.clear();
    
    int32_t numTabs = this->tabBar->count();
    for (int32_t i = (numTabs - 1); i >= 0; i--) {
        void* p = this->tabBar->tabData(i).value<void*>();
        BrowserTabContent* btc = (BrowserTabContent*)p;
        if (btc != NULL) {
            allTabContent.push_back(btc);
        }
        this->tabBar->setTabData(i, qVariantFromValue((void*)NULL));
        this->tabClosed(i);
    }
}

/**
 * Remove the tab that contains the given tab content.
 * Note: The tab content is NOT deleted and the caller must
 * either delete it or move it into a window.
 * After this method completes, the windowo may contain no tabs.
 *
 * @param browserTabContent
 */
void 
BrainBrowserWindowToolBar::removeTabWithContent(BrowserTabContent* browserTabContent)
{
    int32_t numTabs = this->tabBar->count();
    for (int32_t i = 0; i < numTabs; i++) {
        void* p = this->tabBar->tabData(i).value<void*>();
        BrowserTabContent* btc = (BrowserTabContent*)p;
        if (btc == browserTabContent) {
            this->tabBar->setTabData(i, qVariantFromValue((void*)NULL));
            this->tabClosed(i);
            if (this->tabBar->count() <= 0) {
                EventManager::get()->removeAllEventsFromListener(this);  // ignore update requests
            }
            break;
        }
    }    
}


/**
 * Select the next tab.
 */
void 
BrainBrowserWindowToolBar::nextTab()
{
    int32_t numTabs = this->tabBar->count();
    if (numTabs > 1) {
        int32_t tabIndex = this->tabBar->currentIndex();
        tabIndex++;
        if (tabIndex >= numTabs) {
            tabIndex = 0;
        }
        this->tabBar->setCurrentIndex(tabIndex);
    }
}

/**
 * Select the previous tab.
 */
void 
BrainBrowserWindowToolBar::previousTab()
{
    int32_t numTabs = this->tabBar->count();
    if (numTabs > 1) {
        int32_t tabIndex = this->tabBar->currentIndex();
        tabIndex--;
        if (tabIndex < 0) {
            tabIndex = numTabs - 1;
        }
        this->tabBar->setCurrentIndex(tabIndex);
    }
}

/**
 * Rename the current tab.
 */
void 
BrainBrowserWindowToolBar::renameTab()
{
    const int tabIndex = this->tabBar->currentIndex();
    if (tabIndex >= 0) {
        void* p = this->tabBar->tabData(tabIndex).value<void*>();
        BrowserTabContent* btc = (BrowserTabContent*)p;
        AString currentName = btc->getUserName();
        bool ok = false;
        AString newName = QInputDialog::getText(this,
                                                "Set Tab Name",
                                                "New Name (empty to reset)",
                                                QLineEdit::Normal,
                                                currentName,
                                                &ok);
        if (ok) {
            btc->setUserName(newName);
            this->updateTabName(tabIndex);
        }
    }

    
}

/**
 * Update the name of the tab at the given index.  The 
 * name is obtained from the tabs browser content.
 *
 * @param tabIndex
 *   Index of tab.
 */
void 
BrainBrowserWindowToolBar::updateTabName(const int32_t tabIndex)
{
    int32_t tabIndexForUpdate = tabIndex;
    if (tabIndexForUpdate < 0) {
        tabIndexForUpdate = this->tabBar->currentIndex();
    }
    void* p = this->tabBar->tabData(tabIndexForUpdate).value<void*>();
    AString newName = "";
    BrowserTabContent* btc = (BrowserTabContent*)p;   
    if (btc != NULL) {
        newName = btc->getName();
    }
    this->tabBar->setTabText(tabIndexForUpdate, newName);
}

/**
 * Close the selected tab.  This method is typically
 * called by the BrowswerWindow's File Menu.
 */
void 
BrainBrowserWindowToolBar::closeSelectedTab()
{
    const int tabIndex = this->tabBar->currentIndex();
    if (this->tabBar->count() > 1) {
        this->tabClosed(tabIndex);
    }
}

/**
 * Called when the selected tab is changed.
 * @param index
 *    Index of selected tab.
 */
void 
BrainBrowserWindowToolBar::selectedTabChanged(int indx)
{
    this->updateTabName(indx);
    this->updateToolBar();
    this->updateToolBox();
    emit viewedModelChanged();
    this->updateGraphicsWindow();
}

void 
BrainBrowserWindowToolBar::tabClosed(int tabIndex)
{
    CaretAssertArrayIndex(this-tabBar->tabData(), this->tabBar->count(), tabIndex);
    this->removeTab(tabIndex);
    
    if (this->isDestructionInProgress == false) {
        this->updateToolBar();
        this->updateToolBox();
        emit viewedModelChanged();
    }
}

/**
 * Remove the tab at the given index.
 * @param index
 */
void 
BrainBrowserWindowToolBar::removeTab(int tabIndex)
{
    CaretAssertArrayIndex(this-tabBar->tabData(), this->tabBar->count(), tabIndex);
    
    void* p = this->tabBar->tabData(tabIndex).value<void*>();
    if (p != NULL) {
        BrowserTabContent* btc = (BrowserTabContent*)p;
        
        EventBrowserTabDelete deleteTabEvent(btc);
        EventManager::get()->sendEvent(deleteTabEvent.getPointer());
    }
    
    this->tabBar->blockSignals(true);
    this->tabBar->removeTab(tabIndex);
    this->tabBar->blockSignals(false);

    const int numOpenTabs = this->tabBar->count();
    this->tabBar->setTabsClosable(numOpenTabs > 1);    
}

/**
 * Update the toolbar.
 */
void 
BrainBrowserWindowToolBar::updateToolBar()
{
    if (this->isDestructionInProgress) {
        return;
    }
    
    /*
     * If there are no models, close all but the first tab.
     */
    EventModelGetAll getAllModelsEvent;
    EventManager::get()->sendEvent(getAllModelsEvent.getPointer());
    if (getAllModelsEvent.getFirstModel() == NULL) {
        //for (int i = (this->tabBar->count() - 1); i >= 1; i--) {
        for (int i = (this->tabBar->count() - 1); i >= 0; i--) {
            this->removeTab(i);
        }
    }
    
    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    BrowserTabContent* browserTabContent = this->getTabContentFromSelectedTab();
    
    const ModelTypeEnum::Enum viewModel = this->updateViewWidget(browserTabContent);
    
    bool showOrientationWidget = false;
    bool showWholeBrainSurfaceOptionsWidget = false;
    bool showSingleSurfaceOptionsWidget = false;
    bool showSurfaceMontageOptionsWidget = false;
    bool showVolumeIndicesWidget = false;
    bool showVolumePlaneWidget = false;
    bool showVolumeMontageWidget = false;
    
    bool showClippingWidget = true;
    bool showToolsWidget = true;
    bool showWindowWidget = true;
    
    switch (viewModel) {
        case ModelTypeEnum::MODEL_TYPE_INVALID:
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE:
            showOrientationWidget = true;
            showSingleSurfaceOptionsWidget = true;
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE_MONTAGE:
            showOrientationWidget = true;
            showSurfaceMontageOptionsWidget = true;
            break;
        case ModelTypeEnum::MODEL_TYPE_VOLUME_SLICES:
            showVolumeIndicesWidget = true;
            showVolumePlaneWidget = true;
            showVolumeMontageWidget = true;
            break;
        case ModelTypeEnum::MODEL_TYPE_WHOLE_BRAIN:
            showOrientationWidget = true;
            showWholeBrainSurfaceOptionsWidget = true;
            showVolumeIndicesWidget = true;
            break;
        case ModelTypeEnum::MODEL_TYPE_CHART:
            break;
    }
    
    /*
     * Need to turn off display of all widgets, 
     * otherwise, the toolbar width may be overly
     * expanded with empty space as other widgets
     * are turned on and off.
     */
    this->orientationWidget->setVisible(false);
    this->wholeBrainSurfaceOptionsWidget->setVisible(false);
    this->singleSurfaceSelectionWidget->setVisible(false);
    this->surfaceMontageSelectionWidget->setVisible(false);
    this->volumeIndicesWidget->setVisible(false);
    this->volumePlaneWidget->setVisible(false);
    this->volumeMontageWidget->setVisible(false);
    this->modeWidget->setVisible(false);
    this->windowWidget->setVisible(false);
    this->clippingWidget->setVisible(false);
    
    this->orientationWidget->setVisible(showOrientationWidget);
    this->wholeBrainSurfaceOptionsWidget->setVisible(showWholeBrainSurfaceOptionsWidget);
    this->singleSurfaceSelectionWidget->setVisible(showSingleSurfaceOptionsWidget);
    this->surfaceMontageSelectionWidget->setVisible(showSurfaceMontageOptionsWidget);
    this->volumeIndicesWidget->setVisible(showVolumeIndicesWidget);
    this->volumePlaneWidget->setVisible(showVolumePlaneWidget);
    this->volumeMontageWidget->setVisible(showVolumeMontageWidget);
    this->modeWidget->setVisible(showToolsWidget);
    this->clippingWidget->setVisible(showClippingWidget);
    this->windowWidget->setVisible(showWindowWidget);

    if (browserTabContent != NULL) {
        this->updateOrientationWidget(browserTabContent);
        this->updateWholeBrainSurfaceOptionsWidget(browserTabContent);
        this->updateVolumeIndicesWidget(browserTabContent);
        this->updateSingleSurfaceOptionsWidget(browserTabContent);
        this->updateSurfaceMontageOptionsWidget(browserTabContent);
        this->updateVolumeMontageWidget(browserTabContent);
        this->updateVolumePlaneWidget(browserTabContent);
        this->updateModeWidget(browserTabContent);
        this->updateWindowWidget(browserTabContent);
        this->updateClippingWidget(browserTabContent);
    }
    
    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    if (this->updateCounter != 0) {
        CaretLogSevere("Update counter is non-zero at end of updateToolBar()");
    }
    
    this->updateTabName(-1);
    
    BrainBrowserWindow* browserWindow = GuiManager::get()->getBrowserWindowByWindowIndex(this->browserWindowIndex);
    if (browserWindow != NULL) {
        BrainBrowserWindowScreenModeEnum::Enum screenMode = browserWindow->getScreenMode();
        
        bool showToolBar = false;
        bool showToolBarWidget = false;
        switch (screenMode) {
            case BrainBrowserWindowScreenModeEnum::NORMAL:
                showToolBar = true;
                showToolBarWidget = this->toolBarToolButtonAction->isChecked(); // JWH FIX Toolbar turned after ID event
                break;
            case BrainBrowserWindowScreenModeEnum::FULL_SCREEN:
                /*
                 * Display all of tab bar (tabs and tool buttons) only if more than one tab
                 */
                if (this->tabBar->count() > 1) {
                    showToolBar = true;
                    showToolBarWidget = false;
                }
                break;
            case BrainBrowserWindowScreenModeEnum::TAB_MONTAGE:
                break;
            case BrainBrowserWindowScreenModeEnum::TAB_MONTAGE_FULL_SCREEN:
                break;
        }

        this->setVisible(showToolBar);
        if (showToolBar) {
            /*
             * Gets disabled by main window when switching view modes
             */
            const bool enabledStatus = this->toolBarToolButtonAction->isEnabled();
            this->toolBarToolButtonAction->setEnabled(true);
            if (this->toolBarToolButtonAction->isChecked() != showToolBarWidget) {
                this->toolBarToolButtonAction->trigger();
            }
            this->toolBarToolButtonAction->setEnabled(enabledStatus);
        }
    }
}

/**
 * Create the view widget.
 *
 * @return The view widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createViewWidget()
{
    this->viewModeSurfaceRadioButton = new QRadioButton("Surface");
    this->viewModeSurfaceMontageRadioButton = new QRadioButton("Montage");
//    this->viewModeSurfaceMontageRadioButton->setText("Surface\nMontage");
    this->viewModeVolumeRadioButton = new QRadioButton("Volume");
    this->viewModeWholeBrainRadioButton = new QRadioButton("All");
    //this->viewModeChartRadioButton = new QRadioButton("Chart");
    
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    WuQtUtilities::setLayoutMargins(layout, 4, 2);
    layout->addWidget(this->viewModeSurfaceRadioButton);
    layout->addWidget(this->viewModeSurfaceMontageRadioButton);
    layout->addWidget(this->viewModeVolumeRadioButton);
    layout->addWidget(this->viewModeWholeBrainRadioButton);
    //layout->addWidget(this->viewModeChartRadioButton);
    layout->addStretch();

    QButtonGroup* viewModeRadioButtonGroup = new QButtonGroup(this);
    viewModeRadioButtonGroup->addButton(this->viewModeSurfaceRadioButton);
    viewModeRadioButtonGroup->addButton(this->viewModeSurfaceMontageRadioButton);
    viewModeRadioButtonGroup->addButton(this->viewModeVolumeRadioButton);
    viewModeRadioButtonGroup->addButton(this->viewModeWholeBrainRadioButton);
    //viewModeRadioButtonGroup->addButton(this->viewModeChartRadioButton);
    QObject::connect(viewModeRadioButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)),
                     this, SLOT(viewModeRadioButtonClicked(QAbstractButton*)));
    
    this->viewWidgetGroup = new WuQWidgetObjectGroup(this);
    this->viewWidgetGroup->add(this->viewModeSurfaceRadioButton);
    this->viewWidgetGroup->add(this->viewModeSurfaceMontageRadioButton);
    this->viewWidgetGroup->add(this->viewModeVolumeRadioButton);
    this->viewWidgetGroup->add(this->viewModeWholeBrainRadioButton);
    //this->viewWidgetGroup->add(this->viewModeChartRadioButton);
    
    QWidget* w = this->createToolWidget("View", 
                                        widget, 
                                        WIDGET_PLACEMENT_NONE, 
                                        WIDGET_PLACEMENT_TOP,
                                        0);
    return w;
}

/**
 * Update the view widget.
 * 
 * @param browserTabContent
 *   Content in the tab.
 * @return 
 *   An enumerated type indicating the type of model being viewed.
 */
ModelTypeEnum::Enum
BrainBrowserWindowToolBar::updateViewWidget(BrowserTabContent* browserTabContent)
{
    ModelTypeEnum::Enum modelType = ModelTypeEnum::MODEL_TYPE_INVALID;
    if (browserTabContent != NULL) {
        modelType = browserTabContent->getSelectedModelType();
    }
    
    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    this->viewWidgetGroup->blockAllSignals(true);
    
    /*
     * Enable buttons for valid types
     */
    if (browserTabContent != NULL) {
        this->viewModeSurfaceRadioButton->setEnabled(browserTabContent->isSurfaceModelValid());
        this->viewModeSurfaceMontageRadioButton->setEnabled(browserTabContent->isSurfaceMontageModelValid());
        this->viewModeVolumeRadioButton->setEnabled(browserTabContent->isVolumeSliceModelValid());
        this->viewModeWholeBrainRadioButton->setEnabled(browserTabContent->isWholeBrainModelValid());
        //this->viewModeChartRadioButton->setEnabled(browserTabContent->isChartValid());
    }
    
    switch (modelType) {
        case ModelTypeEnum::MODEL_TYPE_INVALID:
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE:
            this->viewModeSurfaceRadioButton->setChecked(true);
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE_MONTAGE:
            this->viewModeSurfaceMontageRadioButton->setChecked(true);
            break;
        case ModelTypeEnum::MODEL_TYPE_VOLUME_SLICES:
            this->viewModeVolumeRadioButton->setChecked(true);
            break;
        case ModelTypeEnum::MODEL_TYPE_WHOLE_BRAIN:
            this->viewModeWholeBrainRadioButton->setChecked(true);
            break;
        case ModelTypeEnum::MODEL_TYPE_CHART:
            this->viewModeChartRadioButton->setChecked(true);
            break;
    }
    
    this->viewWidgetGroup->blockAllSignals(false);

    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    return modelType;
}

/**
 * Create the orientation widget.
 *
 * @return  The orientation widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createOrientationWidget()
{
    this->viewOrientationLeftIcon = WuQtUtilities::loadIcon(":/view-left.png");
    this->viewOrientationRightIcon = WuQtUtilities::loadIcon(":/view-right.png");
    this->viewOrientationAnteriorIcon = WuQtUtilities::loadIcon(":/view-anterior.png");
    this->viewOrientationPosteriorIcon = WuQtUtilities::loadIcon(":/view-posterior.png");
    this->viewOrientationDorsalIcon = WuQtUtilities::loadIcon(":/view-dorsal.png");
    this->viewOrientationVentralIcon = WuQtUtilities::loadIcon(":/view-ventral.png");
    this->viewOrientationLeftLateralIcon = WuQtUtilities::loadIcon(":/view-left-lateral.png");
    this->viewOrientationLeftMedialIcon = WuQtUtilities::loadIcon(":/view-left-medial.png");
    this->viewOrientationRightLateralIcon = WuQtUtilities::loadIcon(":/view-right-lateral.png");
    this->viewOrientationRightMedialIcon = WuQtUtilities::loadIcon(":/view-right-medial.png");
    
    this->orientationLeftOrLateralToolButtonAction = WuQtUtilities::createAction("L", 
                                                                        "View from a LEFT perspective", 
                                                                        this, 
                                                                        this, 
                                                                        SLOT(orientationLeftOrLateralToolButtonTriggered(bool)));
    if (this->viewOrientationLeftIcon != NULL) {
        this->orientationLeftOrLateralToolButtonAction->setIcon(*this->viewOrientationLeftIcon);
    }
    else {
        this->orientationLeftOrLateralToolButtonAction->setIconText("L");
    }
    
    this->orientationRightOrMedialToolButtonAction = WuQtUtilities::createAction("R", 
                                                                         "View from a RIGHT perspective", 
                                                                         this, 
                                                                         this, 
                                                                         SLOT(orientationRightOrMedialToolButtonTriggered(bool)));
    if (this->viewOrientationRightIcon != NULL) {
        this->orientationRightOrMedialToolButtonAction->setIcon(*this->viewOrientationRightIcon);
    }
    else {
        this->orientationRightOrMedialToolButtonAction->setIconText("R");
    }
    
    this->orientationAnteriorToolButtonAction = WuQtUtilities::createAction("A", 
                                                                            "View from an ANTERIOR perspective", 
                                                                            this, 
                                                                            this, 
                                                                            SLOT(orientationAnteriorToolButtonTriggered(bool)));
    if (this->viewOrientationAnteriorIcon != NULL) {
        this->orientationAnteriorToolButtonAction->setIcon(*this->viewOrientationAnteriorIcon);
    }
    else {
        this->orientationAnteriorToolButtonAction->setIconText("A");
    }
    
    this->orientationPosteriorToolButtonAction = WuQtUtilities::createAction("P", 
                                                                             "View from a POSTERIOR perspective", 
                                                                             this, 
                                                                             this, 
                                                                             SLOT(orientationPosteriorToolButtonTriggered(bool)));
    if (this->viewOrientationPosteriorIcon != NULL) {
        this->orientationPosteriorToolButtonAction->setIcon(*this->viewOrientationPosteriorIcon);
    }
    else {
        this->orientationPosteriorToolButtonAction->setIconText("P");
    }
    
    this->orientationDorsalToolButtonAction = WuQtUtilities::createAction("D", 
                                                                          "View from a DORSAL perspective", 
                                                                          this, 
                                                                          this, 
                                                                          SLOT(orientationDorsalToolButtonTriggered(bool)));
    if (this->viewOrientationDorsalIcon != NULL) {
        this->orientationDorsalToolButtonAction->setIcon(*this->viewOrientationDorsalIcon);
    }
    else {
        this->orientationDorsalToolButtonAction->setIconText("D");
    }
    
    this->orientationVentralToolButtonAction = WuQtUtilities::createAction("V", 
                                                                           "View from a VENTRAL perspective", 
                                                                           this, 
                                                                           this, 
                                                                           SLOT(orientationVentralToolButtonTriggered(bool)));
    if (this->viewOrientationVentralIcon != NULL) {
        this->orientationVentralToolButtonAction->setIcon(*this->viewOrientationVentralIcon);
    }
    else {
        this->orientationVentralToolButtonAction->setIconText("V");
    }
    
    
    this->orientationLateralMedialToolButtonAction = WuQtUtilities::createAction("LM",
                                                                                 "View from a Lateral/Medial perspective", 
                                                                                 this, 
                                                                                 this, 
                                                                                 SLOT(orientationLateralMedialToolButtonTriggered(bool)));
    
    this->orientationDorsalVentralToolButtonAction = WuQtUtilities::createAction("DV",
                                                                                    "View from a Dorsal/Ventral perspective", 
                                                                                    this, 
                                                                                    this, 
                                                                                    SLOT(orientationDorsalVentralToolButtonTriggered(bool)));
    
    this->orientationAnteriorPosteriorToolButtonAction = WuQtUtilities::createAction("AP", 
                                                                                        "View from a Anterior/Posterior perspective", 
                                                                                        this, 
                                                                                        this, 
                                                                                        SLOT(orientationAnteriorPosteriorToolButtonTriggered(bool)));
    
    this->orientationResetToolButtonAction = WuQtUtilities::createAction("R\nE\nS\nE\nT", //"Reset",
                                                                         "Reset the view to dorsal and remove any panning or zooming", 
                                                                         this, 
                                                                         this, 
                                                                         SLOT(orientationResetToolButtonTriggered(bool)));
    
    const QString customToolTip = ("Pressing the \"Custom\" button displays a dialog for creating and editing orientations.\n"
                                   "Pressing the arrow button will display a menu for selection of custom orientations.\n"
                                   "Note that custom orientations are stored in your Workbench's preferences and thus\n"
                                   "will be availble in any concurrent or future instances of Workbench.");
    this->orientationCustomViewSelectToolButtonAction = WuQtUtilities::createAction("Custom",
                                                                                  customToolTip,
                                                                                  this,
                                                                                  this,
                                                                                  SLOT(orientationCustomViewToolButtonTriggered()));

    this->orientationLeftOrLateralToolButton = new QToolButton();
    this->orientationLeftOrLateralToolButton->setDefaultAction(this->orientationLeftOrLateralToolButtonAction);
    
    this->orientationRightOrMedialToolButton = new QToolButton();
    this->orientationRightOrMedialToolButton->setDefaultAction(this->orientationRightOrMedialToolButtonAction);
    
    this->orientationAnteriorToolButton = new QToolButton();
    this->orientationAnteriorToolButton->setDefaultAction(this->orientationAnteriorToolButtonAction);
    
    this->orientationPosteriorToolButton = new QToolButton();
    this->orientationPosteriorToolButton->setDefaultAction(this->orientationPosteriorToolButtonAction);
    
    this->orientationDorsalToolButton = new QToolButton();
    this->orientationDorsalToolButton->setDefaultAction(this->orientationDorsalToolButtonAction);
    
    this->orientationVentralToolButton = new QToolButton();
    this->orientationVentralToolButton->setDefaultAction(this->orientationVentralToolButtonAction);
    
    this->orientationLateralMedialToolButton = new QToolButton();
    this->orientationLateralMedialToolButton->setDefaultAction(this->orientationLateralMedialToolButtonAction);
    
    this->orientationDorsalVentralToolButton = new QToolButton();
    this->orientationDorsalVentralToolButton->setDefaultAction(this->orientationDorsalVentralToolButtonAction);
    
    this->orientationAnteriorPosteriorToolButton = new QToolButton();
    this->orientationAnteriorPosteriorToolButton->setDefaultAction(this->orientationAnteriorPosteriorToolButtonAction);
    
    WuQtUtilities::matchWidgetWidths(this->orientationLateralMedialToolButton,
                                     this->orientationDorsalVentralToolButton,
                                     this->orientationAnteriorPosteriorToolButton);
    
    QToolButton* orientationResetToolButton = new QToolButton();
    orientationResetToolButton->setDefaultAction(this->orientationResetToolButtonAction);

    this->orientationCustomViewSelectToolButton = new QToolButton();
    this->orientationCustomViewSelectToolButton->setDefaultAction(this->orientationCustomViewSelectToolButtonAction);
    this->orientationCustomViewSelectToolButton->setSizePolicy(QSizePolicy::Minimum,
                                                               QSizePolicy::Fixed);
    
    QGridLayout* buttonGridLayout = new QGridLayout();
    buttonGridLayout->setColumnStretch(3, 100);
    WuQtUtilities::setLayoutMargins(buttonGridLayout, 0, 0);
    buttonGridLayout->addWidget(this->orientationLeftOrLateralToolButton,      0, 0);
    buttonGridLayout->addWidget(this->orientationRightOrMedialToolButton,     0, 1);
    buttonGridLayout->addWidget(this->orientationDorsalToolButton,    1, 0);
    buttonGridLayout->addWidget(this->orientationVentralToolButton,   1, 1);
    buttonGridLayout->addWidget(this->orientationAnteriorToolButton,  2, 0);
    buttonGridLayout->addWidget(this->orientationPosteriorToolButton, 2, 1);
    buttonGridLayout->addWidget(this->orientationLateralMedialToolButton, 0, 2);
    buttonGridLayout->addWidget(this->orientationDorsalVentralToolButton, 1, 2);
    buttonGridLayout->addWidget(this->orientationAnteriorPosteriorToolButton, 2, 2);
    buttonGridLayout->addWidget(this->orientationCustomViewSelectToolButton, 3, 0, 1, 5, Qt::AlignHCenter);
    buttonGridLayout->addWidget(orientationResetToolButton, 0, 4, 3, 1);
    
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(w);
    WuQtUtilities::setLayoutMargins(layout, 0, 0);
    layout->addLayout(buttonGridLayout);
    
    this->orientationWidgetGroup = new WuQWidgetObjectGroup(this);
    this->orientationWidgetGroup->add(this->orientationLeftOrLateralToolButtonAction);
    this->orientationWidgetGroup->add(this->orientationRightOrMedialToolButtonAction);
    this->orientationWidgetGroup->add(this->orientationAnteriorToolButtonAction);
    this->orientationWidgetGroup->add(this->orientationPosteriorToolButtonAction);
    this->orientationWidgetGroup->add(this->orientationDorsalToolButtonAction);
    this->orientationWidgetGroup->add(this->orientationVentralToolButtonAction);
    this->orientationWidgetGroup->add(this->orientationResetToolButtonAction);
    this->orientationWidgetGroup->add(this->orientationCustomViewSelectToolButtonAction);

    QWidget* orientWidget = this->createToolWidget("Orientation", 
                                                   w, 
                                                   WIDGET_PLACEMENT_LEFT, 
                                                   WIDGET_PLACEMENT_TOP,
                                                   0);
    orientWidget->setVisible(false);
    
    return orientWidget;
}

/**
 * Update the orientation widget.
 * 
 * @param modelDisplayController
 *   The active model display controller (may be NULL).
 */
void 
BrainBrowserWindowToolBar::updateOrientationWidget(BrowserTabContent* /*browserTabContent*/)
{
    if (this->orientationWidget->isHidden()) {
        return;
    }
    
    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    this->orientationWidgetGroup->blockAllSignals(true);
    
    const Model* mdc = this->getDisplayedModelController();
    if (mdc != NULL) {
        const ModelSurface* mdcs = dynamic_cast<const ModelSurface*>(mdc);
        const ModelSurfaceMontage* mdcsm = dynamic_cast<const ModelSurfaceMontage*>(mdc);
        const ModelVolume* mdcv = dynamic_cast<const ModelVolume*>(mdc);
        const ModelWholeBrain* mdcwb = dynamic_cast<const ModelWholeBrain*>(mdc);
        
        bool rightFlag = false;
        bool leftFlag = false;
        bool leftRightFlag = false;
        bool showCombinedViewOrientationButtons = false;
        
        if (mdcs != NULL) {
            const Surface* surface = mdcs->getSurface();
            const StructureEnum::Enum structure = surface->getStructure();
            if (StructureEnum::isLeft(structure)) {
                leftFlag = true;
            }
            else if (StructureEnum::isRight(structure)) {
                rightFlag = true;
            }
            else {
                leftRightFlag = true;
            }
        }
        else if (mdcsm != NULL) {
            showCombinedViewOrientationButtons = true;
        }
        else if (mdcv != NULL) {
            // nothing
        }
        else if (mdcwb != NULL) {
            leftRightFlag = true;
        }
        else {
            CaretAssertMessage(0, "Unknown model display controller type");
        }
        
        if (rightFlag || leftFlag) {
            if (rightFlag) {
                if (this->viewOrientationRightLateralIcon != NULL) {
                    this->orientationLeftOrLateralToolButtonAction->setIcon(*this->viewOrientationRightLateralIcon);
                }
                else {
                    this->orientationLeftOrLateralToolButtonAction->setIconText("L");
                }
                if (this->viewOrientationRightMedialIcon != NULL) {
                    this->orientationRightOrMedialToolButtonAction->setIcon(*this->viewOrientationRightMedialIcon);
                }
                else {
                    this->orientationRightOrMedialToolButtonAction->setIconText("M");
                }
            }
            else if (leftFlag) {
                if (this->viewOrientationLeftLateralIcon != NULL) {
                    this->orientationLeftOrLateralToolButtonAction->setIcon(*this->viewOrientationLeftLateralIcon);
                }
                else {
                    this->orientationLeftOrLateralToolButtonAction->setIconText("L");
                }
                if (this->viewOrientationLeftMedialIcon != NULL) {
                    this->orientationRightOrMedialToolButtonAction->setIcon(*this->viewOrientationLeftMedialIcon);
                }
                else {
                    this->orientationRightOrMedialToolButtonAction->setIconText("M");
                }
            }
            WuQtUtilities::setToolTipAndStatusTip(this->orientationLeftOrLateralToolButtonAction, 
                                                  "View from a LATERAL perspective");
            WuQtUtilities::setToolTipAndStatusTip(this->orientationRightOrMedialToolButtonAction, 
                                                  "View from a MEDIAL perspective");
        }
        else if (leftRightFlag) {
            if (this->viewOrientationLeftIcon != NULL) {
                this->orientationLeftOrLateralToolButtonAction->setIcon(*this->viewOrientationLeftIcon);
            }
            else {
                this->orientationLeftOrLateralToolButtonAction->setIconText("L");
            }
            if (this->viewOrientationRightIcon != NULL) {
                this->orientationRightOrMedialToolButtonAction->setIcon(*this->viewOrientationRightIcon);
            }
            else {
                this->orientationRightOrMedialToolButtonAction->setIconText("R");
            }
            WuQtUtilities::setToolTipAndStatusTip(this->orientationLeftOrLateralToolButtonAction, 
                                                  "View from a LEFT perspective");
            WuQtUtilities::setToolTipAndStatusTip(this->orientationRightOrMedialToolButtonAction, 
                                                  "View from a RIGHT perspective");
        }

        this->orientationLateralMedialToolButton->setVisible(showCombinedViewOrientationButtons);
        this->orientationDorsalVentralToolButton->setVisible(showCombinedViewOrientationButtons);
        this->orientationAnteriorPosteriorToolButton->setVisible(showCombinedViewOrientationButtons);
    
        this->orientationLeftOrLateralToolButton->setVisible(showCombinedViewOrientationButtons == false);
        this->orientationRightOrMedialToolButton->setVisible(showCombinedViewOrientationButtons == false);
        this->orientationDorsalToolButton->setVisible(showCombinedViewOrientationButtons == false);
        this->orientationVentralToolButton->setVisible(showCombinedViewOrientationButtons == false);
        this->orientationAnteriorToolButton->setVisible(showCombinedViewOrientationButtons == false);
        this->orientationPosteriorToolButton->setVisible(showCombinedViewOrientationButtons == false);
    }
    this->orientationWidgetGroup->blockAllSignals(false);
        
    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
}

/**
 * Create the whole brain surface options widget.
 *
 * @return The whole brain surface options widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createWholeBrainSurfaceOptionsWidget()
{
    
    this->wholeBrainSurfaceTypeComboBox = WuQFactory::newComboBox();
    WuQtUtilities::setToolTipAndStatusTip(this->wholeBrainSurfaceTypeComboBox,
                                          "Select the geometric type of surface for display");
    QObject::connect(this->wholeBrainSurfaceTypeComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(wholeBrainSurfaceTypeComboBoxIndexChanged(int)));
    
    /*
     * Left
     */
    this->wholeBrainSurfaceLeftCheckBox = new QCheckBox(" ");
    WuQtUtilities::setToolTipAndStatusTip(this->wholeBrainSurfaceLeftCheckBox,
                                          "Enable/Disable display of the left cortical surface");
    QObject::connect(this->wholeBrainSurfaceLeftCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(wholeBrainSurfaceLeftCheckBoxStateChanged(int)));
    
    QAction* leftSurfaceAction = WuQtUtilities::createAction("Left", 
                                                             "Select the whole brain left surface", 
                                                             this, 
                                                             this, 
                                                             SLOT(wholeBrainSurfaceLeftToolButtonTriggered(bool)));
    QToolButton* wholeBrainLeftSurfaceToolButton = new QToolButton();
    wholeBrainLeftSurfaceToolButton->setDefaultAction(leftSurfaceAction);
    
    /*
     * Right
     */
    this->wholeBrainSurfaceRightCheckBox = new QCheckBox(" ");
    WuQtUtilities::setToolTipAndStatusTip(this->wholeBrainSurfaceRightCheckBox,
                                          "Enable/Disable display of the right cortical surface");
    QObject::connect(this->wholeBrainSurfaceRightCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(wholeBrainSurfaceRightCheckBoxStateChanged(int)));
    
    QAction* rightSurfaceAction = WuQtUtilities::createAction("Right", 
                                                             "Select the whole brain right surface", 
                                                             this,
                                                             this, 
                                                             SLOT(wholeBrainSurfaceRightToolButtonTriggered(bool)));
    QToolButton* wholeBrainRightSurfaceToolButton = new QToolButton();
    wholeBrainRightSurfaceToolButton->setDefaultAction(rightSurfaceAction);
    
    /*
     * Cerebellum
     */
    this->wholeBrainSurfaceCerebellumCheckBox = new QCheckBox(" ");
    WuQtUtilities::setToolTipAndStatusTip(this->wholeBrainSurfaceCerebellumCheckBox,
                                          "Enable/Disable display of the cerebellum surface");
    QObject::connect(this->wholeBrainSurfaceCerebellumCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(wholeBrainSurfaceCerebellumCheckBoxStateChanged(int)));
    
    QAction* cerebellumSurfaceAction = WuQtUtilities::createAction("Cerebellum", 
                                                              "Select the whole brain cerebellum surface", 
                                                              this, 
                                                              this, 
                                                              SLOT(wholeBrainSurfaceCerebellumToolButtonTriggered(bool)));
    QToolButton* wholeBrainCerebellumSurfaceToolButton = new QToolButton();
    wholeBrainCerebellumSurfaceToolButton->setDefaultAction(cerebellumSurfaceAction);

    /*
     * Left/Right separation
     */
    const int separationSpinngerWidth = 48;
    this->wholeBrainSurfaceSeparationLeftRightSpinBox = WuQFactory::newDoubleSpinBox();
    this->wholeBrainSurfaceSeparationLeftRightSpinBox->setDecimals(0);
    this->wholeBrainSurfaceSeparationLeftRightSpinBox->setFixedWidth(separationSpinngerWidth);
    this->wholeBrainSurfaceSeparationLeftRightSpinBox->setMinimum(-100000.0);
    this->wholeBrainSurfaceSeparationLeftRightSpinBox->setMaximum(100000.0);
    WuQtUtilities::setToolTipAndStatusTip(this->wholeBrainSurfaceSeparationLeftRightSpinBox,
                                          "Adjust the separation of the left and right cortical surfaces");
    QObject::connect(this->wholeBrainSurfaceSeparationLeftRightSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(wholeBrainSurfaceSeparationLeftRightSpinBoxValueChanged(double)));
    
    /*
     * Cerebellum separation
     */
    this->wholeBrainSurfaceSeparationCerebellumSpinBox = WuQFactory::newDoubleSpinBox();
    this->wholeBrainSurfaceSeparationCerebellumSpinBox->setDecimals(0);
    this->wholeBrainSurfaceSeparationCerebellumSpinBox->setFixedWidth(separationSpinngerWidth);
    this->wholeBrainSurfaceSeparationCerebellumSpinBox->setMinimum(-100000.0);
    this->wholeBrainSurfaceSeparationCerebellumSpinBox->setMaximum(100000.0);
    WuQtUtilities::setToolTipAndStatusTip(this->wholeBrainSurfaceSeparationCerebellumSpinBox,
                                          "Adjust the separation of the cerebellum from the left and right cortical surfaces");
    QObject::connect(this->wholeBrainSurfaceSeparationCerebellumSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(wholeBrainSurfaceSeparationCerebellumSpinBoxSelected(double)));
    
    

    QLabel* columnTwoSpaceLabel = new QLabel(" ");
    wholeBrainLeftSurfaceToolButton->setText("L");
    wholeBrainRightSurfaceToolButton->setText("R");
    wholeBrainCerebellumSurfaceToolButton->setText("C");
    
    bool originalFlag = false;
    QGridLayout* gridLayout = new QGridLayout();
    if (originalFlag) {
        gridLayout->setVerticalSpacing(2);
        gridLayout->setHorizontalSpacing(2);
        gridLayout->addWidget(this->wholeBrainSurfaceTypeComboBox, 0, 0, 1, 6);
        gridLayout->addWidget(this->wholeBrainSurfaceLeftCheckBox, 1, 0);
        gridLayout->addWidget(wholeBrainLeftSurfaceToolButton, 1, 1);
        gridLayout->addWidget(columnTwoSpaceLabel, 1, 2);
        gridLayout->addWidget(this->wholeBrainSurfaceRightCheckBox, 1, 3);
        gridLayout->addWidget(wholeBrainRightSurfaceToolButton, 1, 4);
        gridLayout->addWidget(this->wholeBrainSurfaceSeparationLeftRightSpinBox, 1, 5);
        gridLayout->addWidget(this->wholeBrainSurfaceCerebellumCheckBox, 2, 0);
        gridLayout->addWidget(wholeBrainCerebellumSurfaceToolButton, 2, 1);
        gridLayout->addWidget(this->wholeBrainSurfaceSeparationCerebellumSpinBox, 2, 5);
    }
    else {
        gridLayout->setVerticalSpacing(2);
        gridLayout->setHorizontalSpacing(2);
        gridLayout->addWidget(this->wholeBrainSurfaceTypeComboBox, 0, 0, 1, 6);
        gridLayout->addWidget(this->wholeBrainSurfaceLeftCheckBox, 1, 0);
        gridLayout->addWidget(wholeBrainLeftSurfaceToolButton, 1, 1);
        gridLayout->addWidget(this->wholeBrainSurfaceRightCheckBox, 2, 0);
        gridLayout->addWidget(wholeBrainRightSurfaceToolButton, 2, 1);
        gridLayout->addWidget(this->wholeBrainSurfaceCerebellumCheckBox, 3, 0);
        gridLayout->addWidget(wholeBrainCerebellumSurfaceToolButton, 3, 1);
        gridLayout->addWidget(this->wholeBrainSurfaceSeparationLeftRightSpinBox, 1, 2, 2, 1);
        gridLayout->addWidget(this->wholeBrainSurfaceSeparationCerebellumSpinBox, 3, 2);
    }
    
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    WuQtUtilities::setLayoutMargins(layout, 0, 0);
    layout->addLayout(gridLayout);
    
    this->wholeBrainSurfaceOptionsWidgetGroup = new WuQWidgetObjectGroup(this);
    this->wholeBrainSurfaceOptionsWidgetGroup->add(this->wholeBrainSurfaceTypeComboBox);
    this->wholeBrainSurfaceOptionsWidgetGroup->add(this->wholeBrainSurfaceLeftCheckBox);
    this->wholeBrainSurfaceOptionsWidgetGroup->add(wholeBrainLeftSurfaceToolButton);
    this->wholeBrainSurfaceOptionsWidgetGroup->add(this->wholeBrainSurfaceRightCheckBox);
    this->wholeBrainSurfaceOptionsWidgetGroup->add(wholeBrainRightSurfaceToolButton);
    this->wholeBrainSurfaceOptionsWidgetGroup->add(this->wholeBrainSurfaceCerebellumCheckBox);
    this->wholeBrainSurfaceOptionsWidgetGroup->add(wholeBrainCerebellumSurfaceToolButton);
    this->wholeBrainSurfaceOptionsWidgetGroup->add(this->wholeBrainSurfaceSeparationLeftRightSpinBox);
    this->wholeBrainSurfaceOptionsWidgetGroup->add(this->wholeBrainSurfaceSeparationCerebellumSpinBox);
    
    QWidget* w = this->createToolWidget("Surface Viewing", 
                                        widget, 
                                        WIDGET_PLACEMENT_LEFT, 
                                        WIDGET_PLACEMENT_TOP,
                                        0);
    w->setVisible(false);
    return w;
}

/**
 * Update the whole brain surface options widget.
 * 
 * @param modelDisplayController
 *   The active model display controller (may be NULL).
 */
void 
BrainBrowserWindowToolBar::updateWholeBrainSurfaceOptionsWidget(BrowserTabContent* browserTabContent)
{
    if (this->wholeBrainSurfaceOptionsWidget->isHidden()) {
        return;
    }
    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
 
    ModelWholeBrain* wholeBrainController = browserTabContent->getDisplayedWholeBrainModel();
    const int32_t tabNumber = browserTabContent->getTabNumber();
    
    this->wholeBrainSurfaceOptionsWidgetGroup->blockAllSignals(true);
    
    std::vector<SurfaceTypeEnum::Enum> availableSurfaceTypes;
    wholeBrainController->getAvailableSurfaceTypes(availableSurfaceTypes);
    
    const SurfaceTypeEnum::Enum selectedSurfaceType = wholeBrainController->getSelectedSurfaceType(tabNumber);
    
    int32_t defaultIndex = 0;
    this->wholeBrainSurfaceTypeComboBox->clear();
    int32_t numSurfaceTypes = static_cast<int32_t>(availableSurfaceTypes.size());
    for (int32_t i = 0; i < numSurfaceTypes; i++) {
        const SurfaceTypeEnum::Enum st = availableSurfaceTypes[i];
        if (st == selectedSurfaceType) {
            defaultIndex = this->wholeBrainSurfaceTypeComboBox->count();
        }
        const AString name = SurfaceTypeEnum::toGuiName(st);
        const int integerCode = SurfaceTypeEnum::toIntegerCode(st);
        this->wholeBrainSurfaceTypeComboBox->addItem(name,
                                                     integerCode);
    }
    if (defaultIndex < this->wholeBrainSurfaceTypeComboBox->count()) {
        this->wholeBrainSurfaceTypeComboBox->setCurrentIndex(defaultIndex);
    }
    
    this->wholeBrainSurfaceLeftCheckBox->setChecked(browserTabContent->isWholeBrainLeftEnabled());
    this->wholeBrainSurfaceRightCheckBox->setChecked(browserTabContent->isWholeBrainRightEnabled());
    this->wholeBrainSurfaceCerebellumCheckBox->setChecked(browserTabContent->isWholeBrainCerebellumEnabled());
    
    this->wholeBrainSurfaceSeparationLeftRightSpinBox->setValue(browserTabContent->getWholeBrainLeftRightSeparation());
    this->wholeBrainSurfaceSeparationCerebellumSpinBox->setValue(browserTabContent->getWholeBrainCerebellumSeparation());
    
    this->wholeBrainSurfaceOptionsWidgetGroup->blockAllSignals(false);
    
    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
}

/**
 * Create the volume indices widget.
 *
 * @return  The volume indices widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createVolumeIndicesWidget()
{
    QAction* volumeIndicesOriginToolButtonAction = WuQtUtilities::createAction("O\nR\nI\nG\nI\nN", 
                                                                              "Set the slice indices to the origin, \n"
                                                                              "stereotaxic coordinate (0, 0, 0)", 
                                                                              this, 
                                                                              this, 
                                                                              SLOT(volumeIndicesOriginActionTriggered()));
    QToolButton* volumeIndicesOriginToolButton = new QToolButton;
    volumeIndicesOriginToolButton->setDefaultAction(volumeIndicesOriginToolButtonAction);
    
    QLabel* parasagittalLabel = new QLabel("P:");
    QLabel* coronalLabel = new QLabel("C:");
    QLabel* axialLabel = new QLabel("A:");
    
    this->volumeIndicesParasagittalCheckBox = new QCheckBox(" ");
    WuQtUtilities::setToolTipAndStatusTip(this->volumeIndicesParasagittalCheckBox,
                                          "Enable/Disable display of PARASAGITTAL slice");
    QObject::connect(this->volumeIndicesParasagittalCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(volumeIndicesParasagittalCheckBoxStateChanged(int)));
    
    this->volumeIndicesCoronalCheckBox = new QCheckBox(" ");
    WuQtUtilities::setToolTipAndStatusTip(this->volumeIndicesCoronalCheckBox,
                                          "Enable/Disable display of CORONAL slice");
    QObject::connect(this->volumeIndicesCoronalCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(volumeIndicesCoronalCheckBoxStateChanged(int)));
    
    this->volumeIndicesAxialCheckBox = new QCheckBox(" ");
    WuQtUtilities::setToolTipAndStatusTip(this->volumeIndicesAxialCheckBox,
                                          "Enable/Disable display of AXIAL slice");
    
    QObject::connect(this->volumeIndicesAxialCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(volumeIndicesAxialCheckBoxStateChanged(int)));
    
    const int sliceIndexSpinBoxWidth = 55;
    const int sliceCoordinateSpinBoxWidth = 60;
    
    this->volumeIndicesParasagittalSpinBox = WuQFactory::newSpinBox();
    this->volumeIndicesParasagittalSpinBox->setFixedWidth(sliceIndexSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(this->volumeIndicesParasagittalSpinBox,
                                          "Change the selected PARASAGITTAL slice");
    QObject::connect(this->volumeIndicesParasagittalSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(volumeIndicesParasagittalSpinBoxValueChanged(int)));
    
    this->volumeIndicesCoronalSpinBox = WuQFactory::newSpinBox();
    this->volumeIndicesCoronalSpinBox->setFixedWidth(sliceIndexSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(this->volumeIndicesCoronalSpinBox,
                                          "Change the selected CORONAL slice");
    QObject::connect(this->volumeIndicesCoronalSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(volumeIndicesCoronalSpinBoxValueChanged(int)));
    
    this->volumeIndicesAxialSpinBox = WuQFactory::newSpinBox();
    this->volumeIndicesAxialSpinBox->setFixedWidth(sliceIndexSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(this->volumeIndicesAxialSpinBox,
                                          "Change the selected AXIAL slice");
    QObject::connect(this->volumeIndicesAxialSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(volumeIndicesAxialSpinBoxValueChanged(int)));
    
    this->volumeIndicesXcoordSpinBox = WuQFactory::newDoubleSpinBox();
    this->volumeIndicesXcoordSpinBox->setDecimals(1);
    this->volumeIndicesXcoordSpinBox->setFixedWidth(sliceCoordinateSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(this->volumeIndicesXcoordSpinBox,
                                          "Adjust coordinate to select PARASAGITTAL slice");
    QObject::connect(this->volumeIndicesXcoordSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(volumeIndicesXcoordSpinBoxValueChanged(double)));
    
    this->volumeIndicesYcoordSpinBox = WuQFactory::newDoubleSpinBox();
    this->volumeIndicesYcoordSpinBox->setDecimals(1);
    this->volumeIndicesYcoordSpinBox->setFixedWidth(sliceCoordinateSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(this->volumeIndicesYcoordSpinBox,
                                          "Adjust coordinate to select CORONAL slice");
    QObject::connect(this->volumeIndicesYcoordSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(volumeIndicesYcoordSpinBoxValueChanged(double)));
    
    this->volumeIndicesZcoordSpinBox = WuQFactory::newDoubleSpinBox();
    this->volumeIndicesZcoordSpinBox->setDecimals(1);
    this->volumeIndicesZcoordSpinBox->setFixedWidth(sliceCoordinateSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(this->volumeIndicesZcoordSpinBox,
                                          "Adjust coordinate to select AXIAL slice");
    QObject::connect(this->volumeIndicesZcoordSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(volumeIndicesZcoordSpinBoxValueChanged(double)));
    
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(2);
    gridLayout->setVerticalSpacing(2);
    gridLayout->addWidget(this->volumeIndicesParasagittalCheckBox, 0, 0);
    gridLayout->addWidget(parasagittalLabel, 0, 1);
    gridLayout->addWidget(this->volumeIndicesParasagittalSpinBox, 0, 2);
    gridLayout->addWidget(this->volumeIndicesXcoordSpinBox, 0, 3);
    
    gridLayout->addWidget(this->volumeIndicesCoronalCheckBox, 1, 0);
    gridLayout->addWidget(coronalLabel, 1, 1);
    gridLayout->addWidget(this->volumeIndicesCoronalSpinBox, 1, 2);
    gridLayout->addWidget(this->volumeIndicesYcoordSpinBox, 1, 3);
    
    gridLayout->addWidget(this->volumeIndicesAxialCheckBox, 2, 0);
    gridLayout->addWidget(axialLabel, 2, 1);
    gridLayout->addWidget(this->volumeIndicesAxialSpinBox, 2, 2);
    gridLayout->addWidget(this->volumeIndicesZcoordSpinBox, 2, 3);

    gridLayout->addWidget(volumeIndicesOriginToolButton, 0, 4, 3, 1);
    
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    WuQtUtilities::setLayoutMargins(layout, 0, 0);
    layout->addLayout(gridLayout);
    layout->addStretch();
    
    this->volumeIndicesWidgetGroup = new WuQWidgetObjectGroup(this);
    this->volumeIndicesWidgetGroup->add(volumeIndicesOriginToolButtonAction);
    this->volumeIndicesWidgetGroup->add(this->volumeIndicesParasagittalCheckBox);
    this->volumeIndicesWidgetGroup->add(this->volumeIndicesParasagittalSpinBox);
    this->volumeIndicesWidgetGroup->add(this->volumeIndicesCoronalCheckBox);
    this->volumeIndicesWidgetGroup->add(this->volumeIndicesCoronalSpinBox);
    this->volumeIndicesWidgetGroup->add(this->volumeIndicesAxialCheckBox);
    this->volumeIndicesWidgetGroup->add(this->volumeIndicesAxialSpinBox);
    this->volumeIndicesWidgetGroup->add(this->volumeIndicesXcoordSpinBox);
    this->volumeIndicesWidgetGroup->add(this->volumeIndicesYcoordSpinBox);
    this->volumeIndicesWidgetGroup->add(this->volumeIndicesZcoordSpinBox);
    
    QWidget* w = this->createToolWidget("Slice Indices/Coords", 
                                        widget, 
                                        WIDGET_PLACEMENT_LEFT, 
                                        WIDGET_PLACEMENT_TOP,
                                        0);
    w->setVisible(false);
    return w;
}

/**
 * Update the volume indices widget.
 * 
 * @param modelDisplayController
 *   The active model display controller (may be NULL).
 */
void 
BrainBrowserWindowToolBar::updateVolumeIndicesWidget(BrowserTabContent* /*browserTabContent*/)
{
    if (this->volumeIndicesWidget->isHidden()) {
        return;
    }
    
    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    this->volumeIndicesWidgetGroup->blockAllSignals(true);
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    const int32_t tabIndex = btc->getTabNumber();
    
    VolumeFile* vf = NULL;
    ModelVolume* volumeController = btc->getDisplayedVolumeModel();
    if (volumeController != NULL) {
        if (this->getDisplayedModelController() == volumeController) {
            vf = volumeController->getUnderlayVolumeFile(tabIndex);
            //sliceSelection = volumeController->getSelectedVolumeSlices(tabIndex);
            this->volumeIndicesAxialCheckBox->setVisible(false);
            this->volumeIndicesCoronalCheckBox->setVisible(false);
            this->volumeIndicesParasagittalCheckBox->setVisible(false);            
        }
    }
    
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController != NULL) {
        if (this->getDisplayedModelController() == wholeBrainController) {
            vf = wholeBrainController->getUnderlayVolumeFile(tabIndex);
            this->volumeIndicesAxialCheckBox->setVisible(true);
            this->volumeIndicesCoronalCheckBox->setVisible(true);
            this->volumeIndicesParasagittalCheckBox->setVisible(true);
        }
    }
    
    if (vf != NULL) {
            this->volumeIndicesAxialCheckBox->setChecked(btc->isSliceAxialEnabled());
            this->volumeIndicesCoronalCheckBox->setChecked(btc->isSliceCoronalEnabled());
            this->volumeIndicesParasagittalCheckBox->setChecked(btc->isSliceParasagittalEnabled());        
    }
    
    this->updateSliceIndicesAndCoordinatesRanges();
    
    this->volumeIndicesWidgetGroup->blockAllSignals(false);

    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
}

/*
 * Set the values/minimums/maximums for volume slice indices and coordinate spin controls.
 */
void 
BrainBrowserWindowToolBar::updateSliceIndicesAndCoordinatesRanges()
{
    const bool blockedStatus = this->volumeIndicesWidget->signalsBlocked();
    this->volumeIndicesWidgetGroup->blockAllSignals(true);
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    const int32_t tabIndex = btc->getTabNumber();
    
    VolumeFile* vf = NULL;
    ModelVolume* volumeController = btc->getDisplayedVolumeModel();
    if (volumeController != NULL) {
        vf = volumeController->getUnderlayVolumeFile(tabIndex);
    }
    
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController != NULL) {
        vf = wholeBrainController->getUnderlayVolumeFile(tabIndex);
    }
    
    if (vf != NULL) {
        this->volumeIndicesAxialSpinBox->setEnabled(true);
        this->volumeIndicesCoronalSpinBox->setEnabled(true);
        this->volumeIndicesParasagittalSpinBox->setEnabled(true);
        
        std::vector<int64_t> dimensions;
        vf->getDimensions(dimensions);
        
        /*
         * Setup minimum and maximum slices for each dimension.
         * Range is unlimited when Yoked.
         */
        int minAxialDim = 0;
        int minCoronalDim = 0;
        int minParasagittalDim = 0;
        int maxAxialDim = (dimensions[2] > 0) ? (dimensions[2] - 1) : 0;
        int maxCoronalDim = (dimensions[1] > 0) ? (dimensions[1] - 1) : 0;
        int maxParasagittalDim = (dimensions[0] > 0) ? (dimensions[0] - 1) : 0;
        
        this->volumeIndicesAxialSpinBox->setRange(minAxialDim,
                                                  maxAxialDim);
        this->volumeIndicesCoronalSpinBox->setRange(minCoronalDim,
                                                    maxCoronalDim);
        this->volumeIndicesParasagittalSpinBox->setRange(minParasagittalDim,
                                                         maxParasagittalDim);
        
        
        /*
         * Setup minimum and maximum coordinates for each dimension.
         * Range is unlimited when Yoked.
         */
        int64_t slicesZero[3] = { 0, 0, 0 };
        float sliceZeroCoords[3];
        vf->indexToSpace(slicesZero,
                         sliceZeroCoords);
        int64_t slicesMax[3] = { maxParasagittalDim, maxCoronalDim, maxAxialDim };
        float sliceMaxCoords[3];
        vf->indexToSpace(slicesMax,
                         sliceMaxCoords);
        
        this->volumeIndicesXcoordSpinBox->setMinimum(std::min(sliceZeroCoords[0],
                                                              sliceMaxCoords[0]));
        this->volumeIndicesYcoordSpinBox->setMinimum(std::min(sliceZeroCoords[1],
                                                              sliceMaxCoords[1]));
        this->volumeIndicesZcoordSpinBox->setMinimum(std::min(sliceZeroCoords[2],
                                                              sliceMaxCoords[2]));
        
        this->volumeIndicesXcoordSpinBox->setMaximum(std::max(sliceZeroCoords[0],
                                                              sliceMaxCoords[0]));
        this->volumeIndicesYcoordSpinBox->setMaximum(std::max(sliceZeroCoords[1],
                                                              sliceMaxCoords[1]));
        this->volumeIndicesZcoordSpinBox->setMaximum(std::max(sliceZeroCoords[2],
                                                              sliceMaxCoords[2])); 
        
        int64_t slicesOne[3] = { 1, 1, 1 };
        float slicesOneCoords[3];
        vf->indexToSpace(slicesOne,
                         slicesOneCoords);
        const float dx = std::fabs(slicesOneCoords[0] - sliceZeroCoords[0]);
        const float dy = std::fabs(slicesOneCoords[1] - sliceZeroCoords[1]);
        const float dz = std::fabs(slicesOneCoords[2] - sliceZeroCoords[2]);
        this->volumeIndicesXcoordSpinBox->setSingleStep(dx);
        this->volumeIndicesYcoordSpinBox->setSingleStep(dy);
        this->volumeIndicesZcoordSpinBox->setSingleStep(dz);
        
            this->volumeIndicesAxialSpinBox->setValue(btc->getSliceIndexAxial(vf));
            this->volumeIndicesCoronalSpinBox->setValue(btc->getSliceIndexCoronal(vf));
            this->volumeIndicesParasagittalSpinBox->setValue(btc->getSliceIndexParasagittal(vf));
            
            int64_t slices[3] = {
                btc->getSliceIndexParasagittal(vf),
                btc->getSliceIndexCoronal(vf),
                btc->getSliceIndexAxial(vf)
            };
            float sliceCoords[3] = { 0.0, 0.0, 0.0 };
            if (vf != NULL) {
                vf->indexToSpace(slices,
                                 sliceCoords);
            }
            this->volumeIndicesXcoordSpinBox->setValue(btc->getSliceCoordinateParasagittal());
            this->volumeIndicesYcoordSpinBox->setValue(btc->getSliceCoordinateCoronal());
            this->volumeIndicesZcoordSpinBox->setValue(btc->getSliceCoordinateAxial());
        
    }

    this->volumeIndicesWidgetGroup->blockAllSignals(blockedStatus);
}

/**
 * Create the mode widget.
 *
 * @return The mode widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createModeWidget()
{
    /*
     * Borders 
     */ 
    this->modeInputModeBordersAction = WuQtUtilities::createAction("Border",
                                                                    "Perform border operations with mouse",
                                                                    this);
    QToolButton* inputModeBordersToolButton = new QToolButton();
    this->modeInputModeBordersAction->setCheckable(true);
    inputModeBordersToolButton->setDefaultAction(this->modeInputModeBordersAction);
    
    /*
     * Foci
     */
    this->modeInputModeFociAction = WuQtUtilities::createAction("Foci",
                                                                 "Perform foci operations with mouse",
                                                                 this);
    QToolButton* inputModeFociToolButton = new QToolButton();
    this->modeInputModeFociAction->setCheckable(true);
    inputModeFociToolButton->setDefaultAction(this->modeInputModeFociAction);
    
    /*
     * View Mode
     */
    this->modeInputModeViewAction = WuQtUtilities::createAction("View",
                                                                 "Perform viewing operations with mouse\n"
                                                                 "\n"
                                                                 "Identify: Click Left Mouse\n"
                                                                 "Pan:      Move mouse with left mouse button down and keyboard shift key down\n"
                                                                 "Rotate:   Move mouse with left mouse button down\n"
#ifdef CARET_OS_MACOSX
                                                                 "Zoom:     Move mouse with left mouse button down and keyboard apple key down",
#else // CARET_OS_MACOSX
                                                                 "Zoom:     Move mouse with left mouse button down and keyboard control key down",
#endif // CARET_OS_MACOSX
                                                                 this);
    this->modeInputModeViewAction->setCheckable(true);
    QToolButton* inputModeViewToolButton = new QToolButton();
    inputModeViewToolButton->setDefaultAction(this->modeInputModeViewAction);
    
    WuQtUtilities::matchWidgetWidths(inputModeBordersToolButton,
                                     inputModeFociToolButton,
                                     inputModeViewToolButton);
    /*
     * Layout for input modes
     */
    QWidget* inputModeWidget = new QWidget();
    QVBoxLayout* inputModeLayout = new QVBoxLayout(inputModeWidget);
    WuQtUtilities::setLayoutMargins(inputModeLayout, 2, 2);
    inputModeLayout->addWidget(inputModeBordersToolButton, 0, Qt::AlignHCenter);
    inputModeLayout->addWidget(inputModeFociToolButton, 0, Qt::AlignHCenter);
    inputModeLayout->addWidget(inputModeViewToolButton, 0, Qt::AlignHCenter);
    
    this->modeInputModeActionGroup = new QActionGroup(this);
    this->modeInputModeActionGroup->addAction(this->modeInputModeBordersAction);
    this->modeInputModeActionGroup->addAction(this->modeInputModeFociAction);
    this->modeInputModeActionGroup->addAction(this->modeInputModeViewAction);
    QObject::connect(this->modeInputModeActionGroup, SIGNAL(triggered(QAction*)),
                     this, SLOT(modeInputModeActionTriggered(QAction*)));
    this->modeInputModeActionGroup->setExclusive(true);
    
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    WuQtUtilities::setLayoutMargins(layout, 0, 0);
    layout->addWidget(inputModeWidget, 0, Qt::AlignHCenter);
    layout->addStretch();
    
    this->modeWidgetGroup = new WuQWidgetObjectGroup(this);
    this->modeWidgetGroup->add(this->modeInputModeActionGroup);
    
    QWidget* w = this->createToolWidget("Mode", 
                                        widget, 
                                        WIDGET_PLACEMENT_LEFT, 
                                        WIDGET_PLACEMENT_NONE,
                                        0);
    return w;
}

/**
 * Called when a tools input mode button is clicked.
 * @param action
 *    Action of tool button that was clicked.
 */
void 
BrainBrowserWindowToolBar::modeInputModeActionTriggered(QAction* action)
{
    BrowserTabContent* tabContent = this->getTabContentFromSelectedTab();
    if (tabContent == NULL) {
        return;
    }

    UserInputReceiverInterface::UserInputMode inputMode = UserInputReceiverInterface::INVALID;
    
    if (action == this->modeInputModeBordersAction) {
        inputMode = UserInputReceiverInterface::BORDERS;
        
        /*
         * If borders are not displayed, display them
         */
        DisplayPropertiesBorders* dpb = GuiManager::get()->getBrain()->getDisplayPropertiesBorders();
        const int32_t browserTabIndex = tabContent->getTabNumber();
        const DisplayGroupEnum::Enum displayGroup = dpb->getDisplayGroupForTab(browserTabIndex);
        if (dpb->isDisplayed(displayGroup,
                             browserTabIndex) == false) {
            dpb->setDisplayed(displayGroup, 
                              browserTabIndex,
                              true);
            this->updateUserInterface();
            this->updateGraphicsWindow();
        }
    }
    else if (action == this->modeInputModeFociAction) {
        inputMode = UserInputReceiverInterface::FOCI;
    }
    else if (action == this->modeInputModeViewAction) {
        inputMode = UserInputReceiverInterface::VIEW;
    }
    else {
        CaretAssertMessage(0, "Tools input mode action is invalid, new action added???");
    }
    
    EventManager::get()->sendEvent(EventGetOrSetUserInputModeProcessor(this->browserWindowIndex,
                                                                       inputMode).getPointer());    
    this->updateModeWidget(tabContent);
    this->updateDisplayedModeUserInputWidget();
}

/**
 * Update the tools widget.
 * 
 * @param modelDisplayController
 *   The active model display controller (may be NULL).
 */
void 
BrainBrowserWindowToolBar::updateModeWidget(BrowserTabContent* /*browserTabContent*/)
{
    if (this->modeWidget->isHidden()) {
        return;
    }
    
    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    this->modeWidgetGroup->blockAllSignals(true);
    
    EventGetOrSetUserInputModeProcessor getInputModeEvent(this->browserWindowIndex);
    EventManager::get()->sendEvent(getInputModeEvent.getPointer());

    switch (getInputModeEvent.getUserInputMode()) {
        case UserInputReceiverInterface::INVALID:
            // may get here when program is exiting and widgets are being destroyed
            break;
        case UserInputReceiverInterface::BORDERS:
            this->modeInputModeBordersAction->setChecked(true);
            break;
        case UserInputReceiverInterface::FOCI:
            this->modeInputModeFociAction->setChecked(true);
            break;
        case UserInputReceiverInterface::VIEW:
            this->modeInputModeViewAction->setChecked(true);
            break;
    }
    
    this->modeWidgetGroup->blockAllSignals(false);

    this->updateDisplayedModeUserInputWidget();
    
    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
}

void
BrainBrowserWindowToolBar::updateDisplayedModeUserInputWidget()
{
    EventGetOrSetUserInputModeProcessor getInputModeEvent(this->browserWindowIndex);
    EventManager::get()->sendEvent(getInputModeEvent.getPointer());
    
    UserInputReceiverInterface* userInputProcessor = getInputModeEvent.getUserInputProcessor();
    QWidget* userInputWidget = userInputProcessor->getWidgetForToolBar();
    
    /*
     * If a widget is display and needs to change,
     * remove the old widget.
     */
    if (this->userInputControlsWidgetActiveInputWidget != NULL) {
        if (userInputWidget != this->userInputControlsWidgetActiveInputWidget) {
            this->userInputControlsWidgetActiveInputWidget->setVisible(false);
            this->userInputControlsWidgetLayout->removeWidget(this->userInputControlsWidgetActiveInputWidget);
            this->userInputControlsWidgetActiveInputWidget = NULL;
        }
    }
    if (this->userInputControlsWidgetActiveInputWidget == NULL) {
        if (userInputWidget != NULL) {
            this->userInputControlsWidgetActiveInputWidget = userInputWidget;
            this->userInputControlsWidgetActiveInputWidget->setVisible(true);
            this->userInputControlsWidgetLayout->addWidget(this->userInputControlsWidgetActiveInputWidget);
            this->userInputControlsWidget->setVisible(true);
            this->userInputControlsWidgetLayout->update();
        }
        else {
            this->userInputControlsWidget->setVisible(false);
        }
    }
}

/**
 * Create the window (yoking) widget.
 *
 * @return  The window (yoking) widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createWindowWidget()
{
    m_yokingGroupComboBox = new EnumComboBoxTemplate(this);
    m_yokingGroupComboBox->setup<YokingGroupEnum, YokingGroupEnum::Enum>();
    m_yokingGroupComboBox->getWidget()->setStatusTip("Select a yoking group (linked views)");
    m_yokingGroupComboBox->getWidget()->setToolTip(("Select a yoking group (linked views).\n"
                                               "Models yoked to a group are displayed in the same view.\n"
                                               "Surface Yoking is applied to Surface, Surface Montage\n"
                                               "and Whole Brain.  Volume Yoking is applied to Volumes."));

    QLabel* yokeToLabel = new QLabel("Yoking:");
    QObject::connect(this->m_yokingGroupComboBox, SIGNAL(itemActivated()),
                     this, SLOT(windowYokeToGroupComboBoxIndexChanged()));
    
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    WuQtUtilities::setLayoutMargins(layout, 4, 0);
    layout->addWidget(yokeToLabel);
    layout->addWidget(m_yokingGroupComboBox->getWidget());
    
    this->windowWidgetGroup = new WuQWidgetObjectGroup(this);
    this->windowWidgetGroup->add(yokeToLabel);
    this->windowWidgetGroup->add(m_yokingGroupComboBox->getWidget());
    
    this->windowYokingWidgetGroup = new WuQWidgetObjectGroup(this);
    this->windowYokingWidgetGroup->add(yokeToLabel);
    this->windowYokingWidgetGroup->add(m_yokingGroupComboBox->getWidget());
    
    QWidget* w = this->createToolWidget("Window", 
                                        widget, 
                                        WIDGET_PLACEMENT_LEFT, 
                                        WIDGET_PLACEMENT_TOP,
                                        0);
    return w;
}

/**
 * Update the window widget.
 * 
 * @param modelDisplayController
 *   The active model display controller (may be NULL).
 */
void 
BrainBrowserWindowToolBar::updateWindowWidget(BrowserTabContent* browserTabContent)
{
    if (this->windowWidget->isHidden()) {
        return;
    }
    
    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    this->windowWidgetGroup->blockAllSignals(true);
    
    m_yokingGroupComboBox->setSelectedItem<YokingGroupEnum,YokingGroupEnum::Enum>(browserTabContent->getYokingGroup());
    
    this->windowWidgetGroup->blockAllSignals(false);

    this->windowYokingWidgetGroup->setEnabled(true);
    
    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
}



/**
 * Create the single surface options widget.
 *
 * @return  The single surface options widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createSingleSurfaceOptionsWidget()
{
    QLabel* structureSurfaceLabel = new QLabel("Brain Structure and Surface: ");
    
    this->surfaceSurfaceSelectionControl = new StructureSurfaceSelectionControl(false);
    QObject::connect(this->surfaceSurfaceSelectionControl, 
                     SIGNAL(selectionChanged(const StructureEnum::Enum,
                                             ModelSurface*)),
                     this,
                     SLOT(surfaceSelectionControlChanged(const StructureEnum::Enum,
                                                         ModelSurface*)));
    
    this->surfaceSurfaceSelectionControl->setMinimumWidth(150); //275);
    this->surfaceSurfaceSelectionControl->setMaximumWidth(1200);
    
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    WuQtUtilities::setLayoutMargins(layout, 2, 2);
    layout->addWidget(structureSurfaceLabel);
    layout->addWidget(this->surfaceSurfaceSelectionControl);
    layout->addStretch();
    
    this->singleSurfaceSelectionWidgetGroup = new WuQWidgetObjectGroup(this);
    this->singleSurfaceSelectionWidgetGroup->add(this->surfaceSurfaceSelectionControl);

    QWidget* w = this->createToolWidget("Selection", 
                                        widget, 
                                        WIDGET_PLACEMENT_LEFT, 
                                        WIDGET_PLACEMENT_TOP, 
                                        100);
    w->setVisible(false);
    return w;
}
/**
 * Update the single surface options widget.
 * 
 * @param browserTabContent
 *   The active model display controller (may be NULL).
 */
void 
BrainBrowserWindowToolBar::updateSingleSurfaceOptionsWidget(BrowserTabContent* browserTabContent)
{
    if (this->singleSurfaceSelectionWidget->isHidden()) {
        return;
    }
    
    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    this->singleSurfaceSelectionWidgetGroup->blockAllSignals(true);
    
    this->surfaceSurfaceSelectionControl->updateControl(browserTabContent->getSurfaceModelSelector());
    
    this->singleSurfaceSelectionWidgetGroup->blockAllSignals(false);
    
    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
}

/**
 * @return Create and return the surface montage options widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createSurfaceMontageOptionsWidget()
{
    this->surfaceMontageLeftCheckBox = new QCheckBox("Left");
    QObject::connect(this->surfaceMontageLeftCheckBox, SIGNAL(toggled(bool)),
                     this, SLOT(surfaceMontageCheckBoxSelected(bool)));
    
    this->surfaceMontageRightCheckBox = new QCheckBox("Right");
    QObject::connect(this->surfaceMontageRightCheckBox, SIGNAL(toggled(bool)),
                     this, SLOT(surfaceMontageCheckBoxSelected(bool)));
    
    this->surfaceMontageFirstSurfaceCheckBox = new QCheckBox(" ");
    QObject::connect(this->surfaceMontageFirstSurfaceCheckBox, SIGNAL(toggled(bool)),
                     this, SLOT(surfaceMontageCheckBoxSelected(bool)));
    
    this->surfaceMontageSecondSurfaceCheckBox = new QCheckBox(" ");
    QObject::connect(this->surfaceMontageSecondSurfaceCheckBox, SIGNAL(toggled(bool)),
                     this, SLOT(surfaceMontageCheckBoxSelected(bool)));
    
    this->surfaceMontageLeftSurfaceViewController = new SurfaceSelectionViewController(this);
    QObject::connect(this->surfaceMontageLeftSurfaceViewController, SIGNAL(surfaceSelected(Surface*)),
                     this, SLOT(surfaceMontageLeftSurfaceSelected(Surface*)));
    
    this->surfaceMontageLeftSecondSurfaceViewController = new SurfaceSelectionViewController(this);
    QObject::connect(this->surfaceMontageLeftSecondSurfaceViewController, SIGNAL(surfaceSelected(Surface*)),
                     this, SLOT(surfaceMontageLeftSecondSurfaceSelected(Surface*)));
    
    this->surfaceMontageRightSurfaceViewController = new SurfaceSelectionViewController(this);
    QObject::connect(this->surfaceMontageRightSurfaceViewController, SIGNAL(surfaceSelected(Surface*)),
                     this, SLOT(surfaceMontageRightSurfaceSelected(Surface*)));

    this->surfaceMontageRightSecondSurfaceViewController = new SurfaceSelectionViewController(this);
    QObject::connect(this->surfaceMontageRightSecondSurfaceViewController, SIGNAL(surfaceSelected(Surface*)),
                     this, SLOT(surfaceMontageRightSecondSurfaceSelected(Surface*)));
    
    int32_t columnIndex = 0;
    const int32_t COLUMN_ONE_TWO     = columnIndex++;
    const int32_t COLUMN_INDEX_LEFT  = columnIndex++;
    const int32_t COLUMN_INDEX_RIGHT = columnIndex++;
    
    QWidget* widget = new QWidget();
    QGridLayout* layout = new QGridLayout(widget);
    layout->setColumnStretch(0,   0);
    layout->setColumnStretch(1, 100);
    layout->setColumnStretch(2, 100);
    WuQtUtilities::setLayoutMargins(layout, 4, 2);
    int row = layout->rowCount();
    layout->addWidget(surfaceMontageLeftCheckBox, row, COLUMN_INDEX_LEFT, Qt::AlignHCenter);
    layout->addWidget(surfaceMontageRightCheckBox, row, COLUMN_INDEX_RIGHT, Qt::AlignHCenter);
    row = layout->rowCount();
    layout->addWidget(this->surfaceMontageFirstSurfaceCheckBox, row, COLUMN_ONE_TWO);
    layout->addWidget(this->surfaceMontageLeftSurfaceViewController->getWidget(), row, COLUMN_INDEX_LEFT);
    layout->addWidget(this->surfaceMontageRightSurfaceViewController->getWidget(), row, COLUMN_INDEX_RIGHT);
    row = layout->rowCount();
    layout->addWidget(this->surfaceMontageSecondSurfaceCheckBox, row, COLUMN_ONE_TWO);
    layout->addWidget(this->surfaceMontageLeftSecondSurfaceViewController->getWidget(), row, COLUMN_INDEX_LEFT);
    layout->addWidget(this->surfaceMontageRightSecondSurfaceViewController->getWidget(), row, COLUMN_INDEX_RIGHT);
    row = layout->rowCount();
    
    this->surfaceMontageSelectionWidgetGroup = new WuQWidgetObjectGroup(this);
    this->surfaceMontageSelectionWidgetGroup->add(this->surfaceMontageLeftSurfaceViewController->getWidget());
    this->surfaceMontageSelectionWidgetGroup->add(this->surfaceMontageLeftSecondSurfaceViewController->getWidget());
    this->surfaceMontageSelectionWidgetGroup->add(this->surfaceMontageRightSurfaceViewController->getWidget());
    this->surfaceMontageSelectionWidgetGroup->add(this->surfaceMontageRightSecondSurfaceViewController->getWidget());
    this->surfaceMontageSelectionWidgetGroup->add(surfaceMontageLeftCheckBox);
    this->surfaceMontageSelectionWidgetGroup->add(surfaceMontageRightCheckBox);
    this->surfaceMontageSelectionWidgetGroup->add(surfaceMontageFirstSurfaceCheckBox);
    this->surfaceMontageSelectionWidgetGroup->add(surfaceMontageSecondSurfaceCheckBox);
    
    QWidget* w = this->createToolWidget("Montage Selection", 
                                        widget, 
                                        WIDGET_PLACEMENT_LEFT, 
                                        WIDGET_PLACEMENT_TOP, 
                                        100);
    w->setVisible(false);
    return w;
}

/**
 * Called when surface montage checkbox is toggled.
 * @param status
 *    New status of check box.
 */
void 
BrainBrowserWindowToolBar::surfaceMontageCheckBoxSelected(bool /*status*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    const int32_t tabIndex = btc->getTabNumber();
    ModelSurfaceMontage* msm = btc->getDisplayedSurfaceMontageModel();
    if (msm != NULL) {
        msm->setLeftEnabled(tabIndex, this->surfaceMontageLeftCheckBox->isChecked());
        msm->setRightEnabled(tabIndex, this->surfaceMontageRightCheckBox->isChecked());
        msm->setFirstSurfaceEnabled(tabIndex, this->surfaceMontageFirstSurfaceCheckBox->isChecked());
        msm->setSecondSurfaceEnabled(tabIndex, this->surfaceMontageSecondSurfaceCheckBox->isChecked());
    }
    EventManager::get()->sendEvent(EventSurfaceColoringInvalidate().getPointer());
    this->updateUserInterface();
    this->updateGraphicsWindow();
}

/**
 * Update the surface montage options widget.
 * 
 * @param browserTabContent
 *   The active model display controller (may be NULL).
 */
void 
BrainBrowserWindowToolBar::updateSurfaceMontageOptionsWidget(BrowserTabContent* browserTabContent)
{
    if (this->surfaceMontageSelectionWidget->isHidden()) {
        return;
    }

    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    this->surfaceMontageSelectionWidgetGroup->blockAllSignals(true);
    
    ModelSurfaceMontage* msm = browserTabContent->getDisplayedSurfaceMontageModel();
    const int32_t tabIndex = browserTabContent->getTabNumber();
    SurfaceSelectionModel* leftSurfaceSelectionModel = NULL;
    SurfaceSelectionModel* leftSecondSurfaceSelectionModel = NULL;
    SurfaceSelectionModel* rightSurfaceSelectionModel = NULL;
    SurfaceSelectionModel* rightSecondSurfaceSelectionModel = NULL;
    if (msm != NULL) {
        this->surfaceMontageLeftCheckBox->setChecked(msm->isLeftEnabled(tabIndex));
        this->surfaceMontageRightCheckBox->setChecked(msm->isRightEnabled(tabIndex));
        this->surfaceMontageFirstSurfaceCheckBox->setChecked(msm->isFirstSurfaceEnabled(tabIndex));
        this->surfaceMontageSecondSurfaceCheckBox->setChecked(msm->isSecondSurfaceEnabled(tabIndex));
        
        leftSurfaceSelectionModel = msm->getLeftSurfaceSelectionModel(tabIndex);
        leftSecondSurfaceSelectionModel = msm->getLeftSecondSurfaceSelectionModel(tabIndex);
        rightSurfaceSelectionModel = msm->getRightSurfaceSelectionModel(tabIndex);
        rightSecondSurfaceSelectionModel = msm->getRightSecondSurfaceSelectionModel(tabIndex);
    }
    this->surfaceMontageLeftSurfaceViewController->updateControl(leftSurfaceSelectionModel);
    this->surfaceMontageLeftSecondSurfaceViewController->updateControl(leftSecondSurfaceSelectionModel);
    this->surfaceMontageRightSurfaceViewController->updateControl(rightSurfaceSelectionModel);
    this->surfaceMontageRightSecondSurfaceViewController->updateControl(rightSecondSurfaceSelectionModel);
    
    this->surfaceMontageSelectionWidgetGroup->blockAllSignals(false);
    
    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
}

/** 
 * Called when montage left surface is selected.
 * @param surface
 *    Surface that was selected.
 */
void 
BrainBrowserWindowToolBar::surfaceMontageLeftSurfaceSelected(Surface* surface)
{
    if (surface != NULL) {
        BrowserTabContent* btc = this->getTabContentFromSelectedTab();
        const int32_t tabIndex = btc->getTabNumber();
        ModelSurfaceMontage* msm = btc->getDisplayedSurfaceMontageModel();
        if (msm != NULL) {
            msm->getLeftSurfaceSelectionModel(tabIndex)->setSurface(surface);
        }
        EventManager::get()->sendEvent(EventSurfaceColoringInvalidate().getPointer());
        this->updateGraphicsWindow();
    }
}

/** 
 * Called when montage left second surface is selected.
 * @param surface
 *    Surface that was selected.
 */
void 
BrainBrowserWindowToolBar::surfaceMontageLeftSecondSurfaceSelected(Surface* surface)
{
    if (surface != NULL) {
        BrowserTabContent* btc = this->getTabContentFromSelectedTab();
        const int32_t tabIndex = btc->getTabNumber();
        ModelSurfaceMontage* msm = btc->getDisplayedSurfaceMontageModel();
        if (msm != NULL) {
            msm->getLeftSecondSurfaceSelectionModel(tabIndex)->setSurface(surface);
        }
        EventManager::get()->sendEvent(EventSurfaceColoringInvalidate().getPointer());
        this->updateGraphicsWindow();
    }
}

/** 
 * Called when montage right surface is selected.
 * @param surface
 *    Surface that was selected.
 */
void 
BrainBrowserWindowToolBar::surfaceMontageRightSurfaceSelected(Surface* surface)
{
    if (surface != NULL) {
        BrowserTabContent* btc = this->getTabContentFromSelectedTab();
        const int32_t tabIndex = btc->getTabNumber();
        ModelSurfaceMontage* msm = btc->getDisplayedSurfaceMontageModel();
        if (msm != NULL) {
            msm->getRightSurfaceSelectionModel(tabIndex)->setSurface(surface);
        }
        EventManager::get()->sendEvent(EventSurfaceColoringInvalidate().getPointer());
        this->updateGraphicsWindow();
    }
}

/** 
 * Called when montage right second surface is selected.
 * @param surface
 *    Surface that was selected.
 */
void 
BrainBrowserWindowToolBar::surfaceMontageRightSecondSurfaceSelected(Surface* surface)
{
    if (surface != NULL) {
        BrowserTabContent* btc = this->getTabContentFromSelectedTab();
        const int32_t tabIndex = btc->getTabNumber();
        ModelSurfaceMontage* msm = btc->getDisplayedSurfaceMontageModel();
        if (msm != NULL) {
            msm->getRightSecondSurfaceSelectionModel(tabIndex)->setSurface(surface);
        }
        EventManager::get()->sendEvent(EventSurfaceColoringInvalidate().getPointer());
        this->updateGraphicsWindow();
    }
}


/**
 * Create the volume montage widget.
 *
 * @return The volume montage widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createVolumeMontageWidget()
{
    QLabel* rowsLabel = new QLabel("Rows:");
    QLabel* columnsLabel = new QLabel("Cols:");
    QLabel* spacingLabel = new QLabel("Step:");
    
    const int spinBoxWidth = 48;
    
    this->montageRowsSpinBox = WuQFactory::newSpinBox();
    this->montageRowsSpinBox->setRange(1, 20);
    this->montageRowsSpinBox->setMaximumWidth(spinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(this->montageRowsSpinBox,
                                          "Select the number of rows in montage of volume slices");
    QObject::connect(this->montageRowsSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(montageRowsSpinBoxValueChanged(int)));
    
    this->montageColumnsSpinBox = WuQFactory::newSpinBox();
    this->montageColumnsSpinBox->setRange(1, 20);
    this->montageColumnsSpinBox->setMaximumWidth(spinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(this->montageColumnsSpinBox,
                                          "Select the number of columns in montage of volume slices");
    QObject::connect(this->montageColumnsSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(montageColumnsSpinBoxValueChanged(int)));

    this->montageSpacingSpinBox = WuQFactory::newSpinBox();
    this->montageSpacingSpinBox->setRange(1, 2500);
    this->montageSpacingSpinBox->setMaximumWidth(spinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(this->montageSpacingSpinBox,
                                          "Select the number of slices stepped (incremented) between displayed montage slices");
    QObject::connect(this->montageSpacingSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(montageSpacingSpinBoxValueChanged(int)));
    
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setVerticalSpacing(2);
    gridLayout->addWidget(rowsLabel, 0, 0);
    gridLayout->addWidget(this->montageRowsSpinBox, 0, 1);
    gridLayout->addWidget(columnsLabel, 1, 0);
    gridLayout->addWidget(this->montageColumnsSpinBox, 1, 1);
    gridLayout->addWidget(spacingLabel, 2, 0);
    gridLayout->addWidget(this->montageSpacingSpinBox, 2, 1);

    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    WuQtUtilities::setLayoutMargins(layout, 0, 0);
    layout->addLayout(gridLayout);
    
    this->volumeMontageWidgetGroup = new WuQWidgetObjectGroup(this);
    this->volumeMontageWidgetGroup->add(this->montageRowsSpinBox);
    this->volumeMontageWidgetGroup->add(this->montageColumnsSpinBox);
    this->volumeMontageWidgetGroup->add(this->montageSpacingSpinBox);
    
    QWidget* w = this->createToolWidget("Montage", 
                                        widget, 
                                        WIDGET_PLACEMENT_LEFT, 
                                        WIDGET_PLACEMENT_TOP,
                                        0);
    w->setVisible(false);
    return w;
}

/**
 * Update the volume montage widget.
 * 
 * @param modelDisplayController
 *   The active model display controller (may be NULL).
 */
void 
BrainBrowserWindowToolBar::updateVolumeMontageWidget(BrowserTabContent* /*browserTabContent*/)
{
    if (this->volumeMontageWidget->isHidden()) {
        return;
    }

    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    this->volumeMontageWidgetGroup->blockAllSignals(true);
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    this->montageRowsSpinBox->setValue(btc->getMontageNumberOfRows());
    this->montageColumnsSpinBox->setValue(btc->getMontageNumberOfColumns());
    this->montageSpacingSpinBox->setValue(btc->getMontageSliceSpacing());
    
    this->volumeMontageWidgetGroup->blockAllSignals(false);

    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
}

/**
 * Create the volume plane widget.
 *
 * @return The volume plane widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createVolumePlaneWidget()
{
    QIcon parasagittalIcon;
    const bool parasagittalIconValid =
    WuQtUtilities::loadIcon(":/view-plane-parasagittal.png", 
                            parasagittalIcon);

    QIcon coronalIcon;
    const bool coronalIconValid =
    WuQtUtilities::loadIcon(":/view-plane-coronal.png", 
                            coronalIcon);

    QIcon axialIcon;
    const bool axialIconValid =
    WuQtUtilities::loadIcon(":/view-plane-axial.png", 
                            axialIcon);
    
    this->volumePlaneParasagittalToolButtonAction = 
    WuQtUtilities::createAction(VolumeSliceViewPlaneEnum::toGuiNameAbbreviation(VolumeSliceViewPlaneEnum::PARASAGITTAL), 
                                                                                      "View the PARASAGITTAL slice", 
                                                                                      this);
    this->volumePlaneParasagittalToolButtonAction->setCheckable(true);
    if (parasagittalIconValid) {
        this->volumePlaneParasagittalToolButtonAction->setIcon(parasagittalIcon);
    }
    
    this->volumePlaneCoronalToolButtonAction = WuQtUtilities::createAction(VolumeSliceViewPlaneEnum::toGuiNameAbbreviation(VolumeSliceViewPlaneEnum::CORONAL), 
                                                                                 "View the CORONAL slice", 
                                                                                 this);
    this->volumePlaneCoronalToolButtonAction->setCheckable(true);
    if (coronalIconValid) {
        this->volumePlaneCoronalToolButtonAction->setIcon(coronalIcon);
    }

    this->volumePlaneAxialToolButtonAction = WuQtUtilities::createAction(VolumeSliceViewPlaneEnum::toGuiNameAbbreviation(VolumeSliceViewPlaneEnum::AXIAL), 
                                                                               "View the AXIAL slice", 
                                                                               this);
    this->volumePlaneAxialToolButtonAction->setCheckable(true);
    if (axialIconValid) {
        this->volumePlaneAxialToolButtonAction->setIcon(axialIcon);
    }

    this->volumePlaneAllToolButtonAction = WuQtUtilities::createAction(VolumeSliceViewPlaneEnum::toGuiNameAbbreviation(VolumeSliceViewPlaneEnum::ALL), 
                                                                             "View the PARASAGITTAL, CORONAL, and AXIAL slices", 
                                                                             this);
    this->volumePlaneAllToolButtonAction->setCheckable(true);
    

    this->volumePlaneActionGroup = new QActionGroup(this);
    this->volumePlaneActionGroup->addAction(this->volumePlaneParasagittalToolButtonAction);
    this->volumePlaneActionGroup->addAction(this->volumePlaneCoronalToolButtonAction);
    this->volumePlaneActionGroup->addAction(this->volumePlaneAxialToolButtonAction);
    this->volumePlaneActionGroup->addAction(this->volumePlaneAllToolButtonAction);
    this->volumePlaneActionGroup->setExclusive(true);
    QObject::connect(this->volumePlaneActionGroup, SIGNAL(triggered(QAction*)),
                     this, SLOT(volumePlaneActionGroupTriggered(QAction*)));

    
    this->volumePlaneViewOrthogonalToolButtonAction = WuQtUtilities::createAction(VolumeSliceViewModeEnum::toGuiNameAbbreviation(VolumeSliceViewModeEnum::ORTHOGONAL),
                                                                                  "View the volume orthogonal axis",                                                                           
                                                                                  this);
    this->volumePlaneViewOrthogonalToolButtonAction->setCheckable(true);
    
    this->volumePlaneViewObliqueToolButtonAction = WuQtUtilities::createAction(VolumeSliceViewModeEnum::toGuiNameAbbreviation(VolumeSliceViewModeEnum::OBLIQUE),
                                                                           "View the volume oblique axis",                                                                           
                                                                           this);
    this->volumePlaneViewObliqueToolButtonAction->setCheckable(true);
    
    this->volumePlaneViewMontageToolButtonAction = WuQtUtilities::createAction(VolumeSliceViewModeEnum::toGuiNameAbbreviation(VolumeSliceViewModeEnum::MONTAGE),
                                                                           "View a montage of parallel slices",                                                                           
                                                                           this);
    this->volumePlaneViewMontageToolButtonAction->setCheckable(true);
    
    this->volumePlaneViewActionGroup = new QActionGroup(this);
    this->volumePlaneViewActionGroup->addAction(this->volumePlaneViewOrthogonalToolButtonAction);
    this->volumePlaneViewActionGroup->addAction(this->volumePlaneViewMontageToolButtonAction);
    this->volumePlaneViewActionGroup->addAction(this->volumePlaneViewObliqueToolButtonAction);
    QObject::connect(this->volumePlaneViewActionGroup, SIGNAL(triggered(QAction*)),
                     this, SLOT(volumePlaneViewActionGroupTriggered(QAction*)));
    
    this->volumePlaneResetToolButtonAction = WuQtUtilities::createAction("Reset", 
                                                                         "Reset to remove panning/zooming", 
                                                                         this, 
                                                                         this, 
                                                                         SLOT(volumePlaneResetToolButtonTriggered(bool)));
    
    
    QToolButton* volumePlaneParasagittalToolButton = new QToolButton();
    volumePlaneParasagittalToolButton->setDefaultAction(this->volumePlaneParasagittalToolButtonAction);
    
    QToolButton* volumePlaneCoronalToolButton = new QToolButton();
    volumePlaneCoronalToolButton->setDefaultAction(this->volumePlaneCoronalToolButtonAction);
    
    QToolButton* volumePlaneAxialToolButton = new QToolButton();
    volumePlaneAxialToolButton->setDefaultAction(this->volumePlaneAxialToolButtonAction);
    
    QToolButton* volumePlaneAllToolButton = new QToolButton();
    volumePlaneAllToolButton->setDefaultAction(this->volumePlaneAllToolButtonAction);
    
    QToolButton* volumePlaneViewMontageToolButton = new QToolButton();
    volumePlaneViewMontageToolButton->setDefaultAction(this->volumePlaneViewMontageToolButtonAction);
    
    QToolButton* volumePlaneViewObliqueToolButton = new QToolButton();
    volumePlaneViewObliqueToolButton->setDefaultAction(this->volumePlaneViewObliqueToolButtonAction);
    
    QToolButton* volumePlaneViewOrthogonalToolButton = new QToolButton();
    volumePlaneViewOrthogonalToolButton->setDefaultAction(this->volumePlaneViewOrthogonalToolButtonAction);
    
    QToolButton* volumePlaneResetToolButton = new QToolButton();
    volumePlaneResetToolButton->setDefaultAction(this->volumePlaneResetToolButtonAction);
    
    WuQtUtilities::matchWidgetHeights(volumePlaneParasagittalToolButton,
                                      volumePlaneCoronalToolButton,
                                      volumePlaneAxialToolButton,
                                      volumePlaneAllToolButton);
    
    
    QHBoxLayout* planeLayout1 = new QHBoxLayout();
    WuQtUtilities::setLayoutMargins(planeLayout1, 0, 0);
    planeLayout1->addStretch();
    planeLayout1->addWidget(volumePlaneParasagittalToolButton);
    planeLayout1->addWidget(volumePlaneCoronalToolButton);
    planeLayout1->addWidget(volumePlaneAxialToolButton);
    planeLayout1->addWidget(volumePlaneAllToolButton);
    planeLayout1->addStretch();

    QHBoxLayout* planeLayout2 = new QHBoxLayout();
    WuQtUtilities::setLayoutMargins(planeLayout2, 0, 0);
    planeLayout2->addStretch();
    planeLayout2->addWidget(volumePlaneViewOrthogonalToolButton);
    planeLayout2->addWidget(volumePlaneViewMontageToolButton);
    planeLayout2->addWidget(volumePlaneViewObliqueToolButton);
    planeLayout2->addStretch();
    
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    WuQtUtilities::setLayoutMargins(layout, 0, 0);
    layout->addLayout(planeLayout1);
    layout->addLayout(planeLayout2);
    layout->addWidget(volumePlaneResetToolButton, 0, Qt::AlignHCenter);
    
    this->volumePlaneWidgetGroup = new WuQWidgetObjectGroup(this);
    this->volumePlaneWidgetGroup->add(this->volumePlaneActionGroup);
    this->volumePlaneWidgetGroup->add(this->volumePlaneResetToolButtonAction);
    
    QWidget* w = this->createToolWidget("Slice Plane", 
                                        widget, 
                                        WIDGET_PLACEMENT_LEFT, 
                                        WIDGET_PLACEMENT_TOP,
                                        0);
    w->setVisible(false);
    return w;
}

/**
 * Update the volume plane orientation widget.
 * 
 * @param modelDisplayController
 *   The active model display controller (may be NULL).
 */
void 
BrainBrowserWindowToolBar::updateVolumePlaneWidget(BrowserTabContent* /*browserTabContent*/)
{
    if (this->volumePlaneWidget->isHidden()) {
        return;
    }
    
    this->incrementUpdateCounter(__CARET_FUNCTION_NAME__);
    
    this->volumePlaneWidgetGroup->blockAllSignals(true);
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
        switch (btc->getSliceViewPlane()) {
            case VolumeSliceViewPlaneEnum::ALL:
                this->volumePlaneAllToolButtonAction->setChecked(true);
                break;
            case VolumeSliceViewPlaneEnum::AXIAL:
                this->volumePlaneAxialToolButtonAction->setChecked(true);
                break;
            case VolumeSliceViewPlaneEnum::CORONAL:
                this->volumePlaneCoronalToolButtonAction->setChecked(true);
                break;
            case VolumeSliceViewPlaneEnum::PARASAGITTAL:
                this->volumePlaneParasagittalToolButtonAction->setChecked(true);
                break;
        }
        
        switch(btc->getSliceViewMode()) {
            case VolumeSliceViewModeEnum::MONTAGE:
                this->volumePlaneViewMontageToolButtonAction->setChecked(true);
                break;
            case VolumeSliceViewModeEnum::OBLIQUE:
                this->volumePlaneViewObliqueToolButtonAction->setChecked(true);
                break;
            case VolumeSliceViewModeEnum::ORTHOGONAL:
                this->volumePlaneViewOrthogonalToolButtonAction->setChecked(true);
                break;
        }

    
    this->volumePlaneWidgetGroup->blockAllSignals(false);

    this->decrementUpdateCounter(__CARET_FUNCTION_NAME__);
}

/**
 * Create a tool widget which is a group of widgets with 
 * a descriptive label added.
 *
 * @param name
 *    Name for the descriptive labe.
 * @param childWidget
 *    Child widget that is in the tool widget.
 * @param verticalBarPlacement
 *    Where to place a vertical bar.  Values other than right or 
 *    left are ignored in which case no vertical bar is displayed.
 * @param contentPlacement
 *    Where to place widget which must be top or bottom.
 * @return The tool widget.
 */
QWidget* 
BrainBrowserWindowToolBar::createToolWidget(const QString& name,
                                            QWidget* childWidget,
                                            const WidgetPlacement verticalBarPlacement,
                                            const WidgetPlacement contentPlacement,
                                            const int /*horizontalStretching*/)
{
    //QLabel* nameLabel = new QLabel("<html><b>" + name + "<b></html>");
    QLabel* nameLabel = new QLabel("<html>" + name + "</html>");
    
    QWidget* w = new QWidget();
    QGridLayout* layout = new QGridLayout(w);
    layout->setColumnStretch(0, 100);
    layout->setColumnStretch(1, 100);    
    WuQtUtilities::setLayoutMargins(layout, 2, 0);
    switch (contentPlacement) {
        case WIDGET_PLACEMENT_BOTTOM:
            //layout->addStretch();
            layout->setRowStretch(0, 100);
            layout->setRowStretch(1, 0);
            layout->addWidget(childWidget, 1, 0, 1, 2);
            break;
        case WIDGET_PLACEMENT_TOP:
            layout->setRowStretch(1, 100);
            layout->setRowStretch(0, 0);
            layout->addWidget(childWidget, 0, 0, 1, 2);
            //layout->addStretch();
            break;
        case WIDGET_PLACEMENT_NONE:
            layout->setRowStretch(0, 0);
            layout->addWidget(childWidget, 0, 0, 1, 2);
            break;
        default:
            CaretAssert(0);
    }
    layout->addWidget(nameLabel, 2, 0, 1, 2, Qt::AlignHCenter);
    
    const bool addVerticalBarOnLeftSide = (verticalBarPlacement == WIDGET_PLACEMENT_LEFT);
    const bool addVerticalBarOnRightSide = (verticalBarPlacement == WIDGET_PLACEMENT_RIGHT);
    
    if (addVerticalBarOnLeftSide
        || addVerticalBarOnRightSide) {
        QWidget* w2 = new QWidget();
        QHBoxLayout* horizLayout = new QHBoxLayout(w2);
        WuQtUtilities::setLayoutMargins(horizLayout, 0, 0);
        if (addVerticalBarOnLeftSide) {
            horizLayout->addWidget(WuQtUtilities::createVerticalLineWidget(), 0);
            horizLayout->addSpacing(3);
        }
        const int widgetStretchFactor = 100;
        horizLayout->addWidget(w, widgetStretchFactor);
        if (addVerticalBarOnRightSide) {
            horizLayout->addSpacing(3);
            horizLayout->addWidget(WuQtUtilities::createVerticalLineWidget(), 0);
        }
        w = w2;
    }
 
    return w;
}

/**
 * Update the graphics windows for the selected tab.
 */
void 
BrainBrowserWindowToolBar::updateGraphicsWindow()
{
    EventManager::get()->sendEvent(
            EventGraphicsUpdateOneWindow(this->browserWindowIndex).getPointer());
}

/**
 * Update the user-interface.
 */
void 
BrainBrowserWindowToolBar::updateUserInterface()
{
    EventManager::get()->sendEvent(EventUserInterfaceUpdate().setWindowIndex(this->browserWindowIndex).getPointer());
}

/**
 * Update the toolbox for the window
 */
void 
BrainBrowserWindowToolBar::updateToolBox()
{
    EventManager::get()->sendEvent(EventUserInterfaceUpdate().setWindowIndex(this->browserWindowIndex).addToolBox().getPointer());
}

/**
 * Called when a view mode is selected.
 */
void 
BrainBrowserWindowToolBar::viewModeRadioButtonClicked(QAbstractButton*)
{
    CaretLogEntering();
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    if (btc == NULL) {
        return;
    }
    
    if (this->viewModeSurfaceRadioButton->isChecked()) {
        btc->setSelectedModelType(ModelTypeEnum::MODEL_TYPE_SURFACE);
    }
    else if (this->viewModeSurfaceMontageRadioButton->isChecked()) {
        btc->setSelectedModelType(ModelTypeEnum::MODEL_TYPE_SURFACE_MONTAGE);
    }
    else if (this->viewModeVolumeRadioButton->isChecked()) {
        btc->setSelectedModelType(ModelTypeEnum::MODEL_TYPE_VOLUME_SLICES);
    }
    else if (this->viewModeWholeBrainRadioButton->isChecked()) {
        btc->setSelectedModelType(ModelTypeEnum::MODEL_TYPE_WHOLE_BRAIN);
    }
    else {
        btc->setSelectedModelType(ModelTypeEnum::MODEL_TYPE_INVALID);
    }
    
    this->checkUpdateCounter();
    this->updateToolBar();
    this->updateTabName(-1);
    this->updateToolBox();
    emit viewedModelChanged();
    this->updateGraphicsWindow();
}

/**
 * Called when orientation left or lateral button is pressed.
 */
void 
BrainBrowserWindowToolBar::orientationLeftOrLateralToolButtonTriggered(bool /*checked*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
        btc->leftView();
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
    
    this->checkUpdateCounter();
}

/**
 * Called when orientation right or medial button is pressed.
 */
void 
BrainBrowserWindowToolBar::orientationRightOrMedialToolButtonTriggered(bool /*checked*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
        btc->rightView();
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
    
    this->checkUpdateCounter();
}

/**
 * Called when orientation anterior button is pressed.
 */
void 
BrainBrowserWindowToolBar::orientationAnteriorToolButtonTriggered(bool /*checked*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->anteriorView();
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
    
    this->checkUpdateCounter();
}

/**
 * Called when orientation posterior button is pressed.
 */
void 
BrainBrowserWindowToolBar::orientationPosteriorToolButtonTriggered(bool /*checked*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->posteriorView();
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
    
    this->checkUpdateCounter();
}

/**
 * Called when orientation dorsal button is pressed.
 */
void 
BrainBrowserWindowToolBar::orientationDorsalToolButtonTriggered(bool /*checked*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->dorsalView();
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
    
    this->checkUpdateCounter();
}

/**
 * Called when orientation ventral button is pressed.
 */
void 
BrainBrowserWindowToolBar::orientationVentralToolButtonTriggered(bool /*checked*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->ventralView();
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
    
    this->checkUpdateCounter();
}

/**
 * Called when orientation reset button is pressed.
 */
void 
BrainBrowserWindowToolBar::orientationResetToolButtonTriggered(bool /*checked*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->resetView();
    
    Model* mdc = btc->getModelControllerForDisplay();
    if (mdc != NULL) {
        this->updateVolumeIndicesWidget(btc);
        this->updateGraphicsWindow();
        this->updateOtherYokedWindows();
    }
    
    this->checkUpdateCounter();
}

/**
 * Called when orientation lateral/medial button is pressed.
 */
void 
BrainBrowserWindowToolBar::orientationLateralMedialToolButtonTriggered(bool /*checked*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->leftView();
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
    
    this->checkUpdateCounter();
}

/**
 * Called when orientation dorsal/ventral button is pressed.
 */
void 
BrainBrowserWindowToolBar::orientationDorsalVentralToolButtonTriggered(bool /*checked*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->dorsalView();
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
    
    this->checkUpdateCounter();
}

/**
 * Called when orientation anterior/posterior button is pressed.
 */
void 
BrainBrowserWindowToolBar::orientationAnteriorPosteriorToolButtonTriggered(bool /*checked*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->anteriorView();
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
    
    this->checkUpdateCounter();
}

/**
 * Called when orientation custom view button is pressed to show
 * custom view menu.
 */
void
BrainBrowserWindowToolBar::orientationCustomViewToolButtonTriggered()
{
    CaretPreferences* prefs = SessionManager::get()->getCaretPreferences();
    prefs->readCustomViews();
    const std::vector<std::pair<AString, AString> > customViewNameAndComments = prefs->getCustomViewNamesAndComments();
    
    QMenu menu;
    
    QAction* editAction = menu.addAction("Create and Edit...");
    editAction->setToolTip("Add and delete Custom Views.\n"
                           "Edit model transformations.");
    
    const int32_t numViews = static_cast<int32_t>(customViewNameAndComments.size());
    if (numViews > 0) {
        menu.addSeparator();
    }
    for (int32_t i = 0; i < numViews; i++) {
        QAction* action = menu.addAction(customViewNameAndComments[i].first);
        action->setToolTip(WuQtUtilities::createWordWrappedToolTipText(customViewNameAndComments[i].second));
    }
    
    QAction* selectedAction = menu.exec(QCursor::pos());
    if (selectedAction != NULL) {
        if (selectedAction == editAction) {
            BrainBrowserWindow* bbw = GuiManager::get()->getBrowserWindowByWindowIndex(browserWindowIndex);
            GuiManager::get()->processShowCustomViewDialog(bbw);
        }
        else {
            const AString customViewName = selectedAction->text();
            
            ModelTransform modelTransform;
            if (prefs->getCustomView(customViewName, modelTransform)) {
                BrowserTabContent* btc = this->getTabContentFromSelectedTab();
                btc->setTransformationsFromModelTransform(modelTransform);
                this->updateGraphicsWindow();
                this->updateOtherYokedWindows();
            }
        }
    }
}

/**
 * Called when the whole brain surface type combo box is changed.
 */
void 
BrainBrowserWindowToolBar::wholeBrainSurfaceTypeComboBoxIndexChanged(int /*indx*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    const int32_t tabIndex = btc->getTabNumber();
    
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController == NULL) {
        return;
    }

    int32_t comboBoxIndex = this->wholeBrainSurfaceTypeComboBox->currentIndex();
    if (comboBoxIndex >= 0) {
        const int32_t integerCode = this->wholeBrainSurfaceTypeComboBox->itemData(comboBoxIndex).toInt();
        bool isValid = false;
        const SurfaceTypeEnum::Enum surfaceType = SurfaceTypeEnum::fromIntegerCode(integerCode, &isValid);
        if (isValid) {
            wholeBrainController->setSelectedSurfaceType(tabIndex, surfaceType);
            this->updateVolumeIndicesWidget(btc); // slices may get deselected
            this->updateGraphicsWindow();
            this->updateOtherYokedWindows();
        }
    }
}

/**
 * Called when whole brain surface left check box is toggled.
 */
void 
BrainBrowserWindowToolBar::wholeBrainSurfaceLeftCheckBoxStateChanged(int /*state*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController == NULL) {
        return;
    }
    
    btc->setWholeBrainLeftEnabled(this->wholeBrainSurfaceLeftCheckBox->isChecked());
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when the left surface tool button is pressed.
 */
void 
BrainBrowserWindowToolBar::wholeBrainSurfaceLeftToolButtonTriggered(bool /*checked*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController == NULL) {
        return;
    }
    const int32_t tabIndex = btc->getTabNumber();
    
    Brain* brain = GuiManager::get()->getBrain();
    BrainStructure* brainStructure = brain->getBrainStructure(StructureEnum::CORTEX_LEFT, false);
    if (brainStructure != NULL) {
        std::vector<Surface*> surfaces;
        brainStructure->getSurfacesOfType(wholeBrainController->getSelectedSurfaceType(tabIndex),
                                          surfaces);
        
        const int32_t numSurfaces = static_cast<int32_t>(surfaces.size());
        if (numSurfaces > 0) {
            Surface* selectedSurface = wholeBrainController->getSelectedSurface(StructureEnum::CORTEX_LEFT,
                                                                                tabIndex);
            QMenu menu;
            QActionGroup* actionGroup = new QActionGroup(&menu);
            actionGroup->setExclusive(true);
            for (int32_t i = 0; i < numSurfaces; i++) {
                QString name = surfaces[i]->getFileNameNoPath();
                QAction* action = actionGroup->addAction(name);
                action->setCheckable(true);
                if (surfaces[i] == selectedSurface) {
                    action->setChecked(true);
                }
                menu.addAction(action);
            }
            QAction* result = menu.exec(QCursor::pos());
            if (result != NULL) {
                QList<QAction*> actionList = actionGroup->actions();
                for (int32_t i = 0; i < numSurfaces; i++) {
                    if (result == actionList.at(i)) {
                        wholeBrainController->setSelectedSurface(StructureEnum::CORTEX_LEFT,
                                                                 tabIndex, 
                                                                 surfaces[i]);
                        this->updateGraphicsWindow();
                        this->updateOtherYokedWindows();
                        break;
                    }
                }
            }
        }
    }
}

/** 
 * Called when the right surface tool button is pressed.
 */
void 
BrainBrowserWindowToolBar::wholeBrainSurfaceRightToolButtonTriggered(bool /*checked*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController == NULL) {
        return;
    }
    const int32_t tabIndex = btc->getTabNumber();
    
    Brain* brain = GuiManager::get()->getBrain();
    BrainStructure* brainStructure = brain->getBrainStructure(StructureEnum::CORTEX_RIGHT, false);
    if (brainStructure != NULL) {
        std::vector<Surface*> surfaces;
        brainStructure->getSurfacesOfType(wholeBrainController->getSelectedSurfaceType(tabIndex),
                                          surfaces);
        
        const int32_t numSurfaces = static_cast<int32_t>(surfaces.size());
        if (numSurfaces > 0) {
            Surface* selectedSurface = wholeBrainController->getSelectedSurface(StructureEnum::CORTEX_RIGHT,
                                                                                tabIndex);
            QMenu menu;
            QActionGroup* actionGroup = new QActionGroup(&menu);
            actionGroup->setExclusive(true);
            for (int32_t i = 0; i < numSurfaces; i++) {
                QString name = surfaces[i]->getFileNameNoPath();
                QAction* action = actionGroup->addAction(name);
                action->setCheckable(true);
                if (surfaces[i] == selectedSurface) {
                    action->setChecked(true);
                }
                menu.addAction(action);
            }
            QAction* result = menu.exec(QCursor::pos());
            if (result != NULL) {
                QList<QAction*> actionList = actionGroup->actions();
                for (int32_t i = 0; i < numSurfaces; i++) {
                    if (result == actionList.at(i)) {
                        wholeBrainController->setSelectedSurface(StructureEnum::CORTEX_RIGHT,
                                                                 tabIndex, 
                                                                 surfaces[i]);
                        this->updateGraphicsWindow();
                        this->updateOtherYokedWindows();
                        break;
                    }
                }
            }
        }
    }
}

/** 
 * Called when the cerebellum surface tool button is pressed.
 */
void 
BrainBrowserWindowToolBar::wholeBrainSurfaceCerebellumToolButtonTriggered(bool /*checked*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController == NULL) {
        return;
    }
    const int32_t tabIndex = btc->getTabNumber();
    
    Brain* brain = GuiManager::get()->getBrain();
    BrainStructure* brainStructure = brain->getBrainStructure(StructureEnum::CEREBELLUM, false);
    if (brainStructure != NULL) {
        std::vector<Surface*> surfaces;
        brainStructure->getSurfacesOfType(wholeBrainController->getSelectedSurfaceType(tabIndex),
                                          surfaces);
        
        const int32_t numSurfaces = static_cast<int32_t>(surfaces.size());
        if (numSurfaces > 0) {
            Surface* selectedSurface = wholeBrainController->getSelectedSurface(StructureEnum::CEREBELLUM,
                                                                                tabIndex);
            QMenu menu;
            QActionGroup* actionGroup = new QActionGroup(&menu);
            actionGroup->setExclusive(true);
            for (int32_t i = 0; i < numSurfaces; i++) {
                QString name = surfaces[i]->getFileNameNoPath();
                QAction* action = actionGroup->addAction(name);
                action->setCheckable(true);
                if (surfaces[i] == selectedSurface) {
                    action->setChecked(true);
                }
                menu.addAction(action);
            }
            QAction* result = menu.exec(QCursor::pos());
            if (result != NULL) {
                QList<QAction*> actionList = actionGroup->actions();
                for (int32_t i = 0; i < numSurfaces; i++) {
                    if (result == actionList.at(i)) {
                        wholeBrainController->setSelectedSurface(StructureEnum::CEREBELLUM,
                                                                 tabIndex, 
                                                                 surfaces[i]);
                        this->updateGraphicsWindow();
                        this->updateOtherYokedWindows();
                        break;
                    }
                }
            }
        }
    }
}

/**
 * Called when whole brain surface right checkbox is toggled.
 */
void 
BrainBrowserWindowToolBar::wholeBrainSurfaceRightCheckBoxStateChanged(int /*state*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController == NULL) {
        return;
    }
    
    btc->setWholeBrainRightEnabled(this->wholeBrainSurfaceRightCheckBox->isChecked());
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when whole brain cerebellum check box is toggled.
 */
void 
BrainBrowserWindowToolBar::wholeBrainSurfaceCerebellumCheckBoxStateChanged(int /*state*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController == NULL) {
        return;
    }
    
    btc->setWholeBrainCerebellumEnabled(this->wholeBrainSurfaceCerebellumCheckBox->isChecked());
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when whole brain separation left/right spin box value is changed.
 */
void 
BrainBrowserWindowToolBar::wholeBrainSurfaceSeparationLeftRightSpinBoxValueChanged(double /*d*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();

    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController == NULL) {
        return;
    }
    
    btc->setWholeBrainLeftRightSeparation(this->wholeBrainSurfaceSeparationLeftRightSpinBox->value());
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when whole brain left&right/cerebellum spin box value is changed.
 */
void 
BrainBrowserWindowToolBar::wholeBrainSurfaceSeparationCerebellumSpinBoxSelected(double /*d*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();

    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController == NULL) {
        return;
    }
    
    btc->setWholeBrainCerebellumSeparation(this->wholeBrainSurfaceSeparationCerebellumSpinBox->value());
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when volume indices ORIGIN tool button is pressed.
 */
void
BrainBrowserWindowToolBar::volumeIndicesOriginActionTriggered()
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->setSlicesToOrigin();
    
    this->updateVolumeIndicesWidget(btc);
    
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when volume indices parasagittal check box is toggled.
 */
void 
BrainBrowserWindowToolBar::volumeIndicesParasagittalCheckBoxStateChanged(int /*state*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->setSliceParasagittalEnabled(this->volumeIndicesParasagittalCheckBox->isChecked());
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when volume indices coronal check box is toggled.
 */
void 
BrainBrowserWindowToolBar::volumeIndicesCoronalCheckBoxStateChanged(int /*state*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    this->updateOtherYokedWindows();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->setSliceCoronalEnabled(this->volumeIndicesCoronalCheckBox->isChecked());
    this->updateGraphicsWindow();
}

/**
 * Called when volume indices axial check box is toggled.
 */
void 
BrainBrowserWindowToolBar::volumeIndicesAxialCheckBoxStateChanged(int /*state*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->setSliceAxialEnabled(this->volumeIndicesAxialCheckBox->isChecked());
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when volume indices parasagittal spin box value is changed.
 */
void 
BrainBrowserWindowToolBar::volumeIndicesParasagittalSpinBoxValueChanged(int /*i*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    this->readVolumeSliceIndicesAndUpdateSliceCoordinates();
}

/**
 * Called when volume indices coronal spin box value is changed.
 */
void 
BrainBrowserWindowToolBar::volumeIndicesCoronalSpinBoxValueChanged(int /*i*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    this->readVolumeSliceIndicesAndUpdateSliceCoordinates();
}

/**
 * Called when volume indices axial spin box value is changed.
 */
void 
BrainBrowserWindowToolBar::volumeIndicesAxialSpinBoxValueChanged(int /*i*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    this->readVolumeSliceIndicesAndUpdateSliceCoordinates();
}

/**
 * Called when X stereotaxic coordinate is changed.
 * @param d
 *    New value.
 */
void 
BrainBrowserWindowToolBar::volumeIndicesXcoordSpinBoxValueChanged(double /*d*/)
{
    this->readVolumeSliceCoordinatesAndUpdateSliceIndices();
}

/**
 * Called when Y stereotaxic coordinate is changed.
 * @param d
 *    New value.
 */
void 
BrainBrowserWindowToolBar::volumeIndicesYcoordSpinBoxValueChanged(double /*d*/)
{
    this->readVolumeSliceCoordinatesAndUpdateSliceIndices();
}

/**
 * Called when Z stereotaxic coordinate is changed.
 * @param d
 *    New value.
 */
void 
BrainBrowserWindowToolBar::volumeIndicesZcoordSpinBoxValueChanged(double /*d*/)
{
    this->readVolumeSliceCoordinatesAndUpdateSliceIndices();
}

/**
 * Read the slice indices and update the slice coordinates.
 */
void 
BrainBrowserWindowToolBar::readVolumeSliceIndicesAndUpdateSliceCoordinates()
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    const int32_t tabIndex = btc->getTabNumber();
    
    VolumeFile* underlayVolumeFile = NULL;
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController != NULL) {
        if (this->getDisplayedModelController() == wholeBrainController) {
            underlayVolumeFile = wholeBrainController->getUnderlayVolumeFile(tabIndex);
        }
    }
    
    ModelVolume* volumeController = btc->getDisplayedVolumeModel();
    if (volumeController != NULL) {
        if (this->getDisplayedModelController() == volumeController) {
            underlayVolumeFile = volumeController->getUnderlayVolumeFile(tabIndex);
        }
    }
    
        if (underlayVolumeFile != NULL) {
            const int64_t parasagittalSlice = this->volumeIndicesParasagittalSpinBox->value();
            const int64_t coronalSlice      = this->volumeIndicesCoronalSpinBox->value();
            const int64_t axialSlice        = this->volumeIndicesAxialSpinBox->value();
            btc->setSliceIndexAxial(underlayVolumeFile, axialSlice);
            btc->setSliceIndexCoronal(underlayVolumeFile, coronalSlice);
            btc->setSliceIndexParasagittal(underlayVolumeFile, parasagittalSlice);
        }
    
    this->updateSliceIndicesAndCoordinatesRanges();
    
    this->updateGraphicsWindow();    
    this->updateOtherYokedWindows();
}

/**
 * Read the slice coordinates and convert to slices indices and then
 * update the displayed slices.
 */
void 
BrainBrowserWindowToolBar::readVolumeSliceCoordinatesAndUpdateSliceIndices()
{    
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    const int32_t tabIndex = btc->getTabNumber();
    
    VolumeFile* underlayVolumeFile = NULL;
    ModelWholeBrain* wholeBrainController = btc->getDisplayedWholeBrainModel();
    if (wholeBrainController != NULL) {
        if (this->getDisplayedModelController() == wholeBrainController) {
            underlayVolumeFile = wholeBrainController->getUnderlayVolumeFile(tabIndex);
        }
    }
    
    ModelVolume* volumeController = btc->getDisplayedVolumeModel();
    if (volumeController != NULL) {
        if (this->getDisplayedModelController() == volumeController) {
            underlayVolumeFile = volumeController->getUnderlayVolumeFile(tabIndex);
        }
    }
    
    if (underlayVolumeFile != NULL) {
        float sliceCoords[3] = {
            this->volumeIndicesXcoordSpinBox->value(),
            this->volumeIndicesYcoordSpinBox->value(),
            this->volumeIndicesZcoordSpinBox->value()
        };
        
        btc->selectSlicesAtCoordinate(sliceCoords);
    }
    
    this->updateSliceIndicesAndCoordinatesRanges();
    
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when window yoke to tab combo box is selected.
 */
void 
BrainBrowserWindowToolBar::windowYokeToGroupComboBoxIndexChanged()
{
    CaretLogEntering();
    this->checkUpdateCounter();

        BrowserTabContent* btc = this->getTabContentFromSelectedTab();
        if (btc == NULL) {
            return;
        }
        
        YokingGroupEnum::Enum yokingGroup = m_yokingGroupComboBox->getSelectedItem<YokingGroupEnum, YokingGroupEnum::Enum>();
        btc->setYokingGroup(yokingGroup);
            this->updateVolumeIndicesWidget(btc);
            this->updateVolumeMontageWidget(btc);
            this->updateVolumePlaneWidget(btc);
    this->updateGraphicsWindow();
}

/**
 * Called when a single surface control is changed.
 * @param structure
 *      Structure that is selected.
 * @param surfaceController
 *     Controller that is selected.
 */
void 
BrainBrowserWindowToolBar::surfaceSelectionControlChanged(
                                    const StructureEnum::Enum structure,
                                    ModelSurface* surfaceController)
{
    if (surfaceController != NULL) {
        BrowserTabContent* btc = this->getTabContentFromSelectedTab();
        ModelSurfaceSelector* surfaceModelSelector = btc->getSurfaceModelSelector();
        surfaceModelSelector->setSelectedStructure(structure);
        surfaceModelSelector->setSelectedSurfaceController(surfaceController);
        EventManager::get()->sendEvent(EventSurfaceColoringInvalidate().getPointer());
        this->updateUserInterface();
        this->updateGraphicsWindow();
    }
    
    this->updateTabName(-1);
    
    this->checkUpdateCounter();    
}

/**
 * Called when volume slice plane button is clicked.
 */
void 
BrainBrowserWindowToolBar::volumePlaneActionGroupTriggered(QAction* action)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    VolumeSliceViewPlaneEnum::Enum plane = VolumeSliceViewPlaneEnum::AXIAL;
    
    if (action == this->volumePlaneAllToolButtonAction) {
        plane = VolumeSliceViewPlaneEnum::ALL;
    }
    else if (action == this->volumePlaneAxialToolButtonAction) {
        plane = VolumeSliceViewPlaneEnum::AXIAL;
        
    }
    else if (action == this->volumePlaneCoronalToolButtonAction) {
        plane = VolumeSliceViewPlaneEnum::CORONAL;
        
    }
    else if (action == this->volumePlaneParasagittalToolButtonAction) {
        plane = VolumeSliceViewPlaneEnum::PARASAGITTAL;
    }
    else {
        CaretLogSevere("Invalid volume plane action: " + action->text());
    }
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    btc->setSliceViewPlane(plane);
    
    /*
     * If ALL and currently Montage, switch from Montage to Orthogonal
     * since ALL and Montage are incompatible.
     */
    if (plane == VolumeSliceViewPlaneEnum::ALL) {
        if (btc->getSliceViewMode() == VolumeSliceViewModeEnum::MONTAGE) {
            btc->setSliceViewMode(VolumeSliceViewModeEnum::ORTHOGONAL);
            this->updateVolumePlaneWidget(btc);
        }
    }
    
    this->updateVolumeIndicesWidget(btc);
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when volume view plane button is clicked.
 */
void 
BrainBrowserWindowToolBar::volumePlaneViewActionGroupTriggered(QAction* action)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    VolumeSliceViewModeEnum::Enum mode = VolumeSliceViewModeEnum::ORTHOGONAL;
    
    if (action == this->volumePlaneViewOrthogonalToolButtonAction) {
        mode = VolumeSliceViewModeEnum::ORTHOGONAL;
    }
    else if (action == this->volumePlaneViewMontageToolButtonAction) {
        mode = VolumeSliceViewModeEnum::MONTAGE;
    }
    else if (action == this->volumePlaneViewObliqueToolButtonAction) {
        mode = VolumeSliceViewModeEnum::OBLIQUE;
    }
    else {
        CaretLogSevere("Invalid volume plane view action: " + action->text());
    }
     
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    btc->setSliceViewMode(mode);
    /*
     * If Montage and currently ALL, switch from ALL to Axial
     * since ALL and Montage are incompatible.
     */
    if (mode == VolumeSliceViewModeEnum::MONTAGE) {
        if (btc->getSliceViewPlane() == VolumeSliceViewPlaneEnum::ALL) {
            btc->setSliceViewPlane(VolumeSliceViewPlaneEnum::AXIAL);
            this->updateVolumePlaneWidget(btc);
        }
    }
    this->updateVolumeIndicesWidget(btc);
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when volume reset slice view button is pressed.
 */
void 
BrainBrowserWindowToolBar::volumePlaneResetToolButtonTriggered(bool /*checked*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->resetView();

    this->updateVolumeIndicesWidget(btc);
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when montage rows spin box value is changed.
 */
void 
BrainBrowserWindowToolBar::montageRowsSpinBoxValueChanged(int /*i*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    btc->setMontageNumberOfRows(this->montageRowsSpinBox->value());
    
    /*
     * When yoked, need to update other toolbars.
     */
   // EventManager::get()->sendEvent(EventUserInterfaceUpdate().addToolBar().getPointer());

    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when montage columns spin box value is changed.
 */
void 
BrainBrowserWindowToolBar::montageColumnsSpinBoxValueChanged(int /*i*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->setMontageNumberOfColumns(this->montageColumnsSpinBox->value());
    
        //EventManager::get()->sendEvent(EventUserInterfaceUpdate().addToolBar().getPointer());
    
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when montage spacing spin box value is changed.
 */
void 
BrainBrowserWindowToolBar::montageSpacingSpinBoxValueChanged(int /*i*/)
{
    CaretLogEntering();
    this->checkUpdateCounter();
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    btc->setMontageSliceSpacing(this->montageSpacingSpinBox->value());
    
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

void 
BrainBrowserWindowToolBar::checkUpdateCounter()
{
    if (this->updateCounter != 0) {
        CaretLogWarning(AString("Update counter is non-zero, this indicates that signal needs to be blocked during update, value=")
                        + AString::number(updateCounter));
    }
}

void 
BrainBrowserWindowToolBar::incrementUpdateCounter(const char* /*methodName*/)
{
    this->updateCounter++;
}

void 
BrainBrowserWindowToolBar::decrementUpdateCounter(const char* /*methodName*/)
{
    this->updateCounter--;
}

/**
 * Receive events from the event manager.
 *  
 * @param event
 *   Event sent by event manager.
 */
void 
BrainBrowserWindowToolBar::receiveEvent(Event* event)
{
    if (event->getEventType() == EventTypeEnum::EVENT_BROWSER_WINDOW_CONTENT_GET) {
        EventBrowserWindowContentGet* getModelEvent =
            dynamic_cast<EventBrowserWindowContentGet*>(event);
        CaretAssert(getModelEvent);
        
        if (getModelEvent->getBrowserWindowIndex() == this->browserWindowIndex) {
            BrainBrowserWindow* browserWindow = GuiManager::get()->getBrowserWindowByWindowIndex(this->browserWindowIndex);
            if (browserWindow != NULL) {
                BrainBrowserWindowScreenModeEnum::Enum screenMode = browserWindow->getScreenMode();
                
                bool showMontage = false;
                switch (screenMode) {
                    case BrainBrowserWindowScreenModeEnum::NORMAL:
                        break;
                    case BrainBrowserWindowScreenModeEnum::FULL_SCREEN:
                        break;
                    case BrainBrowserWindowScreenModeEnum::TAB_MONTAGE:
                        showMontage = true;
                        break;
                    case BrainBrowserWindowScreenModeEnum::TAB_MONTAGE_FULL_SCREEN:
                        showMontage = true;
                        break;
                }
                if (showMontage) {
                    const int32_t numTabs = this->tabBar->count();
                    for (int32_t i = 0; i < numTabs; i++) {
                        BrowserTabContent* btc = this->getTabContentFromTab(i);
                        getModelEvent->addTabContentToDraw(btc);
                    }
                }
                else {
                    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
                    getModelEvent->addTabContentToDraw(btc);
                }
            }
            getModelEvent->setEventProcessed();
        }
    }
    else if (event->getEventType() == EventTypeEnum::EVENT_USER_INTERFACE_UPDATE) {
        EventUserInterfaceUpdate* uiEvent =
        dynamic_cast<EventUserInterfaceUpdate*>(event);
        CaretAssert(uiEvent);
        
        if (uiEvent->isToolBarUpdate()
            && uiEvent->isUpdateForWindow(this->browserWindowIndex)) {
            this->updateToolBar();
            uiEvent->setEventProcessed();
        }
    }
    else if (event->getEventType() == EventTypeEnum::EVENT_BROWSER_WINDOW_CREATE_TABS) {
        EventBrowserWindowCreateTabs* tabEvent =
        dynamic_cast<EventBrowserWindowCreateTabs*>(event);
        CaretAssert(tabEvent);
        
        EventModelGetAll eventAllModels;
        EventManager::get()->sendEvent(eventAllModels.getPointer());
        const bool haveModels = (eventAllModels.getModels().empty() == false);
        
        if (haveModels) {
            switch (tabEvent->getMode()) {
                case EventBrowserWindowCreateTabs::MODE_LOADED_DATA_FILE:
                    if (tabBar->count() == 0) {
                        AString errorMessage;
                        createNewTab(errorMessage);
                        if (errorMessage.isEmpty() == false) {
                            CaretLogSevere(errorMessage);
                        }
                    }
                    break;
                case EventBrowserWindowCreateTabs::MODE_LOADED_SPEC_FILE:
                    this->addDefaultTabsAfterLoadingSpecFile();
                    break;
            }
        }
        tabEvent->setEventProcessed();
    }
    else if (event->getEventType() == EventTypeEnum::EVENT_UPDATE_YOKED_WINDOWS) {
        EventUpdateYokedWindows* yokeUpdateEvent =
            dynamic_cast<EventUpdateYokedWindows*>(event);
        CaretAssert(yokeUpdateEvent);
        
        BrowserTabContent* browserTabContent = getTabContentFromSelectedTab();
        if (browserTabContent != NULL) {
            if (this->browserWindowIndex != yokeUpdateEvent->getBrowserWindowIndexThatIssuedEvent()) {
                if (browserTabContent->getYokingGroup() == yokeUpdateEvent->getYokingGroup()) {
                    this->updateToolBar();
                    this->updateGraphicsWindow();
                }
            }
        }
    }
    else {
        
    }
    
    CaretLogFinest("Toolbar width/height: "
                   + AString::number(width())
                   + "/"
                   + AString::number(height()));
}

/**
 * If this windows is yoked, issue an event to update other
 * windows that are using the same yoking.
 */
void
BrainBrowserWindowToolBar::updateOtherYokedWindows()
{
    BrowserTabContent* browserTabContent = getTabContentFromSelectedTab();
    if (browserTabContent != NULL) {
        if (browserTabContent->isYoked()) {
            EventManager::get()->sendEvent(EventUpdateYokedWindows(this->browserWindowIndex,
                                                                   browserTabContent->getYokingGroup()).getPointer());
        }
    }
}

/**
 * Get the content in the browser tab.
 * @return
 *    Browser tab contents in the selected tab or NULL if none.
 */
BrowserTabContent* 
BrainBrowserWindowToolBar::getTabContentFromSelectedTab()
{
    const int tabIndex = this->tabBar->currentIndex();
    BrowserTabContent* btc = this->getTabContentFromTab(tabIndex);
    return btc;
}

/**
 * Get the content in the given browser tab
 * @param tabIndex
 *    Index of tab.
 * @return
 *    Browser tab contents in the selected tab or NULL if none.
 */
BrowserTabContent* 
BrainBrowserWindowToolBar::getTabContentFromTab(const int tabIndex)
{
    if ((tabIndex >= 0) && (tabIndex < this->tabBar->count())) {
        void* p = this->tabBar->tabData(tabIndex).value<void*>();
        BrowserTabContent* btc = (BrowserTabContent*)p;
        return btc;
    }
    
    return NULL;
}

/**
 * Get the model display controller displayed in the selected tab.
 * @return
 *     Model display controller in the selected tab or NULL if 
 *     no model is displayed.
 */
Model* 
BrainBrowserWindowToolBar::getDisplayedModelController()
{
    Model* mdc = NULL;
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    if (btc != NULL) {
        mdc = btc->getModelControllerForDisplay();
    }
    
    return mdc;
}

/**
 * Create a scene for an instance of a class.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    saving the scene.
 *
 * @return Pointer to SceneClass object representing the state of 
 *    this object.  Under some circumstances a NULL pointer may be
 *    returned.  Caller will take ownership of returned object.
 */
SceneClass* 
BrainBrowserWindowToolBar::saveToScene(const SceneAttributes* sceneAttributes,
                                const AString& instanceName)
{
    SceneClass* sceneClass = new SceneClass(instanceName,
                                            "BrainBrowserWindowToolBar",
                                            1);
    switch (sceneAttributes->getSceneType()) {
        case SceneTypeEnum::SCENE_TYPE_FULL:
            break;
        case SceneTypeEnum::SCENE_TYPE_GENERIC:
            break;
    }    
    
    /*
     * Add tabs
     */
    const int numTabs = this->tabBar->count();
    if (numTabs > 0) {
        SceneIntegerArray* sceneTabIndexArray = new SceneIntegerArray("tabIndices",
                                                                      numTabs);
        for (int32_t i = 0; i < numTabs; i++) {
            BrowserTabContent* btc = this->getTabContentFromTab(i);
            const int32_t tabIndex = btc->getTabNumber();
            sceneTabIndexArray->setValue(i,
                                         tabIndex);
        }
        sceneClass->addChild(sceneTabIndexArray);
    }

    /*
     * Add selected tab to scene
     */
    int32_t selectedTabIndex = -1;
    BrowserTabContent* selectedTab = getTabContentFromSelectedTab();
    if (selectedTab != NULL) {
        selectedTabIndex = selectedTab->getTabNumber();
    }
    sceneClass->addInteger("selectedTabIndex", selectedTabIndex);
    
    /*
     * Toolbar visible
     */
    sceneClass->addBoolean("toolBarVisible", 
                           m_toolbarWidget->isVisible());
    
    return sceneClass;
}

/**
 * Restore the state of an instance of a class.
 * 
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     SceneClass containing the state that was previously 
 *     saved and should be restored.
 */
void 
BrainBrowserWindowToolBar::restoreFromScene(const SceneAttributes* sceneAttributes,
                                     const SceneClass* sceneClass)
{
    if (sceneClass == NULL) {
        return;
    }
    
    switch (sceneAttributes->getSceneType()) {
        case SceneTypeEnum::SCENE_TYPE_FULL:
            break;
        case SceneTypeEnum::SCENE_TYPE_GENERIC:
            break;
    }    
    
    /*
     * Close any tabs
     */
    const int32_t numberOfOpenTabs = this->tabBar->count();
    for (int32_t iClose = (numberOfOpenTabs - 1); iClose >= 0; iClose--) {
        this->tabClosed(iClose);
    }
    
    /*
     * Index of selected browser tab (NOT the tabBar)
     */
    const int32_t selectedTabIndex = sceneClass->getIntegerValue("selectedTabIndex", -1);

    /*
     * Create new tabs
     */
    int32_t defaultTabBarIndex = 0;
    const ScenePrimitiveArray* sceneTabIndexArray = sceneClass->getPrimitiveArray("tabIndices");
    if (sceneTabIndexArray != NULL) {
        const int32_t numValidTabs = sceneTabIndexArray->getNumberOfArrayElements();
        for (int32_t iTab = 0; iTab < numValidTabs; iTab++) {
            const int32_t tabIndex = sceneTabIndexArray->integerValue(iTab);
            
            EventBrowserTabGet getTabContent(tabIndex);
            EventManager::get()->sendEvent(getTabContent.getPointer());
            BrowserTabContent* tabContent = getTabContent.getBrowserTab();
            if (tabContent != NULL) {
                if (tabContent->getTabNumber() == selectedTabIndex) {
                    defaultTabBarIndex = iTab;
                }
                this->addNewTab(tabContent);
            }
            else {
                sceneAttributes->addToErrorMessage("Toolbar in window "
                                                   + AString::number(this->browserWindowIndex)
                                                   + " failed to restore tab " 
                                                   + AString::number(selectedTabIndex));
            }
        }
    }
    
    /*
     * Select tab
     */
    if ((defaultTabBarIndex >= 0) 
        && (defaultTabBarIndex < tabBar->count())) {
        tabBar->setCurrentIndex(defaultTabBarIndex);
    }
    
    /*
     * Show hide toolbar
     */
    const bool showToolBar = sceneClass->getBooleanValue("toolBarVisible",
                                                         true);
    showHideToolBar(showToolBar);
}

/**
 * @return The clipping widget.
 */
QWidget*
BrainBrowserWindowToolBar::createClippingWidget()
{
    QLabel* axisLabel = new QLabel("Axis");
    QLabel* thicknessLabel = new QLabel("Thickness");
    QLabel* coordLabel = new QLabel(" Coords");
    
    this->clippingXCheckBox = new QCheckBox("X: ");
    this->clippingYCheckBox = new QCheckBox("Y: ");
    this->clippingZCheckBox = new QCheckBox("Z: ");

    QObject::connect(this->clippingXCheckBox, SIGNAL(toggled(bool)),
                     this, SLOT(clippingWidgetControlChanged()));
    QObject::connect(this->clippingYCheckBox, SIGNAL(toggled(bool)),
                     this, SLOT(clippingWidgetControlChanged()));
    QObject::connect(this->clippingZCheckBox, SIGNAL(toggled(bool)),
                     this, SLOT(clippingWidgetControlChanged()));
    
    const int   thickBoxWidth = 65;
    const float thicknessMin = 0.0;
    const float thicknessMax = std::numeric_limits<float>::max();
    const float thicknessStep = 1.0;
    const int   thicknessDecimals = 1;
    
    this->clippingXThicknessSpinBox = WuQFactory::newDoubleSpinBox();
    this->clippingXThicknessSpinBox->setRange(thicknessMin,
                                              thicknessMax);
    this->clippingXThicknessSpinBox->setSingleStep(thicknessStep);
    this->clippingXThicknessSpinBox->setDecimals(thicknessDecimals);
    this->clippingXThicknessSpinBox->setFixedWidth(thickBoxWidth);
    
    this->clippingYThicknessSpinBox = WuQFactory::newDoubleSpinBox();
    this->clippingYThicknessSpinBox->setRange(thicknessMin,
                                              thicknessMax);
    this->clippingYThicknessSpinBox->setSingleStep(thicknessStep);
    this->clippingYThicknessSpinBox->setDecimals(thicknessDecimals);
    this->clippingYThicknessSpinBox->setFixedWidth(thickBoxWidth);
    
    this->clippingZThicknessSpinBox = WuQFactory::newDoubleSpinBox();
    this->clippingZThicknessSpinBox->setRange(thicknessMin,
                                              thicknessMax);
    this->clippingZThicknessSpinBox->setSingleStep(thicknessStep);
    this->clippingZThicknessSpinBox->setDecimals(thicknessDecimals);
    this->clippingZThicknessSpinBox->setFixedWidth(thickBoxWidth);
    
    QObject::connect(this->clippingXThicknessSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(clippingWidgetControlChanged()));
    QObject::connect(this->clippingYThicknessSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(clippingWidgetControlChanged()));
    QObject::connect(this->clippingZThicknessSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(clippingWidgetControlChanged()));
    
    const int   coordBoxWidth = 60;
    const float coordMin = -std::numeric_limits<float>::max();
    const float coordMax =  std::numeric_limits<float>::max();
    const float coordStep = 1.0;
    const int   coordDecimals = 1;
    
    this->clippingXCoordSpinBox = WuQFactory::newDoubleSpinBox();
    this->clippingXCoordSpinBox->setRange(coordMin,
                                          coordMax);
    this->clippingXCoordSpinBox->setSingleStep(coordStep);
    this->clippingXCoordSpinBox->setDecimals(coordDecimals);
    this->clippingXCoordSpinBox->setFixedWidth(coordBoxWidth);
    
    this->clippingYCoordSpinBox = WuQFactory::newDoubleSpinBox();
    this->clippingYCoordSpinBox->setRange(coordMin,
                                          coordMax);
    this->clippingYCoordSpinBox->setSingleStep(coordStep);
    this->clippingYCoordSpinBox->setDecimals(coordDecimals);
    this->clippingYCoordSpinBox->setFixedWidth(coordBoxWidth);
    
    this->clippingZCoordSpinBox = WuQFactory::newDoubleSpinBox();
    this->clippingZCoordSpinBox->setRange(coordMin,
                                          coordMax);
    this->clippingZCoordSpinBox->setSingleStep(coordStep);
    this->clippingZCoordSpinBox->setDecimals(coordDecimals);
    this->clippingZCoordSpinBox->setFixedWidth(coordBoxWidth);
    
    QObject::connect(this->clippingXCoordSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(clippingWidgetControlChanged()));
    QObject::connect(this->clippingYCoordSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(clippingWidgetControlChanged()));
    QObject::connect(this->clippingZCoordSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(clippingWidgetControlChanged()));
    
    this->clippingWidgetGroup = new WuQWidgetObjectGroup(this);
    this->clippingWidgetGroup->add(clippingXCheckBox);
    this->clippingWidgetGroup->add(clippingYCheckBox);
    this->clippingWidgetGroup->add(clippingZCheckBox);
    this->clippingWidgetGroup->add(clippingXThicknessSpinBox);
    this->clippingWidgetGroup->add(clippingYThicknessSpinBox);
    this->clippingWidgetGroup->add(clippingZThicknessSpinBox);
    this->clippingWidgetGroup->add(clippingXCoordSpinBox);
    this->clippingWidgetGroup->add(clippingYCoordSpinBox);
    this->clippingWidgetGroup->add(clippingZCoordSpinBox);
    
    QWidget* widget = new QWidget();
    QGridLayout* gridLayout = new QGridLayout(widget);
    WuQtUtilities::setLayoutMargins(gridLayout, 0, 0);
    int row = 0;
    gridLayout->addWidget(axisLabel, row, 0, Qt::AlignHCenter);
    gridLayout->addWidget(coordLabel, row, 1, Qt::AlignLeft);
    gridLayout->addWidget(thicknessLabel, row, 2, Qt::AlignLeft);
    row++;
    gridLayout->addWidget(this->clippingXCheckBox, row, 0);
    gridLayout->addWidget(this->clippingXCoordSpinBox, row, 1);
    gridLayout->addWidget(this->clippingXThicknessSpinBox, row, 2);
    row++;
    gridLayout->addWidget(this->clippingYCheckBox, row, 0);
    gridLayout->addWidget(this->clippingYCoordSpinBox, row, 1);
    gridLayout->addWidget(this->clippingYThicknessSpinBox, row, 2);
    row++;
    gridLayout->addWidget(this->clippingZCheckBox, row, 0);
    gridLayout->addWidget(this->clippingZCoordSpinBox, row, 1);
    gridLayout->addWidget(this->clippingZThicknessSpinBox, row, 2);
    row++;
    
    widget->setSizePolicy(QSizePolicy::Fixed,
                     QSizePolicy::Fixed);
    
    QWidget* w = this->createToolWidget("Clipping",
                                        widget,
                                        WIDGET_PLACEMENT_LEFT,
                                        WIDGET_PLACEMENT_TOP,
                                        0);
    return w;
}

/**
 * @return The chart widget.
 */
QWidget*
BrainBrowserWindowToolBar::createChartWidget()
{
    QLabel* axisLabel = new QLabel("Axis ");
    QLabel* minLabel = new QLabel("Min");
    QLabel* maxLabel = new QLabel("Max");

    const int   minExtentBoxWidth = 65;
    const int   maxExtentBoxWidth = 65;
    const float xMinExtent = 0.0;
    const float xMaxExtent = 1000.0;
    const float yMinExtent = 0.0;
    const float yMaxExtent = 1000.0;
    const float ExtentsStep = 1.0;
    const int   ExtentsDecimals = 1;

    this->xMinExtentSpinBox = new QDoubleSpinBox();
    this->xMaxExtentSpinBox = new QDoubleSpinBox();
    this->yMinExtentSpinBox = new QDoubleSpinBox();
    this->yMaxExtentSpinBox = new QDoubleSpinBox();
    this->autoFitTimeLinesCB = new QCheckBox("Autofit Plot");
    this->showAverageCB = new QCheckBox("Show Average");
    this->zoomXAxisCB = new QCheckBox("Zoom X Axis");
    this->zoomYAxisCB = new QCheckBox("Zoom Y Axis");

    this->clearChartPB = new QToolButton();
    this->clearChartPB->setText("Clear Chart");
    this->resetViewPB = new QToolButton();
    this->resetViewPB->setText("Reset View");
    this->openTimeLinePB = new QToolButton();
    this->openTimeLinePB->setText("Open...");
    this->exportTimeLinePB = new QToolButton();
    this->exportTimeLinePB->setText("Export...");
    
    
    this->xMinExtentSpinBox->setRange(xMinExtent, xMaxExtent);
    this->xMinExtentSpinBox->setSingleStep(ExtentsStep);
    this->xMinExtentSpinBox->setDecimals(ExtentsDecimals);
    this->xMinExtentSpinBox->setFixedWidth(minExtentBoxWidth);

    this->xMaxExtentSpinBox->setRange(xMinExtent, xMaxExtent);
    this->xMaxExtentSpinBox->setSingleStep(ExtentsStep);
    this->xMaxExtentSpinBox->setDecimals(ExtentsDecimals);
    this->xMaxExtentSpinBox->setFixedWidth(maxExtentBoxWidth);
    this->xMaxExtentSpinBox->setValue(1000.0);

    this->yMinExtentSpinBox->setRange(yMinExtent, yMaxExtent);
    this->yMinExtentSpinBox->setSingleStep(ExtentsStep);
    this->yMinExtentSpinBox->setDecimals(ExtentsDecimals);
    this->yMinExtentSpinBox->setFixedWidth(minExtentBoxWidth);

    this->yMaxExtentSpinBox->setRange(yMinExtent, yMaxExtent);
    this->yMaxExtentSpinBox->setSingleStep(ExtentsStep);
    this->yMaxExtentSpinBox->setDecimals(ExtentsDecimals);
    this->yMaxExtentSpinBox->setFixedWidth(maxExtentBoxWidth);
    this->yMaxExtentSpinBox->setValue(1000.0);

    this->clearChartPB->setFixedWidth(minExtentBoxWidth);
    this->resetViewPB->setFixedWidth(maxExtentBoxWidth);
    this->openTimeLinePB->setFixedWidth(minExtentBoxWidth);
    this->exportTimeLinePB->setFixedWidth(maxExtentBoxWidth);    
    

    QObject::connect(this->xMinExtentSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(extentsControlChanged()));
    QObject::connect(this->xMaxExtentSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(extentsControlChanged()));
    QObject::connect(this->yMinExtentSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(extentsControlChanged()));
    QObject::connect(this->yMaxExtentSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(extentsControlChanged()));


    
    this->extentsWidgetGroup = new WuQWidgetObjectGroup(this);
    this->extentsWidgetGroup->add(this->xMinExtentSpinBox);
    this->extentsWidgetGroup->add(this->xMaxExtentSpinBox);
    this->extentsWidgetGroup->add(this->yMinExtentSpinBox);
    this->extentsWidgetGroup->add(this->yMaxExtentSpinBox);

    this->extentsWidgetGroup->add(this->clearChartPB);
    this->extentsWidgetGroup->add(this->resetViewPB);
    this->extentsWidgetGroup->add(this->openTimeLinePB);
    this->extentsWidgetGroup->add(this->exportTimeLinePB);

    this->extentsWidgetGroup->add(this->autoFitTimeLinesCB);
    this->extentsWidgetGroup->add(this->showAverageCB);
    this->extentsWidgetGroup->add(this->zoomXAxisCB);
    this->extentsWidgetGroup->add(this->zoomYAxisCB);

    QWidget* widget = new QWidget();
    QGridLayout* gridLayout = new QGridLayout(widget);
    WuQtUtilities::setLayoutMargins(gridLayout, 0, 0);
    int row = 0;
    gridLayout->addWidget(axisLabel, row, 0 , Qt::AlignHCenter);
    gridLayout->addWidget(minLabel, row, 1, Qt::AlignLeft);
    gridLayout->addWidget(maxLabel, row, 2, Qt::AlignLeft);
    row++;
    gridLayout->addWidget(new QLabel("X: "), row, 0);
    gridLayout->addWidget(this->xMinExtentSpinBox, row, 1);
    gridLayout->addWidget(this->xMaxExtentSpinBox, row, 2);
    gridLayout->addWidget(this->autoFitTimeLinesCB, row, 3, Qt::AlignCenter);
    row++;
    gridLayout->addWidget(new QLabel("Y: "), row, 0);
    gridLayout->addWidget(this->yMinExtentSpinBox, row, 1);
    gridLayout->addWidget(this->yMaxExtentSpinBox, row, 2);
    gridLayout->addWidget(this->showAverageCB, row, 3);
    row++;
    gridLayout->addWidget(this->clearChartPB, row, 1);
    gridLayout->addWidget(this->resetViewPB, row, 2);
    gridLayout->addWidget(this->zoomXAxisCB, row, 3);
    row++;
    gridLayout->addWidget(this->openTimeLinePB, row, 1);
    gridLayout->addWidget(this->exportTimeLinePB, row, 2);
    gridLayout->addWidget(this->zoomYAxisCB, row, 3);
    row++;

    widget->setSizePolicy(QSizePolicy::Fixed,
                          QSizePolicy::Fixed);
    
    
    QWidget* w = this->createToolWidget("Chart",
                                        widget,
                                        WIDGET_PLACEMENT_LEFT,
                                        WIDGET_PLACEMENT_TOP,
                                        0);
    return w;
}
/**
 * Update the clipping widgets.
 * @param browserTabContent
 *    Currently displayed content.
 */
void extentsControlChanged()
{


}

/**
 * Update the clipping widgets.
 * @param browserTabContent
 *    Currently displayed content.
 */
void
BrainBrowserWindowToolBar::updateClippingWidget(BrowserTabContent* browserTabContent)
{
    if (browserTabContent == NULL) {
        return;
    }
    
    this->clippingWidgetGroup->blockAllSignals(true);
    
    this->clippingXCheckBox->setChecked(browserTabContent->isClippingPlaneEnabled(0));
    this->clippingYCheckBox->setChecked(browserTabContent->isClippingPlaneEnabled(1));
    this->clippingZCheckBox->setChecked(browserTabContent->isClippingPlaneEnabled(2));

    this->clippingXCoordSpinBox->setValue(browserTabContent->getClippingPlaneCoordinate(0));
    this->clippingYCoordSpinBox->setValue(browserTabContent->getClippingPlaneCoordinate(1));
    this->clippingZCoordSpinBox->setValue(browserTabContent->getClippingPlaneCoordinate(2));
    
    this->clippingXThicknessSpinBox->setValue(browserTabContent->getClippingPlaneThickness(0));
    this->clippingYThicknessSpinBox->setValue(browserTabContent->getClippingPlaneThickness(1));
    this->clippingZThicknessSpinBox->setValue(browserTabContent->getClippingPlaneThickness(2));
    
    this->clippingWidgetGroup->blockAllSignals(false);
}

/**
 * Gets called when a control in the clipping widget changes due to user input.
 */
void
BrainBrowserWindowToolBar::clippingWidgetControlChanged()
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    
    if (btc != NULL) {
        btc->setClippingPlaneEnabled(0, this->clippingXCheckBox->isChecked());
        btc->setClippingPlaneEnabled(1, this->clippingYCheckBox->isChecked());
        btc->setClippingPlaneEnabled(2, this->clippingZCheckBox->isChecked());
        
        btc->setClippingPlaneCoordinate(0, this->clippingXCoordSpinBox->value());
        btc->setClippingPlaneCoordinate(1, this->clippingYCoordSpinBox->value());
        btc->setClippingPlaneCoordinate(2, this->clippingZCoordSpinBox->value());
        
        btc->setClippingPlaneThickness(0, this->clippingXThicknessSpinBox->value());
        btc->setClippingPlaneThickness(1, this->clippingYThicknessSpinBox->value());
        btc->setClippingPlaneThickness(2, this->clippingZThicknessSpinBox->value());
        
        updateGraphicsWindow();
    }
}



