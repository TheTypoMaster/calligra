/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KFORMDESIGNEROBJECTTREE_H
#define KFORMDESIGNEROBJECTTREE_H

#include <qobject.h>
#include <qptrlist.h>
#include <qmap.h>
#include <qvariant.h>
#include <qdict.h>

namespace KFormDesigner {


class ObjectTreeItem;
typedef QPtrList<ObjectTreeItem> ObjectTreeC;
typedef QDict<ObjectTreeItem> TreeDict;
typedef QMap<QString, QVariant> PropertyMap;
typedef QMap<QString, int> Names;

/**
 *
 * @author Lucijan Busch <lucijan@kde.org>
 * this class holds the classanme, properties and children
 **/
class KFORMEDITOR_EXPORT ObjectTreeItem
{
	public:
		ObjectTreeItem(const QString &className, const QString &name);
		virtual ~ObjectTreeItem();

		ObjectTreeC	children() { return m_children; }
		PropertyMap	properties() { return m_properties; }
		virtual bool	rename(const QString &name);

		QString		name() const { return m_name; }

	private:
		QString		m_className;
		QString		m_name;
		ObjectTreeC	m_children;
		PropertyMap	m_properties;
};

class KFORMEDITOR_EXPORT ObjectTree : public ObjectTreeItem
{
	public:
		ObjectTree(const QString &className=QString::null, const QString &name=QString::null);
		virtual ~ObjectTree();

		virtual bool	rename(const QString &name);

		ObjectTreeItem	*lookup(const QString &name);
		TreeDict	*dict() { return &m_treeDict; }

		void		addChild(ObjectTreeItem *parent, ObjectTreeItem *c);
		void		addChild(ObjectTreeItem *c);

		QString		genName(const QString &base);

	private:
		TreeDict	m_treeDict;
		Names		m_names;
};

}

#endif
