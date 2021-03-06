/* This file is part of the KDE project
 * Copyright (C) 2001-2003 Lennart Kudling <kudling@kde.org>
 * Copyright (C) 2002-2003 Rob Buis <buis@kde.org>
 * Copyright (C) 2002 Tomislav Lukman <tomislav.lukman@ck.t-com.hr>
 * Copyright (C) 2002,2004-2005,2007 David Faure <faure@kde.org>
 * Copyright (C) 2002 Benoît Vautrin <benoit.vautrin@free.fr>
 * Copyright (C) 2003 Lukáš Tinkl <lukas@kde.org>
 * Copyright (C) 2004,2006 Laurent Montel <montel@kde.org>
 * Copyright (C) 2005-2006 Tim Beaulen <tbscope@gmail.com>
 * Copyright (C) 2005,2007 Thomas Zander <zander@kde.org>
 * Copyright (C) 2006-2007 Jan Hambrecht <jaham@gmx.net>
 * Copyright (C) 2007 Boudewijn Rempt <boud@valdyas.org>
 * Copyright (C) 2007 Matthias Kretz <kretz@kde.org>
 * Copyright (C) 2007 Stephan Kulow <coolo@kde.org>
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

#include "KarbonFactory.h"

#include "KarbonPart.h"
#include "KarbonDocument.h"
#include "KarbonAboutData.h"

#include <kaboutdata.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <kdebug.h>

#include <KoPluginLoader.h>
#include <kglobal.h>

KSharedConfig::Ptr s_karbonConfig;

KarbonFactory::KarbonFactory()
    : KPluginFactory()
{
}

KarbonFactory::~KarbonFactory()
{
}

QObject* KarbonFactory::create(const char* /*iface*/, QWidget* /*parentWidget*/, QObject *parent, const QVariantList& args, const QString& keyword)
{
    Q_UNUSED(args);
    Q_UNUSED(keyword);

    KarbonPart *part = new KarbonPart(parent);
    KarbonDocument* doc = new KarbonDocument(part);
    part->setDocument(doc);
    return part;
}

const KSharedConfig::Ptr &KarbonFactory::karbonConfig()
{
    if (!s_karbonConfig) {
        s_karbonConfig = KSharedConfig::openConfig(KAboutData::applicationData().componentName() + QLatin1String("rc"));
        // Add any application-specific resource directories here

        // Tell the iconloader about share/apps/calligra/icons
        KIconLoader::global()->addAppDir("calligra");

        // Load Karbon specific dockers.
        KoPluginLoader::instance()->load(QString::fromLatin1("Karbon/Dock"));
    }
    return s_karbonConfig;
}


