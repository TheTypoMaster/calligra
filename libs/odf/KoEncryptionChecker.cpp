/* This file is part of the KDE project
 * Copyright (C) 2009 Boudewijn Rempt <boud@valdyas.org>
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

#include "KoEncryptionChecker.h"

#ifdef QCA2

// QCA headers have "slots" and "signals", which QT_NO_SIGNALS_SLOTS_KEYWORDS does not like
#define slots Q_SLOTS
#define signals Q_SIGNALS
#include <QtCrypto>
#undef slots
#undef signals
#include <OdfDebug.h>

bool KoEncryptionChecker::isEncryptionSupported()
{
    QCA::Initializer* initializer = new QCA::Initializer();
    bool supported = QCA::isSupported("sha1") && QCA::isSupported("pbkdf2(sha1)") && QCA::isSupported("blowfish-cfb");
    if (!supported) {
        warnOdf << "QCA is enabled but sha1, pbkdf2(sha1) or blowfish-cfb are not supported. Encryption is disabled.";
    }
    delete initializer;
    return supported;
}
#else
bool KoEncryptionChecker::isEncryptionSupported()
{

    return false;
}
#endif
