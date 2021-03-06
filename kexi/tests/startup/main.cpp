/* This file is part of the KDE project
   Copyright (C) 2003 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include <QDebug>
#include <QApplication>

#include <KAboutData>

#include "main/startup/KexiStartupDialog.h"
#include "main/startup/KexiConnSelector.h"
#include "core/kexiprojectset.h"
#include "core/kexiprojectdata.h"

int main(int argc, char* argv[])
{
    KApplication app(argc, argv, "startup");

// Widget w;
// app.setMainWidget(&w);

    /* KexiTableView tv;
      app.setMainWidget(&tv);

      KDbTableViewData data;
      KDbTableViewColumn col;
      col.type = QVariant::Int; col.caption = "Id"; data.addColumn( col );
      col.type = QVariant::String; col.caption = "Name"; data.addColumn( col );
      col.type = QVariant::Int; col.caption = "Age"; data.addColumn( col );
      tv.setData(&data, false);
      tv.show();*/

    //some connection data
    KexiDBConnectionSet connset;
    KDbConnectionData *conndata;
    conndata = new KDbConnectionData();
    conndata->name = "My connection 1";
    conndata->driverName = "mysql";
    conndata->hostName = "host.net";
    conndata->userName = "user";
    connset.addConnectionData(conndata);
    conndata = new KDbConnectionData();
    conndata->name = "My connection 2";
    conndata->driverName = "mysql";
    conndata->hostName = "myhost.org";
    conndata->userName = "otheruser";
    conndata->port = 53121;
    connset.addConnectionData(conndata);

    //some recent projects data
    KexiProjectData *prjdata;
    prjdata = new KexiProjectData(*conndata, "bigdb", "Big DB");
    prjdata->setCaption("My Big Project");
    prjdata->setDescription("This is my first biger project started yesterday. Have fun!");
    KexiProjectSet prj_set;
    prj_set.addProjectData(prjdata);

    KexiStartupDialog startup(KexiStartupDialog::Everything, 0, connset, prj_set, 0, "dlg");
    int e = startup.exec();
    qDebug() << (e == QDialog::Accepted ? "Accepted" : "Rejected");

    if (e == QDialog::Accepted) {
        int r = startup.result();
        if (r == KexiStartupDialog::TemplateResult) {
            qDebug() << "Template key == " << startup.selectedTemplateKey();
            if (startup.selectedTemplateKey() == "blank") {
#if 0
                KexiConnSelectorDialog sel(connset, 0, "sel");
                e = sel.exec();
                qDebug() << (e == QDialog::Accepted ? "Accepted" : "Rejected");
                if (e == QDialog::Accepted) {
                    qDebug() << "Selected conn. type: " << (sel.selectedConnectionType() == KexiConnSelectorWidget::FileBased ? "File based" : "Server based");
                    if (sel.selectedConnectionType() == KexiConnSelectorWidget::ServerBased) {
                        qDebug() << "SERVER: " << sel.selectedConnectionData()->toUserVisibleString();
                    }
                }
#endif
            }
        } else if (r == KexiStartupDialog::OpenExistingResult) {
            qDebug() << "Existing project --------";
            QString selFile = startup.selectedExistingFile();
            if (!selFile.isEmpty())
                qDebug() << "Project File: " << selFile;
            else if (startup.selectedExistingConnection()) {
                qDebug() << "Existing connection: " << startup.selectedExistingConnection()->toUserVisibleString();
                //ok, now we are trying to show daabases for this conenction to this user
                //! @todo
            }
        } else if (r == KexiStartupDialog::OpenRecentResult) {
            qDebug() << "Recent project --------";
            const KexiProjectData *data = startup.selectedProjectData();
            if (data) {
                qDebug() << "Selected project: database=" << data->databaseName()
                << " connection=" << data->connectionData()->toUserVisibleString();
            }
        }
    }
}
