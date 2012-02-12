/*
 * Copyright (C) 2012 Cyrille Berger <cberger@cberger.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 sure
 * USA
 */

#include <QDeclarativeItem>
#include "kis_color_selector_interface.h"
#include <KoColor.h>

class KoCanvasResourceManager;
class KisColorSelectorComponent;

class KisColorSelectorItem : public QDeclarativeItem, public KisColorSelectorInterface
{
    Q_OBJECT
    Q_PROPERTY (KoCanvasResourceManager* resourceManager READ resourceManager WRITE setResourceManager)
public:
    KisColorSelectorItem ( QDeclarativeItem * parent = 0 );
    virtual const KoColorSpace* colorSpace() const;
    virtual KoCanvasResourceManager* resourceManager() const;
    void setResourceManager(KoCanvasResourceManager* _canvasResourceManager);
protected:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* , QWidget* );
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
private:
    KisColorSelectorComponent* m_component;
    KoCanvasResourceManager* m_resourceManager;
    QSize m_lastSize;
};
