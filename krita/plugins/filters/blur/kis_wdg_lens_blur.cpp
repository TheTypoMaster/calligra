/*
 * This file is part of Krita
 *
 * Copyright (c) 2010 Edward Apap <schumifer@hotmail.com>
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

#include "kis_wdg_lens_blur.h"
#include <QLayout>
#include <QToolButton>
#include <QIcon>

#include <knuminput.h>

#include <filter/kis_filter.h>
#include <filter/kis_filter_configuration.h>
#include <kis_selection.h>
#include <kis_paint_device.h>
#include <kis_processing_information.h>

#include "kis_lens_blur_filter.h"

#include "ui_wdg_lens_blur.h"

KisWdgLensBlur::KisWdgLensBlur(QWidget * parent) : KisConfigWidget(parent)
{
    m_widget = new Ui_WdgLensBlur();
    m_widget->setupUi(this);

    connect(m_widget->irisShapeCombo, SIGNAL(currentIndexChanged(int)), SIGNAL(sigConfigurationItemChanged()));
    connect(m_widget->irisRadiusSlider, SIGNAL(valueChanged(int)), SIGNAL(sigConfigurationItemChanged()));
    connect(m_widget->irisRotationSlider, SIGNAL(valueChanged(int)), SIGNAL(sigConfigurationItemChanged()));
}

KisWdgLensBlur::~KisWdgLensBlur()
{
    delete m_widget;
}

KisPropertiesConfiguration* KisWdgLensBlur::configuration() const
{
    KisFilterConfiguration* config = new KisFilterConfiguration("lens blur", 1);
    config->setProperty("irisShape", m_widget->irisShapeCombo->currentText());
    config->setProperty("irisRadius", m_widget->irisRadiusSlider->value());
    config->setProperty("irisRotation", m_widget->irisRotationSlider->value());

    QSize halfSize = KisLensBlurFilter::getKernelHalfSize(config);
    config->setProperty("halfWidth", halfSize.width());
    config->setProperty("halfHeight", halfSize.height());

    return config;
}

void KisWdgLensBlur::setConfiguration(const KisPropertiesConfiguration* config)
{
    QVariant value;
    if (config->getProperty("irisShape", value)) {
        for (int i = 0; i < m_widget->irisShapeCombo->count(); ++i) {
            if (value.toString() == m_widget->irisShapeCombo->itemText(i)) {
                m_widget->irisShapeCombo->setCurrentIndex(i);
            }
        }
    }
    if (config->getProperty("irisRadius", value)) {
        m_widget->irisRadiusSlider->setValue(value.toInt());
    }
    if (config->getProperty("irisRotation", value)) {
        m_widget->irisRotationSlider->setValue(value.toInt());
    }
}

#include "kis_wdg_lens_blur.moc"
