/* This file is part of the KDE project
   Copyright (C) 2011, 2012 Pankaj Kumar <me@panks.in>
   
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
   Boston, MA 02110-1301, USA.
 */

#ifndef PDFIMPORT_H
#define PDFIMPORT_H

#include <KoFilter.h>

#include <QVariantList>

class QByteArray;
class QTextStream;

class KoGenStyles;
class KoOdfWriteStore;
class KoXmlWriter;

class PdfImport : public KoFilter
{
    Q_OBJECT

public:
    PdfImport(QObject *parent, const QVariantList &);
    virtual ~PdfImport();

    virtual KoFilter::ConversionStatus convert(const QByteArray& from, const QByteArray& to);

private:
    void convertAsIs(QTextStream &stream, KoXmlWriter *bodyWriter, const QString &styleName);
    void convertSentence(QTextStream &stream, KoXmlWriter *bodyWriter, const QString &styleName);
    void convertEmptyLine(QTextStream &stream, KoXmlWriter *bodyWriter, const QString &styleName);

    bool createMeta(KoOdfWriteStore &store);
};

#endif /* PDFIMPORT_H */
