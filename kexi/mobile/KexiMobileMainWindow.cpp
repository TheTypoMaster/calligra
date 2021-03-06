/**  This file is part of the KDE project
 *
 *  Copyright (C) 2011 Adam Pigg <adam@piggz.co.uk>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "KexiMobileMainWindow.h"
#include "KexiMobileWidget.h"
#include "KexiMobileToolbar.h"
#include "KexiMobileNavigator.h"
#include <KexiIcon.h>
#include <kexipart.h>
#include <kexipartinfo.h>
#include <KexiWindow.h>
#include <KexiView.h>
#include <core/KexiRecordNavigatorHandler.h>

#include <KDbDriverManager>

#include <KToolBar>

#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QHBoxLayout>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDebug>
#include <QFileDialog>

KexiMobileMainWindow::KexiMobileMainWindow()
{
    m_mobile = new KexiMobileWidget(0);
    m_toolbar = new KexiMobileToolbar(this);
    m_layout = new QHBoxLayout(this);

    //! part info has to be collected at this stage
    KexiPart::PartInfoList *partInfoList = Kexi::partManager().infoList();
    Q_UNUSED(partInfoList)

    m_openFileAction = new QAction(koIcon("document-open"), "Open", this);
    connect(m_openFileAction, SIGNAL(triggered(bool)), this, SLOT(slotOpenDatabase()));
    menuBar()->addAction(m_openFileAction);

    m_layout->addWidget(m_mobile);
    m_layout->setSpacing(2);
    setFixedSize(800,480);

    setCentralWidget(m_mobile);

    addToolBar(Qt::BottomToolBarArea, m_toolbar);

    connect(m_toolbar, SIGNAL(pageNavigator()), m_mobile, SLOT(showNavigator()));
    connect(m_mobile->navigator(), SIGNAL(openItem(KexiPart::Item*)), this, SLOT(openObject(KexiPart::Item*)));
}

void KexiMobileMainWindow::setupToolbar()
{

}

KexiMobileMainWindow::~KexiMobileMainWindow()
{}

void KexiMobileMainWindow::slotOpenDatabase()
{
    QString fileName;

    fileName = QFileDialog::getOpenFileName(this, xi18n("Open Database"), "", xi18n("Database Files (*.kexi)"));

    if (!fileName.isNull()) {
        KexiProject *proj = openProject(QUrl::fromLocalFile(fileName));
        if (proj) {
            m_project = proj;
            m_mobile->databaseOpened(proj);
        } else {
            qWarning() << "Project not returned";
        }
    }
}

KexiProject *KexiMobileMainWindow::openProject(const QUrl &url)
{
    KDbDriverManager driverManager;
    KDbDriver *driver = 0;

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForUrl(url);

    QString driverName = driverManager.lookupByMime(mime.name());
    driver = driverManager.driver(driverName.toLower());

    qDebug() << driverManager.driverNames();
    qDebug() << driverName;

    KexiProjectData *project_data = new KexiProjectData;
    project_data->setDatabaseName(url.path());

    qDebug() << driver;

    if (driver && driver->isFileDriver()) {
        project_data->connectionData()->setFileName(url.path());
    }

    project_data->connectionData()->driverName = driverName;
    KexiProject *project = new KexiProject(*project_data);
    return project;
}

KexiWindow* KexiMobileMainWindow::openObject(KexiPart::Item* item)
{
    bool cancelled;
    KexiWindow* win = openObject(item, Kexi::DataViewMode, &cancelled);

    if (!cancelled)
        return win;

    return 0;
}


KexiWindow *
KexiMobileMainWindow::openObject(KexiPart::Item* item, Kexi::ViewMode viewMode, bool *openingCancelled,
                                 QMap<QString, QVariant>* staticObjectArgs, QString* errorMessage)
{
    Q_ASSERT(openingCancelled);
    qDebug() << "KexiMobileMainWindow::openObject";

    KexiWindow *window = 0;

    if (!openingAllowed(item, viewMode, errorMessage)) {
        if (errorMessage)
            *errorMessage = xi18nc(
                                "opening is not allowed in \"data view/design view/text view\" mode",
                                "opening is not allowed in \"%1\" mode", Kexi::nameForViewMode(viewMode));
        *openingCancelled = true;
        return 0;
    }
    qDebug() << m_project << item;

    if (!m_project || !item)
        return 0;

//!TODO Move this to KexiUtils::WaitCursor
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif

    //Create the window
    window = m_project->openObject(m_mobile, item, viewMode, staticObjectArgs);
    if (window) {
        m_mobile->setActiveObject(window);
    }

    m_toolbar->setRecordHandler(dynamic_cast<KexiRecordNavigatorHandler*>(window->selectedView()));

#if 0
    if (window && !alreadyOpened) {
//  window->setParent(d->tabWidget);
//  KexiWindow* previousWindow = currentWindow();
        // Call switchToViewMode() and propertySetSwitched() again here because
        // this is the time when then new window is the current one - previous call did nothing.
        switchToViewMode(*window, window->currentViewMode());
        currentWindow()->selectedView()->propertySetSwitched();
//  activeWindowChanged(window, previousWindow);
    }
#endif

#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif

    return window;

}

bool KexiMobileMainWindow::openingAllowed(KexiPart::Item* item, Kexi::ViewMode viewMode, QString* errorMessage)
{
    qDebug() << viewMode;
    //! @todo this can be more complex once we deliver ACLs...
    //1 Load the part
    //2 Return true if the part loads AND the part supports the view mode AND the viewmode is Data

    KexiPart::Part * part = Kexi::partManager().part(item);
    if (!part) {
        if (errorMessage) {
            *errorMessage = Kexi::partManager().errorMsg();
        }
    }
    qDebug() << part << item->pluginId();
    /*if (part)
        qDebug() << item->pluginId() << part->supportedUserViewModes();*/
    return part /*&& (part->supportedUserViewModes() & viewMode)*/ && (viewMode == Kexi::DataViewMode);
}

//========KexiMainWindowIFace====================

void KexiMobileMainWindow::acceptProjectClosingRequested(bool *cancel)
{

}

void KexiMobileMainWindow::acceptPropertySetEditing()
{

}

KActionCollection* KexiMobileMainWindow::actionCollection() const
{
    return 0;
}

void KexiMobileMainWindow::addToolBarAction(const QString& toolBarName, QAction* action)
{

}

QList< QAction* > KexiMobileMainWindow::allActions() const
{
    return QList<QAction*>();
}

void KexiMobileMainWindow::appendWidgetToToolbar(const QString& name, QWidget* widget)
{

}

void KexiMobileMainWindow::beforeProjectClosing()
{

}

tristate KexiMobileMainWindow::closeObject(KexiPart::Item* item)
{
    return true;
}

tristate KexiMobileMainWindow::closeWindow(KexiWindow* window)
{
    return true;
}

KexiWindow* KexiMobileMainWindow::currentWindow() const
{
    return 0;
}

tristate KexiMobileMainWindow::executeCustomActionForObject(KexiPart::Item* item, const QString& actionName)
{
    return true;
}

QWidget* KexiMobileMainWindow::focusWidget() const
{
    return 0;
}

tristate KexiMobileMainWindow::getNewObjectInfo(KexiPart::Item* partItem, KexiPart::Part* part,
                                                bool *allowOverwriting,
                                                const QString& messageWhenAskingForName)
{
    return false;
}

void KexiMobileMainWindow::highlightObject(const QString& mime, const QString& name)
{

}

bool KexiMobileMainWindow::newObject(KexiPart::Info* info, bool *openingCancelled)
{
    return false;
}

KexiWindow* KexiMobileMainWindow::openObject(const QString& mime, const QString& name,
                                             Kexi::ViewMode viewMode, bool *openingCancelled,
                                             QMap< QString, QVariant >* staticObjectArgs)
{
    return 0;
}

tristate KexiMobileMainWindow::printItem(KexiPart::Item* item)
{
    return false;
}

tristate KexiMobileMainWindow::printPreviewForItem(KexiPart::Item* item)
{
    return false;
}

KexiProject* KexiMobileMainWindow::project()
{
    return m_project;
}

void KexiMobileMainWindow::projectClosed()
{

}

void KexiMobileMainWindow::propertySetSwitched(KexiWindow* window, bool force, bool preservePrevSelection, bool sortedProperties, const QByteArray& propertyToSelect)
{

}

void KexiMobileMainWindow::registerChild(KexiWindow* window)
{

}

tristate KexiMobileMainWindow::saveObject(KexiWindow* window, const QString& messageWhenAskingForName, bool dontAsk)
{
    return false;
}

void KexiMobileMainWindow::setWidgetVisibleInToolbar(QWidget* widget, bool visible)
{

}

tristate KexiMobileMainWindow::showPageSetupForItem(KexiPart::Item* item)
{
    return false;
}

void KexiMobileMainWindow::slotObjectRenamed(const KexiPart::Item& item, const QString& oldName)
{

}

tristate KexiMobileMainWindow::switchToViewMode(KexiWindow& window, Kexi::ViewMode viewMode)
{
    return false;
}

KToolBar* KexiMobileMainWindow::toolBar(const QString& name) const
{
    return 0;
}

void KexiMobileMainWindow::updatePropertyEditorInfoLabel(const QString& textToDisplayForNullSet)
{

}

bool KexiMobileMainWindow::userMode() const
{
    return true;
}

void KexiMobileMainWindow::addSearchableModel(KexiSearchableModel*)
{

}

tristate KexiMobileMainWindow::getNewObjectInfo(KexiPart::Item*, const QString&, KexiPart::Part*, bool, bool*, const QString&)
{
    return false;
}

KexiWindow* KexiMobileMainWindow::openedWindowFor(const KexiPart::Item*)
{
    return 0;
}

tristate KexiMobileMainWindow::saveObject(KexiWindow*, const QString&, KexiMainWindowIface::SaveObjectOptions)
{
    return false;
}

KexiUserFeedbackAgent* KexiMobileMainWindow::userFeedbackAgent() const
{
    return 0;
}






