/*LICENSE_START*/
/* 
 *  Copyright 1995-2011 Washington University School of Medicine 
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

#include "TimeSeriesManagerForViewController.h"
#include "ConnectivityLoaderFile.h"
#include "ConnectivityTimeSeriesViewController.h"
#include "EventSurfaceColoringInvalidate.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventManager.h"
#include "GuiManager.h"
#include "QCoreApplication"
#include "Brain.h"
#include "QAction"
#include "QSpinBox"
#include "SessionManager.h"
#include "CaretPreferences.h"
using namespace caret;

#if 0
TimeSeriesManagerForViewController::TimeSeriesManagerForViewController(ConnectivityTimeSeriesViewController *ctsvc) : QObject(ctsvc)
{
    m_ctsvc = ctsvc;    
    m_isPlaying = false;
    EventManager::get()->sendEvent(EventSurfaceColoringInvalidate().getPointer());
    EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows(true).getPointer());
    
    m_frameIndex = 0;
    m_updateInterval = 500; //200;    

    ConnectivityLoaderFile *clf = m_ctsvc->getConnectivityLoaderFile();
    if(!clf) return;
    m_timePoints = clf->getNumberOfTimePoints();
    m_timeStep  = clf->getTimeStep();
    m_spinBox = m_ctsvc->getFrameSpinBox();

    //CaretPreferences *prefs = SessionManager::get()->getCaretPreferences();
    
    double time;
    time = 0.0;
    //prefs->getAnimationStartTime( time );
    this->setAnimationStartTime(time);

    QObject::connect(this, SIGNAL(frameSpinBoxValueChanged(const int)),
                     m_spinBox, SLOT(setValue(int)));
    m_timer = new QTimer();

    thread = new QThread(this);
    m_timer->moveToThread(thread);    
    connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
    connect(this,SIGNAL(start_timer(int)), m_timer, SLOT(start(int)));
    connect(this,SIGNAL(stop_timer()), m_timer, SLOT(stop()));
    connect(thread,SIGNAL(finished()), m_timer, SLOT(stop()));

    thread->start();
    
}

TimeSeriesManagerForViewController::~TimeSeriesManagerForViewController()
{
    stop();
    thread->exit();   
    thread->wait();
    delete m_timer;
    delete thread;
}

void TimeSeriesManagerForViewController::update()
{
    m_frameIndex = m_ctsvc->getConnectivityLoaderFile()->getSelectedFrame();
    m_frameIndex++;
    if(m_frameIndex<m_timePoints)
    {
        stop();//prevent timer events from piling up if the CPU is bogging down        
        QCoreApplication::instance()->processEvents();//give mouse events a chance to process
        emit frameSpinBoxValueChanged(m_frameIndex+1);        
        play();
    }
    else {
        m_frameIndex=0;
        stop();
        m_ctsvc->getAnimateAction()->setChecked(false);
        m_ctsvc->animateActionTriggered(false);        
        QCoreApplication::instance()->processEvents();
    }
}

void TimeSeriesManagerForViewController::play()
{
    emit start_timer(m_updateInterval);
    /*m_frameIndex = m_ctsvc->getConnectivityLoaderFile()->getSelectedFrame();
    do
    {
        if(m_frameIndex<m_timePoints)
        {   
            m_frameIndex++;
            emit frameSpinBoxValueChanged(m_frameIndex);
            QCoreApplication::instance()->processEvents();
        }
        else {
            m_frameIndex=0;
            break;
        }        
    }
    while(m_frameIndex>0);*/
}

void TimeSeriesManagerForViewController::stop()
{
    emit stop_timer();
}

void TimeSeriesManagerForViewController::toggleAnimation()
{
    if(m_timer->isActive())
        stop();
    else play();
}

void TimeSeriesManagerForViewController::setAnimationStartTime ( const double& time )
{
   this->m_startTime = time;
   //TODOJS: Remember to update frame label...
}
#endif

