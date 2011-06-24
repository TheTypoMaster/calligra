    /* This file is part of the KDE project
    * Copyright (C) 2011 Aakriti Gupta <aakriti.a.gupta@gmail.com>
    *
    * This library is free software; you can redistribute it and/or
    * modify it under the terms of the GNU Library General Public
    * License as published by the Free Software Foundation; either
    * version 2 of the License, or (at your option) any later version.
    *
    * This library is distributed in the hope that it will be useful,
    * but WITHOUT ANY WARRANTY; without even the implied warranty of
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    * Library General Public License for more details.
    *
    * You should have received a copy of the GNU Library General Public License
    * along with this library; see the file COPYING.LIB.  If not, write to
    * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    * Boston, MA 02110-1301, USA.
    */

    #ifndef FRAME_H
    #define FRAME_H

    #include <QString>
    #include <KoXmlReader.h>
#include <qmap.h>


    /*
    * This class is used to define the attributes of frames.
    * Frames are used in making an animated SVSG presentation.
    * Instead of a new slide, in an animated SVG presentation, 
    * each frame is zoomed into.
    */
  class Frame
    {
      public:
        Frame();
        Frame(const KoXmlElement& e);
      ~Frame();
      /*
        * Sets the default values for all frame attrbutes
        */
      void setDefaultValues();
      /*bool setAttribute(const QString& attrName, const QString& value);
      bool setAttribute(const QString& attrName, const bool& value);
      bool setAttribute(const QString& attrName, const int& value);
      */
      void setTitle(const QString& title);
      void setRefId(const QString& refId); 
      void setTransitionProfile(const QString& transProfile);

      void setHide(bool condition);
      void setClip(bool condition);
      void enableTimeout(bool condition);

      void setSequence(int seq);
      void setZoomPercent(int zoomPercent);
      void setTimeout(int timeoutMs);
      void setTransitionDuration(int timeMs);
      
      QString title() const;
      QString refId() const; 
      QString transitionProfile() const;
      
      bool isHide() const;
      bool isClip() const;
      bool isEnableTimeout() const;
      
      int sequence() const;
      int zoomPercent() const;
      int timeout() const;
      int transitionDuration() const;
      
      QList<QString> attributes() const;
      
    private:
      QString m_title;
      QString m_refId;
      QString m_transitionProfile;
      
      bool m_hide;
      bool m_clip;
      bool m_timeoutEnable;

      int m_sequence;
      int m_transitionZoomPercent;
      int m_transitionDurationMs;
      int m_timeoutMs;

      QList<QString> m_attributes;
      //QMap<QString *, QString * > m_attrrValues; 
    };
    
    #endif /*FRAME_H*/
