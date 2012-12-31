/*
 *  Copyright (c) 2010-2011 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  Copyright (c) 2012 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_experiment_paintop.h"
#include "kis_experiment_paintop_settings.h"

#include <cmath>

#include <KoCompositeOp.h>
#include <KoColorSpace.h>

#include <kis_debug.h>

#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_image.h>


KisExperimentPaintOp::KisExperimentPaintOp(const KisExperimentPaintOpSettings *settings, KisPainter * painter, KisImageSP image)
    : KisPaintOp( painter )
{
    Q_UNUSED(image);
    m_firstRun = true;

    m_experimentOption.readOptionSetting(settings);

    m_displaceEnabled = m_experimentOption.isDisplacementEnabled;
    m_displaceCoeff = (m_experimentOption.displacement * 0.01 * 14) + 1; // 1..15 [7 default according alchemy]

    m_speedEnabled = m_experimentOption.isSpeedEnabled;
    m_speedMultiplier = (m_experimentOption.speed * 0.01 * 35); // 0..35 [15 default according alchemy]
    m_smoothingEnabled = m_experimentOption.isSmoothingEnabled;
    m_smoothingThreshold = m_experimentOption.smoothing;

    m_useMirroring = painter->hasMirroring();

    if (m_useMirroring) {
        m_originalDevice = new KisPaintDevice(painter->device()->colorSpace());
        m_originalPainter = new KisPainter(m_originalDevice);
        m_originalPainter->setCompositeOp(COMPOSITE_COPY);
        m_originalPainter->setPaintColor(painter->paintColor());
        m_originalPainter->setFillStyle(KisPainter::FillStyleForegroundColor);
    } else {
        m_originalPainter = 0;
    }
}

KisExperimentPaintOp::~KisExperimentPaintOp()
{
    delete m_originalPainter;
}

bool checkInTriangle(const QRectF &rect,
                     const QPolygonF &triangle)
{
    return triangle.intersected(rect).boundingRect().isValid();
}


QRegion splitTriangles(const QPointF &center,
                       QVector<QPointF> points)
{
    Q_ASSERT(points.size());
    Q_ASSERT(!(points.size() & 1));

    QVector<QPolygonF> triangles;
    QRect totalRect;

    for (int i = 0; i < points.size(); i += 2) {
        QPolygonF triangle;
        triangle << center;
        triangle << points[i];
        triangle << points[i+1];

        totalRect |= triangle.boundingRect().toAlignedRect();
        triangles << triangle;
    }


    const int step = 64;
    const int right = totalRect.x() + totalRect.width();
    const int bottom = totalRect.y() + totalRect.height();

    QRegion dirtyRegion;

    for (int y = totalRect.y(); y < bottom;) {
        int nextY = qMin((y + step) & ~(step-1), bottom);

        for (int x = totalRect.x(); x < right;) {
            int nextX = qMin((x + step) & ~(step-1), right);

            QRect rect(x, y, nextX - x, nextY - y);

            foreach(const QPolygonF &triangle, triangles) {
                if(checkInTriangle(rect, triangle)) {
                    dirtyRegion |= rect;
                    break;
                }
            }

            x = nextX;
        }
        y = nextY;
    }
    return dirtyRegion;
}

QRegion splitPath(QPainterPath path)
{
    QRect totalRect = path.boundingRect().toAlignedRect();
    totalRect.adjusted(-1,-1,1,1);

    const int step = 64;
    const int right = totalRect.x() + totalRect.width();
    const int bottom = totalRect.y() + totalRect.height();

    QRegion dirtyRegion;


    for (int y = totalRect.y(); y < bottom;) {
        int nextY = qMin((y + step) & ~(step-1), bottom);

        for (int x = totalRect.x(); x < right;) {
            int nextX = qMin((x + step) & ~(step-1), right);

            QRect rect(x, y, nextX - x, nextY - y);

            if(path.intersects(rect)) {
                dirtyRegion |= rect;
            }

            x = nextX;
        }
        y = nextY;
    }

    return dirtyRegion;
}

void KisExperimentPaintOp::paintRegion(const QRegion &changedRegion)
{
    if (m_useMirroring) {
        foreach(const QRect &rect, changedRegion.rects()) {
            m_originalPainter->fillPainterPath(m_path, rect);
            painter()->renderDabWithMirroringNonIncremental(rect, m_originalDevice);
        }
    } else {
        painter()->setFillStyle(KisPainter::FillStyleForegroundColor);
        painter()->setCompositeOp(COMPOSITE_COPY);

        foreach(const QRect &rect, changedRegion.rects()) {
            painter()->fillPainterPath(m_path, rect);
        }
    }
}

QPointF KisExperimentPaintOp::speedCorrectedPosition(const KisPaintInformation& pi1,
                                                     const KisPaintInformation& pi2)
{
    const qreal fadeFactor = 0.6;

    QPointF diff = pi2.pos() - pi1.pos();
    qreal realLength = sqrt(diff.x() * diff.x() + diff.y() * diff.y());

    if (realLength < 0.1) return pi2.pos();

    qreal coeff = 0.5 * realLength * m_speedMultiplier;
    m_savedSpeedCoeff = fadeFactor * m_savedSpeedCoeff + (1 - fadeFactor) * coeff;
    QPointF newPoint = pi1.pos() + diff * m_savedSpeedCoeff / realLength;
    m_savedSpeedPoint = fadeFactor * m_savedSpeedPoint + (1 - fadeFactor) * newPoint;

    return m_savedSpeedPoint;
}

KisDistanceInformation KisExperimentPaintOp::paintLine(const KisPaintInformation& pi1, const KisPaintInformation& pi2, const KisDistanceInformation& /*savedDist*/)
{
    KisDistanceInformation kdi(0,0);
    if (!painter()) return kdi;

    if (m_firstRun){
        m_firstRun = false;

        m_path.moveTo(pi1.pos());
        m_path.lineTo(pi2.pos());

        m_center = pi1.pos();

        m_savedUpdateDistance = 0;
        m_lastPaintTime = 0;

        m_savedSpeedCoeff = 0;
        m_savedSpeedPoint = m_center;

        m_savedSmoothingDistance = 0;
        m_savedSmoothingPoint = m_center;

    }else{

        const QPointF pos1 = pi1.pos();
        QPointF pos2 = pi2.pos();

        if (m_speedEnabled) {
            pos2 = speedCorrectedPosition(pi1, pi2);
        }

        int length = (pos2 - pos1).manhattanLength();
        m_savedUpdateDistance += length;

        if (m_smoothingEnabled) {
            m_savedSmoothingDistance += length;

            if (m_savedSmoothingDistance > m_smoothingThreshold) {
                QPointF pt = (m_savedSmoothingPoint + pos2) * 0.5;

                // for updates approximate curve with two lines
                m_savedPoints << m_path.currentPosition();
                m_savedPoints << m_savedSmoothingPoint;
                m_savedPoints << m_savedSmoothingPoint;
                m_savedPoints << pt;

                m_path.quadTo(m_savedSmoothingPoint, pt);
                m_savedSmoothingPoint = pos2;

                m_savedSmoothingDistance = 0;
            }
        } else {
            m_path.lineTo(pos2);
            m_savedPoints << pos1;
            m_savedPoints << pos2;
        }

        if (m_displaceEnabled) {
            if (m_path.elementCount() % 16 == 0) {
                QRectF bounds = m_path.boundingRect();
                m_path = applyDisplace(m_path, m_displaceCoeff - length);
                bounds |= m_path.boundingRect();

                qreal threshold = simplifyThreshold(bounds);
                m_path = trySimplifyPath(m_path, threshold);
            }
            else {
                m_path = applyDisplace(m_path, m_displaceCoeff - length);
            }
        }

        /**
         * Refresh rate at least 25fps
         */
        const int timeThreshold = 40;
        const int elapsedTime = pi2.currentTime() - m_lastPaintTime;

        QRect pathBounds = m_path.boundingRect().toRect();
        int distanceMetric = qMax(pathBounds.width(), pathBounds.height());

        if(elapsedTime > timeThreshold ||
           (!m_displaceEnabled &&
            m_savedUpdateDistance > distanceMetric / 8)) {

            if (m_displaceEnabled) {
                /**
                 * Rendering the path with diff'ed rects is up to two
                 * times more efficient for really huge shapes (tested
                 * on 2000+ px shapes), however for smaller ones doing
                 * paths arithmetics eats too much time. That's why we
                 * choose the method on the base of the size of the
                 * shape.
                 */
                const int pathSizeThreshold = 128;

                QRegion changedRegion;
                if (distanceMetric < pathSizeThreshold) {

                    QRectF changedRect = m_path.boundingRect().toRect() |
                        m_lastPaintedPath.boundingRect().toRect();
                    changedRect.adjust(-1,-1,1,1);

                    changedRegion = changedRect.toRect();
                } else {
                    QPainterPath diff1 = m_path - m_lastPaintedPath;
                    QPainterPath diff2 = m_lastPaintedPath - m_path;

                    changedRegion = splitPath(diff1 | diff2);
                }

                paintRegion(changedRegion);
                m_lastPaintedPath = m_path;
            } else if (!m_savedPoints.isEmpty()) {
                QRegion changedRegion = splitTriangles(m_center, m_savedPoints);
                paintRegion(changedRegion);
            }

            m_savedPoints.clear();
            m_savedUpdateDistance = 0;
            m_lastPaintTime = pi2.currentTime();
        }
    }

    return kdi;
}


qreal KisExperimentPaintOp::paintAt(const KisPaintInformation& info)
{
    Q_UNUSED(info);
    return 1.0;
}

bool tryMergePoints(QPainterPath &path,
                    const QPointF &startPoint,
                    const QPointF &endPoint,
                    qreal &distance,
                    qreal distanceThreshold,
                    bool lastSegment)
{
    qreal length = (endPoint - startPoint).manhattanLength();

    if (lastSegment || length > distanceThreshold) {
        if (distance != 0) {
            path.lineTo(startPoint);
        }
        distance = 0;
        return false;
    }

    distance += length;

    if (distance > distanceThreshold) {
        path.lineTo(endPoint);
        distance = 0;
    }

    return true;
}

qreal KisExperimentPaintOp::simplifyThreshold(const QRectF &bounds)
{
    qreal maxDimension = qMax(bounds.width(), bounds.height());
    return qMax(0.01 * maxDimension, 1.0);
}

QPainterPath KisExperimentPaintOp::trySimplifyPath(const QPainterPath &path, qreal lengthThreshold)
{
    QPainterPath newPath;
    QPointF startPoint;
    qreal distance = 0;

    int count = path.elementCount();
    for (int i = 0; i < count; i++){
        QPainterPath::Element e = path.elementAt(i);
        QPointF endPoint = QPointF(e.x, e.y);

        switch(e.type){
        case QPainterPath::MoveToElement:
            newPath.moveTo(endPoint);
            break;
        case QPainterPath::LineToElement:
            if (!tryMergePoints(newPath, startPoint, endPoint,
                                distance, lengthThreshold, i == count - 1)) {

                newPath.lineTo(endPoint);
            }
            break;
        case QPainterPath::CurveToElement:{
            Q_ASSERT(i + 2 < count);

            if (!tryMergePoints(newPath, startPoint, endPoint,
                                distance, lengthThreshold, i == count - 1)) {

                e = path.elementAt(i + 1);
                Q_ASSERT(e.type == QPainterPath::CurveToDataElement);
                QPointF ctrl1 = QPointF(e.x, e.y);
                e = path.elementAt(i + 2);
                Q_ASSERT(e.type == QPainterPath::CurveToDataElement);
                QPointF ctrl2 = QPointF(e.x, e.y);
                newPath.cubicTo(ctrl1, ctrl2, endPoint);
            }

            i += 2;
        }
        }
        startPoint = endPoint;
    }

    return newPath;
}

QPointF KisExperimentPaintOp::getAngle(const QPointF& p1, const QPointF& p2, qreal distance)
{
    QPointF diff = p1 - p2;
    qreal realLength = sqrt(diff.x() * diff.x() + diff.y() * diff.y());
    return realLength > 0.5 ? p1 + diff * distance / realLength : p1;
}

QPainterPath KisExperimentPaintOp::applyDisplace(const QPainterPath& path, int speed)
{
    QPointF lastPoint = path.currentPosition();

    QPainterPath newPath;
    int count = path.elementCount();
    int curveElementCounter = 0;
    QPointF ctrl1;
    QPointF ctrl2;
    QPointF endPoint;
    for (int i = 0; i < count; i++){
        QPainterPath::Element e = path.elementAt(i);
        switch(e.type){
            case QPainterPath::MoveToElement:{
                newPath.moveTo(getAngle(QPointF(e.x,e.y),lastPoint,speed));
                break;
            }
            case QPainterPath::LineToElement:{
                newPath.lineTo(getAngle(QPointF(e.x,e.y),lastPoint,speed));
                break;
            }
            case QPainterPath::CurveToElement:{
                curveElementCounter = 0;
                endPoint = getAngle(QPointF(e.x,e.y),lastPoint,speed);
                break;
            }
            case QPainterPath::CurveToDataElement:{
                curveElementCounter++;

                if (curveElementCounter == 1){
                    ctrl1 = getAngle(QPointF(e.x,e.y),lastPoint,speed);
                }
                else if (curveElementCounter == 2){
                    ctrl2 = getAngle(QPointF(e.x,e.y),lastPoint,speed);
                    newPath.cubicTo(ctrl1,ctrl2,endPoint);
                }
                break;
            }
        }

    }// for

    return newPath;
}

