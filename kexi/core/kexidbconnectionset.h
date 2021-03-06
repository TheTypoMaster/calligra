/* This file is part of the KDE project
   Copyright (C) 2003-2014 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KEXIDBCONNSET_H
#define KEXIDBCONNSET_H

#include "kexicore_export.h"

#include <QObject>
#include <KDbResult>
#include <KDbConnectionData>

class KexiDBConnectionSetPrivate;

/*! Stores information about multiple connection-data items. */
class KEXICORE_EXPORT KexiDBConnectionSet : public QObject, public KDbResultable
{
public:
    KexiDBConnectionSet();
    ~KexiDBConnectionSet();

    /*! Loads connection data set from storage, currently from
     .kexic files saved in dirs returned by
     QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "kexi/connections") */
    void load();

    /*! Adds \a data as connection data.
     \a data will be owned by a KexiDBConnectionSet object.
     If \a filename is not empty, it will be kept for use in saveConnectionData().
     saveConnectionData() is called automatically, if there's no \a filename provided
     or the filename is already used, a new unique will be generated.
     \return true on successful creating corresponding .kexic file */
    bool addConnectionData(KDbConnectionData *data, const QString& filename = QString());

    /*! Saves changes made to \a oldData to a file which name has been provided by addConnectionData().
     This function does nothing if \a oldData hasn't been added to this set.
     \return true on success (data is then copied from \a newData to \a oldData) */
    bool saveConnectionData(KDbConnectionData *oldData, const KDbConnectionData &newData);

    /*! Removed \a data from this set.
     \return true on successful removing of corresponding .kexic file */
    bool removeConnectionData(KDbConnectionData *data);

    /*! \return the list of connection data items. */
    QList<KDbConnectionData*> list() const;

    /*! \return a filename of a connection data file for \a data. */
    QString fileNameForConnectionData(const KDbConnectionData &data) const;

    /*! \return a connection data for a .kexic shortcut filename.
     0 is returned if the filename does not match. */
    KDbConnectionData* connectionDataForFileName(const QString& fileName) const;

    /*! \return key for connection data @a data, basically a comma-separated string with
     all properties serialized: "driverId,userName,...". Used in internal structures. */
    static QString key(const KDbConnectionData &data);

private:
    /*! Removes all connection data items from this set. */
    void clear();
    void addConnectionDataInternal(KDbConnectionData *data, const QString& filename);
    void removeConnectionDataInternal(KDbConnectionData *data);

    KexiDBConnectionSetPrivate * const d;
};

#endif // KEXIDBCONNSET_H
