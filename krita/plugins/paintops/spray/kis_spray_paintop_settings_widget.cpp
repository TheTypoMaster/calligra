/*
 *  Copyright (c) 2008,2009,2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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
#include "kis_spray_paintop_settings_widget.h"

#include "kis_sprayop_option.h"
#include "kis_spray_paintop_settings.h"
#include "kis_spray_shape_option.h"

#include <kis_color_option.h>
#include <kis_paintop_settings_widget.h>
#include <kis_paint_action_type_option.h>

#include <kis_pressure_rotation_option.h>
#include <kis_pressure_opacity_option.h>
#include <kis_pressure_size_option.h>
#include <kis_curve_option_widget.h>
#include <kis_brush_option_widget.h>
#include "kis_spray_shape_dynamics.h"
#include <kis_airbrush_option.h>
#include <kis_compositeop_option.h>

KisSprayPaintOpSettingsWidget:: KisSprayPaintOpSettingsWidget(QWidget* parent)
    : KisPaintOpSettingsWidget(parent)
    , m_sprayArea(new KisSprayOpOption())
{
    addPaintOpOption(m_sprayArea, i18n("Spray shape"));
    addPaintOpOption(new KisSprayShapeOption(), i18n("Spray shape"));
    addPaintOpOption(new KisBrushOptionWidget(), i18n("Brush Tip"));
    addPaintOpOption(new KisCurveOptionWidget(new KisPressureOpacityOption(), i18n("Transparent"), i18n("Opaque")), i18n("Opacity"));
    addPaintOpOption(new KisCurveOptionWidget(new KisPressureSizeOption(), i18n("0%"), i18n("100%")), i18n("Size"));
    addPaintOpOption(new KisCompositeOpOption(true), i18n("Blending Mode"));

    addPaintOpOption(new KisSprayShapeDynamicsOption(), i18n("Shape dynamics"));
    addPaintOpOption(new KisColorOption(), i18n("Color options"));

    addPaintOpOption(new KisCurveOptionWidget(new KisPressureRotationOption(), i18n("0°"), i18n("360°")), i18n("Rotation"));
    addPaintOpOption(new KisAirbrushOption(), i18n("Airbrush"));
    addPaintOpOption(new KisPaintActionTypeOption(), i18n("Painting Mode"));
}

KisSprayPaintOpSettingsWidget::~ KisSprayPaintOpSettingsWidget()
{
}

KisPropertiesConfiguration*  KisSprayPaintOpSettingsWidget::configuration() const
{
    KisSprayPaintOpSettings* config = new KisSprayPaintOpSettings();
    config->setOptionsWidget(const_cast<KisSprayPaintOpSettingsWidget*>(this));
    config->setProperty("paintop", "spraybrush"); // XXX: make this a const id string
    writeConfiguration(config);
    return config;
}

void KisSprayPaintOpSettingsWidget::changePaintOpSize(qreal x, qreal y)
{
    Q_UNUSED(y);
    m_sprayArea->setDiameter(m_sprayArea->diameter() + qRound(x));
}

QSizeF KisSprayPaintOpSettingsWidget::paintOpSize() const
{
    qreal width = m_sprayArea->diameter();
    qreal height = width * m_sprayArea->brushAspect();
    return QSizeF(width, height);
}
