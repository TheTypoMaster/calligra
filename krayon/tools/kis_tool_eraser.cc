/*
 *  kis_tool_eraser.cc - part of Krayon
 *
 *  Copyright (c) 1999 Matthias Elter <me@kde.org>
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

#include <kaction.h>

#include "kis_brush.h"
#include "kis_doc.h"
#include "kis_view.h"
#include "kis_canvas.h"
#include "kis_vec.h"
#include "kis_cursor.h"
#include "kis_util.h"

#include "kis_tool_eraser.h"
#include "kis_dlg_toolopts.h"

EraserTool::EraserTool(KisDoc *doc, KisBrush *_brush) : KisTool(doc)
{
    m_dragging = false;
    m_dragdist = 0;
    m_pDoc = doc;

    // initialize eraser tool settings
    usePattern = false;
    useGradient = true;
    opacity = 255;

    setBrush(_brush);
}

EraserTool::~EraserTool() {}

void EraserTool::setBrush(KisBrush *_brush)
{
    m_pBrush = _brush;

    int w = m_pBrush->pixmap().width();
    int h = m_pBrush->pixmap().height();

    // cursor cannot be larger than 32x32
    if((w < 33 && h < 33) && (w > 9 && h > 9))
    {
        QBitmap mask(w, h);
        QPixmap pix(m_pBrush->pixmap());
        mask = pix.createHeuristicMask();
        pix.setMask(mask);
        m_pView->kisCanvas()->setCursor(QCursor(pix));
        m_Cursor = QCursor(pix);
    }
    else
    {
        m_pView->kisCanvas()->setCursor(KisCursor::eraserCursor());
        m_Cursor = KisCursor::eraserCursor();
    }
}

void EraserTool::mousePress(QMouseEvent *e)
{
    KisImage * img = m_pDoc->current();
    if (!img) return;

    if(!img->getCurrentLayer())
        return;

    if(!img->getCurrentLayer()->visible())
        return;

    if (e->button() != QMouseEvent::LeftButton)
        return;

    m_dragging = true;

    QPoint pos = e->pos();
    pos = zoomed(pos);
    m_dragStart = pos;
    m_dragdist = 0;

    if(paint(pos))
    {
         m_pDoc->current()->markDirty(QRect(pos
            - m_pBrush->hotSpot(), m_pBrush->size()));
    }
}


bool EraserTool::paint(QPoint pos)
{
    KisImage * img = m_pDoc->current();
    KisLayer *lay = img->getCurrentLayer();

    if (!img)	return false;
    if (!lay)   return false;
    if (!m_pBrush) return false;

    // FIXME: Implement this for non-RGB modes.
    if (!img->colorMode() == cm_RGB  && !img->colorMode() == cm_RGBA)
	return false;

    int startx = (pos - m_pBrush->hotSpot()).x();
    int starty = (pos - m_pBrush->hotSpot()).y();

    QRect clipRect(startx, starty, m_pBrush->width(), m_pBrush->height());

    if (!clipRect.intersects(img->getCurrentLayer()->imageExtents()))
        return false;

    clipRect = clipRect.intersect(img->getCurrentLayer()->imageExtents());

    int sx = clipRect.left() - startx;
    int sy = clipRect.top() - starty;
    int ex = clipRect.right() - startx;
    int ey = clipRect.bottom() - starty;

    uchar *sl;
    uchar bv, invbv;

    bool alpha = (img->colorMode() == cm_RGBA);

    if (alpha)
    {
        uchar a;
	    int   v;

        for (int y = sy; y <= ey; y++)
	    {
	        sl = m_pBrush->scanline(y);

	        for (int x = sx; x <= ex; x++)
	        {
	            bv = *(sl + x);
		        if (bv == 0) continue;

		        a = lay->pixel(3, startx + x, starty + y);
		        v = a - bv;
		        if (v < 0 ) v = 0;
		        if (v > 255 ) v = 255;
		        a = (uchar) v;

		        lay->setPixel(3, startx + x, starty + y, a);
	        }
	    }
    }
    else   // no alpha channel -> erase to background color
    {
        uchar r, g, b;
	    int red = m_pView->bgColor().R();
	    int green = m_pView->bgColor().G();
	    int blue = m_pView->bgColor().B();

	    for (int y = sy; y <= ey; y++)
	    {
	        sl = m_pBrush->scanline(y);

	        for (int x = sx; x <= ex; x++)
	        {
		        r = lay->pixel(0, startx + x, starty + y);
		        g = lay->pixel(1, startx + x, starty + y);
		        b = lay->pixel(2, startx + x, starty + y);

		        bv = *(sl + x);
		        if (bv == 0) continue;

		        invbv = 255 - bv;

		        b = ((blue * bv) + (b * invbv))/255;
		        g = ((green * bv) + (g * invbv))/255;
		        r = ((red * bv) + (r * invbv))/255;

		        lay->setPixel(0, startx + x, starty + y, r);
		        lay->setPixel(1, startx + x, starty + y, g);
		        lay->setPixel(2, startx + x, starty + y, b);
	        }
	    }
    }

    return true;
}


void EraserTool::mouseMove(QMouseEvent *e)
{
    KisImage * img = m_pDoc->current();
    if (!img) return;

    int spacing = m_pBrush->spacing();
    if (spacing <= 0) spacing = 1;

    if(m_dragging)
    {
        if( !img->getCurrentLayer()->visible() )
        	return;

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
               img->markDirty(QRect(p - m_pBrush->hotSpot(), m_pBrush->size()));

 	        dist -= spacing;
	    }
        //save for next movevent
        if (dist > 0) m_dragdist = dist;
        m_dragStart = pos;
    }

}

void EraserTool::mouseRelease(QMouseEvent *e)
{
    if (e->button() != LeftButton)
        return;
    m_dragging = false;
}

void EraserTool::optionsDialog()
{
    ToolOptsStruct ts;

    ts.usePattern  = usePattern;
    ts.useGradient = useGradient;
    ts.opacity     = opacity;

    bool old_usePattern     = usePattern;
    bool old_useGradient    = useGradient;
    unsigned int  old_opacity    = opacity;

    ToolOptionsDialog OptsDialog(tt_erasertool, ts);

    OptsDialog.exec();

    if(OptsDialog.result() == QDialog::Rejected)
        return;
        
    opacity   = OptsDialog.eraserToolTab()->opacity();
    usePattern    = OptsDialog.eraserToolTab()->usePattern();
    useGradient   = OptsDialog.eraserToolTab()->useGradient();

    // User change value ?
    if ( old_usePattern != usePattern || old_useGradient != useGradient 
		    || old_opacity != opacity ) {
	    // set eraser tool settings
	    m_pDoc->setModified( true );
    }
}

void EraserTool::setupAction(QObject *collection)
{
	KToggleAction *toggle = new KToggleAction(i18n("&Eraser tool"), "eraser", 0, this, SLOT(toolSelect()), collection, "tool_eraser");

	toggle -> setExclusiveGroup("tools");
}

QDomElement EraserTool::saveSettings(QDomDocument& doc) const
{
	QDomElement eraserTool = doc.createElement("eraserTool");

	eraserTool.setAttribute("opacity", opacity);
	eraserTool.setAttribute("blendWithCurrentGradient", static_cast<int>(useGradient));
	eraserTool.setAttribute("blendWithCurrentPattern", static_cast<int>(usePattern));
	return eraserTool;
}

bool EraserTool::loadSettings(QDomElement& elem)
{
        bool rc = elem.tagName() == "eraserTool";

	if (rc) {
		opacity = elem.attribute("opacity").toInt();
		useGradient = static_cast<bool>(elem.attribute("blendWithCurrentGradient").toInt());
		usePattern = static_cast<bool>(elem.attribute("blendWithCurrentPattern" ).toInt());
	}

	return rc;
}

