/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004-2012 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005 Sebastian Sauer <mail@dipe.org>

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

#include "kexiscriptdesignview.h"
#include "kexiscripteditor.h"
#include <KexiIcon.h>
#include <KexiMainWindowIface.h>

#include <Kross/Manager>
#include <Kross/Action>
#include <Kross/Interpreter>

#include <KDbConnection>

#include <KActionMenu>
#include <KMessageBox>

#include <QSplitter>
#include <QTimer>
#include <QDomDocument>
#include <QTextBrowser>
#include <QDebug>
#include <QFileDialog>

/// @internal
class KexiScriptDesignViewPrivate
{
public:

    QSplitter* splitter;

    /**
     * The \a Kross::Action instance which provides
     * us access to the scripting framework Kross.
     */
    Kross::Action* scriptaction;

    /// The \a KexiScriptEditor to edit the scripting code.
    KexiScriptEditor* editor;

    /// The \a KPropertySet used in the propertyeditor.
    KPropertySet* properties;

    /// Boolean flag to avoid infinite recursion.
    bool updatesProperties;

    /// Used to display statusmessages.
    QTextBrowser* statusbrowser;

    /** The type of script
     *  executable = regular script that can be executed by the user
     *  module = a script which doesn't contain a 'main', only
     *           functions that can be used by other scripts
     *  object = a script which contains code to be loaded into another
                 object such as a report or form
     */
    QString scriptType;
};

KexiScriptDesignView::KexiScriptDesignView(
    QWidget *parent, Kross::Action* scriptaction)
        : KexiView(parent)
        , d(new KexiScriptDesignViewPrivate())
{
    setObjectName("KexiScriptDesignView");
    d->scriptaction = scriptaction;
    d->updatesProperties = false;

    d->splitter = new QSplitter(this);
    d->splitter->setOrientation(Qt::Vertical);

    d->editor = new KexiScriptEditor(d->splitter);
    d->splitter->addWidget(d->editor);
    d->editor->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    d->splitter->setStretchFactor(d->splitter->indexOf(d->editor), 3);
    d->splitter->setFocusProxy(d->editor);
    //addChildView(d->editor);
    setViewWidget(d->splitter);

    d->statusbrowser = new QTextBrowser(d->splitter);
    d->splitter->addWidget(d->statusbrowser);
    d->splitter->setStretchFactor(d->splitter->indexOf(d->statusbrowser), 1);
    d->statusbrowser->setObjectName("ScriptStatusBrowser");
    d->statusbrowser->setReadOnly(true);
    //d->browser->setWordWrap(QTextEdit::WidgetWidth);
    d->statusbrowser->installEventFilter(this);

    /*
    plugSharedAction( "data_execute", this, SLOT(execute()) );
    if(KexiEditor::isAdvancedEditor()) // the configeditor is only in advanced mode avaiable.
        plugSharedAction( "script_config_editor", d->editor, SLOT(slotConfigureEditor()) );
    */

    // setup local actions
    QList<QAction*> viewActions;

    {
        QAction *a = new QAction(koIcon("system-run"), xi18n("Execute"), this);
        a->setObjectName("script_execute");
        a->setToolTip(xi18n("Execute the scripting code"));
        a->setWhatsThis(xi18n("Executes the scripting code."));
        connect(a, SIGNAL(triggered()), this, SLOT(execute()));
        viewActions << a;
    }

    QAction *a = new QAction(this);
    a->setSeparator(true);
    viewActions << a;

    KActionMenu *menu = new KActionMenu(koIcon("document-properties"), xi18n("Edit"), this);
    menu->setObjectName("script_edit_menu");
    menu->setToolTip(xi18n("Edit actions"));
    menu->setWhatsThis(xi18n("Provides Edit menu."));
    menu->setDelayed(false);
    foreach(QAction *a, d->editor->defaultContextMenu()->actions()) {
        menu->addAction(a);
    }
    if (KexiEditor::isAdvancedEditor()) { // the configeditor is only in advanced mode available.
        menu->addSeparator();
        QAction *a = new QAction(koIcon("configure"), xi18n("Configure Editor..."), this);
        a->setObjectName("script_config_editor");
        a->setToolTip(xi18n("Configure the scripting editor"));
        a->setWhatsThis(xi18n("Configures the scripting editor."));
        connect(a, SIGNAL(triggered()), d->editor, SLOT(slotConfigureEditor()));
        menu->addAction(a);
    }
    viewActions << menu;
    setViewActions(viewActions);

    // setup main menu actions
    QList<QAction*> mainMenuActions;
    a = new QAction(koIcon("document-import"), xi18n("&Import..."), this);
    a->setObjectName("script_import");
    a->setToolTip(xi18n("Import script"));
    a->setWhatsThis(xi18n("Imports script from a file."));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotImport()));
    mainMenuActions << a;

    a = new QAction(this);
    a->setSeparator(true);
    mainMenuActions << a;

    //a = new QAction(this);
    //a->setObjectName("project_saveas"); // placeholder for real?
    a = sharedAction("project_saveas");
    mainMenuActions << a;

    a = new QAction(koIcon("document-export"), xi18n("&Export..."), this);
    a->setObjectName("script_export");
    a->setToolTip(xi18n("Export script"));
    a->setWhatsThis(xi18n("Exports script to a file."));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotExport()));
    mainMenuActions << a;

    setMainMenuActions(mainMenuActions);

    loadData();

    d->properties = new KPropertySet(this, "KexiScripting");
    connect(d->properties, SIGNAL(propertyChanged(KPropertySet&,KProperty&)),
            this, SLOT(slotPropertyChanged(KPropertySet&,KProperty&)));

    // To schedule the initialize fixes a crasher in Kate.
    QTimer::singleShot(50, this, SLOT(initialize()));
}

KexiScriptDesignView::~KexiScriptDesignView()
{
    delete d->properties;
    delete d;
}

Kross::Action* KexiScriptDesignView::scriptAction() const
{
    return d->scriptaction;
}

void KexiScriptDesignView::initialize()
{
    setDirty(false);
    updateProperties();
    d->editor->initialize(d->scriptaction);
    connect(d->editor, SIGNAL(textChanged()), this, SLOT(setDirty()));
    d->splitter->setSizes( QList<int>() << height() * 2 / 3 << height() * 1 / 3 );
}

void KexiScriptDesignView::slotImport()
{
    QStringList filters;
    foreach(const QString &interpreter, Kross::Manager::self().interpreters()) {
        filters << Kross::Manager::self().interpreterInfo(interpreter)->mimeTypes();
    }
    //! @todo KEXI3 add equivalent of kfiledialog:///
    //! @todo KEXI3 multiple filters
    // for now support jsut one filter
    QString filterString;
    if (filters.count() == 1) {
        const QMimeDatabase db;
        const QString filterString = db.mimeTypeForName(filters.first()).filterString();
    }
    //QUrl("kfiledialog:///kexiscriptingdesigner"),
    const QUrl result = QFileDialog::getOpenFileUrl(this, xi18nc("@title:window", "Import Script"),
                                                    QUrl(), filterString);
    if (!result.isValid()) {
        return;
    }
    //! @todo support remote files?
    QFile f(result.toLocalFile());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        KMessageBox::sorry(this,
            xi18nc("@info", "Could not read <filename>%1</filename>.", file));
        return;
    }
    d->editor->setText(f.readAll());
    f.close();
}

void KexiScriptDesignView::slotExport()
{
    QStringList filters;
    foreach(const QString &interpreter, Kross::Manager::self().interpreters()) {
        filters << Kross::Manager::self().interpreterInfo(interpreter)->mimeTypes();
    }
    const QString file = KFileDialog::getSaveFileName(
        QUrl("kfiledialog:///kexiscriptingdesigner"),
        filters.join(" "), this, xi18nc("@title:window", "Export Script"));
    if (file.isEmpty())
        return;
    QFile f(file);
    if (! f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        KMessageBox::sorry(this,
            xi18nc("@info", "Could not write <filename>%1</filename>.", file));
        return;
    }
    f.write(d->editor->text().toUtf8());
    f.close();
}

void KexiScriptDesignView::updateProperties()
{
    if (d->updatesProperties)
        return;
    d->updatesProperties = true;

    Kross::Manager* manager = &Kross::Manager::self();

    QString interpretername = d->scriptaction->interpreter();
    Kross::InterpreterInfo* info = interpretername.isEmpty() ? 0 : manager->interpreterInfo(interpretername);

    if (!info) {
        // if interpreter isn't defined or invalid, try to fallback.
        foreach (const QString& interpretername,
            QStringList() << "python" << "ruby" << "qtscript" << "javascript" << "java")
        {
            info = manager->interpreterInfo(interpretername);
            if (info) {
                d->scriptaction->setInterpreter(interpretername);
                break;
            }
        }
    }

    if (!info) {
        d->updatesProperties = false;
        return;
    }

    d->properties->clear();

    QStringList types;
    types << "executable" << "module" << "object";
    KPropertyListData* typelist = new KPropertyListData(types, types);
    KProperty* t = new KProperty(
        "type", // name
        typelist, // ListData
        (d->scriptType.isEmpty() ? "executable" : d->scriptType), // value
        xi18n("Script Type"), // caption
        xi18n("The type of script"), // description
        KProperty::List // type
    );
    d->properties->addProperty(t);

    QStringList interpreters = manager->interpreters();

    qDebug() << interpreters;

    KPropertyListData* proplist = new KPropertyListData(interpreters, interpreters);
    KProperty* prop = new KProperty(
        "language", // name
        proplist, // ListData
        d->scriptaction->interpreter(), // value
        xi18n("Interpreter"), // caption
        xi18n("The used scripting interpreter."), // description
        KProperty::List // type
    );
    d->properties->addProperty(prop);

    Kross::InterpreterInfo::Option::Map options = info->options();
    Kross::InterpreterInfo::Option::Map::ConstIterator it, end(options.constEnd());
    for (it = options.constBegin(); it != end; ++it) {
        Kross::InterpreterInfo::Option* option = it.value();
        KProperty* prop = new KProperty(
            it.key().toLatin1(), // name
            d->scriptaction->option(it.key(), option->value), // value
            it.key(), // caption
            option->comment, // description
            KProperty::Auto // type
        );
        d->properties->addProperty(prop);
    }

    //propertySetSwitched();
    propertySetReloaded(true);
    d->updatesProperties = false;
}

KPropertySet* KexiScriptDesignView::propertySet()
{
    return d->properties;
}

void KexiScriptDesignView::slotPropertyChanged(KPropertySet& /*set*/, KProperty& property)
{
    qDebug();

    if (property.isNull())
        return;

    if (property.name() == "language") {
        QString language = property.value().toString();
        qDebug() << "language:" << language;
        d->scriptaction->setInterpreter(language);
        // We assume Kross and the HighlightingInterface are using same
        // names for the support languages...
        d->editor->setHighlightMode(language);
        updateProperties();
    }
    else if (property.name() == "type") {
        d->scriptType = property.value().toString();
    }
    else {
        bool ok = d->scriptaction->setOption(property.name(), property.value());
        if (! ok) {
            qWarning() << "unknown property:" << property.name();
            return;
        }
    }

    setDirty(true);
}

void KexiScriptDesignView::execute()
{
    d->statusbrowser->clear();
    QTime time;
    time.start();
    d->statusbrowser->append(xi18n("Execution of the script \"%1\" started.", d->scriptaction->name()));

    d->scriptaction->trigger();
    if (d->scriptaction->hadError()) {
        QString errormessage = d->scriptaction->errorMessage();
        d->statusbrowser->append(QString("<b>%2</b><br>").arg(errormessage.toHtmlEscaped()));

        QString tracedetails = d->scriptaction->errorTrace();
        d->statusbrowser->append(tracedetails.toHtmlEscaped());

        long lineno = d->scriptaction->errorLineNo();
        if (lineno >= 0)
            d->editor->setLineNo(lineno);
    }
    else {
        // xgettext: no-c-format
        d->statusbrowser->append(xi18n("Successfully executed. Time elapsed: %1ms", time.elapsed()));
    }
}

bool KexiScriptDesignView::loadData()
{
    QString data;
    if (!loadDataBlock(&data)) {
        qDebug() << "no DataBlock";
        return false;
    }

    QString errMsg;
    int errLine;
    int errCol;

    QDomDocument domdoc;
    bool parsed = domdoc.setContent(data, false, &errMsg, &errLine, &errCol);

    if (! parsed) {
        qDebug() << "XML parsing error line: " << errLine << " col: " << errCol << " message: " << errMsg;
        return false;
    }

    QDomElement scriptelem = domdoc.namedItem("script").toElement();
    if (scriptelem.isNull()) {
        qDebug() << "script domelement is null";
        return false;
    }

    d->scriptType = scriptelem.attribute("scripttype");
    if (d->scriptType.isEmpty()) {
        d->scriptType = "executable";
    }

    QString interpretername = scriptelem.attribute("language");
    Kross::Manager* manager = &Kross::Manager::self();
    Kross::InterpreterInfo* info = interpretername.isEmpty() ? 0 : manager->interpreterInfo(interpretername);
    if (info) {
        d->scriptaction->setInterpreter(interpretername);

        Kross::InterpreterInfo::Option::Map options = info->options();
        Kross::InterpreterInfo::Option::Map::ConstIterator it, end = options.constEnd();
        for (it = options.constBegin(); it != end; ++it) {
            QString value = scriptelem.attribute(it.key());
            if (! value.isNull()) {
                QVariant v(value);
                if (v.convert(it.value()->value.type()))    // preserve the QVariant's type
                    d->scriptaction->setOption(it.key(), v);
            }
        }
    }

    d->scriptaction->setCode(scriptelem.text().toUtf8());

    return true;
}

KDbObject* KexiScriptDesignView::storeNewData(const KDbObject& object,
                                                       KexiView::StoreNewDataOptions options,
                                                       bool *cancel)
{
    KDbObject *s = KexiView::storeNewData(object, options, cancel);
    qDebug() << "new id:" << s->id();

    if (!s || *cancel) {
        delete s;
        return 0;
    }

    if (! storeData()) {
        qWarning() << "Failed to store the data.";
        //failure: remove object's object data to avoid garbage
        KDbConnection *conn = KexiMainWindowIface::global()->project()->dbConnection();
        conn->removeObject(s->id());
        delete s;
        return 0;
    }

    return s;
}

tristate KexiScriptDesignView::storeData(bool /*dontAsk*/)
{
    qDebug(); //<< window()->partItem()->name() << " [" << window()->id() << "]";

    QDomDocument domdoc("script");
    QDomElement scriptelem = domdoc.createElement("script");
    domdoc.appendChild(scriptelem);

    QString language = d->scriptaction->interpreter();
    scriptelem.setAttribute("language", language);
    //! @todo move different types to their own part??
    scriptelem.setAttribute("scripttype", d->scriptType);

    Kross::InterpreterInfo* info = Kross::Manager::self().interpreterInfo(language);
    if (info) {
        Kross::InterpreterInfo::Option::Map defoptions = info->options();
        QMap<QString, QVariant> options = d->scriptaction->options();
        QMap<QString, QVariant>::ConstIterator it, end(options.constEnd());
        for (it = options.constBegin(); it != end; ++it)
            if (defoptions.contains(it.key()))  // only remember options which the InterpreterInfo knows about...
                scriptelem.setAttribute(it.key(), it.value().toString());
    }

    QDomText scriptcode = domdoc.createTextNode(d->scriptaction->code());
    scriptelem.appendChild(scriptcode);

    return storeDataBlock(domdoc.toString());
}


