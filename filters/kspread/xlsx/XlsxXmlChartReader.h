/*
 * This file is part of Office 2007 Filters for KOffice
 *
 * Copyright (C) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Suresh Chande suresh.chande@nokia.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef XLSXXMLCHARTREADER_H
#define XLSXXMLCHARTREADER_H

//#include <KoGenStyle.h>
//#include <styles/KoCharacterStyle.h>
//#include <KoUnit.h>
//#include <KoXmlWriter.h>
//#include <KoGenStyles.h>
//#include <KoOdfNumberStyles.h>
//#include <KoOdfGraphicStyles.h>
//#include <styles/KoCharacterStyle.h>

#include <MsooXmlCommonReader.h>
//#include <MsooXmlThemesReader.h>
//#include "XlsxSharedString.h"
//#include <kspread/Util.h>
//#include <math.h>
//#include <QBrush>
//#include <QRegExp>
//#include "NumberFormatParser.h"

class XlsxXmlChartReaderContext : public MSOOXML::MsooXmlReaderContext
{
public:
    XlsxXmlChartReaderContext();
    virtual ~XlsxXmlChartReaderContext();
};

class XlsxXmlChartReader : public MSOOXML::MsooXmlCommonReader
{
public:
    XlsxXmlChartReader(KoOdfWriters *writers);
    virtual ~XlsxXmlChartReader();
    virtual KoFilter::ConversionStatus read(MSOOXML::MsooXmlReaderContext* context = 0);
protected:
    KoFilter::ConversionStatus read_plotArea();
    KoFilter::ConversionStatus read_ser();
    KoFilter::ConversionStatus read_legend();
    
private:
    XlsxXmlChartReaderContext *m_context;
};

#endif
