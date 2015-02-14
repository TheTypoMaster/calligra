/*
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_paint_layer_test.h"
#include <qtest_kde.h>
#include <QImage>
#include <QCoreApplication>

#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>

#include "kis_group_layer.h"
#include "kis_types.h"
#include "kis_paint_layer.h"
#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_transparency_mask.h"
#include "testutil.h"
#include "kis_selection.h"
#include "kis_fill_painter.h"
#include "kis_pixel_selection.h"
#include <kis_iterator_ng.h>
#include "kis_multi_paint_device.h"

void KisPaintLayerTest::testProjection()
{

    QImage qimage(QString(FILES_DATA_DIR) + QDir::separator() + "hakonepa.png");
    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, qimage.width(), qimage.height(), cs, "merge test");

    KisPaintLayerSP layer = new KisPaintLayer(image, "test", OPACITY_OPAQUE_U8);
    layer->paintDevice()->convertFromQImage(qimage, 0, 0, 0);
    image->addNode(layer.data());

    // Make sure the projection and the paint device are the same -- we don't have masks yet
    QVERIFY(layer->paintDevice().data() == layer->projection().data());

    KisTransparencyMaskSP transparencyMask = new KisTransparencyMask();
    transparencyMask->initSelection(layer);
    transparencyMask->selection()->pixelSelection()->invert();
    image->addNode(transparencyMask.data(), layer.data());

    // Now there are masks. Verify that
    Q_ASSERT(layer->hasEffectMasks());

    // And now we're going to update the projection, but nothing is dirty yet
    layer->updateProjection(qimage.rect(), KisNode::N_FILTHY);

    // Which also means that the projection is no longer the paint device
    QVERIFY(layer->paintDevice().data() != layer->projection().data());

    // Now the machinery will start to roll
    layer->setDirty(qimage.rect());

    // And now we're going to update the projection, but nothing is dirty yet
    layer->updateProjection(qimage.rect(), KisNode::N_FILTHY);

    // Which also means that the projection is no longer the paint device
    QVERIFY(layer->paintDevice().data() != layer->projection().data());

    // And the projection is no longer 0, because while we've updated it, nothing is dirty,
    // so nothing gets updated
    QVERIFY(layer->projection().data() != 0);

    // The selection is initially empty, so after an update, all pixels are still visible
    layer->updateProjection(qimage.rect(), KisNode::N_FILTHY);

    // We've inverted the mask, so now nothing is seen
    KisSequentialConstIterator it(layer->projection(), qimage.rect());
    do {
        QVERIFY(cs->opacityU8(it.oldRawData()) == OPACITY_TRANSPARENT_U8);
    } while (it.nextPixel());

    // Now fill the layer with some opaque pixels
    transparencyMask->select(qimage.rect());
    transparencyMask->setDirty(qimage.rect());
    image->waitForDone();

    layer->projection()->convertToQImage(0, 0, 0, qimage.width(), qimage.height()).save("aaa.png");
    // Nothing is transparent anymore, so the projection and the paint device should be identical again
    QPoint errpoint;
    if (!TestUtil::compareQImages(errpoint, qimage, layer->projection()->convertToQImage(0, 0, 0, qimage.width(), qimage.height()))) {
        QFAIL(QString("Failed to create identical image, first different pixel: %1,%2 ").arg(errpoint.x()).arg(errpoint.y()).toLatin1());
    }

}

void KisPaintLayerTest::testKeyframing()
{
    const KoColorSpace *cs = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, 512, 512, cs, "");
    KisPaintLayerSP layer = new KisPaintLayer(image, "", OPACITY_OPAQUE_U8);
    KisMultiPaintDevice *dev = qobject_cast<KisMultiPaintDevice*>(layer->paintDevice().data());

    KisKeyframeChannel *contentChannel = layer->keyframes()->getChannel("content");

    QCOMPARE(contentChannel->times().count(), 0);

    layer->addBlankFrame(7);
    QCOMPARE(contentChannel->times().count(), 2); // Original content AND added frame
    QVERIFY(contentChannel->getValueAt(0) != contentChannel->getValueAt(7));

    layer->addBlankFrame(5);
    QCOMPARE(contentChannel->times().count(), 3);
    QVERIFY(contentChannel->getValueAt(5) != contentChannel->getValueAt(0));
    QVERIFY(contentChannel->getValueAt(5) != contentChannel->getValueAt(7));

    layer->seekToTime(5);

    QCOMPARE(QVariant(dev->currentContext()), contentChannel->getValueAt(5));

    layer->seekToTime(0);

    QCOMPARE(QVariant(dev->currentContext()), contentChannel->getValueAt(0));
}


QTEST_KDEMAIN(KisPaintLayerTest, GUI)
#include "kis_paint_layer_test.moc"
