/*
 *  Copyright (c) 1999 Michael Koch <koch@kde.org>
 *                2001 John Califf <jcaliff@compuzone.net>
 *                2002 Patrick Julien <freak@codepimps.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qpainter.h>
#include <qpen.h>
#include <kaction.h>
#include <kcommand.h>
#include <klocale.h>

#include "kis_doc.h"
#include "kis_selection.h"
#include "kis_view.h"
#include "kis_tool_select_rectangular.h"
#include "kis_undo_adapter.h"

namespace {
	class RectSelectCmd : public KNamedCommand {
		typedef KNamedCommand super;

	public:
		RectSelectCmd(KisSelectionSP selection);
		virtual ~RectSelectCmd();

	public:
		virtual void execute();
		virtual void unexecute();

	private:
		KisSelectionSP m_selection;
		KisImageSP m_owner;
	};

	RectSelectCmd::RectSelectCmd(KisSelectionSP selection) : super(i18n("Rectangular Selection"))
	{
		m_selection = selection;
		m_owner = selection -> image();
	}

	RectSelectCmd::~RectSelectCmd()
	{
	}

	void RectSelectCmd::execute()
	{
		m_selection -> clearParentOnMove(true);
		m_owner -> setSelection(m_selection);
		m_owner -> notify(m_selection -> bounds());
	}

	void RectSelectCmd::unexecute()
	{
		m_owner -> unsetSelection(false);
	}
}

KisToolRectangularSelect::KisToolRectangularSelect(KisView *view, KisDoc *doc) : super(view, doc)
{
	setCursor(KisCursor::selectCursor());
	m_view = view;
	m_doc = doc;
	m_selecting = false;
	m_startPos = QPoint(0, 0);
	m_endPos = QPoint(0, 0);
}

KisToolRectangularSelect::~KisToolRectangularSelect()
{
}

void KisToolRectangularSelect::paint(QPainter& gc)
{
	if (m_selecting)
		paintOutline(gc, QRect());
}

void KisToolRectangularSelect::paint(QPainter& gc, const QRect& rc)
{
	if (m_selecting)
		paintOutline(gc, rc);
}

void KisToolRectangularSelect::clearSelection()
{
	KisImageSP img = m_view -> currentImg();

	if (img) {
		img -> unsetSelection();
		m_view -> updateCanvas();
	}

	m_startPos = QPoint(0, 0);
	m_endPos = QPoint(0, 0);
	m_selecting = false;
}

void KisToolRectangularSelect::mousePress(QMouseEvent *e)
{
	KisImageSP img = m_view -> currentImg();

	if (img && img -> activeDevice() && e -> button() == LeftButton) {
		clearSelection();
		m_startPos = m_view -> windowToView(e -> pos());
		m_endPos = m_view -> windowToView(e -> pos());
		m_selecting = true;
	}
}

void KisToolRectangularSelect::mouseMove(QMouseEvent *e)
{
	if (m_selecting) {
		if (m_startPos != m_endPos)
			paintOutline();

		m_endPos = m_view -> windowToView(e -> pos());
		paintOutline();
	}
}

void KisToolRectangularSelect::mouseRelease(QMouseEvent *e)
{
	if (m_selecting) {
		if (m_startPos == m_endPos) {
			clearSelection();
		} else {
			KisImageSP img = m_view -> currentImg();

			if (!img)
				return;
			
			m_startPos = m_view -> viewToWindow(m_startPos);
			m_endPos = e -> pos();

			if (m_endPos.y() < 0)
				m_endPos.setY(0);

			if (m_endPos.y() > img -> height())
				m_endPos.setY(img -> height());

			if (m_endPos.x() < 0)
				m_endPos.setX(0);

			if (m_endPos.x() > img -> width())
				m_endPos.setX(img -> width());

			if (img) {
				KisPaintDeviceSP parent;
				KisSelectionSP selection;
				QRect rc(m_startPos.x(), m_startPos.y(), m_endPos.x() - m_startPos.x(), m_endPos.y() - m_startPos.y());

				parent = img -> activeDevice();

				if (parent) {
					rc = rc.normalize();
					selection = new KisSelection(parent, img, "rectangular selection tool frame", OPACITY_OPAQUE);
					selection -> setBounds(rc);
					img -> setSelection(selection);

					if (img -> undoAdapter())
						img -> undoAdapter() -> addCommand(new RectSelectCmd(selection));
				}
			}
		}

		m_selecting = false;
	}
}

void KisToolRectangularSelect::paintOutline()
{
	QWidget *canvas = m_view -> canvas();
	QPainter gc(canvas);
	QRect rc;

	paintOutline(gc, rc);
}

void KisToolRectangularSelect::paintOutline(QPainter& gc, const QRect&)
{
	RasterOp op = gc.rasterOp();
	QPen old = gc.pen();
	QPen pen(Qt::DotLine);
	QPoint start;
	QPoint end;

	start = m_view -> viewToWindow(m_startPos);
	end = m_view -> viewToWindow(m_endPos);
	start.setX(start.x() - m_view -> horzValue());
	start.setY(start.y() - m_view -> vertValue());
	end.setX(end.x() - m_view -> horzValue());
	end.setY(end.y() - m_view -> vertValue());
	end.setX((end.x() - start.x()));
	end.setY((end.y() - start.y()));
	start *= m_view -> zoom();
	end *= m_view -> zoom();
	gc.setRasterOp(Qt::NotROP);
	gc.setPen(pen);
	gc.drawRect(start.x(), start.y(), end.x(), end.y());
	gc.setRasterOp(op);
	gc.setPen(old);
}

void KisToolRectangularSelect::setup()
{
	KToggleAction *toggle;

	toggle = new KToggleAction(i18n("&Rectangular Select"), "rectangular", 0, this, 
			SLOT(activateSelf()), m_view -> actionCollection(), "tool_select_rectangular");
	toggle -> setExclusiveGroup("tools");
}

