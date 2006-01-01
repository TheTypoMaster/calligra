/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
 *
 *  this program is free software; you can redistribute it and/or modify
 *  it under the terms of the gnu general public license as published by
 *  the free software foundation; either version 2 of the license, or
 *  (at your option) any later version.
 *
 *  this program is distributed in the hope that it will be useful,
 *  but without any warranty; without even the implied warranty of
 *  merchantability or fitness for a particular purpose.  see the
 *  gnu general public license for more details.
 *
 *  you should have received a copy of the gnu general public license
 *  along with this program; if not, write to the free software
 *  foundation, inc., 675 mass ave, cambridge, ma 02139, usa.
 */

#include <kdebug.h>
#include <qimage.h>

#include "kis_debug_areas.h"
#include "kis_group_layer.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_painter.h"
#include "kis_undo_adapter.h"

namespace {

    class KisLayerCommand : public KNamedCommand {
        typedef KNamedCommand super;

    public:
        KisLayerCommand(const QString& name, KisLayerSP layer);
        virtual ~KisLayerCommand() {}

        virtual void execute() = 0;
        virtual void unexecute() = 0;

    protected:
        void setUndo(bool undo);
        void notifyPropertyChanged();

        KisLayerSP m_layer;
    };

    KisLayerCommand::KisLayerCommand(const QString& name, KisLayerSP layer) :
        super(name), m_layer(layer)
    {
    }

    void KisLayerCommand::setUndo(bool undo)
    {
        if (m_layer -> undoAdapter()) {
            m_layer -> undoAdapter() -> setUndo(undo);
        }
    }

    void KisLayerCommand::notifyPropertyChanged()
    {
        if (m_layer -> image()) {
            m_layer -> image() -> notifyPropertyChanged(m_layer);
        }
    }

    class KisLayerLockedCommand : public KisLayerCommand {
        typedef KisLayerCommand super;

    public:
        KisLayerLockedCommand(KisLayerSP layer, bool oldLocked, bool newLocked);

        virtual void execute();
        virtual void unexecute();

    private:
        bool m_oldLocked;
        bool m_newLocked;
    };

    KisLayerLockedCommand::KisLayerLockedCommand(KisLayerSP layer, bool oldLocked, bool newLocked) :
        super(i18n("Lock Layer"), layer)
    {
        m_oldLocked = oldLocked;
        m_newLocked = newLocked;
    }

    void KisLayerLockedCommand::execute()
    {
        setUndo(false);
        m_layer -> setLocked(m_newLocked);
        setUndo(true);
    }

    void KisLayerLockedCommand::unexecute()
    {
        setUndo(false);
        m_layer -> setLocked(m_oldLocked);
        setUndo(true);
    }

    class KisLayerOpacityCommand : public KisLayerCommand {
        typedef KisLayerCommand super;

    public:
        KisLayerOpacityCommand(KisLayerSP layer, Q_UINT8 oldOpacity, Q_UINT8 newOpacity);

        virtual void execute();
        virtual void unexecute();

    private:
        Q_UINT8 m_oldOpacity;
        Q_UINT8 m_newOpacity;
    };

    KisLayerOpacityCommand::KisLayerOpacityCommand(KisLayerSP layer, Q_UINT8 oldOpacity, Q_UINT8 newOpacity) :
        super(i18n("Layer Opacity"), layer)
    {
        m_oldOpacity = oldOpacity;
        m_newOpacity = newOpacity;
    }

    void KisLayerOpacityCommand::execute()
    {
        setUndo(false);
        m_layer -> setOpacity(m_newOpacity);
        notifyPropertyChanged();
        setUndo(true);
    }

    void KisLayerOpacityCommand::unexecute()
    {
        setUndo(false);
        m_layer -> setOpacity(m_oldOpacity);
        notifyPropertyChanged();
        setUndo(true);
    }

    class KisLayerVisibilityCommand : public KisLayerCommand {
        typedef KisLayerCommand super;

    public:
        KisLayerVisibilityCommand(KisLayerSP layer, bool oldVisibility, bool newVisibility);

        virtual void execute();
        virtual void unexecute();

    private:
        bool m_oldVisibility;
        bool m_newVisibility;
    };

    KisLayerVisibilityCommand::KisLayerVisibilityCommand(KisLayerSP layer, bool oldVisibility, bool newVisibility) :
        super(i18n("Layer Visibility"), layer)
    {
        m_oldVisibility = oldVisibility;
        m_newVisibility = newVisibility;
    }

    void KisLayerVisibilityCommand::execute()
    {
        setUndo(false);
        m_layer -> setVisible(m_newVisibility);
        setUndo(true);
    }

    void KisLayerVisibilityCommand::unexecute()
    {
        setUndo(false);
        m_layer -> setVisible(m_oldVisibility);
        setUndo(true);
    }

    class KisLayerCompositeOpCommand : public KisLayerCommand {
        typedef KisLayerCommand super;

    public:
        KisLayerCompositeOpCommand(KisLayerSP layer, const KisCompositeOp& oldCompositeOp, const KisCompositeOp& newCompositeOp);

        virtual void execute();
        virtual void unexecute();

    private:
        KisCompositeOp m_oldCompositeOp;
        KisCompositeOp m_newCompositeOp;
    };

    KisLayerCompositeOpCommand::KisLayerCompositeOpCommand(KisLayerSP layer, const KisCompositeOp& oldCompositeOp,
                                       const KisCompositeOp& newCompositeOp) :
        super(i18n("Layer Composite Mode"), layer)
    {
        m_oldCompositeOp = oldCompositeOp;
        m_newCompositeOp = newCompositeOp;
    }

    void KisLayerCompositeOpCommand::execute()
    {
        setUndo(false);
        m_layer -> setCompositeOp(m_newCompositeOp);
        notifyPropertyChanged();
        setUndo(true);
    }

    void KisLayerCompositeOpCommand::unexecute()
    {
        setUndo(false);
        m_layer -> setCompositeOp(m_oldCompositeOp);
        notifyPropertyChanged();
        setUndo(true);
    }

}

static int getID()
{
    static int id = 1;
    return id++;
}


KisLayer::KisLayer(KisImage *img, const QString &name, Q_UINT8 opacity) :
    QObject(),
    KShared(),
    m_id(getID()),
    m_index(-1),
    m_opacity(opacity),
    m_locked(false),
    m_visible(true),
    m_name(name),
    m_parent(0),
    m_image(img),
    m_compositeOp(COMPOSITE_OVER)
{
}

KisLayer::KisLayer(const KisLayer& rhs) :
    QObject(),
    KShared(rhs)
{
    if (this != &rhs) {
        m_id = getID();
        m_index = -1;
        m_opacity = rhs.m_opacity;
        m_locked = rhs.m_locked;
        m_visible = rhs.m_visible;
        m_name = rhs.m_name;
        m_image = rhs.m_image;
        m_parent = 0;
        m_compositeOp = rhs.m_compositeOp;
    }
}

KisLayer::~KisLayer()
{
}

KisGroupLayerSP KisLayer::parent() const
{
    return m_parent;
}

KisLayerSP KisLayer::prevSibling() const
{
    if (!parent())
        return 0;
    return parent() -> at(index() - 1);
}

KisLayerSP KisLayer::nextSibling() const
{
    if (!parent())
        return 0;
    return parent() -> at(index() + 1);
}

int KisLayer::index() const
{
    return m_index;
}

void KisLayer::setIndex(int i)
{
    if (!parent())
        return;
    parent() -> setIndex(this, i);
}

KisLayerSP KisLayer::findLayer(const QString& n) const
{
    if (name() == n)
        return const_cast<KisLayer*>(this); //HACK any less ugly way? findLayer() is conceptually const...
    for (KisLayerSP layer = firstChild(); layer; layer = layer -> nextSibling())
        if (KisLayerSP found = layer -> findLayer(n))
            return found;
    return 0;
}

KisLayerSP KisLayer::findLayer(int i) const
{
    if (id() == i)
        return const_cast<KisLayer*>(this); //HACK
    for (KisLayerSP layer = firstChild(); layer; layer = layer -> nextSibling())
        if (KisLayerSP found = layer -> findLayer(i))
            return found;
    return 0;
}

int KisLayer::numLayers(int flags) const
{
    int num = 0;
    if (matchesFlags(flags)) num++;
    for (KisLayerSP layer = firstChild(); layer; layer = layer -> nextSibling())
        num += layer -> numLayers(flags);
    return num; 
}

bool KisLayer::matchesFlags(int flags) const
{
    if ((flags & Visible) && !visible())
        return false;
    if ((flags & Hidden) && visible())
        return false;
    if ((flags & Locked) && !locked())
        return false;
    if ((flags & Unlocked) && locked())
        return false;
    return true;
}

Q_UINT8 KisLayer::opacity() const
{
    return m_opacity;
}

void KisLayer::setOpacity(Q_UINT8 val)
{
    m_opacity = val;
}

KNamedCommand *KisLayer::setOpacityCommand(Q_UINT8 newOpacity)
{
    return new KisLayerOpacityCommand(this, opacity(), newOpacity);
}

const bool KisLayer::visible() const
{
    return m_visible && m_opacity != OPACITY_TRANSPARENT; //XXX hmm is this right?
}

void KisLayer::setVisible(bool v)
{
    if (m_visible != v) {
        m_visible = v;
        emit visibilityChanged(this);
        if (image())
            image() -> notifyPropertyChanged(this);
    }
}

KNamedCommand *KisLayer::setVisibleCommand(bool newVisibility)
{
    return new KisLayerVisibilityCommand(this, m_visible, newVisibility);
}

bool KisLayer::locked() const
{
    return m_locked;
}

void KisLayer::setLocked(bool l)
{
    if (m_locked != l)
    {
        m_locked = l;
        if (image())
            image() -> notifyPropertyChanged(this);
    }
}

KNamedCommand *KisLayer::setLockedCommand(bool newLocked)
{
    return new KisLayerLockedCommand(this, locked(), newLocked);
}

QString KisLayer::name() const
{
        return m_name;
}

void KisLayer::setName(const QString& name)
{
        if (!name.isEmpty())
                m_name = name;
}

KNamedCommand *KisLayer::setCompositeOpCommand(const KisCompositeOp& newCompositeOp)
{
    return new KisLayerCompositeOpCommand(this, compositeOp(), newCompositeOp);
}

KisUndoAdapter *KisLayer::undoAdapter() const
{
    if (m_image) {
        return m_image -> undoAdapter();
    }
    return 0;
}

void KisLayer::paintMaskInactiveLayers(QImage &, Q_INT32, Q_INT32, Q_INT32, Q_INT32)
{
}

void KisLayer::paintSelection(QImage &, Q_INT32, Q_INT32, Q_INT32, Q_INT32)
{
}

#include "kis_layer.moc"
