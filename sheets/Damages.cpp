/* This file is part of the KDE project
   Copyright 2006-2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>
   Copyright 2004 Ariya Hidayat <ariya@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; only
   version 2 of the License.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Local
#include "Damages.h"

#include <QPoint>

#include "Cell.h"
#include "Sheet.h"
#include "Region.h"

using namespace Calligra::Sheets;

class Q_DECL_HIDDEN WorkbookDamage::Private
{
public:
    Calligra::Sheets::Map* map;
    Changes changes;
};

class Q_DECL_HIDDEN SheetDamage::Private
{
public:
    Calligra::Sheets::Sheet* sheet;
    Changes changes;
};

class Q_DECL_HIDDEN CellDamage::Private
{
public:
    Calligra::Sheets::Sheet* sheet;
    Region region;
    Changes changes;
};

class Q_DECL_HIDDEN SelectionDamage::Private
{
public:
    Region region;
};

CellDamage::CellDamage(const Calligra::Sheets::Cell& cell, Changes changes)
        : d(new Private)
{
    d->sheet = cell.sheet();
    if (Region::isValid(QPoint(cell.column(), cell.row())))
        d->region = Region(cell.column(), cell.row(), d->sheet);
    d->changes = changes;
}

CellDamage::CellDamage(Calligra::Sheets::Sheet* sheet, const Region& region, Changes changes)
        : d(new Private)
{
    d->sheet = sheet;
    d->region = region;
    d->changes = changes;
}

CellDamage::~CellDamage()
{
    delete d;
}

Sheet* CellDamage::sheet() const
{
    return d->sheet;
}

const Calligra::Sheets::Region& CellDamage::region() const
{
    return d->region;
}

CellDamage::Changes CellDamage::changes() const
{
    return d->changes;
}


SheetDamage::SheetDamage(Calligra::Sheets::Sheet* sheet, Changes changes)
        : d(new Private)
{
    d->sheet = sheet;
    d->changes = changes;
}

SheetDamage::~SheetDamage()
{
    delete d;
}

Sheet* SheetDamage::sheet() const
{
    return d->sheet;
}

SheetDamage::Changes SheetDamage::changes() const
{
    return d->changes;
}


WorkbookDamage::WorkbookDamage(Calligra::Sheets::Map* map, Changes changes)
        : d(new Private)
{
    d->map = map;
    d->changes = changes;
}

WorkbookDamage::~WorkbookDamage()
{
    delete d;
}

Map* WorkbookDamage::map() const
{
    return d->map;
}

WorkbookDamage::Changes WorkbookDamage::changes() const
{
    return d->changes;
}


SelectionDamage::SelectionDamage(const Region& region)
        : d(new Private)
{
    d->region = region;
}

SelectionDamage::~SelectionDamage()
{
    delete d;
}

const Calligra::Sheets::Region& SelectionDamage::region() const
{
    return d->region;
}


/***************************************************************************
  kDebug support
****************************************************************************/

QDebug operator<<(QDebug str, const Calligra::Sheets::Damage& d)
{
    switch (d.type()) {
    case Damage::Nothing:   return str << "NoDamage";
    case Damage::Document:  return str << "Document";
    case Damage::Workbook:  return str << "Workbook";
    case Damage::Sheet:     return str << "Sheet";
    case Damage::Range:     return str << "Range";
    case Damage::Cell:      return str << "Cell";
    case Damage::Selection: return str << "Selection";
    }
    return str;
}

QDebug operator<<(QDebug str, const Calligra::Sheets::CellDamage& d)
{
    str << "CellDamage: " << d.region().name(d.sheet());
    if (d.changes() & CellDamage::Appearance) str << " Appearance";
    if (d.changes() & CellDamage::Binding)    str << " Binding";
    if (d.changes() & CellDamage::Formula)    str << " Formula";
    if (d.changes() & CellDamage::Value)      str << " Value";
    return str;
}

QDebug operator<<(QDebug str, const Calligra::Sheets::SheetDamage& d)
{
    str << "SheetDamage: " << (d.sheet() ? d.sheet()->sheetName() : "NULL POINTER!");
    switch (d.changes()) {
    case SheetDamage::None:               return str << " None";
    case SheetDamage::ContentChanged:     return str << " Content";
    case SheetDamage::PropertiesChanged:  return str << " Properties";
    case SheetDamage::Hidden:             return str << " Hidden";
    case SheetDamage::Shown:              return str << " Shown";
    case SheetDamage::Name:               return str << "Name";
    case SheetDamage::ColumnsChanged:     return str << "Columns";
    case SheetDamage::RowsChanged:        return str << "Rows";
    }
    return str;
}

QDebug operator<<(QDebug str, const Calligra::Sheets::SelectionDamage& d)
{
    str << "SelectionDamage: " << d.region().name();
    return str;
}
