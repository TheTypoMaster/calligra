/* This file is part of the KDE project
   Copyright (C) 2002 Peter Simonsson <psn@linux.se>
   Copyright (C) 2004, 2006 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _KEXIBLOBTABLEEDIT_H_
#define _KEXIBLOBTABLEEDIT_H_

#include "kexitableedit.h"
#include "kexicelleditorfactory.h"

class QTemporaryFile;
class QUrl;

class KexiBlobTableEdit : public KexiTableEdit
{
    Q_OBJECT
public:
    explicit KexiBlobTableEdit(KDbTableViewColumn &column, QWidget *parent = 0);
    virtual ~KexiBlobTableEdit();

    bool valueIsNull();
    bool valueIsEmpty();

    virtual QVariant value();

    virtual bool cursorAtStart();
    virtual bool cursorAtEnd();

    /*! Reimplemented: resizes a view(). */
    virtual void resize(int w, int h);

    virtual void showFocus(const QRect& r, bool readOnly);

    virtual void hideFocus();

    /*! \return total size of this editor, including popup button. */
    virtual QSize totalSize() const;

    virtual void paintFocusBorders(QPainter *p, QVariant &, int x, int y, int w, int h);

    /*! Reimplemented to handle the key events. */
    virtual bool handleKeyPress(QKeyEvent* ke, bool editorActive);

    /*! Handles double click request coming from the table view.
     \return true if it has been consumed.
     Reimplemented in KexiBlobTableEdit (to execute "insert file" action. */
    virtual bool handleDoubleClick();

    /*! Handles action having standard name \a actionName.
     Action could be: "edit_cut", "edit_paste", etc. */
    virtual void handleAction(const QString& actionName);

    /*! Handles copy action for value. The \a value is copied to clipboard in format appropriate
     for the editor's impementation, e.g. for image cell it can be a pixmap.
     \a visibleValue is unused here. Reimplemented after KexiTableEdit. */
    virtual void handleCopyAction(const QVariant& value, const QVariant& visibleValue);

    virtual void setupContents(QPainter *p, bool focused, const QVariant& val,
                               QString &txt, int &align, int &x, int &y_offset, int &w, int &h);

protected Q_SLOTS:
    void slotUpdateActionsAvailabilityRequested(bool *valueIsNull, bool *valueIsReadOnly);

    void handleInsertFromFileAction(const QUrl &url);
    void handleAboutToSaveAsAction(QString *origFilename, QString *mimeType, bool *dataIsEmpty);
    void handleSaveAsAction(const QString& fileName);
    void handleCutAction();
    void handleCopyAction();
    void handlePasteAction();
    virtual void clear();
    void handleShowPropertiesAction();

protected:
    //! initializes this editor with \a add value
    virtual void setValueInternal(const QVariant& add, bool removeOld);

    //! @todo QString openWithDlg(const QString& file);
    //! @todo void execute(const QString& app, const QString& file);

    //! @todo QString openWithDlg(const QString& file);

    //! @todo void execute(const QString& app, const QString& file);

    //! @internal
    void updateFocus(const QRect& r);

    void signalEditRequested();

    //! @internal
    void executeCopyAction(const QByteArray& data);

    virtual bool eventFilter(QObject *o, QEvent *e);

    class Private;
    Private * const d;
    //! @todo  QTemporaryFile* m_tempFile;
    //! @todo  KProcess* m_proc;
    //! @todo  QTextEdit *m_content;
};

KEXI_DECLARE_CELLEDITOR_FACTORY_ITEM(KexiBlobEditorFactoryItem)


//=======================
//This class is temporarily here:

/*! @short Cell editor for displaying kde icon (using icon name provided as string).
 Read only.
*/
class KexiKIconTableEdit : public KexiTableEdit
{
public:
    explicit KexiKIconTableEdit(KDbTableViewColumn &column, QWidget *parent = 0);

    virtual ~KexiKIconTableEdit();

    //! \return true if editor's value is null (not empty)
    virtual bool valueIsNull();

    //! \return true if editor's value is empty (not null).
    //! Only few field types can accept "EMPTY" property
    //! (check this with KDbField::hasEmptyProperty()),
    virtual bool valueIsEmpty();

    virtual QVariant value();

    virtual bool cursorAtStart();
    virtual bool cursorAtEnd();

    virtual void clear();

    virtual void setupContents(QPainter *p, bool focused, const QVariant& val,
                               QString &txt, int &align, int &x, int &y_offset, int &w, int &h);

    /*! Handles copy action for value. Does nothing.
     \a visibleValue is unused here. Reimplemented after KexiTableEdit. */
    virtual void handleCopyAction(const QVariant& value, const QVariant& visibleValue);

protected:
    //! initializes this editor with \a add value
    virtual void setValueInternal(const QVariant& add, bool removeOld);

    void showHintButton();
    void init();

    class Private;
    Private * const d;
};

KEXI_DECLARE_CELLEDITOR_FACTORY_ITEM(KexiKIconTableEditorFactoryItem)

#endif
