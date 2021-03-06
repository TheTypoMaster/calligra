/* This file is part of the KDE project
   Copyright (C) 2003-2015 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXI_STARTUPHANDLER_H
#define KEXI_STARTUPHANDLER_H

#include <core/kexistartupdata.h>
#include <core/kexi.h>

#include <KDbTristate>

class KCmdLineArgs;
class KDbConnectionData;
class KexiProjectData;

/*! Handles startup actions for Kexi application.
*/
class KexiStartupHandler : public QObject, public KexiStartupData, public Kexi::ObjectStatus
{
    Q_OBJECT

public:
    KexiStartupHandler();
    virtual ~KexiStartupHandler();

    virtual tristate init();

    /*! Options for detectActionForFile() */
    enum DetectActionForFileOptions {
        DontConvert = 1, //!< Skip asking for conversion (used e.g. when dropdb is called).
        ThisIsAProjectFile = 2, //!< A hint, forces detection of the file as a project file.
        ThisIsAShortcutToAProjectFile = 4,    //!< A hint, forces detection of the file
                                              //!< as a shortcut to a project file.
        ThisIsAShortcutToAConnectionData = 8, //!< A hint, forces detection of the file
                                              //!< as a shortcut to a connection data.
        SkipMessages = 0x10, //!< Do not display error or warning messages.
        OpenReadOnly = 0x20  //!< Open in readonly mode.
    };

    /*! Used for opening existing file-based projects.
     Detects actions that should be performed for by looking at the file's mime type.
     \return true if actions should be performed or cancelled if action should be cancelled
     In this case there are two possibilities:
     - \a detectedImportAction == true means "import action" should be performed
     - nonempty \a detectedDriverId means "open action" should be performed.

     \a detectedDriverId can contain following special strings:
     - "shortcut" if the file looks like a shortcut to a project/connection file
     - "connection" if the file looks like a connection data file.

     \a parent is passed as a parent for potential error message boxes.
     \a _suggestedDriverId is a preferred driver ID.
     \a options should be a combination of DetectActionForFileOptions enum values.
     \a forceReadOnly points to a flag that will be set to true if it was detected
        that the file cannot be written. */
    static tristate detectActionForFile(
        KexiStartupData::Import* detectedImportAction, QString* detectedDriverId,
        const QString& _suggestedDriverId,
        const QString &databaseName, QWidget *parent = 0, int options = 0, bool *forceReadOnly = 0);

    /*! Allows user to select a project with KexiProjectSelectorDialog.
      \return selected project's data
      Returns NULL and sets cancelled to true if the dialog was cancelled.
      Returns NULL and sets cancelled to false if there was an error.
    */
    KexiProjectData* selectProject(KDbConnectionData *cdata, bool *cancelled, QWidget *parent = 0);

protected Q_SLOTS:
    void slotSaveShortcutFileChanges();

    //! Reaction to application's quit, needed because it is safer to destroy filewidget-related GUIs before
    void slotAboutToAppQuit();

protected:
//! @todo KEXI3 port getAutoopenObjects()
//    bool getAutoopenObjects(KCmdLineArgs *args, const QByteArray &action_name);

    //! Handle higher-prioroty options.
    /*! When such options are present, handle them and immediately exit without showing
     the GUI even if other options or arguments are present.
     These options are currently:
     - options that display configuration or state of Kexi installation
     @return true if such an option has been found, false if there's failure, cancelled
             if no such option has been found (in this case processing of other options
             should continue) */
    tristate handleHighPriorityOptions();

    class Private;
    Private * const d;
};

namespace Kexi
{
//! \return singleton Startup Handler singleton.
    KexiStartupHandler& startupHandler();
}

#endif
