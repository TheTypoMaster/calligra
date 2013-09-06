/* This file is part of the KDE project
 * Copyright (C)  2006, 2010 Thomas Zander <zander@kde.org>
 * Copyright (C)  2008 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C)  2011 Pierre Ducroquet <pinaraf@pinaraf.info>
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
#ifndef KOTEXT_H
#define KOTEXT_H

#include "kotext_export.h"

#include <KoCanvasResourceManager.h>
#include <KoDocumentResourceManager.h>

#include <QStringList>
#include <QChar>
#include <QTextCharFormat>
#include <QMetaType>
#include <QTextOption>
#include <styles/KoCharacterStyle.h>


/**
 * Generic namespace of the Calligra Text library for helper methods and data.
 */
namespace KoText
{
KOTEXT_EXPORT QStringList underlineTypeList();
KOTEXT_EXPORT QStringList underlineStyleList();

/// This enum contains values to be used as keys in KoCanvasResourceManager
enum CanvasResource {
    CurrentTextDocument = 382490375, ///< set by the text plugin whenever the document is changed
    CurrentTextPosition = 183523,   ///<  used by the text plugin whenever the position is changed
    CurrentTextAnchor = 341899485,   ///<  used by the text plugin whenever the anchor-position is changed
    SelectedTextPosition = 21314576,   ///<  used by the text plugin whenever the alternative selection is changed
    ///  used by the text plugin whenever the alternative selection anchor-position is changed
    SelectedTextAnchor = 3344189
};

/**
 * Text resources per calligra-document.
 * \sa KoDocumentResourceManager KoShapeController::resourceManager()
 */
enum DocumentResource {
    ChangeTracker = KoDocumentResourceManager::KoTextStart + 1, ///< KoChangeTracker
    InlineTextObjectManager, ///< The KoText inline-text-object manager. KoInlineTextObjectManager
    TextRangeManager, ///< The KoText inline-text-object manager. KoInlineTextObjectManager
    StyleManager,           ///< The KoStyleManager
    PageProvider,            ///< The KoPageProvider
    /** The KoDocumentRdf for the document,
        this will be a KoDocumentRdfBase when Soprano support is not compiled in. */
    DocumentRdf

};

enum KoTextFrameProperty {
    SubFrameType = QTextFormat::UserProperty + 1
};

enum KoSubFrameType {
    AuxillaryFrameType = 1,
    NoteFrameType
};

/// Text in the objects will be positioned according to the direction.
enum Direction {
    AutoDirection,      ///< Take the direction from the text.
    LeftRightTopBottom, ///< Text layout for most western languages
    RightLeftTopBottom, ///< Text layout for languages like Hebrew
    TopBottomRightLeft,  ///< Vertical text layout.
    TopBottomLeftRight,  ///< Vertical text layout. ?
    InheritDirection    ///< Direction is unspecified and should come from the container
};

}

Q_DECLARE_METATYPE(KoText::Tab)

#endif
