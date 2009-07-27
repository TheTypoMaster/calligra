/* This file is part of the KDE project
 * Copyright (C) 2007, 2009 Thomas Zander <zander@kde.org>
 * Copyright (C) 2007 Jan Hambrecht <jaham@gmx.net>
 * Copyright (C) 2008 Casper Boemann <cbr@boemann.dk>
 * Copyright (C) 2008 Thorsten Zachmann <zachmann@kde.org>
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

#include "KoImageData_p.h"
#include "KoImageCollection.h"

#include <QTemporaryFile>
#include <QImageWriter>
#include <KDebug>

KoImageDataPrivate::KoImageDataPrivate()
    : collection(0),
    errorCode(KoImageData::Success),
    dataStoreState(StateEmpty),
    temporaryFile(0)
{
}

KoImageDataPrivate::~KoImageDataPrivate()
{
    if (collection)
        collection->removeOnKey(key);
    delete temporaryFile;
}

bool KoImageDataPrivate::saveData(QIODevice &device)
{
    switch (dataStoreState) {
    case KoImageDataPrivate::StateEmpty:
        return false;
    case KoImageDataPrivate::StateNotLoaded:
        // spool directly.
        Q_ASSERT(temporaryFile); // otherwise the collection should not have called this
        if (temporaryFile) {
            if (!temporaryFile->open()) {
                kWarning(30006) << "Read file from temporary store failed";
                return false;
            }
            char buf[4096];
            while (true) {
                device.waitForReadyRead(-1);
                qint64 bytes = temporaryFile->read(buf, sizeof(buf));
                if (bytes == -1)
                    break; // done!
                do {
                    bytes -= device.write(buf, bytes);
                } while (bytes > 0);
            }
            temporaryFile->close();
        }
        return true;
    case KoImageDataPrivate::StateImageLoaded:
    case KoImageDataPrivate::StateImageOnly: {
        // save image
        QImageWriter writer(&device, suffix.toLatin1());
        return writer.write(image);
      }
    }
    return false;
}

void KoImageDataPrivate::setSuffix(const QString &name)
{
    QRegExp rx("\\.([^/]+$)"); // TODO does this work on windows or do we have to use \ instead of / for a path separator?
    if (rx.indexIn(name) != -1) {
        suffix = rx.cap(1);
    }
}
