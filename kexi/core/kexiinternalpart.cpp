/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>

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

#include "kexiinternalpart.h"

#include <kexidialogbase.h>

#include <qasciidict.h>

#include <kdebug.h>
#include <klibloader.h>
#include <ktrader.h>
#include <kparts/componentfactory.h>

class KexiInternalPartManager
{
	public:
		KexiInternalPartManager()
		{
			m_parts.setAutoDelete(false);
		}
		
		KexiInternalPart* findPart(const char* partName)
		{
			KexiInternalPart *part = m_parts[partName];
			if (!part) {
				QCString fullname = "kexihandler_";
				fullname += partName;
				part = KParts::ComponentFactory::createInstanceFromLibrary<KexiInternalPart>(fullname, 0, fullname);
				m_parts.insert(partName, part);
			}
			return part;
		}
	
	private:
		
		QAsciiDict<KexiInternalPart> m_parts;
};

KexiInternalPartManager internalPartManager;

//----------------------------------------------

KexiInternalPart::KexiInternalPart(QObject *parent, const char *name, const QStringList &)
 : QObject(parent, name)
 , m_uniqueDialog(true)
{
}

KexiInternalPart::~KexiInternalPart()
{
}

//static
const KexiInternalPart *
KexiInternalPart::part(const char* partName)
{
	return internalPartManager.findPart(partName);
}

//static
QWidget* KexiInternalPart::createWidgetInstance(const char* partName, 
 const char* widgetClass, KexiMainWindow* mainWin, QWidget *parent, const char *objName)
{
	KexiInternalPart *part = internalPartManager.findPart(partName);
	if (!part)
		return 0; //fatal!
	return part->createWidget(widgetClass, mainWin, parent, objName ? objName : partName);
}

KexiDialogBase* KexiInternalPart::findOrCreateDialog(KexiMainWindow* mainWin, 
 const char *objName)
{
	if (m_uniqueDialog && !m_dialog.isNull())
		return m_dialog;
	KexiDialogBase *dlg = createDialog(mainWin, objName);
	if (m_uniqueDialog)
		m_dialog = dlg; //recall unique!
	return dlg;
}

//static
KexiDialogBase* KexiInternalPart::createDialogInstance(const char* partName, 
	KexiMainWindow* mainWin, const char *objName)
{
	KexiInternalPart *part = internalPartManager.findPart(partName);
	if (!part) {
		kdDebug() << "KexiInternalPart::createDialogInstance() !part" << endl;
		return 0; //fatal!
	}
	return part->findOrCreateDialog(mainWin, objName ? objName : partName);
}

#include "kexiinternalpart.moc"

