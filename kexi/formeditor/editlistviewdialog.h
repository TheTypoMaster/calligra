/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>

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

#ifndef EDITLISTVIEW_DIALOG_H
#define EDITLISTVIEW_DIALOG_H

#include "kformdesigner_export.h"

#include <QHash>
#include <QToolButton>
#include <KPageDialog>

class QListView;
class QListViewItem;
class QListBoxItem;
class KListViewItem;
class KListView;
class KListBox;
class KPropertySet;
class KProperty;
class KPropertyEditorView;

namespace KFormDesigner
{

//! A dialog to edit the contents of a listview (KListView or QListView)
/*! The dialog contains two pages, one to edit columns and one to edit ist items.
 KPropertyEditorView is used in columns to edit column properties
 (there are two properties not supported by Qt Designer: 'width' and 'resizable').
 The user can enter list contents inside the list
 using KListViewItem::setRenameable(). Pixmaps are not yet supported. */
class KFORMDESIGNER_EXPORT EditListViewDialog : public KPageDialog
{
    Q_OBJECT

public:
    explicit EditListViewDialog(QWidget *parent);
    ~EditListViewDialog() {}

    int exec(QListView *listview);

public Q_SLOTS:
    // Columns page
    void updateItemProperties(QListBoxItem*);
    void newItem();
    void removeItem();
    void MoveItemUp();
    void MoveItemDown();
    void changeProperty(KPropertySet& set, KProperty& property);

    // Contents page
    void updateButtons(QListViewItem*);
    void newRow();
    void newChildRow();
    void removeRow();
    void MoveRowUp();
    void MoveRowDown();

protected:
    /*! Loads all child items of \a item into \a listview (may be different from the \a items 's listview) as child of \a parent item.
    This is used to copy the contents of a listview into another listview. */
    void loadChildNodes(QListView *listview, QListViewItem *item, QListViewItem *parent);

protected:
    enum Buttons { BNewRow = 10, BNewChild, BRemRow, BRowUp, BRowDown , BColAdd = 20, BColRem, BColUp, BColDown };
    KPropertyEditorView  *m_editor;
    KPropertySet  *m_propSet;
    KPageWidgetItem *m_columnsPageItem, *m_contentsPageItem;
    KListBox  *m_listbox;
    KListView  *m_listview;
    QHash<Buttons, QToolButton*>  m_buttons;
};

}

#endif
