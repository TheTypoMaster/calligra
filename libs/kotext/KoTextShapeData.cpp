/* This file is part of the KDE project
 * Copyright (C) 2006 Thomas Zander <zander@kde.org>
 * Copyright (C) 2008 Thorsten Zachmann <zachmann@kde.org>
 * Copyright (C) 2008 Girish Ramakrishnan <girish@forwardbias.in>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KoTextShapeData.h"
#include <KoXmlWriter.h>

#include <KDebug>
#include <QUrl>
#include <QTextDocument>
#include <QTextBlock>

#include <KoGenStyle.h>
#include <KoGenStyles.h>
#include <KoShapeLoadingContext.h>
#include <KoOdfLoadingContext.h>
#include <KoShapeSavingContext.h>

#include "KoInlineObject.h"
#include "KoInlineTextObjectManager.h"
#include "styles/KoStyleManager.h"
#include "styles/KoCharacterStyle.h"
#include "styles/KoParagraphStyle.h"
#include "styles/KoListStyle.h"
#include "styles/KoListLevelProperties.h"
#include "KoTextDocumentLayout.h"

#include "opendocument/KoTextLoader.h"

class KoTextShapeData::Private {
public:
    Private()
        : ownsDocument(true),
        dirty(true),
        offset(0.0),
        position(-1),
        endPosition(-1),
        pageNumber(-1),
        direction(KoText::AutoDirection)
    {
    }
    QTextDocument *document;
    bool ownsDocument, dirty;
    double offset;
    int position, endPosition, pageNumber;
    KoInsets margins;
    KoText::Direction direction;
};


KoTextShapeData::KoTextShapeData()
: d(new Private())
{
    d->document = new QTextDocument();
    d->document->setUseDesignMetrics(true);
    d->document->setDefaultFont(QFont("Sans Serif", 12, QFont::Normal, false));
}

KoTextShapeData::~KoTextShapeData() {
    if(d->ownsDocument)
        delete d->document;
    delete d;
}

void KoTextShapeData::setDocument(QTextDocument *document, bool transferOwnership) {
    Q_ASSERT(document);
    if(d->ownsDocument && document != d->document)
        delete d->document;
    d->document = document;
    // The following avoids the normal case where the glyph metrices are rounded to integers and
    // hinted to the screen by freetype, which you of course don't want for WYSIWYG
    if(! d->document->useDesignMetrics())
        d->document->setUseDesignMetrics(true);
    d->ownsDocument = transferOwnership;
}

QTextDocument *KoTextShapeData::document() {
    return d->document;
}

double KoTextShapeData::documentOffset() const {
    return d->offset;
}

void KoTextShapeData::setDocumentOffset(double offset) {
    d->offset = offset;
}

int KoTextShapeData::position() const {
    return d->position;
}

void KoTextShapeData::setPosition(int position) {
    d->position = position;
}

int KoTextShapeData::endPosition() const {
    return d->endPosition;
}

void KoTextShapeData::setEndPosition(int position) {
    d->endPosition = position;
}

void KoTextShapeData::foul() {
    d->dirty = true;
}

void KoTextShapeData::wipe() {
    d->dirty = false;
}

bool KoTextShapeData::isDirty() const {
    return d->dirty;
}

void KoTextShapeData::fireResizeEvent() {
    emit relayout();
}

void KoTextShapeData::setShapeMargins(const KoInsets &margins) {
    d->margins = margins;
}

KoInsets KoTextShapeData::shapeMargins() const {
    return d->margins;
}

void KoTextShapeData::setPageNumber(int page) {
    if (page == d->pageNumber)
        return;
    d->pageNumber = page;
    d->dirty = true;
}

int KoTextShapeData::pageNumber() const {
    return d->pageNumber;
}

void KoTextShapeData::setPageDirection(KoText::Direction direction) {
    d->direction = direction;
}

KoText::Direction KoTextShapeData::pageDirection() const {
    return d->direction;
}

bool KoTextShapeData::loadOdf(const KoXmlElement & element, KoShapeLoadingContext & context) {
    KoTextLoader loader( context );

    QTextCursor cursor( document() );
    document()->clear();
    loader.loadBody( element, cursor ); // now let's load the body from the ODF KoXmlElement.

    return true;
}

void KoTextShapeData::saveOdf(KoShapeSavingContext & context, int from, int to) const {
    KoXmlWriter *writer = &context.xmlWriter();
    QTextBlock block = d->document->findBlock(from);
    kDebug(32500) << "The document is " << d->document;

    KoTextDocumentLayout *layout = dynamic_cast<KoTextDocumentLayout*>(d->document->documentLayout());
    Q_ASSERT(layout);
    Q_ASSERT(layout->inlineObjectTextManager());

    QHash<int, QString> styleNames; // Store an int index for a QTextFormat => ODF style name
    QVector<QTextFormat> allFormats = d->document->allFormats(); // Will be used to get the indexes
    QTextFormat firstFragmentFormat;
    QList<QTextList*> textLists;	// Store the current lists being stored.

    if (KoStyleManager *styleManager = layout->styleManager()) {
        foreach (QTextFormat textFormat, allFormats) { // iterate over the QTextFormat contained in this textFrameSet
            QString displayName, internalName, generatedName;
            switch( textFormat.type() ) {
                case QTextFormat::BlockFormat: { // This is a QTextBlockFormat.
                    KoParagraphStyle *originalParagraphStyle = styleManager->paragraphStyle(textFormat.intProperty(KoParagraphStyle::StyleId));
                    // we'll convert it to a KoParagraphStyle to check for local changes.
                    KoParagraphStyle paragStyle(textFormat);
                    if (originalParagraphStyle) {
                        displayName = originalParagraphStyle->name();
                        internalName = QString(QUrl::toPercentEncoding(displayName, "", " ")).replace("%", "_");
                    }
                    QString generatedName;
                    if (originalParagraphStyle && (paragStyle == (*originalParagraphStyle))) { // This is the real, unmodified character style.
                        KoGenStyle style(KoGenStyle::StyleUser, "paragraph");
                        originalParagraphStyle->saveOdf(style);
                        generatedName = context.mainStyles().lookup(style, internalName, KoGenStyles::DontForceNumbering);
                    } else { // There are manual changes... We'll have to store them then
                        KoGenStyle style(KoGenStyle::StyleAuto, "paragraph", internalName);
                        if ( context.isSet( KoShapeSavingContext::AutoStyleInStyleXml ) )
                            style.setAutoStyleInStylesDotXml( true );
                        if (originalParagraphStyle)
                            paragStyle.removeDuplicates(*originalParagraphStyle);
                        paragStyle.saveOdf(style);
                        generatedName = context.mainStyles().lookup(style, "P");
                    }
                    styleNames[allFormats.indexOf(textFormat)] = generatedName;
                } 
                break;

                case QTextFormat::CharFormat: { // This is a QTextCharFormat.
                    if (textFormat.objectType() == 1001)
                        continue;
                    KoCharacterStyle *originalCharStyle = styleManager->characterStyle(textFormat.intProperty(KoCharacterStyle::StyleId));
                    if (originalCharStyle) {
                        displayName = originalCharStyle->name();
                        internalName = QString(QUrl::toPercentEncoding(displayName, "", " ")).replace("%", "_");
                    }
                    // we'll convert it to a KoCharacterStyle to check for local changes.
                    KoCharacterStyle charStyle(textFormat.toCharFormat());
                    if (originalCharStyle && (charStyle == (*originalCharStyle))) { // This is the real, unmodified character style.
                        KoGenStyle style(KoGenStyle::StyleUser, "text");
                        originalCharStyle->saveOdf(style);
                        generatedName = context.mainStyles().lookup(style, internalName, KoGenStyles::DontForceNumbering);
                        if (styleManager->defaultParagraphStyle()) { // Check whether it is the default char format...
                            KoParagraphStyle *defaultParagraph = styleManager->defaultParagraphStyle();
                            if ((*(defaultParagraph->characterStyle())) == charStyle)
                                firstFragmentFormat = textFormat;
                        }
                    } else { // There are manual changes... We'll have to store them then
                        KoGenStyle style(KoGenStyle::StyleAuto, "text", internalName);
                        if ( context.isSet( KoShapeSavingContext::AutoStyleInStyleXml ) )
                            style.setAutoStyleInStylesDotXml( true );
                        if (originalCharStyle)
                            charStyle.removeDuplicates(*originalCharStyle);
                        charStyle.saveOdf(style);
                        generatedName = context.mainStyles().lookup(style, "T");
                    }
                    styleNames[allFormats.indexOf(textFormat)] = generatedName;
                } 
                break;

                default: {
                    // It doesn't matter yet.
                }
                break;
            }
        }
    }
    
    //TODO: The list formats are currently not stored in the KoStyleManager ??
    QMap<QTextList *, QString> listStyleNames;
    QTextBlock startBlock = block;
    while(block.isValid() && ((to == -1) || (block.position() < to))) {
	if ((block.textList()) and (!listStyleNames.contains(block.textList()))) {
	    // Generate a style from that...
	    KoGenStyle style(KoGenStyle::StyleList);
            KoListStyle *listStyle = KoListStyle::fromTextList(block.textList());
	    listStyle->saveOdf(style);
            QString generatedName = context.mainStyles().lookup(style, listStyle->name());
	    listStyleNames[block.textList()] = generatedName;
	    delete(listStyle);
	}
	block = block.next();
    }
    block = startBlock;
    
    // Ok, now that the styles are done, we can store the blocks themselves.
    while(block.isValid() && ((to == -1) || (block.position() < to))) {
        if ((block.begin().atEnd()) && (!block.next().isValid()))   // Do not add an extra empty line at the end...
            break;

        KoParagraphStyle *paragstyle = KoParagraphStyle::fromBlock(block);
	if (block.textList()) {
	    kDebug() << "Ok, we've got a list for the block with text " << block.begin().fragment().text();
	    kDebug() << "The list :" << block.textList();
	    if ((textLists.isEmpty()) or (!textLists.contains(block.textList()))) {
		kDebug() << "This is a text list we never met before, adding it.";
		writer->startElement( "text:list", false );
		writer->addAttribute("text:style-name", listStyleNames[block.textList()]);
		textLists.append(block.textList());
	    } else if (block.textList() != textLists.last()) {
		kDebug() << "We will close every text:list element until we reach the right list...";
		while ((!textLists.isEmpty()) and (block.textList() != textLists.last())) {
		    textLists.removeLast();
		    writer->endElement();
		}
	    }
	    writer->startElement( "text:list-item", false );
	} else {
	    kDebug() << "We close any remaining list...";
	    // Close any remaining list...
	    while (!textLists.isEmpty()) {
		textLists.removeLast();
		writer->endElement();
	    }
	}
        writer->startElement( "text:p", false );

        if (styleNames.contains(allFormats.indexOf(block.blockFormat())))
            writer->addAttribute("text:style-name", styleNames[allFormats.indexOf(block.blockFormat())]);

        QTextBlock::iterator it;
        for (it = block.begin(); !(it.atEnd()); ++it) {
            QTextFragment currentFragment = it.fragment();
            if (currentFragment.isValid()) {
                QTextFormat charFormat = currentFragment.charFormat();
                KoInlineObject *inlineObject = layout->inlineObjectTextManager()->inlineTextObject((const QTextCharFormat &)charFormat);
                if (inlineObject) {
                    inlineObject->saveOdf(context);
                } else {
                    if (currentFragment.charFormat() != firstFragmentFormat) {
                        writer->startElement( "text:span", false );
                        if (styleNames.contains(allFormats.indexOf(charFormat)))
                            writer->addAttribute("text:style-name", styleNames[allFormats.indexOf(charFormat)]);
                    }
                    writer->addTextSpan( currentFragment.text() );
                    if (currentFragment.charFormat() != firstFragmentFormat)
                        writer->endElement( );
                }
            }
        }
        writer->endElement();
	if (block.textList())	// This block is a list item too...
	    writer->endElement();
        block = block.next();
    }
}

#include "KoTextShapeData.moc"
