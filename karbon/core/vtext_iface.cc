/* This file is part of the KDE project
   Copyright (C) 2002, The Karbon Developers

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
   Boston, MA 0214111-1307, USA.
*/

#include "vtext_iface.h"
#include "vtext.h"

VTextIface::VTextIface( VText *text )
	: VObjectIface( text ), m_text( text )
{
}

void 
VTextIface::setText( QString text )
{
	m_text->setText( text );
}

QString
VTextIface::text()
{
	return m_text->text();
}

