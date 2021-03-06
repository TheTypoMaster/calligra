/*
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
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
#ifndef KIS_TEXTURE_TILE_H_
#define KIS_TEXTURE_TILE_H_

#include <opengl/kis_opengl.h>

#ifdef HAVE_OPENGL

#include "kis_texture_tile_update_info.h"

#include <QRect>
#include <QRectF>
#include <QOpenGLFunctions>

#if !defined(QT_OPENGL_ES)
#define USE_PIXEL_BUFFERS
#include <QOpenGLBuffer>
#endif


struct KisGLTexturesInfo {

    // real width and height
    int width;
    int height;

    // width and height minus border padding?
    int effectiveWidth;
    int effectiveHeight;

    // size of the border padding
    int border;

    GLint internalFormat;
    GLint format;
    GLint type;
};

inline QRect stretchRect(const QRect &rc, int delta)
{
    return rc.adjusted(-delta, -delta, delta, delta);
}

class KisTextureTile
{
public:
    enum FilterMode {
        NearestFilterMode,  // nearest
        BilinearFilterMode, // linear, no mipmap
        TrilinearFilterMode, // LINEAR_MIPMAP_LINEAR
        HighQualityFiltering // Mipmaps + custom shader
    };

    KisTextureTile(QRect imageRect, const KisGLTexturesInfo *texturesInfo,
                   const QByteArray &fillData, FilterMode mode,
                   bool useBuffer, int numMipmapLevels, QOpenGLFunctions *f);
    ~KisTextureTile();

    void setUseBuffer(bool useBuffer) {
        m_useBuffer = useBuffer;
    }

    void setNumMipmapLevels(int num) {
        m_numMipmapLevels = num;
    }

    void update(const KisTextureTileUpdateInfo &updateInfo);

    inline QRect tileRectInImagePixels() {
        return m_tileRectInImagePixels;
    }

    inline QRect textureRectInImagePixels() {
        return m_textureRectInImagePixels;
    }

    inline QRectF tileRectInTexturePixels() {
        return m_tileRectInTexturePixels;
    }

    inline void setTextureParameters()
    {
        if(f) {
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, m_numMipmapLevels);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, m_numMipmapLevels);

            f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }
    }


    void bindToActiveTexture();

private:
    void setNeedsMipmapRegeneration();

    GLuint m_textureId;

#ifdef USE_PIXEL_BUFFERS
    void createTextureBuffer(const char*data, int size);
    QOpenGLBuffer *m_glBuffer;
#endif

    QRect m_tileRectInImagePixels;
    QRectF m_tileRectInTexturePixels;
    QRect m_textureRectInImagePixels;
    FilterMode m_filter;
    const KisGLTexturesInfo *m_texturesInfo;
    bool m_needsMipmapRegeneration;
    bool m_useBuffer;
    int m_numMipmapLevels;
    QOpenGLFunctions *f;
    Q_DISABLE_COPY(KisTextureTile)
};


#endif /* HAVE_OPENGL */

#endif /* KIS_TEXTURE_TILE_H_ */

