/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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

#include "kis_image.h"

#include <KoConfig.h> // WORDS_BIGENDIAN

#include <stdlib.h>
#include <math.h>

#include <QImage>
#include <QPainter>
#include <QSize>
#include <QDateTime>
#include <QRect>
#include <QRegion>
#include <QtConcurrent>

#include <klocalizedstring.h>

#include "KoColorSpaceRegistry.h"
#include "KoColor.h"
#include "KoColorConversionTransformation.h"
#include "KoColorProfile.h"
#include <KoCompositeOpRegistry.h>

#include "recorder/kis_action_recorder.h"
#include "kis_adjustment_layer.h"
#include "kis_annotation.h"
#include "kis_change_profile_visitor.h"
#include "kis_colorspace_convert_visitor.h"
#include "kis_count_visitor.h"
#include "kis_filter_strategy.h"
#include "kis_group_layer.h"
#include "commands/kis_image_commands.h"
#include "kis_layer.h"
#include "kis_meta_data_merge_strategy_registry.h"
#include "kis_name_server.h"
#include "kis_paint_device.h"
#include "kis_paint_layer.h"
#include "kis_painter.h"
#include "kis_perspective_grid.h"
#include "kis_selection.h"
#include "kis_transaction.h"
#include "kis_types.h"
#include "kis_meta_data_merge_strategy.h"
#include "kis_memory_statistics_server.h"

#include "kis_image_config.h"
#include "kis_update_scheduler.h"
#include "kis_image_signal_router.h"
#include "kis_stroke_strategy.h"
#include "kis_image_barrier_locker.h"


#include "kis_undo_stores.h"
#include "kis_legacy_undo_adapter.h"
#include "kis_post_execution_undo_adapter.h"

#include "kis_transform_worker.h"
#include "kis_processing_applicator.h"
#include "processing/kis_crop_processing_visitor.h"
#include "processing/kis_crop_selections_processing_visitor.h"
#include "processing/kis_transform_processing_visitor.h"
#include "commands_new/kis_image_resize_command.h"
#include "commands_new/kis_image_set_resolution_command.h"
#include "commands_new/kis_activate_selection_mask_command.h"
#include "kis_composite_progress_proxy.h"
#include "kis_layer_composition.h"
#include "kis_wrapped_rect.h"
#include "kis_crop_saved_extra_data.h"

#include "kis_layer_projection_plane.h"

#include "kis_update_time_monitor.h"
#include <QtCore>
#include <boost/bind.hpp>



// #define SANITY_CHECKS

#ifdef SANITY_CHECKS
#define SANITY_CHECK_LOCKED(name)                                       \
    if (!locked()) warnKrita() << "Locking policy failed:" << name          \
                               << "has been called without the image"       \
                                  "being locked";
#else
#define SANITY_CHECK_LOCKED(name)
#endif


class KisImage::KisImagePrivate
{
public:
    KisImagePrivate(KisImage *_q) : q(_q) {}

    KisImage *q;

    quint32 lockCount;
    KisPerspectiveGrid* perspectiveGrid;

    qint32 width;
    qint32 height;

    double xres;
    double yres;

    const KoColorSpace * colorSpace;

    KisSelectionSP deselectedGlobalSelection;
    KisGroupLayerSP rootLayer; // The layers are contained in here
    QList<KisLayer*> dirtyLayers; // for thumbnails
    QList<KisLayerComposition*> compositions;
    KisNodeSP isolatedRootNode;
    bool wrapAroundModePermitted;

    KisNameServer *nserver;

    KisUndoStore *undoStore;
    KisUndoAdapter *legacyUndoAdapter;
    KisPostExecutionUndoAdapter *postExecutionUndoAdapter;

    KisActionRecorder *recorder;

    vKisAnnotationSP annotations;

    QAtomicInt disableUIUpdateSignals;
    QAtomicInt disableDirtyRequests;
    KisImageSignalRouter *signalRouter;
    KisUpdateScheduler *scheduler;

    KisCompositeProgressProxy *compositeProgressProxy;

    bool startProjection;

    bool tryCancelCurrentStrokeAsync();

    void notifyProjectionUpdatedInPatches(const QRect &rc);
};

KisImage::KisImage(KisUndoStore *undoStore, qint32 width, qint32 height, const KoColorSpace * colorSpace, const QString& name, bool startProjection)
        : QObject(0)
        , KisShared()
        , m_d(new KisImagePrivate(this))
{
    setObjectName(name);
    dbgImage << "creating" << name;
    m_d->startProjection = startProjection;

    if (colorSpace == 0) {
        colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    }

    m_d->lockCount = 0;
    m_d->perspectiveGrid = 0;
    m_d->scheduler = 0;
    m_d->wrapAroundModePermitted = false;

    m_d->compositeProgressProxy = new KisCompositeProgressProxy();

    {
        KisImageConfig cfg;

        m_d->scheduler = new KisUpdateScheduler(this);
        if (cfg.enableProgressReporting()) {
            m_d->scheduler->setProgressProxy(m_d->compositeProgressProxy);
        }
    }

    m_d->signalRouter = new KisImageSignalRouter(this);

    if (!undoStore) {
        undoStore = new KisDumbUndoStore();
    }

    m_d->undoStore = undoStore;
    m_d->legacyUndoAdapter = new KisLegacyUndoAdapter(m_d->undoStore, this);
    m_d->postExecutionUndoAdapter = new KisPostExecutionUndoAdapter(m_d->undoStore, this);

    m_d->nserver = new KisNameServer(1);

    m_d->colorSpace = colorSpace;

    setRootLayer(new KisGroupLayer(this, "root", OPACITY_OPAQUE_U8));

    m_d->xres = 1.0;
    m_d->yres = 1.0;
    m_d->width = width;
    m_d->height = height;

    m_d->recorder = new KisActionRecorder(this);

    connect(this, SIGNAL(sigImageModified()), KisMemoryStatisticsServer::instance(), SLOT(notifyImageChanged()));
}

KisImage::~KisImage()
{
    dbgImage << "deleting kisimage" << objectName();

    /**
     * Request the tools to end currently running strokes
     */
    waitForDone();

    /**
     * First delete the nodes, while strokes
     * and undo are still alive
     */
    m_d->rootLayer = 0;


    KisUpdateScheduler *scheduler = m_d->scheduler;
    m_d->scheduler = 0;
    delete scheduler;

    delete m_d->postExecutionUndoAdapter;
    delete m_d->legacyUndoAdapter;
    delete m_d->undoStore;
    delete m_d->compositeProgressProxy;

    delete m_d->signalRouter;
    delete m_d->perspectiveGrid;
    delete m_d->nserver;
    delete m_d;

    disconnect(); // in case Qt gets confused
}

void KisImage::aboutToAddANode(KisNode *parent, int index)
{
    KisNodeGraphListener::aboutToAddANode(parent, index);
    SANITY_CHECK_LOCKED("aboutToAddANode");
}

void KisImage::nodeHasBeenAdded(KisNode *parent, int index)
{
    KisNodeGraphListener::nodeHasBeenAdded(parent, index);

    SANITY_CHECK_LOCKED("nodeHasBeenAdded");
    m_d->signalRouter->emitNodeHasBeenAdded(parent, index);

    KisNodeSP newNode = parent->at(index);
    if (!dynamic_cast<KisSelectionMask*>(newNode.data())) {
        stopIsolatedMode();
    }
}

void KisImage::aboutToRemoveANode(KisNode *parent, int index)
{
    KisNodeSP deletedNode = parent->at(index);
    if (!dynamic_cast<KisSelectionMask*>(deletedNode.data())) {
        stopIsolatedMode();
    }

    KisNodeGraphListener::aboutToRemoveANode(parent, index);

    SANITY_CHECK_LOCKED("aboutToRemoveANode");
    m_d->signalRouter->emitAboutToRemoveANode(parent, index);
}

void KisImage::nodeChanged(KisNode* node)
{
    KisNodeGraphListener::nodeChanged(node);
    requestStrokeEnd();
    m_d->signalRouter->emitNodeChanged(node);
}

KisSelectionSP KisImage::globalSelection() const
{
    KisSelectionMaskSP selectionMask = m_d->rootLayer->selectionMask();
    if (selectionMask) {
        return selectionMask->selection();
    } else {
        return 0;
    }
}

void KisImage::setGlobalSelection(KisSelectionSP globalSelection)
{
    KisSelectionMaskSP selectionMask = m_d->rootLayer->selectionMask();

    if (!globalSelection) {
        if (selectionMask) {
            removeNode(selectionMask);
        }
    }
    else {
        if (!selectionMask) {
            selectionMask = new KisSelectionMask(this);
            selectionMask->initSelection(m_d->rootLayer);
            addNode(selectionMask);
            // If we do not set the selection now, the setActive call coming next
            // can be very, very expensive, depending on the size of the image.
            selectionMask->setSelection(globalSelection);
            selectionMask->setActive(true);
        }
        else {
            selectionMask->setSelection(globalSelection);
        }

        Q_ASSERT(m_d->rootLayer->childCount() > 0);
        Q_ASSERT(m_d->rootLayer->selectionMask());
    }

    m_d->deselectedGlobalSelection = 0;
    m_d->legacyUndoAdapter->emitSelectionChanged();
}

void KisImage::deselectGlobalSelection()
{
    KisSelectionSP savedSelection = globalSelection();
    setGlobalSelection(0);
    m_d->deselectedGlobalSelection = savedSelection;
}

bool KisImage::canReselectGlobalSelection()
{
    return m_d->deselectedGlobalSelection;
}

void KisImage::reselectGlobalSelection()
{
    if(m_d->deselectedGlobalSelection) {
        setGlobalSelection(m_d->deselectedGlobalSelection);
    }
}

QString KisImage::nextLayerName() const
{
    if (m_d->nserver->currentSeed() == 0) {
        m_d->nserver->number();
        return i18n("background");
    }

    return i18n("Layer %1", m_d->nserver->number());
}

void KisImage::rollBackLayerName()
{
    m_d->nserver->rollback();
}

KisCompositeProgressProxy* KisImage::compositeProgressProxy()
{
    return m_d->compositeProgressProxy;
}

bool KisImage::locked() const
{
    return m_d->lockCount != 0;
}

void KisImage::barrierLock()
{
    if (!locked()) {
        requestStrokeEnd();

        if (m_d->scheduler) {
            m_d->scheduler->barrierLock();
        }
    }
    m_d->lockCount++;
}

bool KisImage::tryBarrierLock()
{
    bool result = true;

    if (!locked()) {
        if (m_d->scheduler) {
            result = m_d->scheduler->tryBarrierLock();
        }
    }

    if (result) {
        m_d->lockCount++;
    }

    return result;
}

void KisImage::lock()
{
    if (!locked()) {
        requestStrokeEnd();

        if (m_d->scheduler) {
            m_d->scheduler->lock();
        }
    }
    m_d->lockCount++;
}

void KisImage::unlock()
{
    Q_ASSERT(locked());

    if (locked()) {
        m_d->lockCount--;

        if (m_d->lockCount == 0) {
            if (m_d->scheduler) {
                m_d->scheduler->unlock();
            }
        }
    }
}

void KisImage::blockUpdates()
{
    m_d->scheduler->blockUpdates();
}

void KisImage::unblockUpdates()
{
    m_d->scheduler->unblockUpdates();
}

void KisImage::setSize(const QSize& size)
{
    m_d->width = size.width();
    m_d->height = size.height();
}

void KisImage::resizeImageImpl(const QRect& newRect, bool cropLayers)
{
    if (newRect == bounds() && !cropLayers) return;

    KUndo2MagicString actionName = cropLayers ?
        kundo2_i18n("Crop Image") :
        kundo2_i18n("Resize Image");

    KisImageSignalVector emitSignals;
    emitSignals << ComplexSizeChangedSignal(newRect, newRect.size());
    emitSignals << ModifiedSignal;

    KisCropSavedExtraData *extraData =
        new KisCropSavedExtraData(cropLayers ?
                                  KisCropSavedExtraData::CROP_IMAGE :
                                  KisCropSavedExtraData::RESIZE_IMAGE,
                                  newRect);

    KisProcessingApplicator applicator(this, m_d->rootLayer,
                                       KisProcessingApplicator::RECURSIVE |
                                       KisProcessingApplicator::NO_UI_UPDATES,
                                       emitSignals, actionName, extraData);

    if (cropLayers || !newRect.topLeft().isNull()) {
        KisProcessingVisitorSP visitor =
            new KisCropProcessingVisitor(newRect, cropLayers, true);
        applicator.applyVisitor(visitor, KisStrokeJobData::CONCURRENT);
    }
    applicator.applyCommand(new KisImageResizeCommand(this, newRect.size()));
    applicator.end();
}

void KisImage::resizeImage(const QRect& newRect)
{
    resizeImageImpl(newRect, false);
}

void KisImage::cropImage(const QRect& newRect)
{
    resizeImageImpl(newRect, true);
}


void KisImage::cropNode(KisNodeSP node, const QRect& newRect)
{
    bool isLayer = dynamic_cast<KisLayer*>(node.data());
    KUndo2MagicString actionName = isLayer ?
        kundo2_i18n("Crop Layer") :
        kundo2_i18n("Crop Mask");

    KisImageSignalVector emitSignals;
    emitSignals << ModifiedSignal;

    KisCropSavedExtraData *extraData =
        new KisCropSavedExtraData(KisCropSavedExtraData::CROP_LAYER,
                                  newRect, node);

    KisProcessingApplicator applicator(this, node,
                                       KisProcessingApplicator::RECURSIVE,
                                       emitSignals, actionName, extraData);

    KisProcessingVisitorSP visitor =
        new KisCropProcessingVisitor(newRect, true, false);
    applicator.applyVisitor(visitor, KisStrokeJobData::CONCURRENT);
    applicator.end();
}

void KisImage::scaleImage(const QSize &size, qreal xres, qreal yres, KisFilterStrategy *filterStrategy)
{
    bool resolutionChanged = xres != xRes() && yres != yRes();
    bool sizeChanged = size != this->size();

    if (!resolutionChanged && !sizeChanged) return;

    KisImageSignalVector emitSignals;
    if (resolutionChanged) emitSignals << ResolutionChangedSignal;
    if (sizeChanged) emitSignals << ComplexSizeChangedSignal(bounds(), size);
    emitSignals << ModifiedSignal;

    KUndo2MagicString actionName = sizeChanged ?
        kundo2_i18n("Scale Image") :
        kundo2_i18n("Change Image Resolution");

    KisProcessingApplicator::ProcessingFlags signalFlags =
        (resolutionChanged || sizeChanged) ?
                KisProcessingApplicator::NO_UI_UPDATES :
                KisProcessingApplicator::NONE;

    KisProcessingApplicator applicator(this, m_d->rootLayer,
                                       KisProcessingApplicator::RECURSIVE | signalFlags,
                                       emitSignals, actionName);

    qreal sx = qreal(size.width()) / this->size().width();
    qreal sy = qreal(size.height()) / this->size().height();

    QTransform shapesCorrection;

    if (resolutionChanged) {
        shapesCorrection = QTransform::fromScale(xRes() / xres, yRes() / yres);
    }

    KisProcessingVisitorSP visitor =
        new KisTransformProcessingVisitor(sx, sy,
                                          0, 0,
                                          QPointF(),
                                          0,
                                          0, 0,
                                          filterStrategy,
                                          shapesCorrection);

    applicator.applyVisitor(visitor, KisStrokeJobData::CONCURRENT);

    if (resolutionChanged) {
        KUndo2Command *parent =
            new KisResetShapesCommand(m_d->rootLayer);
        new KisImageSetResolutionCommand(this, xres, yres, parent);
        applicator.applyCommand(parent);
    }

    if (sizeChanged) {
        applicator.applyCommand(new KisImageResizeCommand(this, size));
    }

    applicator.end();
}

void KisImage::scaleNode(KisNodeSP node, qreal sx, qreal sy, KisFilterStrategy *filterStrategy)
{
    KUndo2MagicString actionName(kundo2_i18n("Scale Layer"));
    KisImageSignalVector emitSignals;
    emitSignals << ModifiedSignal;

    KisProcessingApplicator applicator(this, node,
                                       KisProcessingApplicator::RECURSIVE,
                                       emitSignals, actionName);

    KisProcessingVisitorSP visitor =
        new KisTransformProcessingVisitor(sx, sy,
                                          0, 0,
                                          QPointF(),
                                          0,
                                          0, 0,
                                          filterStrategy);

    applicator.applyVisitor(visitor, KisStrokeJobData::CONCURRENT);
    applicator.end();
}

void KisImage::rotateImpl(const KUndo2MagicString &actionName,
                          KisNodeSP rootNode,
                          bool resizeImage,
                          double radians)
{
    QPointF offset;
    QSize newSize;

    {
        KisTransformWorker worker(0,
                                  1.0, 1.0,
                                  0, 0, 0, 0,
                                  radians,
                                  0, 0, 0, 0);
        QTransform transform = worker.transform();

        if (resizeImage) {
            QRect newRect = transform.mapRect(bounds());
            newSize = newRect.size();
            offset = -newRect.topLeft();
        }
        else {
            QPointF origin = QRectF(rootNode->exactBounds()).center();

            newSize = size();
            offset = -(transform.map(origin) - origin);
        }
    }

    bool sizeChanged = resizeImage &&
        (newSize.width() != width() || newSize.height() != height());

    // These signals will be emitted after processing is done
    KisImageSignalVector emitSignals;
    if (sizeChanged) emitSignals << ComplexSizeChangedSignal(bounds(), newSize);
    emitSignals << ModifiedSignal;

    // These flags determine whether updates are transferred to the UI during processing
    KisProcessingApplicator::ProcessingFlags signalFlags =
        sizeChanged ?
        KisProcessingApplicator::NO_UI_UPDATES :
        KisProcessingApplicator::NONE;


    KisProcessingApplicator applicator(this, rootNode,
                                       KisProcessingApplicator::RECURSIVE | signalFlags,
                                       emitSignals, actionName);

    KisFilterStrategy *filter = KisFilterStrategyRegistry::instance()->value("Bicubic");

    KisProcessingVisitorSP visitor =
            new KisTransformProcessingVisitor(1.0, 1.0, 0.0, 0.0,
                                              QPointF(),
                                              radians,
                                              offset.x(), offset.y(),
                                              filter);

    applicator.applyVisitor(visitor, KisStrokeJobData::CONCURRENT);

    if (sizeChanged) {
        applicator.applyCommand(new KisImageResizeCommand(this, newSize));
    }
    applicator.end();
}


void KisImage::rotateImage(double radians)
{
    rotateImpl(kundo2_i18n("Rotate Image"), root(), true, radians);
}

void KisImage::rotateNode(KisNodeSP node, double radians)
{
    rotateImpl(kundo2_i18n("Rotate Layer"), node, false, radians);
}

void KisImage::shearImpl(const KUndo2MagicString &actionName,
                         KisNodeSP rootNode,
                         bool resizeImage,
                         double angleX, double angleY,
                         const QPointF &origin)
{
    //angleX, angleY are in degrees
    const qreal pi = 3.1415926535897932385;
    const qreal deg2rad = pi / 180.0;

    qreal tanX = tan(angleX * deg2rad);
    qreal tanY = tan(angleY * deg2rad);

    QPointF offset;
    QSize newSize;

    {
        KisTransformWorker worker(0,
                                  1.0, 1.0,
                                  tanX, tanY, origin.x(), origin.y(),
                                  0,
                                  0, 0, 0, 0);

        QRect newRect = worker.transform().mapRect(bounds());
        newSize = newRect.size();
        if (resizeImage) offset = -newRect.topLeft();
    }

    if (newSize == size()) return;

    KisImageSignalVector emitSignals;
    if (resizeImage) emitSignals << ComplexSizeChangedSignal(bounds(), newSize);
    emitSignals << ModifiedSignal;

    KisProcessingApplicator::ProcessingFlags signalFlags =
        KisProcessingApplicator::RECURSIVE;
    if (resizeImage) signalFlags |= KisProcessingApplicator::NO_UI_UPDATES;

    KisProcessingApplicator applicator(this, rootNode,
                                       signalFlags,
                                       emitSignals, actionName);

    KisFilterStrategy *filter = KisFilterStrategyRegistry::instance()->value("Bilinear");

    KisProcessingVisitorSP visitor =
            new KisTransformProcessingVisitor(1.0, 1.0,
                                              tanX, tanY, origin,
                                              0,
                                              offset.x(), offset.y(),
                                              filter);

    applicator.applyVisitor(visitor, KisStrokeJobData::CONCURRENT);

    if (resizeImage) {
        applicator.applyCommand(new KisImageResizeCommand(this, newSize));
    }

    applicator.end();
}

void KisImage::shearNode(KisNodeSP node, double angleX, double angleY)
{
    QPointF shearOrigin = QRectF(bounds()).center();

    shearImpl(kundo2_i18n("Shear layer"), node, false,
              angleX, angleY, shearOrigin);
}

void KisImage::shear(double angleX, double angleY)
{
    shearImpl(kundo2_i18n("Shear Image"), m_d->rootLayer, true,
              angleX, angleY, QPointF());
}

void KisImage::convertImageColorSpace(const KoColorSpace *dstColorSpace,
                                      KoColorConversionTransformation::Intent renderingIntent,
                                      KoColorConversionTransformation::ConversionFlags conversionFlags)
{
    if (!dstColorSpace) return;

    const KoColorSpace *srcColorSpace = m_d->colorSpace;

    undoAdapter()->beginMacro(kundo2_i18n("Convert Image Color Space"));
    undoAdapter()->addCommand(new KisImageLockCommand(KisImageWSP(this), true));
    undoAdapter()->addCommand(new KisImageSetProjectionColorSpaceCommand(KisImageWSP(this), dstColorSpace));

    KisColorSpaceConvertVisitor visitor(this, srcColorSpace, dstColorSpace, renderingIntent, conversionFlags);
    m_d->rootLayer->accept(visitor);

    undoAdapter()->addCommand(new KisImageLockCommand(KisImageWSP(this), false));
    undoAdapter()->endMacro();

    setModified();
}

bool KisImage::assignImageProfile(const KoColorProfile *profile)
{
    if (!profile) return false;

    const KoColorSpace *dstCs = KoColorSpaceRegistry::instance()->colorSpace(colorSpace()->colorModelId().id(), colorSpace()->colorDepthId().id(), profile);
    const KoColorSpace *srcCs = colorSpace();

    if (!dstCs) return false;

    m_d->colorSpace = dstCs;

    KisChangeProfileVisitor visitor(srcCs, dstCs);
    return m_d->rootLayer->accept(visitor);

}

void KisImage::convertProjectionColorSpace(const KoColorSpace *dstColorSpace)
{
    if (*m_d->colorSpace == *dstColorSpace) return;

    undoAdapter()->beginMacro(kundo2_i18n("Convert Projection Color Space"));
    undoAdapter()->addCommand(new KisImageLockCommand(KisImageWSP(this), true));
    undoAdapter()->addCommand(new KisImageSetProjectionColorSpaceCommand(KisImageWSP(this), dstColorSpace));
    undoAdapter()->addCommand(new KisImageLockCommand(KisImageWSP(this), false));
    undoAdapter()->endMacro();

    setModified();
}

void KisImage::setProjectionColorSpace(const KoColorSpace * colorSpace)
{
    m_d->colorSpace = colorSpace;
    m_d->rootLayer->resetCache();
    m_d->signalRouter->emitNotification(ColorSpaceChangedSignal);
}

const KoColorSpace * KisImage::colorSpace() const
{
    return m_d->colorSpace;
}

const KoColorProfile * KisImage::profile() const
{
    return colorSpace()->profile();
}

double KisImage::xRes() const
{
    return m_d->xres;
}

double KisImage::yRes() const
{
    return m_d->yres;
}

void KisImage::setResolution(double xres, double yres)
{
    m_d->xres = xres;
    m_d->yres = yres;
    m_d->signalRouter->emitNotification(ResolutionChangedSignal);
}

QPointF KisImage::documentToPixel(const QPointF &documentCoord) const
{
    return QPointF(documentCoord.x() * xRes(), documentCoord.y() * yRes());
}

QPoint KisImage::documentToIntPixel(const QPointF &documentCoord) const
{
    QPointF pixelCoord = documentToPixel(documentCoord);
    return QPoint((int)pixelCoord.x(), (int)pixelCoord.y());
}

QRectF KisImage::documentToPixel(const QRectF &documentRect) const
{
    return QRectF(documentToPixel(documentRect.topLeft()), documentToPixel(documentRect.bottomRight()));
}

QRect KisImage::documentToIntPixel(const QRectF &documentRect) const
{
    return documentToPixel(documentRect).toAlignedRect();
}

QPointF KisImage::pixelToDocument(const QPointF &pixelCoord) const
{
    return QPointF(pixelCoord.x() / xRes(), pixelCoord.y() / yRes());
}

QPointF KisImage::pixelToDocument(const QPoint &pixelCoord) const
{
    return QPointF((pixelCoord.x() + 0.5) / xRes(), (pixelCoord.y() + 0.5) / yRes());
}

QRectF KisImage::pixelToDocument(const QRectF &pixelCoord) const
{
    return QRectF(pixelToDocument(pixelCoord.topLeft()), pixelToDocument(pixelCoord.bottomRight()));
}

qint32 KisImage::width() const
{
    return m_d->width;
}

qint32 KisImage::height() const
{
    return m_d->height;
}

KisGroupLayerSP KisImage::rootLayer() const
{
    Q_ASSERT(m_d->rootLayer);
    return m_d->rootLayer;
}

KisPaintDeviceSP KisImage::projection() const
{
    if (m_d->isolatedRootNode) {
        return m_d->isolatedRootNode->projection();
    }


    Q_ASSERT(m_d->rootLayer);
    KisPaintDeviceSP projection = m_d->rootLayer->projection();
    Q_ASSERT(projection);
    return projection;
}

qint32 KisImage::nlayers() const
{
    QStringList list;
    list << "KisLayer";

    KisCountVisitor visitor(list, KoProperties());
    m_d->rootLayer->accept(visitor);
    return visitor.count();
}

qint32 KisImage::nHiddenLayers() const
{
    QStringList list;
    list << "KisLayer";
    KoProperties properties;
    properties.setProperty("visible", false);
    KisCountVisitor visitor(list, properties);
    m_d->rootLayer->accept(visitor);

    return visitor.count();
}

QRect realNodeExactBounds(KisNodeSP rootNode, QRect currentRect = QRect())
{
    KisNodeSP node = rootNode->firstChild();

    while(node) {
        currentRect |= realNodeExactBounds(node, currentRect);
        node = node->nextSibling();
    }

    // TODO: it would be better to count up changeRect inside
    // node's extent() method
    currentRect |= rootNode->projectionPlane()->changeRect(rootNode->exactBounds());

    return currentRect;
}

void KisImage::refreshHiddenArea(KisNodeSP rootNode, const QRect &preparedArea)
{
    QRect realNodeRect = realNodeExactBounds(rootNode);
    if (!preparedArea.contains(realNodeRect)) {

        QRegion dirtyRegion = realNodeRect;
        dirtyRegion -= preparedArea;

        foreach(const QRect &rc, dirtyRegion.rects()) {
            refreshGraph(rootNode, rc, realNodeRect);
        }
    }
}

void KisImage::flatten()
{
    KisGroupLayerSP oldRootLayer = m_d->rootLayer;
    KisGroupLayerSP newRootLayer =
        new KisGroupLayer(this, "root", OPACITY_OPAQUE_U8);

    refreshHiddenArea(oldRootLayer, bounds());

    lock();
    KisPaintDeviceSP projectionCopy = new KisPaintDevice(oldRootLayer->projection()->colorSpace());
    projectionCopy->makeCloneFrom(oldRootLayer->projection(), oldRootLayer->exactBounds());
    unlock();

    KisPaintLayerSP flattenLayer =
        new KisPaintLayer(this, nextLayerName(), OPACITY_OPAQUE_U8, projectionCopy);
    Q_CHECK_PTR(flattenLayer);

    addNode(flattenLayer, newRootLayer, 0);

    undoAdapter()->beginMacro(kundo2_i18n("Flatten Image"));
    // NOTE: KisImageChangeLayersCommand performs all the locking for us
    undoAdapter()->addCommand(new KisImageChangeLayersCommand(KisImageWSP(this), oldRootLayer, newRootLayer));
    undoAdapter()->endMacro();

    setModified();
}

bool checkIsSourceForClone(KisNodeSP src, const QList<KisNodeSP> &nodes)
{
    foreach (KisNodeSP node, nodes) {
        if (node == src) continue;

        KisCloneLayer *clone = dynamic_cast<KisCloneLayer*>(node.data());

        if (clone && KisNodeSP(clone->copyFrom()) == src) {
            return true;
        }
    }

    return false;
}

void KisImage::safeRemoveMultipleNodes(QList<KisNodeSP> nodes)
{
    while (!nodes.isEmpty()) {
        QList<KisNodeSP>::iterator it = nodes.begin();

        while (it != nodes.end()) {
            if (!checkIsSourceForClone(*it, nodes)) {
                KisNodeSP node = *it;
                undoAdapter()->addCommand(new KisImageLayerRemoveCommand(this, node));
                it = nodes.erase(it);
            } else {
                ++it;
            }
        }

    }
}

bool checkIsChildOf(KisNodeSP node, const QList<KisNodeSP> &parents)
{
    QList<KisNodeSP> nodeParents;

    KisNodeSP parent = node->parent();
    while (parent) {
        nodeParents << parent;
        parent = parent->parent();
    }

    foreach(KisNodeSP perspectiveParent, parents) {
        if (nodeParents.contains(perspectiveParent)) {
            return true;
        }
    }

    return false;
}

void filterMergableNodes(QList<KisNodeSP> &nodes)
{
    QList<KisNodeSP>::iterator it = nodes.begin();

    while (it != nodes.end()) {
        if (!dynamic_cast<KisLayer*>(it->data()) ||
            checkIsChildOf(*it, nodes)) {

            dbgKrita << "Skipping node" << ppVar((*it)->name());
            it = nodes.erase(it);
        } else {
            ++it;
        }
    }
}

void sortMergableNodes(KisNodeSP root, QList<KisNodeSP> &inputNodes, QList<KisNodeSP> &outputNodes)
{
    QList<KisNodeSP>::iterator it = std::find(inputNodes.begin(), inputNodes.end(), root);

    if (it != inputNodes.end()) {
        outputNodes << *it;
        inputNodes.erase(it);
    }

    if (inputNodes.isEmpty()) {
        return;
    }

    KisNodeSP child = root->firstChild();
    while (child) {
        sortMergableNodes(child, inputNodes, outputNodes);
        child = child->nextSibling();
    }

    /**
     * By the end of recursion \p inputNodes must be empty
     */
    KIS_ASSERT_RECOVER_NOOP(root->parent() || inputNodes.isEmpty());
}

void fetchSelectionMasks(QList<KisNodeSP> mergedNodes, QVector<KisSelectionMaskSP> &selectionMasks)
{
    foreach (KisNodeSP node, mergedNodes) {
        KisLayerSP layer = dynamic_cast<KisLayer*>(node.data());

        KisSelectionMaskSP mask;

        if (layer && (mask = layer->selectionMask())) {
            selectionMasks.append(mask);
        }
    }
}

void reparentSelectionMasks(KisLayerSP newLayer, const QVector<KisSelectionMaskSP> &selectionMasks)
{
    KisImageSP image = newLayer->image();
    foreach (KisSelectionMaskSP mask, selectionMasks) {
        image->undoAdapter()->addCommand(new KisImageLayerMoveCommand(image, mask, newLayer, newLayer->lastChild()));
        image->undoAdapter()->addCommand(new KisActivateSelectionMaskCommand(mask, false));
    }
}

KisNodeSP tryMergeSelectionMasks(KisImageSP image, QList<KisNodeSP> mergedNodes)
{
    if (mergedNodes.isEmpty()) return 0;

    QList<KisSelectionMaskSP> selectionMasks;

    foreach (KisNodeSP node, mergedNodes) {
        KisSelectionMaskSP mask = dynamic_cast<KisSelectionMask*>(node.data());
        if (!mask) return 0;

        selectionMasks.append(mask);
    }

    KisLayerSP parentLayer = dynamic_cast<KisLayer*>(selectionMasks.first()->parent().data());
    KIS_ASSERT_RECOVER(parentLayer) { return 0; }

    KisSelectionSP selection = new KisSelection();

    foreach (KisMaskSP mask, selectionMasks) {
        selection->pixelSelection()->applySelection(
            mask->selection()->pixelSelection(), SELECTION_ADD);
    }

    image->undoAdapter()->beginMacro(kundo2_i18n("Merge Selection Masks"));

    KisSelectionMaskSP mergedMask = new KisSelectionMask(image);
    mergedMask->initSelection(parentLayer);

    image->undoAdapter()->addCommand(new KisImageLayerAddCommand(image, mergedMask, parentLayer, parentLayer->lastChild()));
    mergedMask->setSelection(selection);
    image->undoAdapter()->addCommand(new KisActivateSelectionMaskCommand(mergedMask, true));

    image->safeRemoveMultipleNodes(mergedNodes);

    image->undoAdapter()->endMacro();

    return mergedMask;
}

KisNodeSP KisImage::mergeMultipleLayers(QList<KisNodeSP> mergedNodes, KisNodeSP putAfter)
{
    {
        KisNodeSP mask;
        if ((mask = tryMergeSelectionMasks(this, mergedNodes))) {
            return mask;
        }
    }

    filterMergableNodes(mergedNodes);

    {
        QList<KisNodeSP> tempNodes;
        qSwap(mergedNodes, tempNodes);
        sortMergableNodes(m_d->rootLayer, tempNodes, mergedNodes);
    }

    if (mergedNodes.size() <= 1) return KisNodeSP();

    // fetch selection masks to move them into the destination layer
    QVector<KisSelectionMaskSP> selectionMasks;
    fetchSelectionMasks(mergedNodes, selectionMasks);

    foreach (KisNodeSP layer, mergedNodes) {
        refreshHiddenArea(layer, bounds());
    }

    KisPaintDeviceSP mergedDevice = new KisPaintDevice(colorSpace());
    KisPainter gc(mergedDevice);

    {
        KisImageBarrierLocker l(this);

        foreach (KisNodeSP layer, mergedNodes) {
            QRect rc = layer->exactBounds() | bounds();
            layer->projectionPlane()->apply(&gc, rc);
        }
    }

    const QString mergedLayerSuffix = i18n("Merged");
    QString mergedLayerName = mergedNodes.first()->name();

    if (!mergedLayerName.endsWith(mergedLayerSuffix)) {
        mergedLayerName = QString("%1 %2")
            .arg(mergedLayerName).arg(mergedLayerSuffix);
    }

    KisLayerSP newLayer = new KisPaintLayer(this, mergedLayerName, OPACITY_OPAQUE_U8, mergedDevice);


    undoAdapter()->beginMacro(kundo2_i18n("Merge Selected Nodes"));

    if (!putAfter) {
        putAfter = mergedNodes.last();
    }

    // Add the new merged node on top of the active node -- checking
    // whether the parent is going to be deleted
    KisNodeSP parent = putAfter->parent();
    while (mergedNodes.contains(parent)) {
        parent = parent->parent();
    }

    if (parent == putAfter->parent()) {
        undoAdapter()->addCommand(new KisImageLayerAddCommand(this, newLayer, parent, putAfter));
    } else {
        undoAdapter()->addCommand(new KisImageLayerAddCommand(this, newLayer, parent, parent->lastChild()));
    }

    // reparent selection masks into the newly created node
    reparentSelectionMasks(newLayer, selectionMasks);

    safeRemoveMultipleNodes(mergedNodes);

    undoAdapter()->endMacro();

    return newLayer;
}

KisLayerSP KisImage::mergeDown(KisLayerSP layer, const KisMetaData::MergeStrategy* strategy)
{
    if (!layer->prevSibling()) return 0;

    // XXX: this breaks if we allow free mixing of masks and layers
    KisLayerSP prevLayer = dynamic_cast<KisLayer*>(layer->prevSibling().data());
    if (!prevLayer) return 0;

    refreshHiddenArea(layer, bounds());
    refreshHiddenArea(prevLayer, bounds());

    QVector<KisSelectionMaskSP> selectionMasks;
    {
        QList<KisNodeSP> mergedNodes;
        mergedNodes << layer;
        mergedNodes << prevLayer;
        fetchSelectionMasks(mergedNodes, selectionMasks);
    }

    QRect layerProjectionExtent = this->projection()->extent();
    QRect prevLayerProjectionExtent = prevLayer->projection()->extent();

    // actual merging done by KisLayer::createMergedLayer (or specialized decendant)
    KisLayerSP mergedLayer = layer->createMergedLayer(prevLayer);
    Q_CHECK_PTR(mergedLayer);

    // Merge meta data
    QList<const KisMetaData::Store*> srcs;
    srcs.append(prevLayer->metaData());
    srcs.append(layer->metaData());
    QList<double> scores;
    int prevLayerArea = prevLayerProjectionExtent.width() * prevLayerProjectionExtent.height();
    int layerArea = layerProjectionExtent.width() * layerProjectionExtent.height();
    double norm = qMax(prevLayerArea, layerArea);
    scores.append(prevLayerArea / norm);
    scores.append(layerArea / norm);
    strategy->merge(mergedLayer->metaData(), srcs, scores);

    KisNodeSP parent = layer->parent(); // parent is set to null when the layer is removed from the node
    dbgImage << ppVar(parent);

    // FIXME: "Merge Down"?
    undoAdapter()->beginMacro(kundo2_i18n("Merge with Layer Below"));

    undoAdapter()->addCommand(new KisImageLayerAddCommand(this, mergedLayer, parent, layer));

    // reparent selection masks into the newly created node
    reparentSelectionMasks(mergedLayer, selectionMasks);

    safeRemoveTwoNodes(layer, prevLayer);

    undoAdapter()->endMacro();

    return mergedLayer;
}

/**
 * The removal of two nodes in one go may be a bit tricky, because one
 * of them may be the clone of another. If we remove the source of a
 * clone layer, it will reincarnate into a paint layer. In this case
 * the pointer to the second layer will be lost.
 *
 * That's why we need to care about the order of the nodes removal:
 * the clone --- first, the source --- last.
 */
void KisImage::safeRemoveTwoNodes(KisNodeSP node1, KisNodeSP node2)
{
    KisCloneLayer *clone1 = dynamic_cast<KisCloneLayer*>(node1.data());

    if (clone1 && KisNodeSP(clone1->copyFrom()) == node2) {
        undoAdapter()->addCommand(new KisImageLayerRemoveCommand(this, node1));
        undoAdapter()->addCommand(new KisImageLayerRemoveCommand(this, node2));
    } else {
        undoAdapter()->addCommand(new KisImageLayerRemoveCommand(this, node2));
        undoAdapter()->addCommand(new KisImageLayerRemoveCommand(this, node1));
    }
}

KisLayerSP KisImage::flattenLayer(KisLayerSP layer)
{
    if (!layer->firstChild()) return layer;

    refreshHiddenArea(layer, bounds());

    bool resetComposition = false;

    KisPaintDeviceSP mergedDevice;

    lock();
    if (layer->layerStyle()) {
        mergedDevice = new KisPaintDevice(layer->colorSpace());
        mergedDevice->prepareClone(layer->projection());

        QRect updateRect = layer->projection()->extent() | bounds();

        KisPainter gc(mergedDevice);
        layer->projectionPlane()->apply(&gc, updateRect);

        resetComposition = true;

    } else {
        mergedDevice = new KisPaintDevice(*layer->projection());
    }
    unlock();

    KisPaintLayerSP newLayer = new KisPaintLayer(this, layer->name(), layer->opacity(), mergedDevice);

    if (!resetComposition) {
        newLayer->setCompositeOp(layer->compositeOp()->id());
        newLayer->setChannelFlags(layer->channelFlags());
    }

    undoAdapter()->beginMacro(kundo2_i18n("Flatten Layer"));
    undoAdapter()->addCommand(new KisImageLayerAddCommand(this, newLayer, layer->parent(), layer));
    undoAdapter()->addCommand(new KisImageLayerRemoveCommand(this, layer));

    QList<const KisMetaData::Store*> srcs;
    srcs.append(layer->metaData());

    const KisMetaData::MergeStrategy* strategy = KisMetaData::MergeStrategyRegistry::instance()->get("Smart");
    QList<double> scores;
    scores.append(1.0); //Just give some score, there only is one layer
    strategy->merge(newLayer->metaData(), srcs, scores);

    undoAdapter()->endMacro();

    return newLayer;
}


void KisImage::setModified()
{
    m_d->signalRouter->emitNotification(ModifiedSignal);
}

QImage KisImage::convertToQImage(QRect imageRect,
                                 const KoColorProfile * profile)
{
    qint32 x;
    qint32 y;
    qint32 w;
    qint32 h;
    imageRect.getRect(&x, &y, &w, &h);
    return convertToQImage(x, y, w, h, profile);
}

QImage KisImage::convertToQImage(qint32 x,
                                 qint32 y,
                                 qint32 w,
                                 qint32 h,
                                 const KoColorProfile * profile)
{
    KisPaintDeviceSP dev = projection();
    if (!dev) return QImage();
    QImage image = dev->convertToQImage(const_cast<KoColorProfile*>(profile), x, y, w, h,
                                        KoColorConversionTransformation::InternalRenderingIntent,
                                        KoColorConversionTransformation::InternalConversionFlags);

    return image;
}



QImage KisImage::convertToQImage(const QRect& scaledRect, const QSize& scaledImageSize, const KoColorProfile *profile)
{

    if (scaledRect.isEmpty() || scaledImageSize.isEmpty()) {
        return QImage();
    }

    try {
        qint32 imageWidth = width();
        qint32 imageHeight = height();
        quint32 pixelSize = colorSpace()->pixelSize();

        double xScale = static_cast<double>(imageWidth) / scaledImageSize.width();
        double yScale = static_cast<double>(imageHeight) / scaledImageSize.height();

        QRect srcRect;

        srcRect.setLeft(static_cast<int>(scaledRect.left() * xScale));
        srcRect.setRight(static_cast<int>(ceil((scaledRect.right() + 1) * xScale)) - 1);
        srcRect.setTop(static_cast<int>(scaledRect.top() * yScale));
        srcRect.setBottom(static_cast<int>(ceil((scaledRect.bottom() + 1) * yScale)) - 1);

        KisPaintDeviceSP mergedImage = projection();
        quint8 *scaledImageData = new quint8[scaledRect.width() * scaledRect.height() * pixelSize];

        quint8 *imageRow = new quint8[srcRect.width() * pixelSize];
        const qint32 imageRowX = srcRect.x();

        for (qint32 y = 0; y < scaledRect.height(); ++y) {

            qint32 dstY = scaledRect.y() + y;
            qint32 dstX = scaledRect.x();
            qint32 srcY = (dstY * imageHeight) / scaledImageSize.height();

            mergedImage->readBytes(imageRow, imageRowX, srcY, srcRect.width(), 1);

            quint8 *dstPixel = scaledImageData + (y * scaledRect.width() * pixelSize);
            quint32 columnsRemaining = scaledRect.width();

            while (columnsRemaining > 0) {

                qint32 srcX = (dstX * imageWidth) / scaledImageSize.width();

                memcpy(dstPixel, imageRow + ((srcX - imageRowX) * pixelSize), pixelSize);

                ++dstX;
                dstPixel += pixelSize;
                --columnsRemaining;
            }
        }
        delete [] imageRow;

        QImage image = colorSpace()->convertToQImage(scaledImageData, scaledRect.width(), scaledRect.height(), const_cast<KoColorProfile*>(profile),
                                                     KoColorConversionTransformation::InternalRenderingIntent,
                                                     KoColorConversionTransformation::InternalConversionFlags);

        delete [] scaledImageData;

        return image;
    }
    catch (std::bad_alloc) {
        warnKrita << "KisImage::convertToQImage ran out of memory";
        return QImage();
    }
}

void KisImage::notifyLayersChanged()
{
    m_d->signalRouter->emitNotification(LayersChangedSignal);
}

QRect KisImage::bounds() const
{
    return QRect(0, 0, width(), height());
}

KisPostExecutionUndoAdapter* KisImage::postExecutionUndoAdapter() const
{
    return m_d->postExecutionUndoAdapter;
}

void KisImage::setUndoStore(KisUndoStore *undoStore)
{

    m_d->legacyUndoAdapter->setUndoStore(undoStore);
    m_d->postExecutionUndoAdapter->setUndoStore(undoStore);
    delete m_d->undoStore;
    m_d->undoStore = undoStore;
}

KisUndoStore* KisImage::undoStore()
{
    return m_d->undoStore;
}

KisUndoAdapter* KisImage::undoAdapter() const
{
    return m_d->legacyUndoAdapter;
}

KisActionRecorder* KisImage::actionRecorder() const
{
    return m_d->recorder;
}

void KisImage::setDefaultProjectionColor(const KoColor &color)
{
    KIS_ASSERT_RECOVER_RETURN(m_d->rootLayer);
    m_d->rootLayer->setDefaultProjectionColor(color);
}

KoColor KisImage::defaultProjectionColor() const
{
    KIS_ASSERT_RECOVER(m_d->rootLayer) {
        return KoColor(Qt::transparent, m_d->colorSpace);
    }

    return m_d->rootLayer->defaultProjectionColor();
}

void KisImage::setRootLayer(KisGroupLayerSP rootLayer)
{
    stopIsolatedMode();

    KoColor defaultProjectionColor(Qt::transparent, m_d->colorSpace);

    if (m_d->rootLayer) {
        m_d->rootLayer->setGraphListener(0);
        m_d->rootLayer->disconnect();

        KisPaintDeviceSP original = m_d->rootLayer->original();
        defaultProjectionColor.setColor(original->defaultPixel(), original->colorSpace());
    }

    m_d->rootLayer = rootLayer;
    m_d->rootLayer->disconnect();
    m_d->rootLayer->setGraphListener(this);

    KisPaintDeviceSP newOriginal = m_d->rootLayer->original();
    defaultProjectionColor.convertTo(newOriginal->colorSpace());
    newOriginal->setDefaultPixel(defaultProjectionColor.data());

    setRoot(m_d->rootLayer.data());
}

void KisImage::addAnnotation(KisAnnotationSP annotation)
{
    // Find the icc annotation, if there is one
    vKisAnnotationSP_it it = m_d->annotations.begin();
    while (it != m_d->annotations.end()) {
        if ((*it)->type() == annotation->type()) {
            *it = annotation;
            return;
        }
        ++it;
    }
    m_d->annotations.push_back(annotation);
}

KisAnnotationSP KisImage::annotation(const QString& type)
{
    vKisAnnotationSP_it it = m_d->annotations.begin();
    while (it != m_d->annotations.end()) {
        if ((*it)->type() == type) {
            return *it;
        }
        ++it;
    }
    return KisAnnotationSP(0);
}

void KisImage::removeAnnotation(const QString& type)
{
    vKisAnnotationSP_it it = m_d->annotations.begin();
    while (it != m_d->annotations.end()) {
        if ((*it)->type() == type) {
            m_d->annotations.erase(it);
            return;
        }
        ++it;
    }
}

vKisAnnotationSP_it KisImage::beginAnnotations()
{
    return m_d->annotations.begin();
}

vKisAnnotationSP_it KisImage::endAnnotations()
{
    return m_d->annotations.end();
}

void KisImage::notifyAboutToBeDeleted()
{
    emit sigAboutToBeDeleted();
}

KisPerspectiveGrid* KisImage::perspectiveGrid()
{
    if (m_d->perspectiveGrid == 0)
        m_d->perspectiveGrid = new KisPerspectiveGrid();
    return m_d->perspectiveGrid;
}

KisImageSignalRouter* KisImage::signalRouter()
{
    return m_d->signalRouter;
}

void KisImage::waitForDone()
{
    requestStrokeEnd();

    if (m_d->scheduler) {
        m_d->scheduler->waitForDone();
    }
}

KisStrokeId KisImage::startStroke(KisStrokeStrategy *strokeStrategy)
{
    /**
     * Ask open strokes to end gracefully. All the strokes clients
     * (including the one calling this method right now) will get
     * a notification that they should probably end their strokes.
     * However this is purely their choice whether to end a stroke
     * or not.
     */
    requestStrokeEnd();

    /**
     * Some of the strokes can cancel their work with undoing all the
     * changes they did to the paint devices. The problem is that undo
     * stack will know nothing about it. Therefore, just notify it
     * explicitly
     */
    if (strokeStrategy->clearsRedoOnStart()) {
        m_d->undoStore->purgeRedoState();
    }

    KisStrokeId id;

    if (m_d->scheduler) {
        id = m_d->scheduler->startStroke(strokeStrategy);
    }

    return id;
}

void KisImage::KisImagePrivate::notifyProjectionUpdatedInPatches(const QRect &rc)
{
    KisImageConfig imageConfig;
    int patchWidth = imageConfig.updatePatchWidth();
    int patchHeight = imageConfig.updatePatchHeight();

    for (int y = 0; y < rc.height(); y += patchHeight) {
        for (int x = 0; x < rc.width(); x += patchWidth) {
            QRect patchRect(x, y, patchWidth, patchHeight);
            patchRect &= rc;

            QtConcurrent::run(boost::bind(&KisImage::notifyProjectionUpdated, q, patchRect));
        }
    }
}

bool KisImage::startIsolatedMode(KisNodeSP node)
{
    if (!tryBarrierLock()) return false;

    unlock();

    m_d->isolatedRootNode = node;
    emit sigIsolatedModeChanged();

    // the GUI uses our thread to do the color space conversion so we
    // need to emit this signal in multiple threads
    m_d->notifyProjectionUpdatedInPatches(bounds());

    return true;
}

void KisImage::stopIsolatedMode()
{
    if (!m_d->isolatedRootNode)  return;

    KisNodeSP oldRootNode = m_d->isolatedRootNode;
    m_d->isolatedRootNode = 0;

    emit sigIsolatedModeChanged();

    // the GUI uses our thread to do the color space conversion so we
    // need to emit this signal in multiple threads
    m_d->notifyProjectionUpdatedInPatches(bounds());

    // TODO: Substitute notifyProjectionUpdated() with this code
    // when update optimization is implemented
    //
    // QRect updateRect = bounds() | oldRootNode->extent();
    // oldRootNode->setDirty(updateRect);
}

KisNodeSP KisImage::isolatedModeRoot() const
{
    return m_d->isolatedRootNode;
}

void KisImage::addJob(KisStrokeId id, KisStrokeJobData *data)
{
    KisUpdateTimeMonitor::instance()->reportJobStarted(data);

    if (m_d->scheduler) {
        m_d->scheduler->addJob(id, data);
    }
}

void KisImage::endStroke(KisStrokeId id)
{
    if (m_d->scheduler) {
        m_d->scheduler->endStroke(id);
    }
}

bool KisImage::cancelStroke(KisStrokeId id)
{
    bool result = false;
    if (m_d->scheduler) {
        result = m_d->scheduler->cancelStroke(id);
    }
    return result;
}

bool KisImage::KisImagePrivate::tryCancelCurrentStrokeAsync()
{
    bool result = false;
    if (scheduler) {
        result = scheduler->tryCancelCurrentStrokeAsync();
    }
    return result;
}

void KisImage::requestUndoDuringStroke()
{
    emit sigUndoDuringStrokeRequested();
}

void KisImage::requestStrokeCancellation()
{
    if (!m_d->tryCancelCurrentStrokeAsync()) {
        emit sigStrokeCancellationRequested();
    }
}

void KisImage::requestStrokeEnd()
{
    emit sigStrokeEndRequested();
}

void KisImage::refreshGraph(KisNodeSP root)
{
    refreshGraph(root, bounds(), bounds());
}

void KisImage::refreshGraph(KisNodeSP root, const QRect &rc, const QRect &cropRect)
{
    if (!root) root = m_d->rootLayer;

    if (m_d->scheduler) {
        m_d->scheduler->fullRefresh(root, rc, cropRect);
    }
}

void KisImage::initialRefreshGraph()
{
    /**
     * NOTE: Tricky part. We set crop rect to null, so the clones
     * will not rely on precalculated projections of their sources
     */

    refreshGraphAsync(0, bounds(), QRect());
    waitForDone();
}

void KisImage::refreshGraphAsync(KisNodeSP root)
{
    refreshGraphAsync(root, bounds(), bounds());
}

void KisImage::refreshGraphAsync(KisNodeSP root, const QRect &rc)
{
    refreshGraphAsync(root, rc, bounds());
}

void KisImage::refreshGraphAsync(KisNodeSP root, const QRect &rc, const QRect &cropRect)
{
    if (!root) root = m_d->rootLayer;

    if (m_d->scheduler) {
        m_d->scheduler->fullRefreshAsync(root, rc, cropRect);
    }
}

void KisImage::requestProjectionUpdateNoFilthy(KisNodeSP pseudoFilthy, const QRect &rc, const QRect &cropRect)
{
    KIS_ASSERT_RECOVER_RETURN(pseudoFilthy);

    if (m_d->scheduler) {
        m_d->scheduler->updateProjectionNoFilthy(pseudoFilthy, rc, cropRect);
    }
}

void KisImage::addSpontaneousJob(KisSpontaneousJob *spontaneousJob)
{
    if (m_d->scheduler) {
        m_d->scheduler->addSpontaneousJob(spontaneousJob);
    }
}

void KisImage::disableDirtyRequests()
{
    m_d->disableDirtyRequests.ref();
}

void KisImage::enableDirtyRequests()
{
    m_d->disableDirtyRequests.deref();
}

void KisImage::disableUIUpdates()
{
    m_d->disableUIUpdateSignals.ref();
}

void KisImage::enableUIUpdates()
{
    m_d->disableUIUpdateSignals.deref();
}

void KisImage::notifyProjectionUpdated(const QRect &rc)
{
    KisUpdateTimeMonitor::instance()->reportUpdateFinished(rc);

    if (!m_d->disableUIUpdateSignals) {
        emit sigImageUpdated(rc);
    }
}

void KisImage::notifySelectionChanged()
{
    /**
     * The selection is calculated asynchromously, so it is not
     * handled by disableUIUpdates() and other special signals of
     * KisImageSignalRouter
     */
    m_d->legacyUndoAdapter->emitSelectionChanged();

    /**
     * Editing of selection masks doesn't necessary produce a
     * setDirty() call, so in the end of the stroke we need to request
     * direct update of the UI's cache.
     */
    if (m_d->isolatedRootNode &&
        dynamic_cast<KisSelectionMask*>(m_d->isolatedRootNode.data())) {

        notifyProjectionUpdated(bounds());
    }
}

void KisImage::requestProjectionUpdateImpl(KisNode *node,
                                           const QRect &rect,
                                           const QRect &cropRect)
{
    KisNodeGraphListener::requestProjectionUpdate(node, rect);

    if (m_d->scheduler) {
        m_d->scheduler->updateProjection(node, rect, cropRect);
    }
}

void KisImage::requestProjectionUpdate(KisNode *node, const QRect& rect)
{
    if (m_d->disableDirtyRequests) return;

    /**
     * Here we use 'permitted' instead of 'active' intentively,
     * because the updates may come after the actual stroke has been
     * finished. And having some more updates for the stroke not
     * supporting the wrap-around mode will not make much harm.
     */
    if (m_d->wrapAroundModePermitted) {
        QRect boundRect = bounds();
        KisWrappedRect splitRect(rect, boundRect);

        foreach (const QRect &rc, splitRect) {
            requestProjectionUpdateImpl(node, rc, boundRect);
        }
    } else {
        requestProjectionUpdateImpl(node, rect, bounds());
    }
}

QList<KisLayerComposition*> KisImage::compositions()
{
    return m_d->compositions;
}

void KisImage::addComposition(KisLayerComposition* composition)
{
    m_d->compositions.append(composition);
}

void KisImage::removeComposition(KisLayerComposition* composition)
{
    m_d->compositions.removeAll(composition);
    delete composition;
}

bool checkMasksNeedConversion(KisNodeSP root, const QRect &bounds)
{
    KisSelectionMask *mask = dynamic_cast<KisSelectionMask*>(root.data());
    if (mask &&
        (!bounds.contains(mask->paintDevice()->exactBounds()) ||
         mask->selection()->hasShapeSelection())) {

        return true;
    }

    KisNodeSP node = root->firstChild();

    while (node) {
        if (checkMasksNeedConversion(node, bounds)) {
            return true;
        }

        node = node->nextSibling();
    }

    return false;
}

void KisImage::setWrapAroundModePermitted(bool value)
{
    m_d->wrapAroundModePermitted = value;

    if (m_d->wrapAroundModePermitted &&
        checkMasksNeedConversion(root(), bounds())) {

        KisProcessingApplicator applicator(this, root(),
                                           KisProcessingApplicator::RECURSIVE,
                                           KisImageSignalVector() << ModifiedSignal,
                                           kundo2_i18n("Crop Selections"));

        KisProcessingVisitorSP visitor =
            new KisCropSelectionsProcessingVisitor(bounds());

        applicator.applyVisitor(visitor, KisStrokeJobData::CONCURRENT);
        applicator.end();
    }
}

bool KisImage::wrapAroundModePermitted() const
{
    return m_d->wrapAroundModePermitted;
}

bool KisImage::wrapAroundModeActive() const
{
    return m_d->wrapAroundModePermitted && m_d->scheduler &&
        m_d->scheduler->wrapAroundModeSupported();
}

void KisImage::notifyNodeCollpasedChanged()
{
    emit sigNodeCollapsedChanged();
}



