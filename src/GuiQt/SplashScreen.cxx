
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

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#define __SPLASH_SCREEN_DECLARE__
#include "SplashScreen.h"
#undef __SPLASH_SCREEN_DECLARE__

#include "ApplicationInformation.h"
#include "CaretAssert.h"
#include "CaretFileDialog.h"
#include "CaretPreferences.h"
#include "DataFileTypeEnum.h"
#include "FileInformation.h"
#include "SessionManager.h"
#include "WuQtUtilities.h"

using namespace caret;


    
/**
 * \class caret::SplashScreen 
 * \brief Splash Screen display when Workbench is started.
 */

/**
 * Constructor.
 */
SplashScreen::SplashScreen(QWidget* parent)
: WuQDialogModal("",
            parent)
{
    QLabel* imageLabel = NULL;
    QPixmap pixmap;
    if (WuQtUtilities::loadPixmap(":/splash_startup_image.png", pixmap)) {
        imageLabel = new QLabel();
        imageLabel->setPixmap(pixmap);
        imageLabel->setAlignment(Qt::AlignCenter);
    }

    const QString labelStyle = ("QLabel { "
                                " font: 20px bold "
                                "}");
    
    QLabel* workbenchLabel  = new QLabel("Workbench");
    workbenchLabel->setStyleSheet(labelStyle);
    workbenchLabel->setAlignment(Qt::AlignCenter);

    ApplicationInformation appInfo;
    QLabel* versionLabel = new QLabel("Version "
                                      + appInfo.getVersion());
    versionLabel->setAlignment(Qt::AlignCenter);
    
    QLabel* hcpWebsiteLabel = new QLabel("<html>"
                                         "Visit<br>"
                                         "<bold><a href=\"http://www.humanconnectome.org\">Human Connectome Project</a></bold><br>"
                                         "Website"
                                         "</html>");
    hcpWebsiteLabel->setStyleSheet(labelStyle);
    hcpWebsiteLabel->setAlignment(Qt::AlignCenter);
    QObject::connect(hcpWebsiteLabel, SIGNAL(linkActivated(const QString&)),
                     this, SLOT(websiteLinkActivated(const QString&)));
    
    QStringList headerText;
    headerText.append("Spec File");
    headerText.append("Path");
    m_specFileTreeWidget = new QTreeWidget();
    m_specFileTreeWidget->setHeaderLabels(headerText);
    QObject::connect(m_specFileTreeWidget, SIGNAL(itemSelectionChanged()),
                     this, SLOT(specFileTreeWidgetItemSelected()));
    
    QVBoxLayout* leftColumnLayout = new QVBoxLayout();
    if (imageLabel != NULL) {
        leftColumnLayout->addWidget(imageLabel);
        leftColumnLayout->addSpacing(15);
    }
    leftColumnLayout->addWidget(workbenchLabel);
    leftColumnLayout->addWidget(versionLabel);
    leftColumnLayout->addSpacing(15);
    leftColumnLayout->addWidget(hcpWebsiteLabel);
    leftColumnLayout->addStretch();
    
    QWidget* widget = new QWidget();
    QHBoxLayout* horizLayout = new QHBoxLayout(widget);
    horizLayout->addLayout(leftColumnLayout);
    horizLayout->addWidget(m_specFileTreeWidget);
    
    loadSpecFileTreeWidget();
    
    m_openOtherSpecFilePushButton = addUserPushButton("Open Other...");
    
    setCancelButtonText("Skip");
    setOkButtonText("Open");

    setCentralWidget(widget);
}

/**
 * Destructor.
 */
SplashScreen::~SplashScreen()
{
    
}

/**
 * @return The selected spec file name.
 */
AString 
SplashScreen::getSelectedSpecFileName() const
{
    return m_selectedSpecFileName;
}

/**
 * Called when a label's hyperlink is selected.
 * @param link
 *   The URL.
 */
void
SplashScreen::websiteLinkActivated(const QString& link)
{
    if (link.isEmpty() == false) {
        QDesktopServices::openUrl(QUrl(link));
    }
}

/**
 * Called when spec file tree widget item is selected.
 */
void 
SplashScreen::specFileTreeWidgetItemSelected()
{
    m_selectedSpecFileName = "";
    
    QTreeWidgetItem* twi = m_specFileTreeWidget->currentItem();
    if (twi != NULL) {
        m_selectedSpecFileName = twi->data(0, Qt::UserRole).toString();
    }
}

/**
 * Called when a push button was added using addUserPushButton().
 *
 * @param userPushButton
 *    User push button that was pressed.
 */
void 
SplashScreen::userButtonPressed(QPushButton* userPushButton)
{
    if (userPushButton == m_openOtherSpecFilePushButton) {
        chooseSpecFileViaOpenFileDialog();
    }
    else {
        CaretAssertMessage(0, "Unrecognized user pushbutton clicked \""
                           + userPushButton->text()
                           + "\"");
    }
}

/**
 * Choose a spec file with the Open File Dialog.
 */
void 
SplashScreen::chooseSpecFileViaOpenFileDialog()
{
    QStringList filenameFilterList;
    filenameFilterList.append(DataFileTypeEnum::toQFileDialogFilter(DataFileTypeEnum::SPECIFICATION));
    CaretFileDialog fd(this);
    fd.setAcceptMode(CaretFileDialog::AcceptOpen);
    fd.setNameFilters(filenameFilterList);
    fd.setFileMode(CaretFileDialog::ExistingFile);
    fd.setViewMode(CaretFileDialog::List);
    
    AString errorMessages;
    
    if (fd.exec()) {
        QStringList selectedFiles = fd.selectedFiles();
        if (selectedFiles.empty() == false) {   
            m_selectedSpecFileName = selectedFiles.at(0);
            
            /*
             * Accept indicates user has 'accepted' the
             * dialog so dialog is closed (like OK clicked)
             */
            accept();
        }
    }
}

/**
 * Load Spec Files into the tree widget.
 */
void 
SplashScreen::loadSpecFileTreeWidget()
{
    m_specFileTreeWidget->clear();
    
    CaretPreferences* prefs = SessionManager::get()->getCaretPreferences();
    std::vector<AString> recentSpecFiles;
    prefs->getPreviousSpecFiles(recentSpecFiles);
    
    QTreeWidgetItem* firstItem = NULL;
    
    const int32_t numRecentSpecFiles = static_cast<int>(recentSpecFiles.size());
    for (int32_t i = 0; i < numRecentSpecFiles; i++) {
        FileInformation fileInfo(recentSpecFiles[i]);
        if (fileInfo.exists()) {
            const QString path = fileInfo.getPathName().trimmed();
            const QString name = fileInfo.getFileName().trimmed();
            const QString fullPath = fileInfo.getFilePath();

            QStringList treeText;
            treeText.append(name);
            treeText.append(path);
            
            QTreeWidgetItem* lwi = new QTreeWidgetItem(treeText);
            lwi->setData(0,
                         Qt::UserRole, 
                         fullPath);            
            lwi->setData(1,
                         Qt::UserRole, 
                         fullPath);            
            m_specFileTreeWidget->addTopLevelItem(lwi);
            
            if (firstItem == NULL) {
                firstItem = lwi;
            }
        }
    } 

    int nameColWidth = 0;
    int pathColWidth = 0;
    if (firstItem != NULL) {
        m_specFileTreeWidget->setCurrentItem(firstItem);
        
        nameColWidth = m_specFileTreeWidget->QAbstractItemView::sizeHintForColumn(0) + 25;
        pathColWidth = m_specFileTreeWidget->QAbstractItemView::sizeHintForColumn(1);
        m_specFileTreeWidget->setColumnWidth(0,
                                             nameColWidth);
    }
    
    int treeWidgetWidth = (nameColWidth
                           + pathColWidth);
    if (treeWidgetWidth > 600) {
        treeWidgetWidth = 600;
    }
    else if (treeWidgetWidth < 250) {
        treeWidgetWidth = 250;
    }
    m_specFileTreeWidget->setMinimumWidth(treeWidgetWidth);
}
