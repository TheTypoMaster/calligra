/* This file is part of the KDE project
   Copyright (C) 2008 Sharan Rao <sharanrao@gmail.com>

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

#ifndef SYBASEMIGRATE_H
#define SYBASEMIGRATE_H

#include <migration/keximigrate.h>
#include <kexidb/drivers/sybase/sybaseconnection_p.h>

namespace KexiMigration
{

class SybaseMigrate : public KexiMigrate
{
    Q_OBJECT
    KEXIMIGRATION_DRIVER

public:
    explicit SybaseMigrate(QObject *parent, const QVariantList& args = QVariantList());
    virtual ~SybaseMigrate();

protected:
    //! Driver specific function to return table names
    virtual bool drv_tableNames(QStringList& tablenames);

    //! Driver specific implementation to read a table schema
    virtual bool drv_readTableSchema(
        const QString& originalName, KDbTableSchema& tableSchema);

    //! Driver specific connection implementation
    virtual bool drv_connect();

    virtual bool drv_disconnect();

    virtual tristate drv_queryStringListFromSQL(
        const QString& sqlStatement, int columnNumber,
        QStringList& stringList, int numRecords = -1);

    virtual tristate drv_fetchRecordFromSQL(const QString& sqlStatement,
                                            KDbRecordData *data, bool *firstRecord);

    virtual bool drv_copyTable(const QString& srcTable,
                               KDbConnection *destConn, KDbTableSchema* dstTable);

    virtual bool drv_progressSupported() {
        return true;
    }

    virtual bool drv_getTableSize(const QString& table, quint64 *size);

private:
    bool query(const QString& sqlStatement) const;
    bool primaryKey(const QString& tableName, const QString& fieldName) const;
    bool uniqueKey(const QString& tableName, const QString& fieldName) const;

    //! @return value of a column at position pos as a QString
    QString value(int pos) const;

    KDbField::Type type(const QString& table, int columnType);

    QList<KDbIndexSchema*> readIndexes(const QString& tableName, KDbTableSchema& tableSchema);

    SybaseConnectionInternal * const d;
};
}

#endif
