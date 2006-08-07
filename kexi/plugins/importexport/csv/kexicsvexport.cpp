/* This file is part of the KDE project
   Copyright (C) 2005,2006 Jaroslaw Staniek <js@iidea.pl>

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

#include "kexicsvexport.h"
#include "kexicsvwidgets.h"
#include <main/startup/KexiStartupFileDialog.h>
#include <kexidb/cursor.h>
#include <kexidb/utils.h>
#include <core/keximainwindow.h>
#include <core/kexiproject.h>
#include <core/kexipartinfo.h>
#include <core/kexipartmanager.h>
#include <core/kexiguimsghandler.h>
#include <kexiutils/utils.h>
#include <widget/kexicharencodingcombobox.h>

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qclipboard.h>
#include <kapplication.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kactivelabel.h>
#include <kpushbutton.h>
#include <kapplication.h>
#include <kdebug.h>
#include <ksavefile.h>


using namespace KexiCSVExport;

Options::Options()
 : mode(File), itemId(0), addColumnNames(true)
{
}

bool Options::assign( QMap<QString,QString>& args )
{
	mode = (args["destinationType"]=="file")
		? KexiCSVExport::File : KexiCSVExport::Clipboard;

	if (args.contains("delimiter"))
		delimiter = args["delimiter"];
	else
		delimiter = (mode==File) ? KEXICSV_DEFAULT_FILE_DELIMITER : KEXICSV_DEFAULT_CLIPBOARD_DELIMITER;

	if (args.contains("textQuote"))
		textQuote = args["textQuote"];
	else
		textQuote = (mode==File) ? KEXICSV_DEFAULT_FILE_TEXT_QUOTE : KEXICSV_DEFAULT_CLIPBOARD_TEXT_QUOTE;

	bool ok;
	itemId = args["itemId"].toInt(&ok);
	if (!ok || itemId<=0)
		return false;
	if (args.contains("forceDelimiter"))
		forceDelimiter = args["forceDelimiter"];
	if (args.contains("addColumnNames"))
		addColumnNames = (args["addColumnNames"]=="1");
	return true;
}

//------------------------------------

bool KexiCSVExport::exportData(KexiDB::TableOrQuerySchema& tableOrQuery, 
	const Options& options, int rowCount, QTextStream *predefinedTextStream)
{
	KexiDB::Connection* conn = tableOrQuery.connection();
	if (!conn)
		return false;

	if (rowCount == -1)
		rowCount = KexiDB::rowCount(tableOrQuery);
	if (rowCount == -1)
		return false;

//! @todo move this to non-GUI location so it can be also used via command line
//! @todo add a "finish" page with a progressbar.
//! @todo look at rowCount whether the data is really large; 
//!       if so: avoid copying to clipboard (or ask user) because of system memory

//! @todo OPTIMIZATION: use fieldsExpanded(true /*UNIQUE*/)
//! @todo OPTIMIZATION? (avoid multiple data retrieving) look for already fetched data within KexiProject..

	KexiDB::QuerySchema* query = tableOrQuery.query();
	if (!query)
		query = tableOrQuery.table()->query();

	KexiDB::QueryColumnInfo::Vector fields( query->fieldsExpanded() );
	QString buffer;

	KSaveFile *kSaveFile = 0;
	QTextStream *stream = 0;

	const bool copyToClipboard = options.mode==Clipboard;
	if (copyToClipboard) {
//! @todo (during exporting): enlarge bufSize by factor of 2 when it became too small
		uint bufSize = QMIN((rowCount<0 ? 10 : rowCount) * fields.count() * 20, 128000);
		buffer.reserve( bufSize );
		if (buffer.capacity() < bufSize) {
			kdWarning() << "KexiCSVExportWizard::exportData() cannot allocate memory for " << bufSize 
				<< " characters" << endl;
			return false;
		}
	}
	else {
		if (predefinedTextStream) {
			stream = predefinedTextStream;
		}
		else {
			if (options.fileName.isEmpty()) {//sanity
				kdWarning() << "KexiCSVExportWizard::exportData(): fname is empty" << endl;
				return false;
			}
			kSaveFile = new KSaveFile(options.fileName);
			if (0 == kSaveFile->status())
				stream = kSaveFile->textStream();
			if (0 != kSaveFile->status() || !stream) {//sanity
				kdWarning() << "KexiCSVExportWizard::exportData(): status != 0 or stream == 0" << endl;
				delete kSaveFile;
				return false;
			}
		}
	}

//! @todo escape strings

#define _ERR \
	delete [] isText; \
	if (kSaveFile) { kSaveFile->abort(); delete kSaveFile; } \
	return false

#define APPEND(what) \
		if (copyToClipboard) buffer.append(what); else (*stream) << (what)

// line endings should be as in RFC 4180
#define CSV_EOLN "\r\n"

	// 0. Cache information
	const uint fieldsCount = fields.count();
	const QCString delimiter( options.delimiter.left(1).latin1() );
	const bool hasTextQuote = !options.textQuote.isEmpty();
	const QCString textQuote( options.textQuote.left(1).latin1() );
	const QCString escapedTextQuote( textQuote + textQuote ); //ok?
	//cache for faster checks
	bool *isText = new bool[fieldsCount]; 
	bool *isDateTime = new bool[fieldsCount]; 
//	bool isInteger[fieldsCount]; //cache for faster checks
//	bool isFloatingPoint[fieldsCount]; //cache for faster checks
	for (uint i=0; i<fieldsCount; i++) {
		isText[i] = fields[i]->field->isTextType();
		isDateTime[i] = fields[i]->field->type()==KexiDB::Field::DateTime;
//		isInteger[i] = fields[i]->field->isIntegerType() 
//			|| fields[i]->field->type()==KexiDB::Field::Boolean;
//		isFloatingPoint[i] = fields[i]->field->isFPNumericType();
	}

	// 1. Output column names
	if (options.addColumnNames) {
		for (uint i=0; i<fieldsCount; i++) {
			if (i>0)
				APPEND( delimiter );
			if (hasTextQuote){
				APPEND( textQuote );
				APPEND( fields[i]->captionOrAliasOrName().replace(textQuote, escapedTextQuote) );
				APPEND( textQuote );
			}
			else {
				APPEND( fields[i]->captionOrAliasOrName() );
			}
		}
		APPEND(CSV_EOLN);
	}
	
	KexiGUIMessageHandler handler;
	KexiDB::Cursor *cursor = conn->executeQuery(*query);
	if (!cursor) {
		handler.showErrorMessage(conn);
		_ERR;
	}
	for (cursor->moveFirst(); !cursor->eof() && !cursor->error(); cursor->moveNext()) {
		const uint realFieldCount = QMIN(cursor->fieldCount(), fieldsCount);
		for (uint i=0; i<realFieldCount; i++) {
			if (i>0)
				APPEND( delimiter );
			if (cursor->value(i).isNull())
				continue;
			if (hasTextQuote && isText[i]) {
				APPEND( textQuote );
				APPEND( QString(cursor->value(i).toString()).replace(textQuote, escapedTextQuote) );
				APPEND( textQuote );
			}
			else if (isDateTime[i]) { //avoid "T" in ISO DateTime
				APPEND( cursor->value(i).toDateTime().date().toString(Qt::ISODate)+" "
					+ cursor->value(i).toDateTime().time().toString(Qt::ISODate) );
			}
			else {
				APPEND( cursor->value(i).toString() );
			}
		}
		APPEND(CSV_EOLN);
	}

	if (copyToClipboard)
		buffer.squeeze();

	if (!conn->deleteCursor(cursor)) {
		handler.showErrorMessage(conn);
		_ERR;
	}

	if (copyToClipboard)
		kapp->clipboard()->setText(buffer, QClipboard::Clipboard);

	delete [] isText;

	if (kSaveFile) {
		if (!kSaveFile->close()) {
			kdWarning() << "KexiCSVExportWizard::exportData(): error close(); status == " 
				<< kSaveFile->status() << endl;
		}
		delete kSaveFile;
	}
	return true;
}
