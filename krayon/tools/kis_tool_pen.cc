/*
 *  kis_tool_pen.cc - part of Krayon
 *
 *  Copyright (c) 1999 Matthias Elter <me@kde.org>
 *                2001 John Califf
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qbitmap.h>
#include <qpainter.h>

#include <kaction.h>
#include <kdebug.h>

#include "kis_brush.h"
#include "kis_doc.h"
#include "kis_view.h"
#include "kis_vec.h"
#include "kis_framebuffer.h"
#include "kis_cursor.h"
#include "kis_tool_pen.h"
#include "kis_dlg_toolopts.h"

PenTool::PenTool(KisDoc *doc, KisCanvas *canvas, KisBrush *_brush) : KisTool(doc)
{
	m_dragging = false;
	m_canvas = canvas;
	m_doc = doc;

	// initialize pen tool settings
	m_penColorThreshold = 128;
	m_opacity = 255;
	m_usePattern = false;
	m_useGradient = false;
	m_lineThickness = 1;
	setBrush(_brush);
}

PenTool::~PenTool()
{
}

void PenTool::setBrush(KisBrush *_brush)
{
    m_brush = _brush;

    int w = m_brush->pixmap().width();
    int h = m_brush->pixmap().height();

    if((w < 33 && h < 33) && (w > 9 && h > 9))
    {
        QBitmap mask(w, h);
        QPixmap pix(m_brush->pixmap());
        mask = pix.createHeuristicMask();
        pix.setMask(mask);
        m_view->kisCanvas()->setCursor(QCursor(pix));
        m_cursor = QCursor(pix);
    }
    else
    {
        m_view->kisCanvas()->setCursor(KisCursor::penCursor());
        m_cursor = KisCursor::penCursor();
    }
}


void PenTool::mousePress(QMouseEvent *e)
{
    /* do all status checking on mouse press only!
    Not needed elsewhere and slows things down if
    done in mouseMove and Paint routines.  Nothing
    happens unless mouse is first pressed anyway */

    KisImage * img = m_doc->current();
    if (!img) return;

    if(!img->getCurrentLayer())
        return;

    if(!img->getCurrentLayer()->visible())
        return;

    if (e->button() != QMouseEvent::LeftButton)
        return;

    if(!m_doc->frameBuffer())
        return;

    m_dragging = true;

    QPoint pos = e->pos();
    pos = zoomed(pos);
    m_dragStart = pos;
    m_dragdist = 0;

    if(paint(pos))
    {
         m_doc->current()->markDirty(QRect(pos
            - m_brush->hotSpot(), m_brush->size()));
    }
}


bool PenTool::paint(QPoint pos)
{
    KisImage * img = m_doc->current();
    KisLayer *lay = img->getCurrentLayer();
    KisFrameBuffer *m_fb = m_doc->frameBuffer();

    int startx = (pos - m_brush->hotSpot()).x();
    int starty = (pos - m_brush->hotSpot()).y();

    QRect clipRect(startx, starty, m_brush->width(), m_brush->height());

    if (!clipRect.intersects(lay->imageExtents()))
        return false;

    clipRect = clipRect.intersect(lay->imageExtents());

    int sx = clipRect.left() - startx;
    int sy = clipRect.top() - starty;
    int ex = clipRect.right() - startx;
    int ey = clipRect.bottom() - starty;

    uchar *sl;
    uchar bv;
    uchar r, g, b;

    int red = m_view->fgColor().R();
    int green = m_view->fgColor().G();
    int blue = m_view->fgColor().B();

    bool alpha = (img->colorMode() == cm_RGBA);

    for (int y = sy; y <= ey; y++)
    {
        sl = m_brush->scanline(y);

        for (int x = sx; x <= ex; x++)
	    {
            // no color blending with pen tool (only with brush)
	        // alpha blending only (maybe)

	        bv = *(sl + x);
	        if (bv < m_penColorThreshold) continue;

		    r   = red;
            g   = green;
            b   = blue;

            // use foreround color
            if(!m_usePattern)
            {
	            lay->setPixel(0, startx + x, starty + y, r);
	            lay->setPixel(1, startx + x, starty + y, g);
	            lay->setPixel(2, startx + x, starty + y, b);
            }
            // map pattern to pen pixel
            else
            {
	            m_fb->setPatternToPixel(lay, startx + x, starty + y, 0);
            }

            if (alpha)
	        {
		        lay->setPixel(3, startx + x, starty + y, bv);
	        }
	    }
    }

    return true;
}


void PenTool::mouseMove(QMouseEvent *e)
{
    KisImage * img = m_doc->current();

    int spacing = m_brush->spacing();
    if (spacing <= 0) spacing = 1;

    if(m_dragging)
    {
        QPoint pos = e->pos();
        int mouseX = e->x();
        int mouseY = e->y();

        pos = zoomed(pos);
        mouseX = zoomed(mouseX);
        mouseY = zoomed(mouseY);

        KisVector end(mouseX, mouseY);
        KisVector start(m_dragStart.x(), m_dragStart.y());

        KisVector dragVec = end - start;
        float saved_dist = m_dragdist;
        float new_dist = dragVec.length();
        float dist = saved_dist + new_dist;

        if ((int)dist < spacing)
	    {
            // save for next movevent
	        m_dragdist += new_dist;
	        m_dragStart = pos;
	        return;
	    }
        else
	        m_dragdist = 0;

        dragVec.normalize();
        KisVector step = start;

        while (dist >= spacing)
	    {
	        if (saved_dist > 0)
	        {
	            step += dragVec * (spacing-saved_dist);
	            saved_dist -= spacing;
	        }
	        else
	            step += dragVec * spacing;

	        QPoint p(qRound(step.x()), qRound(step.y()));

	        if (paint(p))
               img->markDirty(QRect(p - m_brush->hotSpot(), m_brush->size()));

 	        dist -= spacing;
	    }
        //save for next movevent
        if (dist > 0) m_dragdist = dist;
        m_dragStart = pos;
    }
}


void PenTool::mouseRelease(QMouseEvent *e)
{
    if (e->button() != LeftButton)
        return;

    m_dragging = false;
}

void PenTool::optionsDialog()
{
	ToolOptsStruct ts;

	ts.usePattern       = m_usePattern;
	ts.useGradient      = m_useGradient;
	ts.penThreshold     = m_penColorThreshold;
	ts.opacity          = m_opacity;

	bool old_usePattern         = m_usePattern;
	bool old_useGradient        = m_useGradient;
	int old_penColorThreshold   = m_penColorThreshold;
	unsigned int old_opacity          = m_opacity;

	ToolOptionsDialog opt_dlg(tt_pentool, ts);

	opt_dlg.exec();

	if (opt_dlg.result() == QDialog::Rejected)
		return;

	m_usePattern          = opt_dlg.penToolTab()->usePattern();
	m_useGradient         = opt_dlg.penToolTab()->useGradient();
	m_penColorThreshold   = opt_dlg.penToolTab()->penThreshold();
	m_opacity          = opt_dlg.penToolTab()->opacity();

	// User change value ?
	if ( old_usePattern != m_usePattern || old_useGradient != m_useGradient
			|| old_penColorThreshold != m_penColorThreshold || old_opacity != m_opacity ) {
		m_doc->setModified( true );
	}
}

void PenTool::setupAction(QObject *collection)
{
	KToggleAction *toggle = new KToggleAction( i18n("&Pen tool"), "pencil", 0, this, SLOT(toolSelect()), collection, "tool_pen");

	toggle -> setExclusiveGroup("tools");
}

QDomElement PenTool::saveSettings(QDomDocument& doc) const
{ 
	QDomElement tool = doc.createElement("penTool");

	tool.setAttribute("opacity", m_opacity);
	tool.setAttribute("paintThreshold", m_penColorThreshold);
	tool.setAttribute("paintWithPattern", static_cast<int>(m_usePattern));
	tool.setAttribute("paintWithGradient", static_cast<int>(m_useGradient));
	return tool;
}

bool PenTool::loadSettings(QDomElement& tool)
{
	bool rc = tool.tagName() == "penTool";

	if (rc) {
		kdDebug() << "PenTool::loadSettings\n";
		m_opacity = tool.attribute("opacity").toInt();
		m_penColorThreshold = tool.attribute("paintThreshold").toInt();
		m_usePattern = static_cast<bool>(tool.attribute("paintWithPattern").toInt());
		m_useGradient = static_cast<bool>(tool.attribute("paintWithGradient").toInt());
	}

	return rc;
}

