/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#ifndef __ko_app_h__
#define __ko_app_h__

#include <opApplication.h>
#include <kded_instance.h>
#include <kstartparams.h>

class KoMainWindow;

inline QString colorToName( const QColor &c ) {
    QString r;
    r.sprintf( "#%02X%02X%02X", c.red(), c.green(), c.blue() );
    return r;
}

class KoApplication : public OPApplication
{
    Q_OBJECT

public:

    KoApplication( int &argc, char **argv, const QString& rAppName = 0);
    virtual ~KoApplication();

    virtual KoMainWindow* createNewShell() { return 0; };
    virtual void start();
	
    void aboutKDE() { KApplication::aboutKDE(); }

private:
    KdedInstance kded;
    KStartParams m_params;
    bool m_bWithGUI;
};

#endif
