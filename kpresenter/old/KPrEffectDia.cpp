// -*- Mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-
/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Reginald Stadlbauer <reggie@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "KPrView.h"
#include "KPrDocument.h"
#include "KPrEffectDia.h"
#include "KPrCommand.h"
#include "KPrObject.h"
#include "KPrSoundPlayer.h"

#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <q3groupbox.h>
#include <QCheckBox>
#include <QLineEdit>
#include <QList>
#include <QLayout>
#include <QSpinBox>
#include <qstringlist.h>
#include <QDir>
#include <QToolTip>
#include <q3whatsthis.h>
//Added by qt3to4:
#include <QResizeEvent>
#include <Q3GridLayout>
#include <Q3PtrList>
#include <Q3VBoxLayout>

#include <klocale.h>
#include <kglobal.h>
#include <kbuttonbox.h>
#include <knuminput.h>
#include <kurlrequester.h>
#include <kurl.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>

KPrEffectDia::KPrEffectDia( QWidget* parent, const char* name, const Q3PtrList<KPrObject>& _objs,
                      KPrView *_view )
    : KDialog( parent ), objs( _objs )
{
    view = _view;
    KPrObject *obj = objs.at( 0 );
    soundPlayer1 = 0;
    soundPlayer2 = 0;

    QWidget *page = new QWidget( this );
    setMainWidget(page);
    topLayout = new Q3VBoxLayout( page, 0, spacingHint() );

    Q3GroupBox *grp1 = new Q3GroupBox(0, Qt::Vertical, i18n( "Appear" ), page );
    grp1->layout()->setSpacing(KDialog::spacingHint());
    grp1->layout()->setMargin(KDialog::marginHint());
    topLayout->addWidget(grp1);
    Q3GridLayout *upperRow = new Q3GridLayout(grp1->layout(), 6, 4);

    lAppear = new QLabel( i18n( "Order of appearance:" ), grp1 );
    lAppear->setAlignment( Qt::AlignVCenter );
    upperRow->addWidget(lAppear, 0, 0);

    eAppearStep = new QSpinBox( 0, 100, 1, grp1 );
    eAppearStep->setValue( obj->getAppearStep() );
    upperRow->addWidget(eAppearStep, 0, 1);

    //( void )new QWidget( grp1 );
    //( void )new QWidget( grp1 );

    lEffect = new QLabel( i18n( "Effect (appearing):" ), grp1 );
    lEffect->setAlignment( Qt::AlignVCenter );
    upperRow->addWidget(lEffect, 1, 0);

    cEffect = new QComboBox( grp1 );
    cEffect->setObjectName( "cEffect" );
    cEffect->setEditable( false );
    cEffect->addItem( i18n( "No Effect" ) );
    cEffect->addItem( i18n( "Come From Right" ) );
    cEffect->addItem( i18n( "Come From Left" ) );
    cEffect->addItem( i18n( "Come From Top" ) );
    cEffect->addItem( i18n( "Come From Bottom" ) );
    cEffect->addItem( i18n( "Come From Right/Top" ) );
    cEffect->addItem( i18n( "Come From Right/Bottom" ) );
    cEffect->addItem( i18n( "Come From Left/Top" ) );
    cEffect->addItem( i18n( "Come From Left/Bottom" ) );
    cEffect->addItem( i18n( "Wipe From Left" ) );
    cEffect->addItem( i18n( "Wipe From Right" ) );
    cEffect->addItem( i18n( "Wipe From Top" ) );
    cEffect->addItem( i18n( "Wipe From Bottom" ) );
    cEffect->setCurrentIndex( static_cast<int>( obj->getEffect() ) );
    upperRow->addWidget(cEffect, 1, 1);

    connect( cEffect, SIGNAL( activated( int ) ), this, SLOT( appearEffectChanged( int ) ) );

    lAppearSpeed = new QLabel( i18n( "Speed:" ), grp1 );
    lAppearSpeed->setAlignment( Qt::AlignVCenter );
    upperRow->addWidget(lAppearSpeed, 2, 0);

    cAppearSpeed = new QComboBox( grp1, "cEffect" );
    cAppearSpeed->setEditable(false);
    cAppearSpeed->addItem( i18n( "Slow" ) );
    cAppearSpeed->addItem( i18n( "Medium" ) );
    cAppearSpeed->addItem( i18n( "Fast" ) );
    cAppearSpeed->setCurrentIndex( static_cast<int>( obj->getAppearSpeed() ) );
    upperRow->addWidget(cAppearSpeed, 2, 1);
    appearEffectChanged( cEffect->currentIndex() );

    lEffect2 = new QLabel( i18n( "Effect (object specific):" ), grp1 );
    lEffect2->setAlignment( Qt::AlignVCenter );
    upperRow->addWidget(lEffect2, 3, 0);

    cEffect2 = new QComboBox( grp1, "cEffect2" );
    cEffect2->setEditable(false);
    cEffect2->addItem( i18n( "No Effect" ) );
    upperRow->addWidget(cEffect2, 3, 1);

    switch ( obj->getType() ) {
    case OT_TEXT: {
        cEffect2->addItem( i18n( "Paragraph After Paragraph" ) );
    } break;
    default:
        lEffect2->setEnabled(false);
        cEffect2->setEnabled(false);
        break;
    }

    if ( obj->getEffect2() == EF2_NONE )
        cEffect2->setCurrentIndex( static_cast<int>( obj->getEffect2() ) );
    else {
        switch ( obj->getType() )
        {
        case OT_TEXT:
            cEffect2->setCurrentIndex( static_cast<int>( obj->getEffect2() + TxtObjOffset ) );
            break;
        default: break;
        }
    }

    QLabel *lTimerOfAppear = new QLabel( i18n( "Timer of the object:" ), grp1 );
    lTimerOfAppear->setAlignment( Qt::AlignVCenter );
    upperRow->addWidget( lTimerOfAppear, 4, 0 );

    timerOfAppear = new KIntNumInput( obj->getAppearTimer(), grp1 );
    timerOfAppear->setRange( 1, 600, 1 );
    timerOfAppear->setSuffix( i18n( " seconds" ) );
    upperRow->addWidget( timerOfAppear, 4, 1 );

    if ( view->kPresenterDoc()->spManualSwitch() )
        timerOfAppear->setEnabled( false );


    // setup the Sound Effect stuff
    appearSoundEffect = new QCheckBox( i18n( "Sound effect" ), grp1 );
    appearSoundEffect->setChecked( obj->getAppearSoundEffect() );
    upperRow->addWidget( appearSoundEffect, 5, 0 );

    connect( appearSoundEffect, SIGNAL( clicked() ), this, SLOT( appearSoundEffectChanged() ) );

    lSoundEffect1 = new QLabel( i18n( "File name:" ), grp1 );
    lSoundEffect1->setAlignment( Qt::AlignVCenter );
    upperRow->addWidget( lSoundEffect1, 6, 0 );

    requester1 = new KUrlRequester( grp1 );
    requester1->setMode( KFile::File | KFile::ExistingOnly | KFile::LocalOnly );
    requester1->setUrl( obj->getAppearSoundEffectFileName() );
    upperRow->addWidget( requester1, 6, 1 );

    connect( requester1, SIGNAL( openFileDialog( KUrlRequester * ) ),
             this, SLOT( slotRequesterClicked( KUrlRequester * ) ) );

    connect( requester1, SIGNAL( textChanged( const QString& ) ),
             this, SLOT( slotAppearFileChanged( const QString& ) ) );

    buttonTestPlaySoundEffect1 = new QPushButton( grp1 );
    buttonTestPlaySoundEffect1->setIconSet( KIcon("media-playback-start") );
    buttonTestPlaySoundEffect1->setToolTip( i18n("Play") );
    upperRow->addWidget( buttonTestPlaySoundEffect1, 6, 2 );

    connect( buttonTestPlaySoundEffect1, SIGNAL( clicked() ), this, SLOT( playSound1() ) );

    buttonTestStopSoundEffect1 = new QPushButton( grp1 );
    buttonTestStopSoundEffect1->setIconSet( KIcon("media-playback-stop") );
    buttonTestStopSoundEffect1->setToolTip( i18n("Stop") );
    upperRow->addWidget( buttonTestStopSoundEffect1, 6, 3 );

    connect( buttonTestStopSoundEffect1, SIGNAL( clicked() ), this, SLOT( stopSound1() ) );


    disappear = new QCheckBox( i18n( "Disappear" ), page );
    disappear->setChecked( obj->getDisappear() );
    topLayout->addWidget(disappear);

    Q3GroupBox *grp2 = new Q3GroupBox(0, Qt::Vertical, i18n( "Disappear" ), page);
    grp2->layout()->setSpacing(KDialog::spacingHint());
    grp2->layout()->setMargin(KDialog::marginHint());
    topLayout->addWidget(grp2);
    Q3GridLayout *lowerRow = new Q3GridLayout(grp2->layout(), 5, 4);

    lDisappear = new QLabel( i18n( "Order of disappearance:" ), grp2 );
    lDisappear->setAlignment( Qt::AlignVCenter );
    lowerRow->addWidget(lDisappear, 0, 0);

    eDisappearStep = new QSpinBox( 0, 100, 1, grp2 );
    eDisappearStep->setValue( obj->getDisappearStep() );
    lowerRow->addWidget(eDisappearStep, 0, 1);

    lDEffect = new QLabel( i18n( "Effect (disappearing):" ), grp2 );
    lDEffect->setAlignment( Qt::AlignVCenter );
    lowerRow->addWidget(lDEffect, 1, 0);

    cDisappear = new QComboBox( grp2, "cDisappear" );
    cDisappear->setEditable(false);
    cDisappear->addItem( i18n( "No Effect" ) );
    cDisappear->addItem( i18n( "Disappear to Right" ) );
    cDisappear->addItem( i18n( "Disappear to Left" ) );
    cDisappear->addItem( i18n( "Disappear to Top" ) );
    cDisappear->addItem( i18n( "Disappear to Bottom" ) );
    cDisappear->addItem( i18n( "Disappear to Right/Top" ) );
    cDisappear->addItem( i18n( "Disappear to Right/Bottom" ) );
    cDisappear->addItem( i18n( "Disappear to Left/Top" ) );
    cDisappear->addItem( i18n( "Disappear to Left/Bottom" ) );
    cDisappear->addItem( i18n( "Wipe to Left" ) );
    cDisappear->addItem( i18n( "Wipe to Right" ) );
    cDisappear->addItem( i18n( "Wipe to Top" ) );
    cDisappear->addItem( i18n( "Wipe to Bottom" ) );
    cDisappear->setCurrentIndex( static_cast<int>( obj->getEffect3() ) );
    lowerRow->addWidget(cDisappear, 1, 1);

    connect( cDisappear, SIGNAL( activated( int ) ), this, SLOT( disappearEffectChanged( int ) ) );

    lDisappearSpeed = new QLabel( i18n( "Speed:" ), grp2 );
    lDisappearSpeed->setAlignment( Qt::AlignVCenter );
    lowerRow->addWidget(lDisappearSpeed, 2, 0);

    cDisappearSpeed = new QComboBox( grp2 );
    cDisappearSpeed->setEditable(false);
    cDisappearSpeed->addItem( i18n( "Slow" ) );
    cDisappearSpeed->addItem( i18n( "Medium" ) );
    cDisappearSpeed->addItem( i18n( "Fast" ) );
    cDisappearSpeed->setCurrentIndex( static_cast<int>( obj->getDisappearSpeed() ) );
    lowerRow->addWidget(cDisappearSpeed, 2, 1);

    //appearEffectChanged( cEffect->currentItem() );

    QLabel *lTimerOfDisappear = new QLabel( i18n( "Timer of the object:" ), grp2 );
    lTimerOfDisappear->setAlignment( Qt::AlignVCenter );
    lowerRow->addWidget( lTimerOfDisappear, 3, 0 );

    timerOfDisappear = new KIntNumInput( obj->getDisappearTimer(), grp2 );
    timerOfDisappear->setRange( 1, 600, 1 );
    timerOfDisappear->setSuffix( i18n( " seconds" ) );
    lowerRow->addWidget( timerOfDisappear, 3, 1 );

    if ( view->kPresenterDoc()->spManualSwitch() )
        timerOfDisappear->setEnabled( false );


    // setup the Sound Effect stuff
    disappearSoundEffect = new QCheckBox( i18n( "Sound effect" ), grp2 );
    disappearSoundEffect->setChecked( obj->getDisappearSoundEffect() );
    lowerRow->addWidget( disappearSoundEffect, 4, 0 );
    disappearSoundEffect->setEnabled( disappear->isChecked() );

    connect( disappearSoundEffect, SIGNAL( clicked() ), this, SLOT( disappearSoundEffectChanged() ) );

    lSoundEffect2 = new QLabel( i18n( "File name:" ), grp2 );
    lSoundEffect2->setAlignment( Qt::AlignVCenter );
    lowerRow->addWidget( lSoundEffect2, 5, 0 );

    requester2 = new KUrlRequester( grp2 );
    requester2->setUrl( obj->getDisappearSoundEffectFileName() );
    requester2->setMode( KFile::File | KFile::ExistingOnly | KFile::LocalOnly );
    lowerRow->addWidget( requester2, 5, 1 );

    connect( requester2, SIGNAL( openFileDialog( KUrlRequester * ) ),
             this, SLOT( slotRequesterClicked( KUrlRequester * ) ) );

    connect( requester2, SIGNAL( textChanged( const QString& ) ),
             this, SLOT( slotDisappearFileChanged( const QString& ) ) );

    buttonTestPlaySoundEffect2 = new QPushButton( grp2 );
    buttonTestPlaySoundEffect2->setIconSet( KIcon("media-playback-start") );
    buttonTestPlaySoundEffect2->setToolTip( i18n("Play") );
    lowerRow->addWidget( buttonTestPlaySoundEffect2, 5, 2 );

    connect( buttonTestPlaySoundEffect2, SIGNAL( clicked() ), this, SLOT( playSound2() ) );

    buttonTestStopSoundEffect2 = new QPushButton( grp2 );
    buttonTestStopSoundEffect2->setIconSet( KIcon("media-playback-stop") );
    buttonTestStopSoundEffect2->setToolTip( i18n("Stop") );
    lowerRow->addWidget( buttonTestStopSoundEffect2, 5, 3 );

    connect( buttonTestStopSoundEffect2, SIGNAL( clicked() ), this, SLOT( stopSound2() ) );


    topLayout->activate();

    connect( this, SIGNAL( okClicked() ), this, SLOT( slotEffectDiaOk() ) );
    connect( this, SIGNAL( okClicked() ), this, SLOT( accept() ) );
    connect( disappear, SIGNAL( clicked() ), this, SLOT( disappearChanged() ) );
    disappearChanged();
    appearSoundEffectChanged();
}

KPrEffectDia::~KPrEffectDia()
{
    stopSound1();
    stopSound2();

    delete soundPlayer1;
    delete soundPlayer2;
}

void KPrEffectDia::slotEffectDiaOk()
{
    QList<KPrEffectCmd::EffectStruct> oldEffects;
    for ( unsigned int i = 0; i < objs.count(); ++i ) {
        KPrObject *o = objs.at( i );
        KPrEffectCmd::EffectStruct e;
        e.appearStep = o->getAppearStep();
        e.disappearStep = o->getDisappearStep();
        e.effect = o->getEffect();
        e.effect2 = o->getEffect2();
        e.effect3 = o->getEffect3();
        e.m_appearSpeed = o->getAppearSpeed();
        e.m_disappearSpeed = o->getDisappearSpeed();
        e.disappear = o->getDisappear();
        e.appearTimer = o->getAppearTimer();
        e.disappearTimer = o->getDisappearTimer();
        e.appearSoundEffect = o->getAppearSoundEffect();
        e.disappearSoundEffect = o->getDisappearSoundEffect();
        e.a_fileName = o->getAppearSoundEffectFileName();
        e.d_fileName = o->getDisappearSoundEffectFileName();
        oldEffects << e;
    }

    KPrEffectCmd::EffectStruct eff;
    eff.appearStep = eAppearStep->value();
    eff.disappearStep = eDisappearStep->value();
    eff.effect = ( Effect )cEffect->currentIndex();
    eff.effect2 = ( Effect2 )cEffect2->currentIndex();
    eff.effect3 = ( Effect3 )cDisappear->currentIndex();
    eff.m_appearSpeed = ( EffectSpeed )cAppearSpeed->currentIndex();
    eff.m_disappearSpeed = ( EffectSpeed )cDisappearSpeed->currentIndex();
    eff.disappear = disappear->isChecked();
    eff.appearTimer = timerOfAppear->value();
    eff.disappearTimer = timerOfDisappear->value();
    eff.appearSoundEffect = (requester1->url().isEmpty() ? false : appearSoundEffect->isChecked());
    eff.disappearSoundEffect = (requester2->url().isEmpty() ? false : disappearSoundEffect->isChecked());
    eff.a_fileName = requester1->url().path();
    eff.d_fileName = requester2->url().path();

    KPrEffectCmd *effectCmd = new KPrEffectCmd( i18n( "Assign Object Effects" ), objs, oldEffects, eff );
    effectCmd->execute();
    view->kPresenterDoc()->addCommand( effectCmd );
    accept();
}

void KPrEffectDia::resizeEvent( QResizeEvent *e )
{
    QDialog::resizeEvent( e );
    //topLayout->resize( size() );
}

void KPrEffectDia::disappearChanged()
{
    cDisappear->setEnabled( disappear->isChecked() );
    eDisappearStep->setEnabled( disappear->isChecked() );
    disappearSoundEffect->setEnabled( disappear->isChecked() );
    disappearSoundEffectChanged();
    disappearEffectChanged( cDisappear->currentIndex() );

    if ( !view->kPresenterDoc()->spManualSwitch() )
        timerOfDisappear->setEnabled( disappear->isChecked() );
}

void KPrEffectDia::appearEffectChanged( int /*num*/ )
{
    bool b = ( cEffect->currentIndex() != 0 );
    lAppearSpeed->setEnabled( b );
    cAppearSpeed->setEnabled( b );
}

void KPrEffectDia::disappearEffectChanged( int /*num*/ )
{
    bool b = ( cDisappear->currentIndex() !=0 && disappear->isChecked() );
    lDisappearSpeed->setEnabled( b );
    cDisappearSpeed->setEnabled( b );
}

void KPrEffectDia::appearSoundEffectChanged()
{
    lSoundEffect1->setEnabled( appearSoundEffect->isChecked() );
    requester1->setEnabled( appearSoundEffect->isChecked() );

    if ( !requester1->url().isEmpty() ) {
        buttonTestPlaySoundEffect1->setEnabled( appearSoundEffect->isChecked() );
        buttonTestStopSoundEffect1->setEnabled( appearSoundEffect->isChecked() );
    }
    else {
        buttonTestPlaySoundEffect1->setEnabled( false );
        buttonTestStopSoundEffect1->setEnabled( false );
    }
}

void KPrEffectDia::disappearSoundEffectChanged()
{
    lSoundEffect2->setEnabled( disappear->isChecked() && disappearSoundEffect->isChecked() );
    requester2->setEnabled( disappear->isChecked() && disappearSoundEffect->isChecked() );

    if ( !requester2->url().isEmpty() ) {
        buttonTestPlaySoundEffect2->setEnabled( disappear->isChecked() && disappearSoundEffect->isChecked() );
        buttonTestStopSoundEffect2->setEnabled( disappear->isChecked() && disappearSoundEffect->isChecked() );
    }
    else {
        buttonTestPlaySoundEffect2->setEnabled( false );
        buttonTestStopSoundEffect2->setEnabled( false );
    }
}

void KPrEffectDia::slotRequesterClicked( KUrlRequester *requester )
{
    QString filter = getSoundFileFilter();
    requester->fileDialog()->setFilter( filter );

    // find the first "sound"-resource that contains files
    QStringList soundDirs = KGlobal::dirs()->resourceDirs( "sound" );
    if ( !soundDirs.isEmpty() ) {
        KUrl soundURL;
        QDir dir;
        dir.setFilter( QDir::Files | QDir::Readable );
        QStringList::ConstIterator it = soundDirs.begin();
        while ( it != soundDirs.end() ) {
            dir = *it;
            if ( dir.isReadable() && dir.count() > 2 ) {
                soundURL.setPath( *it );
                requester->fileDialog()->setUrl( soundURL );
                break;
            }
            ++it;
        }
    }
}

void KPrEffectDia::slotAppearFileChanged( const QString &text )
{
    buttonTestPlaySoundEffect1->setEnabled( !text.isEmpty() );
    buttonTestStopSoundEffect1->setEnabled( !text.isEmpty() );
}

void KPrEffectDia::slotDisappearFileChanged( const QString &text )
{
    buttonTestPlaySoundEffect2->setEnabled( !text.isEmpty() );
    buttonTestStopSoundEffect2->setEnabled( !text.isEmpty() );
}

void KPrEffectDia::playSound1()
{
    delete soundPlayer1;
    soundPlayer1 = new KPrSoundPlayer( requester1->url().path() );
    soundPlayer1->play();

    buttonTestPlaySoundEffect1->setEnabled( false );
    buttonTestStopSoundEffect1->setEnabled( true );
}

void KPrEffectDia::playSound2()
{
    delete soundPlayer2;
    soundPlayer2 = new KPrSoundPlayer( requester2->url().path() );
    soundPlayer2->play();

    buttonTestPlaySoundEffect2->setEnabled( false );
    buttonTestStopSoundEffect2->setEnabled( true );
}

void KPrEffectDia::stopSound1()
{
    if ( soundPlayer1 ) {
        soundPlayer1->stop();
        delete soundPlayer1;
        soundPlayer1 = 0;

        buttonTestPlaySoundEffect1->setEnabled( true );
        buttonTestStopSoundEffect1->setEnabled( false );
    }
}

void KPrEffectDia::stopSound2()
{
    if ( soundPlayer2 ) {
        soundPlayer2->stop();
        delete soundPlayer2;
        soundPlayer2 = 0;

        buttonTestPlaySoundEffect2->setEnabled( true );
        buttonTestStopSoundEffect2->setEnabled( false );
    }
}

QString KPrEffectDia::getSoundFileFilter() const
{
    QStringList fileList;
    fileList << "wav" << "au" << "mp3" << "mp1" << "mp2" << "mpg" << "dat"
             << "mpeg" << "ogg" << "cdda" << "cda " << "vcd" << "null";
    fileList.sort();

    bool comma = false;
    QString full, str;
    for ( QStringList::ConstIterator it = fileList.begin(); it != fileList.end(); ++it ) {
        if ( comma )
            str += '\n';
        comma = true;
        str +=  i18n( "*.%1|%2 Files" , *it , (*it).upper() );

        full += QString( "*.") + (*it) + ' ';
    }

    str = full + '|' + i18n( "All Supported Files" ) + '\n' + str;
    str += "\n*|" + i18n( "All Files" );

    return str;
}

#include "KPrEffectDia.moc"
