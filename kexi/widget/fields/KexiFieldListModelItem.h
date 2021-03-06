/*
    This file is part of the KDE project
    Copyright (C) 2010 Adam Pigg <adam@piggz.co.uk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef KEXIFIELDLISTMODELITEM_H
#define KEXIFIELDLISTMODELITEM_H

#include <QString>
#include <QIcon>

class KexiFieldListModelItem
{

public:
    KexiFieldListModelItem(const QString &fname, const QString &dtype, bool pkey);
    virtual ~KexiFieldListModelItem();
    QVariant data(int column) const;
    QIcon icon() const;
    QString caption() const;
    void setCaption(const QString &caption);
    Qt::ItemFlags flags() const;

private:
    class Private;
    Private * const d;
};

#endif // KEXIFIELDLISTMODELITEM_H
