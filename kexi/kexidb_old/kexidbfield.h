/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>
   Joseph Wenninger <jowenn@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
 */

#ifndef KEXIDBFIELD_H
#define KEXIDBFIELD_H

#include <qvariant.h>
#include <qstring.h>

enum ColumnType
{
	SQLInvalid,
	SQLBigInt,
	SQLBinary,
	SQLBit,
	SQlDate,
	SQLDecimal,
	SQLDouble,
	SQLFloat,
	SQLInteger,
	SQLLongVarBinary,
	SQLLongVarChar,
	SQLNumeric,
	SQLSmallInt,
	SQLTime,
	SQLTimeStamp,
	SQLTinyInt,
	SQLVarBinary,
	SQLVarchar
};

enum Keys
{
	NoKey,
	PrimaryKey,
	UniqueKey,
	ForignKey
};

class KexiDBField
{

	public:
		KexiDBField(QString table, QString filed, int field);
		~KexiDBField();

		virtual QString		name() const;
		
		//key section
		virtual Keys		keys();
		virtual	bool		primary_key();
		virtual bool		unique_key();
		virtual bool		forign_key();
		
		virtual QVariant::Type	qtType();
		virtual ColumnType	sqlType();
};

#endif
