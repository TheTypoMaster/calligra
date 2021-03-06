/*
 * This file is part of Krita
 *
 * Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#include "colorsfilters.h"


#include <math.h>

#include <stdlib.h>
#include <string.h>

#include <QSlider>
#include <QPoint>
#include <QColor>

#include <klocalizedstring.h>

#include <kis_debug.h>
#include <kpluginfactory.h>

#include "KoBasicHistogramProducers.h"
#include <KoColorSpace.h>
#include <KoColorTransformation.h>
#include <filter/kis_filter_configuration.h>
#include <kis_paint_device.h>
#include <kis_processing_information.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_global.h>
#include <kis_types.h>
#include <kis_selection.h>
#include <kis_histogram.h>
#include <filter/kis_filter_registry.h>
#include <kis_painter.h>
#include <KoUpdater.h>
#include <KoColorSpaceConstants.h>
#include <KoCompositeOp.h>
#include <kis_iterator_ng.h>


#include "kis_hsv_adjustment_filter.h"
#include "kis_brightness_contrast_filter.h"
#include "kis_perchannel_filter.h"
#include "kis_color_balance_filter.h"
#include "kis_desaturate_filter.h"

K_PLUGIN_FACTORY_WITH_JSON(ColorsFiltersFactory, "kritacolorsfilter.json", registerPlugin<ColorsFilters>();)

ColorsFilters::ColorsFilters(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    KisFilterRegistry * manager = KisFilterRegistry::instance();
    manager->add(new KisBrightnessContrastFilter());
    manager->add(new KisAutoContrast());
    manager->add(new KisPerChannelFilter());
    manager->add(new KisDesaturateFilter());
    manager->add(new KisHSVAdjustmentFilter());
    manager->add(new KisColorBalanceFilter());

}

ColorsFilters::~ColorsFilters()
{
}


//==================================================================


KisAutoContrast::KisAutoContrast() : KisFilter(id(), categoryAdjust(), i18n("&Auto Contrast"))
{
    setSupportsPainting(false);
    setSupportsThreading(false);
    setSupportsAdjustmentLayers(false);
    setColorSpaceIndependence(TO_LAB16);
    setShowConfigurationWidget(false);
}

void KisAutoContrast::processImpl(KisPaintDeviceSP device,
                                  const QRect& applyRect,
                                  const KisFilterConfiguration* config,
                                  KoUpdater* progressUpdater) const
{
    Q_ASSERT(device != 0);
    Q_UNUSED(config);
    // initialize
    KoHistogramProducer *producer = new KoGenericLabHistogramProducer();
    KisHistogram histogram(device, applyRect, producer, LINEAR);
    int minvalue = int(255 * histogram.calculations().getMin() + 0.5);
    int maxvalue = int(255 * histogram.calculations().getMax() + 0.5);

    if (maxvalue > 255)
        maxvalue = 255;

    histogram.setChannel(0);
    int twoPercent = int(0.005 * histogram.calculations().getCount());
    int pixCount = 0;
    int binnum = 0;

    while (binnum < histogram.producer()->numberOfBins()) {
        pixCount += histogram.getValue(binnum);
        if (pixCount > twoPercent) {
            minvalue = binnum;
            break;
        }
        binnum++;
    }
    pixCount = 0;
    binnum = histogram.producer()->numberOfBins() - 1;
    while (binnum > 0) {
        pixCount += histogram.getValue(binnum);
        if (pixCount > twoPercent) {
            maxvalue = binnum;
            break;
        }
        binnum--;
    }
    // build the transferfunction
    int diff = maxvalue - minvalue;

    quint16* transfer = new quint16[256];
    for (int i = 0; i < 255; i++)
        transfer[i] = 0xFFFF;

    if (diff != 0) {
        for (int i = 0; i < minvalue; i++)
            transfer[i] = 0x0;
        for (int i = minvalue; i < maxvalue; i++) {
            qint32 val = (i - minvalue) / diff;

            val = int((0xFFFF * (i - minvalue)) / diff);
            if (val > 0xFFFF)
                val = 0xFFFF;
            if (val < 0)
                val = 0;

            transfer[i] = val;
        }
        for (int i = maxvalue; i < 256; i++)
            transfer[i] = 0xFFFF;
    }
    // apply
    KoColorTransformation *adj = device->colorSpace()->createBrightnessContrastAdjustment(transfer);

    KisSequentialIterator it(device, applyRect);

    qint32 totalCost = (applyRect.width() * applyRect.height()) / 100;
    if (totalCost == 0) totalCost = 1;
    qint32 pixelsProcessed = 0;

    quint32 npix;
    do {
        npix = it.nConseqPixels();
        // adjust
        adj->transform(it.oldRawData(), it.rawData(), npix);
        pixelsProcessed += npix;
        if (progressUpdater) progressUpdater->setProgress(pixelsProcessed / totalCost);
    } while(it.nextPixels(npix)  && !(progressUpdater && progressUpdater->interrupted()));
    delete[] transfer;
    delete adj;
}

#include "colorsfilters.moc"
