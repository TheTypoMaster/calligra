/* This file is part of the KDE project
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __KWordSerialLetterDatabaseIface_h__
#define  __KWordSerialLetterDatabaseIface_h__

#include <dcopobject.h>
#include <qstringlist.h>
/**
 * DCOP interface for the SerialLetterDataBase class
 */
class KWordSerialLetterDatabaseIface : public DCOPObject
{
    K_DCOP
public:
    KWordSerialLetterDatabaseIface(const QCString &name);
    ~KWordSerialLetterDatabaseIface();
k_dcop:
    virtual void refresh(bool)=0; //will not be blocked when the configuration dialog is open

    virtual QStringList availablePlugins()=0; //will not be blocked when configuration dialog is open
    virtual bool isConfigDialogShown()=0; // will not be blocked when the configuration dialog is open
    
    /* first parameter plugin name
       second parameter:
       		open	:shows the open dialog if possible
		create	:shows the create dialog if possible
		silent	:doesn't do any further actions than loading
	This call will return false, when the plugin couldn't be loaded,
	the user rejected to change the datasource or the configuration dialog is open
    */
    virtual bool loadPlugin(QString name,QString command)=0;
};

#endif

