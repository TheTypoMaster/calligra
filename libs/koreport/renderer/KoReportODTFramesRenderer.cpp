/*
 * Calligra Report Engine
 * Copyright (C) 2010 by Adam Pigg (adam@piggz.co.uk)
 * Copyright (C) 2012 by Dag Andersen (danders@get2net.dk)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "KoReportODTFramesRenderer.h"
#include "odt/KoSimpleOdtDocument.h"
#include "odt/KoSimpleOdtTextBox.h"
#include "odt/KoSimpleOdtImage.h"
#include "odt/KoSimpleOdtPicture.h"
#include "odt/KoSimpleOdtLine.h"
#include "odt/KoSimpleOdtCheckBox.h"
#include "renderobjects.h"

#include <kdebug.h>

KoReportODTFramesRenderer::KoReportODTFramesRenderer()
{

}

KoReportODTFramesRenderer::~KoReportODTFramesRenderer()
{
}

bool KoReportODTFramesRenderer::render(const KoReportRendererContext& context, ORODocument* document, int /*page*/)
{
    int uid = 1;
    KoSimpleOdtDocument doc;
    doc.setPageOptions(document->pageOptions());
    for (int page = 0; page < document->pages(); page++) {
        OROPage *p = document->page(page);
        for (int i = 0; i < p->primitives(); i++) {
            OROPrimitive *prim = p->primitive(i);
            if (prim->type() == OROTextBox::TextBox) {
                KoSimpleOdtPrimitive *sp = new KoSimpleOdtTextBox(static_cast<OROTextBox*>(prim));
                sp->setUID(uid++);
                doc.addPrimitive(sp);
            } else if (prim->type() == OROImage::Image) {
                KoSimpleOdtPrimitive *sp = new KoSimpleOdtImage(static_cast<OROImage*>(prim));
                sp->setUID(uid++);
                doc.addPrimitive(sp);
            } else if (prim->type() == OROPicture::Picture) {
                KoSimpleOdtPrimitive *sp = new KoSimpleOdtPicture(static_cast<OROPicture*>(prim));
                sp->setUID(uid++);
                doc.addPrimitive(sp);
            } else if (prim->type() == OROLine::Line) {
                KoSimpleOdtPrimitive *sp = new KoSimpleOdtLine(static_cast<OROLine*>(prim));
                sp->setUID(uid++);
                doc.addPrimitive(sp);
            } else if (prim->type() == OROCheck::Check) {
                KoSimpleOdtPrimitive *sp = new KoSimpleOdtCheckBox(static_cast<OROCheck*>(prim));
                sp->setUID(uid++);
                doc.addPrimitive(sp);
            } else {
                kDebug() << "unhandled primitive type."<<prim->type();
            }
        }
    }
    return doc.saveDocument(context.destinationUrl.path()) == QFile::NoError;
}

