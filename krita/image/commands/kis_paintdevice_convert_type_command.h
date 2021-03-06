/*
 *  Copyright (c) 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KIS_PAINTDEVICE_CONVERT_TYPE_COMMAND_H
#define KIS_PAINTDEVICE_CONVERT_TYPE_COMMAND_H

#include <kundo2command.h>
#include <QSize>
#include <QBitArray>

#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoColorSpaceRegistry.h>

#include "kis_types.h"
#include "kis_paint_device.h"

class KisPaintDeviceConvertTypeCommand : public KUndo2Command
{

public:
    KisPaintDeviceConvertTypeCommand(KisPaintDeviceSP paintDevice,
                                     KisDataManagerSP beforeData, const KoColorSpace * beforeColorSpace,
                                     KisDataManagerSP afterData, const KoColorSpace * afterColorSpace) {
        m_firstRedo = true;

        Q_ASSERT(paintDevice);
        Q_ASSERT(beforeData);
        Q_ASSERT(afterData);
        Q_ASSERT(beforeData != afterData);

        m_paintDevice = paintDevice;
        m_beforeData = beforeData;
        m_beforeColorSpaceModelId = beforeColorSpace->colorModelId().id();
        m_beforeColorSpaceDepthId = beforeColorSpace->colorDepthId().id();
        if (beforeColorSpace->profile()) {
            m_beforeProfileName = beforeColorSpace->profile()->name();
        }
        m_afterData = afterData;
        m_afterColorSpaceModelId = afterColorSpace->colorModelId().id();
        m_afterColorSpaceDepthId = afterColorSpace->colorDepthId().id();
        if (afterColorSpace->profile()) {
            m_afterProfileName = afterColorSpace->profile()->name();
        }
    }

    virtual ~KisPaintDeviceConvertTypeCommand() {
    }

public:

    virtual void redo() {

        //KUndo2QStack calls redo(), so the first call needs to be blocked
        if (m_firstRedo) {
            m_firstRedo = false;
            return;
        }
        dbgImage << m_paintDevice << m_beforeColorSpaceModelId << m_beforeColorSpaceDepthId << "to" << m_afterColorSpaceModelId << m_afterColorSpaceDepthId;
        const KoColorSpace* cs = KoColorSpaceRegistry::instance()->colorSpace(m_afterColorSpaceModelId, m_afterColorSpaceDepthId, m_afterProfileName);
        m_paintDevice->setDataManager(m_afterData, cs);
        m_paintDevice->setDirty();
    }

    virtual void undo() {

        dbgImage << m_paintDevice << m_afterColorSpaceModelId << m_afterColorSpaceDepthId << "to" << m_beforeColorSpaceModelId << m_beforeColorSpaceDepthId ;
        const KoColorSpace* cs = KoColorSpaceRegistry::instance()->colorSpace(m_beforeColorSpaceModelId, m_beforeColorSpaceDepthId, m_beforeProfileName);
        m_paintDevice->setDataManager(m_beforeData, cs);
        m_paintDevice->setDirty();
    }

private:

    bool m_firstRedo; // there is a mismatch between the Qt undo system and what we need for krita...

    KisPaintDeviceSP m_paintDevice;

    KisDataManagerSP m_beforeData;
    QString m_beforeColorSpaceModelId;
    QString m_beforeColorSpaceDepthId;
    QString m_beforeProfileName;

    KisDataManagerSP m_afterData;
    QString m_afterColorSpaceModelId;
    QString m_afterColorSpaceDepthId;
    QString m_afterProfileName;
};

#endif
