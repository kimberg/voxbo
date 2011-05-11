/****************************************************************************
** Form implementation generated from reading ui file 'paired.ui'
**
** Created: Tue Jan 23 10:57:45 2007
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.4   edited Nov 24 2003 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "paired.h"

#include <qvariant.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <q3buttongroup.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <qaction.h>
#include <qmenubar.h>
#include <q3popupmenu.h>
#include <q3toolbar.h>

/*
 *  Constructs a PairDesign as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
PairDesign::PairDesign( QWidget* parent, const char* name, Qt::WFlags fl )
    : Q3MainWindow( parent, name, fl )
{
    (void)statusBar();
    if ( !name )
	setName( "PairDesign" );
    setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 0, 0, sizePolicy().hasHeightForWidth() ) );
    setCentralWidget( new QWidget( this, "qt_central_widget" ) );

    textLabel2 = new QLabel( centralWidget(), "textLabel2" );
    textLabel2->setGeometry( QRect( 20, 50, 90, 20 ) );
    QFont textLabel2_font(  textLabel2->font() );
    textLabel2_font.setPointSize( 11 );
    textLabel2->setFont( textLabel2_font ); 
    textLabel2->setAlignment( int( Qt::AlignCenter ) );

    numberEditor = new QLineEdit( centralWidget(), "numberEditor" );
    numberEditor->setGeometry( QRect( 160, 150, 100, 25 ) );
    QFont numberEditor_font(  numberEditor->font() );
    numberEditor_font.setPointSize( 12 );
    numberEditor->setFont( numberEditor_font ); 
    numberEditor->setCursorPosition( 0 );

    orderGroup = new Q3ButtonGroup( centralWidget(), "orderGroup" );
    orderGroup->setGeometry( QRect( 20, 90, 240, 50 ) );
    QFont orderGroup_font(  orderGroup->font() );
    orderGroup_font.setPointSize( 9 );
    orderGroup->setFont( orderGroup_font ); 
    orderGroup->setLineWidth( 0 );
    orderGroup->setAlignment( int( Qt::AlignHCenter ) );

    subject = new QRadioButton( orderGroup, "subject" );
    subject->setGeometry( QRect( 10, 0, 230, 20 ) );
    QFont subject_font(  subject->font() );
    subject_font.setPointSize( 11 );
    subject->setFont( subject_font ); 
    subject->setChecked( TRUE );

    group = new QRadioButton( orderGroup, "group" );
    group->setGeometry( QRect( 10, 30, 230, 20 ) );
    QFont group_font(  group->font() );
    group_font.setPointSize( 11 );
    group->setFont( group_font ); 

    textLabel1 = new QLabel( centralWidget(), "textLabel1" );
    textLabel1->setGeometry( QRect( 90, 10, 110, 30 ) );
    QFont textLabel1_font(  textLabel1->font() );
    textLabel1_font.setPointSize( 12 );
    textLabel1_font.setBold( TRUE );
    textLabel1_font.setUnderline( TRUE );
    textLabel1->setFont( textLabel1_font ); 

    textLabel5 = new QLabel( centralWidget(), "textLabel5" );
    textLabel5->setGeometry( QRect( 20, 150, 140, 20 ) );
    QFont textLabel5_font(  textLabel5->font() );
    textLabel5_font.setPointSize( 11 );
    textLabel5->setFont( textLabel5_font ); 
    textLabel5->setAlignment( int( Qt::AlignCenter ) );

    nameEditor = new QLineEdit( centralWidget(), "nameEditor" );
    nameEditor->setGeometry( QRect( 110, 50, 150, 25 ) );
    QFont nameEditor_font(  nameEditor->font() );
    nameEditor_font.setPointSize( 12 );
    nameEditor->setFont( nameEditor_font ); 

    cancelButton = new QPushButton( centralWidget(), "cancelButton" );
    cancelButton->setGeometry( QRect( 160, 190, 70, 30 ) );

    okButton = new QPushButton( centralWidget(), "okButton" );
    okButton->setGeometry( QRect( 60, 190, 70, 30 ) );

    // toolbars

    languageChange();
    resize( QSize(278, 242).expandedTo(minimumSizeHint()) );
    // clearWState( WState_Polished );

    // tab order
    setTabOrder( nameEditor, subject );
    setTabOrder( subject, numberEditor );
    setTabOrder( numberEditor, okButton );
    setTabOrder( okButton, cancelButton );
    setTabOrder( cancelButton, group );
}

/*
 *  Destroys the object and frees any allocated resources
 */
PairDesign::~PairDesign()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void PairDesign::languageChange()
{
    setCaption( tr( "Paired Design" ) );
    textLabel2->setText( tr( "Effect Name:" ) );
    numberEditor->setInputMask( QString::null );
    orderGroup->setTitle( QString::null );
    subject->setText( tr( "Subjects together (AABBCC...)" ) );
    group->setText( tr( "Groups together (ABCABC...)" ) );
    textLabel1->setText( tr( "Paired t-Test" ) );
    textLabel5->setText( tr( "Number of Subjects:" ) );
    nameEditor->setText( tr( "group" ) );
    cancelButton->setText( tr( "Cancel" ) );
    okButton->setText( tr( "OK" ) );
}

