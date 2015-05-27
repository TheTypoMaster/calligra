/*
 *  Copyright (c) 2015 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef KIS_STACKED_PAINTOP_H_
#define KIS_STACKED_PAINTOP_H_

#include <kis_paintop.h>
#include <kis_types.h>

#include "stacked_brush.h"
#include "stacked_paintop_settings.h"

class KisPainter;

class StackedPaintOp : public KisPaintOp
{

public:

    StackedPaintOp(const StackedPaintOpSettings *settings, KisPainter * painter, KisNodeSP node, KisImageSP image);
    virtual ~StackedPaintOp();

    KisSpacingInformation paintAt(const KisPaintInformation& info);

private:

    StackedBrush *m_stackedBrush;
};

#endif // KIS_STACKED_PAINTOP_H_
