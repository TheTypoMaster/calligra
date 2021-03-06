/* This file is part of the KDE project
   Copyright (C) 2005-2011 Jarosław Staniek <staniek@kde.org>

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

#include "kexiblobbuffer.h"

#include <assert.h>

#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QPixmap>
#include <QHash>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDebug>
#include <QImageReader>

#include <KDbConnection>

Q_GLOBAL_STATIC(KexiBLOBBuffer, _buffer)

//-----------------

class KexiBLOBBuffer::Private
{
public:
    Private()
            : maxId(0)
            , conn(0)
    {
    }
    ~Private() {
        foreach(Item* item, inMemoryItems) {
            delete item;
        }
        inMemoryItems.clear();
        foreach(Item* item, storedItems) {
            delete item;
        }
        storedItems.clear();
    }
    Id_t maxId; //!< Used to compute maximal recently used identifier for unstored BLOB
//! @todo will be changed to QHash<quint64, Item>
    QHash<Id_t, Item*> inMemoryItems; //!< for unstored BLOBs
    QHash<Id_t, Item*> storedItems; //!< for stored items
    QHash<QString, Item*> itemsByURL;
    //! @todo KEXI3 use equivalent of QPointer<KDbConnection>
    KDbConnection *conn;
};

//-----------------

KexiBLOBBuffer::Handle::Handle(Item* item)
        : m_item(item)
{
    if (m_item)
        m_item->refs++;
}

KexiBLOBBuffer::Handle::Handle(const Handle& handle)
{
    *this = handle;
}

KexiBLOBBuffer::Handle::Handle()
        : m_item(0)
{
}

KexiBLOBBuffer::Handle::~Handle()
{
    if (m_item) {
        m_item->refs--;
        if (m_item->refs <= 0)
            KexiBLOBBuffer::self()->removeItem(m_item->id, m_item->stored);
    }
}

KexiBLOBBuffer::Handle& KexiBLOBBuffer::Handle::operator=(const Handle & handle)
{
    m_item = handle.m_item;
    if (m_item)
        m_item->refs++;
    return *this;
}

void KexiBLOBBuffer::Handle::setStoredWidthID(KexiBLOBBuffer::Id_t id)
{
    if (!m_item)
        return;
    if (m_item->stored) {
        qWarning() << "object for id=" << id << " is aleady stored";
        return;
    }

    KexiBLOBBuffer::self()->takeItem(m_item);
    m_item->id = id; //new id
    m_item->stored = true;
//! @todo What about other handles for this item?
//! @todo They were assuming it's unstored item, but it's stored now....
    KexiBLOBBuffer::self()->insertItem(m_item);
}

//-----------------

KexiBLOBBuffer::Item::Item(const QByteArray& data, KexiBLOBBuffer::Id_t ident, bool _stored,
                           const QString& _name, const QString& _caption, const QString& _mimeType,
                           Id_t _folderId, const QPixmap& pixmap)
        : name(_name), caption(_caption), mimeType(_mimeType), refs(0),
        id(ident), folderId(_folderId), stored(_stored),
        m_pixmapLoaded(new bool(false)/*workaround for pixmap() const*/)
{
    if (pixmap.isNull())
        m_pixmap = new QPixmap();
    else
        m_pixmap = new QPixmap(pixmap);

    if (data.isEmpty())
        m_data = new QByteArray();
    else
        m_data = new QByteArray(data);
}

KexiBLOBBuffer::Item::~Item()
{
    qDebug();
    delete m_pixmap;
    m_pixmap = 0;
    delete m_data;
    m_data = 0;
    delete m_pixmapLoaded;
}

QPixmap KexiBLOBBuffer::Item::pixmap() const
{
//! @todo ...
    if (!*m_pixmapLoaded && m_pixmap->isNull() && !m_data->isEmpty()) {
        const QMimeDatabase db;
        const QMimeType mime(db.mimeTypeForName(mimeType));
        if (!mime.isValid()
            || !QImageReader::supportedMimeTypes().contains(mimeType.toLatin1())
            || !m_pixmap->loadFromData(*m_data, mime.preferredSuffix().toLatin1()))
        {
            //! @todo inform about error?
        }
        *m_pixmapLoaded = true;
    }
    return *m_pixmap;
}

/*! @return Extension for QPixmap::save() from @a mimeType.
    @todo default PNG ok? */
static QString formatFromMimeType(const QString& mimeType, const QString& defaultFormat = "PNG")
{
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForName(mimeType);
    if (!mime.isValid()) {
        return defaultFormat;
    }
    return mime.preferredSuffix();
}

QByteArray KexiBLOBBuffer::Item::data() const
{
    if (!m_data->isEmpty())
        return *m_data;

    if (m_data->isEmpty() && m_pixmap->isNull())
        return QByteArray();

    if (m_data->isEmpty() && !m_pixmap->isNull()) {
        //convert pixmap to byte array
        //(do it only on demand)
        QBuffer buffer(m_data);
        if (!buffer.open(QIODevice::WriteOnly)) {
            //! @todo err msg
            qWarning() << "!QBuffer::open()";
        }
        if (!m_pixmap->save(&buffer, formatFromMimeType(mimeType).toLatin1()))
        {
            //! @todo err msg
            qWarning() << "!QPixmap::save()";
        }
    }
    return *m_data;
}

//-----------------

KexiBLOBBuffer::KexiBLOBBuffer()
        : QObject()
        , d(new Private())
{
}

KexiBLOBBuffer::~KexiBLOBBuffer()
{
    delete d;
}

KexiBLOBBuffer::Handle KexiBLOBBuffer::insertPixmap(const QUrl &url)
{
    if (url.isEmpty())
        return KexiBLOBBuffer::Handle();
    if (!url.isValid()) {
        qWarning() << "INVALID URL" << url;
        return KexiBLOBBuffer::Handle();
    }
//! @todo what about searching by filename only and then compare data?
    Item * item = d->itemsByURL.value(url.toDisplayString());
    if (item)
        return KexiBLOBBuffer::Handle(item);

    QString fileName = url.isLocalFile() ? url.toLocalFile() : url.toDisplayString();
//! @todo download the file if remote, then set fileName properly
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        //! @todo err msg
        return KexiBLOBBuffer::Handle();
    }
    QByteArray data(f.readAll());
    if (f.error() != QFile::NoError) {
        //! @todo err msg
        return KexiBLOBBuffer::Handle();
    }
    QFileInfo fi(url.fileName());
    QString caption(fi.baseName().replace('_', ' ').simplified());
    QMimeDatabase db;
    const QMimeType mimeType(db.mimeTypeForFileNameAndData(fileName, data));

    item = new Item(data, ++d->maxId, /*!stored*/false, url.fileName(), caption, mimeType.name());
    insertItem(item);

    //cache
    item->prettyURL = url.toDisplayString();
    d->itemsByURL.insert(url.toDisplayString(), item);
    return KexiBLOBBuffer::Handle(item);
}

KexiBLOBBuffer::Handle KexiBLOBBuffer::insertObject(const QByteArray& data,
        const QString& name, const QString& caption, const QString& mimeType, KexiBLOBBuffer::Id_t identifier)
{
    KexiBLOBBuffer::Id_t newIdentifier;
    if (identifier > 0)
        newIdentifier = identifier;
    else
        newIdentifier = ++d->maxId;

    Item *item = new Item(data, newIdentifier, identifier > 0, name, caption, mimeType);
    insertItem(item);
    return KexiBLOBBuffer::Handle(item);
}

KexiBLOBBuffer::Handle KexiBLOBBuffer::insertPixmap(const QPixmap& pixmap)
{
    if (pixmap.isNull())
        return KexiBLOBBuffer::Handle();

    Item * item = new Item(
        QByteArray(), //(pixmap will be converted to byte array on demand)
        ++d->maxId,
        false, //not stored
        QString(),
        QString(),
        "image/png", //!< @todo OK? What about jpegs?
        0, //folder id
        pixmap);

    insertItem(item);
    return KexiBLOBBuffer::Handle(item);
}

KexiBLOBBuffer::Handle KexiBLOBBuffer::objectForId(Id_t id, bool stored)
{
    if (id <= 0)
        return KexiBLOBBuffer::Handle();
    if (stored) {
        Item *item = d->storedItems.value(id);
        if (item || !d->conn)
            return KexiBLOBBuffer::Handle(item);
        //retrieve stored BLOB:
        assert(d->conn);
        KDbTableSchema *blobsTable = d->conn->tableSchema("kexi__blobs");
        if (!blobsTable) {
            //! @todo err msg
            return KexiBLOBBuffer::Handle();
        }
        /*  QStringList where;
            where << "o_id";
            KDbPreparedStatement::Ptr st = d->conn->prepareStatement(
              KDbPreparedStatement::SelectStatement, *blobsTable, where);*/
//! @todo use KDbPreparedStatement
        KDbQuerySchema schema;
        schema.addField(blobsTable->field("o_data"));
        schema.addField(blobsTable->field("o_name"));
        schema.addField(blobsTable->field("o_caption"));
        schema.addField(blobsTable->field("o_mime"));
        schema.addField(blobsTable->field("o_folder_id"));
        schema.addToWhereExpression(blobsTable->field("o_id"), QVariant((qint64)id));

        KDbRecordData recordData;
        tristate res = d->conn->querySingleRecord(&schema, &recordData);
        if (res != true || recordData.size() < 4) {
            //! @todo err msg
            qWarning() << "id=" << id << "stored=" << stored
                << ": res!=true || recordData.size()<4; res==" << res.toString()
                << "recordData.size()==" << recordData.size();
            return KexiBLOBBuffer::Handle();
        }

        item = new Item(
            recordData.at(0).toByteArray(),
            id,
            true, //stored
            recordData.at(1).toString(),
            recordData.at(2).toString(),
            recordData.at(3).toString(),
            (Id_t)recordData.at(4).toInt() //!< @todo folder id: fix Id_t for Qt4
        );

        insertItem(item);
        return KexiBLOBBuffer::Handle(item);
    }
    return KexiBLOBBuffer::Handle(d->inMemoryItems.value(id));
}

KexiBLOBBuffer::Handle KexiBLOBBuffer::objectForId(Id_t id)
{
    KexiBLOBBuffer::Handle h(objectForId(id, false/* !stored */));
    if (h)
        return h;
    return objectForId(id, true/*stored*/);
}

void KexiBLOBBuffer::removeItem(Id_t id, bool stored)
{
    Item *item;
    if (stored)
        item = d->storedItems.take(id);
    else
        item = d->inMemoryItems.take(id);

    if (item && !item->prettyURL.isEmpty()) {
        d->itemsByURL.remove(item->prettyURL);
    }
    delete item;
}

void KexiBLOBBuffer::takeItem(Item *item)
{
    assert(item);
    if (item->stored)
        d->storedItems.take(item->id);
    else
        d->inMemoryItems.take(item->id);
}

void KexiBLOBBuffer::insertItem(Item *item)
{
    assert(item);
    if (item->stored)
        d->storedItems.insert(item->id, item);
    else
        d->inMemoryItems.insert(item->id, item);
}

void KexiBLOBBuffer::setConnection(KDbConnection *conn)
{
    KexiBLOBBuffer::self()->d->conn = conn;
}

KexiBLOBBuffer* KexiBLOBBuffer::self()
{
    return _buffer;
}

