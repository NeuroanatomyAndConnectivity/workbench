
/*LICENSE_START*/
/*
 * Copyright 2012 Washington University,
 * All rights reserved.
 *
 * Connectome DB and Connectome Workbench are part of the integrated Connectome 
 * Informatics Platform.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of Washington University nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR  
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*LICENSE_END*/


#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

#define __OVERLAY_SET_VIEW_CONTROLLER_DECLARE__
#include "OverlaySetViewController.h"
#undef __OVERLAY_SET_VIEW_CONTROLLER_DECLARE__

#include "BrainConstants.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "EventManager.h"
#include "EventToolBoxUpdate.h"
#include "EventUserInterfaceUpdate.h"
#include "GuiManager.h"
#include "OverlaySet.h"
#include "OverlayViewController.h"
#include "WuQtUtilities.h"

using namespace caret;


    
/**
 * \class caret::OverlaySetViewController 
 * \brief View Controller for an overlay set.
 *
 */

/**
 * Constructor.
 * @param parent
 *    Parent widget.
 */
OverlaySetViewController::OverlaySetViewController(const int32_t browserWindowIndex,
                                                   QWidget* parent)
: QWidget(parent)
{
    this->browserWindowIndex = browserWindowIndex;
    
    for (int32_t i = 0; i < BrainConstants::MAXIMUM_NUMBER_OF_OVERLAYS; i++) {
        const bool showTopHorizontalBar = (i > 0);
        
        OverlayViewController* ovc = new OverlayViewController(browserWindowIndex,
                                                               showTopHorizontalBar);
        this->overlayViewControllers.push_back(ovc);
    }
    
    QLabel* overlayCountLabel = new QLabel("Number of Overlays: ");
    this->overlayCountSpinBox = new QSpinBox();
    this->overlayCountSpinBox->setRange(BrainConstants::MINIMUM_NUMBER_OF_OVERLAYS,
                                        BrainConstants::MAXIMUM_NUMBER_OF_OVERLAYS);
    this->overlayCountSpinBox->setSingleStep(1);
    QObject::connect(this->overlayCountSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(overlayCountSpinBoxValueChanged(int)));
    
    QHBoxLayout* overlayCountLayout = new QHBoxLayout();
    overlayCountLayout->addWidget(overlayCountLabel);
    overlayCountLayout->addWidget(this->overlayCountSpinBox);
    overlayCountLayout->addStretch();
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    WuQtUtilities::setLayoutMargins(layout, 2, 2);
    for (std::vector<OverlayViewController*>::iterator iter = this->overlayViewControllers.begin();
         iter != this->overlayViewControllers.end();
         iter++) {
        layout->addWidget(*iter);
    }
    layout->addLayout(overlayCountLayout);
    
    EventManager::get()->addEventListener(this, EventTypeEnum::EVENT_USER_INTERFACE_UPDATE);
    EventManager::get()->addEventListener(this, EventTypeEnum::EVENT_TOOLBOX_UPDATE);
}

/**
 * Destructor.
 */
OverlaySetViewController::~OverlaySetViewController()
{
    EventManager::get()->removeAllEventsFromListener(this);
}

/**
 * Called when overlay count spin box value changed.
 * @param value
 *    New value.
 */
void 
OverlaySetViewController::overlayCountSpinBoxValueChanged(int value)
{
    OverlaySet* overlaySet = this->getOverlaySet();
    if (overlaySet != NULL) {
        overlaySet->setNumberOfDisplayedOverlays(value);
        this->updateViewController();
    }
}

/**
 * @return The overlay set in this view controller.
 */
OverlaySet* 
OverlaySetViewController::getOverlaySet()
{
    OverlaySet* overlaySet = NULL;
    BrowserTabContent* browserTabContent = 
    GuiManager::get()->getBrowserTabContentForBrowserWindow(this->browserWindowIndex, true);
    if (browserTabContent != NULL) {
        overlaySet = browserTabContent->getOverlaySet();
    }
    
    return overlaySet;
}

/**
 * Update this overlay set view controller using the given overlay set.
 */
void 
OverlaySetViewController::updateViewController()
{
    /*
    BrowserTabContent* browserTabContent = 
        GuiManager::get()->getBrowserTabContentForBrowserWindow(this->browserWindowIndex, true);
    if (browserTabContent == NULL) {
        return;
    }
    
    OverlaySet* overlaySet = browserTabContent->getOverlaySet();
    */
    OverlaySet* overlaySet = this->getOverlaySet();
    if (overlaySet == NULL) {
        return;
    }
    
    const int32_t numberOfOverlays = static_cast<int32_t>(this->overlayViewControllers.size());
    const int32_t numberOfDisplayedOverlays = overlaySet->getNumberOfDisplayedOverlays();

    this->overlayCountSpinBox->blockSignals(true);
    this->overlayCountSpinBox->setValue(numberOfDisplayedOverlays);
    this->overlayCountSpinBox->blockSignals(false);
    
    for (int32_t i = 0; i < numberOfOverlays; i++) {
        Overlay* overlay = NULL;
        if (overlaySet != NULL) {
            overlay = overlaySet->getOverlay(i);
        }
        this->overlayViewControllers[i]->updateViewController(overlay);
        
        bool displayOverlay = (overlay != NULL);
        if (i >= numberOfDisplayedOverlays) {
            displayOverlay = false;
        }
        this->overlayViewControllers[i]->setVisible(displayOverlay);
    }
}

/**
 * Receive events from the event manager.
 * 
 * @param event
 *   Event sent by event manager.
 */
void 
OverlaySetViewController::receiveEvent(Event* event)
{
    if (event->getEventType() == EventTypeEnum::EVENT_USER_INTERFACE_UPDATE) {
        EventUserInterfaceUpdate* uiEvent =
            dynamic_cast<EventUserInterfaceUpdate*>(event);
        CaretAssert(uiEvent);
        
        this->updateViewController();
        
        uiEvent->setEventProcessed();
    }
    else if (event->getEventType() == EventTypeEnum::EVENT_TOOLBOX_UPDATE) {
        EventToolBoxUpdate* tbEvent =
        dynamic_cast<EventToolBoxUpdate*>(event);
        bool doUpdate = false;
        if (tbEvent->isUpdateAllWindows()) {
            doUpdate = true;
        }
        else if (tbEvent->getBrowserWindowIndex() == this->browserWindowIndex) {
            doUpdate = true;
        }
        
        if (doUpdate) {
            this->updateViewController();
        }
        
        tbEvent->setEventProcessed();
    }
    else {
    }
}
