/* This file is part of the KDE project
 * Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
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

#ifndef KIS_PRESSURE_ROTATION_OPTION_H
#define KIS_PRESSURE_ROTATION_OPTION_H

#include "kis_curve_option.h"
#include <kis_paint_information.h>
#include <kritapaintop_export.h>

/**
 * The pressure opacity option defines a curve that is used to
 * calculate the effect of pressure on the size of the dab
 */
class PAINTOP_EXPORT KisPressureRotationOption : public KisCurveOption
{
public:
    KisPressureRotationOption();
    double apply(const KisPaintInformation & info) const;
    void readOptionSetting(const KisPropertiesConfiguration* setting);
    void applyFanCornersInfo(KisPaintOp *op);

private:
    qreal m_defaultAngle;
    bool m_canvasAxisXMirrored;
    bool m_canvasAxisYMirrored;





};

#endif
