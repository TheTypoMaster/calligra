/* This file is part of the KDE project
 *
 * Copyright (c) 2008,2010 C. Boemann <cbo@boemann.dk>
 * Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "DockerManager.h"
#include "KoDockFactoryBase.h"

#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <ktoolbar.h>

#include "ToolDocker.h"

#include "MainWindow.h"

#include <QList>
#include <QGridLayout>

class ToolDockerFactory : public KoDockFactoryBase
{
public:
    ToolDockerFactory() : KoDockFactoryBase() { }

    QString id() const {
        return "sharedtooldocker";
    }

    QDockWidget* createDockWidget() {
        ToolDocker * dockWidget = new ToolDocker();
        return dockWidget;
    }

    DockPosition defaultDockPosition() const {
        return DockRight;
    }
};

class ToolBarsDockerFactory : public KoDockFactoryBase
{
public:
    ToolBarsDockerFactory() : KoDockFactoryBase() { }

    QString id() const {
        return "ToolBarDocker";
    }

    QDockWidget* createDockWidget() {
        return new QDockWidget(i18n("Tool Bars"));
    }

    DockPosition defaultDockPosition() const {
        return DockTop;
    }
};

class DockerManager::Private
{
public:
    Private(MainWindow *mw) :
        dockedToolBarsLayout(0)
        , mainWindow(mw)
        , ignore(true) {
    }

    ToolDocker *toolOptionsDocker;
    QDockWidget *toolBarsDocker;
    QGridLayout *dockedToolBarsLayout;
    QList<KToolBar *> toolBarList;
    MainWindow *mainWindow;
    bool ignore;

    void restoringDone() {
        ignore = false;
        moveToolBars();
        toolOptionsDocker->setVisible(true); // should always be visible
    }

    void moveToolBarsBack() {
        foreach(KToolBar * toolBar, toolBarList) {
            mainWindow->addToolBar(toolBar);
        }
        toolBarList.clear();
    }

    void moveToolBars() {
        if(ignore)
            return;

        // Move toolbars to docker or back depending on visibllity of docker
        if(toolBarsDocker->isVisible()) {
            QList<KToolBar *> tmpList = mainWindow->toolBars();
            toolBarList.append(tmpList);
            foreach(KToolBar * toolBar, tmpList) {
                dockedToolBarsLayout->addWidget(toolBar);
            }
        } else {
            moveToolBarsBack();
        }
    }
};

DockerManager::DockerManager(MainWindow *mainWindow)
    : QObject(mainWindow), d(new Private(mainWindow))
{
    ToolDockerFactory toolDockerFactory;
    ToolBarsDockerFactory toolBarsDockerFactory;
    d->toolOptionsDocker =
        qobject_cast<ToolDocker*>(mainWindow->createDockWidget(&toolDockerFactory));
    Q_ASSERT(d->toolOptionsDocker);
    d->toolOptionsDocker->setVisible(false);

    d->toolBarsDocker = mainWindow->createDockWidget(&toolBarsDockerFactory);
    Q_ASSERT(d->toolBarsDocker);

    QWidget *dockedToolBarsWidget = new QWidget();
    d->dockedToolBarsLayout = new QGridLayout();
    d->dockedToolBarsLayout->setHorizontalSpacing(2);
    d->dockedToolBarsLayout->setVerticalSpacing(0);
    dockedToolBarsWidget->setLayout(d->dockedToolBarsLayout);
    d->toolBarsDocker->setAllowedAreas(Qt::TopDockWidgetArea);
    d->toolBarsDocker->setFeatures(QDockWidget::DockWidgetClosable);
    d->toolBarsDocker->setWidget(dockedToolBarsWidget);
    d->toolBarsDocker->setTitleBarWidget(new QWidget());
    d->toolBarsDocker->setVisible(false);

    connect(mainWindow, SIGNAL(restoringDone()), this, SLOT(restoringDone()));
    connect(d->toolBarsDocker, SIGNAL(visibilityChanged(bool)), this, SLOT(moveToolBars()));
    connect(mainWindow, SIGNAL(beforeHandlingToolBars()), this, SLOT(moveToolBarsBack()));
    connect(mainWindow, SIGNAL(afterHandlingToolBars()), this, SLOT(moveToolBars()));
}

DockerManager::~DockerManager()
{
    delete d;
}

void DockerManager::newOptionWidgets(const QList<QWidget*> &optionWidgetMap)
{
    d->toolOptionsDocker->setOptionWidgets(optionWidgetMap);
}

#include <DockerManager.moc>
