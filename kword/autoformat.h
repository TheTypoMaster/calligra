/******************************************************************/
/* KWord - (c) by Reginald Stadlbauer and Torben Weis 1997-1998   */
/* Version: 0.0.1                                                 */
/* Author: Reginald Stadlbauer, Torben Weis                       */
/* E-Mail: reggie@kde.org, weis@kde.org                           */
/* Homepage: http://boch35.kfunigraz.ac.at/~rs                    */
/* needs c++ library Qt (http://www.troll.no)                     */
/* written for KDE (http://www.kde.org)                           */
/* needs mico (http://diamant.vsb.cs.uni-frankfurt.de/~mico/)     */
/* needs OpenParts and Kom (weis@kde.org)                         */
/* License: GNU GPL                                               */
/******************************************************************/
/* Module: KWAutoFormat (header)                                  */
/******************************************************************/

#ifndef kwautoformat_h
#define kwautoformat_h

#include <qstring.h>
#include <qcolor.h>

#include "format.h"
#include "char.h"

class KWordDocument;
class KWFormatContext;
class KWParag;

/******************************************************************/
/* Class: KWAutoFormatEntry                                       */
/******************************************************************/

class KWAutoFormatEntry
{
public:
  KWAutoFormatEntry();

protected:
  QString expr;
  bool checkFamily,checkColor,checkSize,checkBold,checkItalic,checkUnderline,checkVertAlign;
  QString family;
  QColor color;
  int size;
  bool bold,italic,underline;
  KWFormat::VertAlign vertAlign;
  bool caseSensitive,regexp,wildcard;

};

/******************************************************************/
/* Class: KWAutoFormat                                            */
/******************************************************************/

class KWAutoFormat
{
public:
  struct TypographicQuotes
  {
    TypographicQuotes() : begin('�'), end('�'), replace(true)
    {}

    QChar begin,end;
    bool replace;
  };

  KWAutoFormat(KWordDocument *_doc);

  void startAutoFormat(KWParag *parag,KWFormatContext *fc);
  bool doAutoFormat(KWParag *parag,KWFormatContext *fc);
  void endAutoFormat(KWParag *parag,KWFormatContext *fc);
  bool doTypographicQuotes(KWParag *parag,KWFormatContext *fc);
  bool doUpperCase(KWParag *parag,KWFormatContext *fc);

  void setEnabled(bool e) { enabled = e; }
  bool isEnabled() { return enabled; }

  void configTypographicQuotes(TypographicQuotes _tq);
  void configUpperCase(bool _uc);
  void configUpperUpper(bool _uu);
  
  TypographicQuotes getConfigTypographicQuotes()
  { return typographicQuotes; }
  bool getConfigUpperCase()
  { return convertUpperCase; }
  bool getConfigUpperUpper()
  { return convertUpperUpper; }
  
  static bool isUpper(const QChar &c);

protected:
  KWordDocument *doc;
  TypographicQuotes typographicQuotes;
  bool enabled;
  KWString *tmpBuffer;
  bool lastWasDotSpace,convertUpperCase;
  bool lastWasUpper,convertUpperUpper;

};

#endif
