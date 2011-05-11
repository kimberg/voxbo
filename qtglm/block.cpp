/****************************************************************************
** Form implementation generated from reading ui file 'block.ui'
**
** Created: Tue Jan 23 11:56:53 2007
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.4   edited Nov 24 2003 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "block.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <q3buttongroup.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <qaction.h>
#include <qmenubar.h>
#include <q3popupmenu.h>
#include <q3toolbar.h>

/*
 *  Constructs a BlockDesign as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
BlockDesign::BlockDesign( QWidget* parent, const char* name, Qt::WFlags fl )
    : Q3MainWindow( parent, name, fl )
{
    (void)statusBar();
    if ( !name )
	setName( "BlockDesign" );
    setCentralWidget( new QWidget( this, "qt_central_widget" ) );

    cancelButton = new QPushButton( centralWidget(), "cancelButton" );
    cancelButton->setGeometry( QRect( 180, 300, 70, 31 ) );

    textLabel5 = new QLabel( centralWidget(), "textLabel5" );
    textLabel5->setGeometry( QRect( 10, 260, 140, 20 ) );
    QFont textLabel5_font(  textLabel5->font() );
    textLabel5_font.setPointSize( 11 );
    textLabel5->setFont( textLabel5_font ); 
    textLabel5->setAlignment( int( Qt::AlignCenter ) );

    offEditor = new QLineEdit( centralWidget(), "offEditor" );
    offEditor->setGeometry( QRect( 150, 220, 170, 25 ) );
    QFont offEditor_font(  offEditor->font() );
    offEditor_font.setPointSize( 12 );
    offEditor->setFont( offEditor_font ); 

    orderGroup = new Q3ButtonGroup( centralWidget(), "orderGroup" );
    orderGroup->setGeometry( QRect( 20, 90, 130, 80 ) );
    QFont orderGroup_font(  orderGroup->font() );
    orderGroup_font.setPointSize( 9 );
    orderGroup->setFont( orderGroup_font ); 
    orderGroup->setLineWidth( 1 );
    orderGroup->setAlignment( int( Qt::AlignHCenter ) );

    onFirst = new QRadioButton( orderGroup, "onFirst" );
    onFirst->setGeometry( QRect( 10, 20, 80, 20 ) );
    QFont onFirst_font(  onFirst->font() );
    onFirst_font.setPointSize( 11 );
    onFirst->setFont( onFirst_font ); 
    onFirst->setChecked( TRUE );

    offFirst = new QRadioButton( orderGroup, "offFirst" );
    offFirst->setGeometry( QRect( 10, 50, 70, 20 ) );
    QFont offFirst_font(  offFirst->font() );
    offFirst_font.setPointSize( 11 );
    offFirst->setFont( offFirst_font ); 

    textLabel1 = new QLabel( centralWidget(), "textLabel1" );
    textLabel1->setGeometry( QRect( 110, 10, 130, 30 ) );
    QFont textLabel1_font(  textLabel1->font() );
    textLabel1_font.setPointSize( 12 );
    textLabel1_font.setBold( TRUE );
    textLabel1_font.setUnderline( TRUE );
    textLabel1->setFont( textLabel1_font ); 

    unitGroup = new Q3ButtonGroup( centralWidget(), "unitGroup" );
    unitGroup->setGeometry( QRect( 160, 90, 160, 80 ) );
    unitGroup->setLineWidth( 1 );
    unitGroup->setAlignment( int( Qt::AlignHCenter ) );

    ms = new QRadioButton( unitGroup, "ms" );
    ms->setGeometry( QRect( 10, 50, 40, 20 ) );
    QFont ms_font(  ms->font() );
    ms_font.setPointSize( 11 );
    ms->setFont( ms_font ); 

    TR = new QRadioButton( unitGroup, "TR" );
    TR->setGeometry( QRect( 10, 20, 50, 20 ) );
    QFont TR_font(  TR->font() );
    TR_font.setPointSize( 11 );
    TR->setFont( TR_font ); 
    TR->setChecked( TRUE );

    numberEditor = new QLineEdit( centralWidget(), "numberEditor" );
    numberEditor->setGeometry( QRect( 150, 260, 170, 25 ) );
    QFont numberEditor_font(  numberEditor->font() );
    numberEditor_font.setPointSize( 12 );
    numberEditor->setFont( numberEditor_font ); 

    okButton = new QPushButton( centralWidget(), "okButton" );
    okButton->setGeometry( QRect( 70, 300, 80, 30 ) );

    textLabel3 = new QLabel( centralWidget(), "textLabel3" );
    textLabel3->setGeometry( QRect( 20, 180, 120, 20 ) );
    QFont textLabel3_font(  textLabel3->font() );
    textLabel3_font.setPointSize( 11 );
    textLabel3->setFont( textLabel3_font ); 
    textLabel3->setAlignment( int( Qt::AlignVCenter | Qt::AlignRight ) );

    textLabel4 = new QLabel( centralWidget(), "textLabel4" );
    textLabel4->setGeometry( QRect( 20, 220, 120, 20 ) );
    QFont textLabel4_font(  textLabel4->font() );
    textLabel4_font.setPointSize( 11 );
    textLabel4->setFont( textLabel4_font ); 
    textLabel4->setAlignment( int( Qt::AlignVCenter | Qt::AlignLeft ) );

    textLabel2 = new QLabel( centralWidget(), "textLabel2" );
    textLabel2->setGeometry( QRect( 20, 50, 90, 20 ) );
    QFont textLabel2_font(  textLabel2->font() );
    textLabel2_font.setPointSize( 11 );
    textLabel2->setFont( textLabel2_font ); 
    textLabel2->setAlignment( int( Qt::AlignVCenter | Qt::AlignLeft ) );

    nameEditor = new QLineEdit( centralWidget(), "nameEditor" );
    nameEditor->setGeometry( QRect( 105, 50, 210, 25 ) );
    QFont nameEditor_font(  nameEditor->font() );
    nameEditor_font.setPointSize( 12 );
    nameEditor->setFont( nameEditor_font ); 

    onEditor = new QLineEdit( centralWidget(), "onEditor" );
    onEditor->setGeometry( QRect( 150, 180, 170, 25 ) );
    QFont onEditor_font(  onEditor->font() );
    onEditor_font.setPointSize( 12 );
    onEditor->setFont( onEditor_font ); 
    onEditor->setCursorPosition( 0 );

    // toolbars

    languageChange();
    resize( QSize(341, 359).expandedTo(minimumSizeHint()) );
    // setAttribute(Qt::WState_Polished,0);

    // tab order
    setTabOrder( nameEditor, onFirst );
    setTabOrder( onFirst, offFirst );
    setTabOrder( offFirst, TR );
    setTabOrder( TR, ms );
    setTabOrder( ms, onEditor );
    setTabOrder( onEditor, offEditor );
    setTabOrder( offEditor, numberEditor );
    setTabOrder( numberEditor, okButton );
    setTabOrder( okButton, cancelButton );
}

/*
 *  Destroys the object and frees any allocated resources
 */
BlockDesign::~BlockDesign()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void BlockDesign::languageChange()
{
    setCaption( tr( "Block Design" ) );
    cancelButton->setText( tr( "Cancel" ) );
    textLabel5->setText( tr( "Number of Blocks:" ) );
    orderGroup->setTitle( tr( "Set Order of Blocks" ) );
    onFirst->setText( tr( "On First" ) );
    offFirst->setText( tr( "Off First" ) );
    textLabel1->setText( tr( "Block Design" ) );
    unitGroup->setTitle( tr( "Set Unit of Block Length" ) );
    ms->setText( tr( "ms" ) );
    TR->setText( tr( "TR" ) );
    okButton->setText( tr( "OK" ) );
    textLabel3->setText( tr( "Block Length (On):" ) );
    textLabel4->setText( tr( "Block Length (Off):" ) );
    textLabel2->setText( tr( "Effect Name:" ) );
    onEditor->setInputMask( QString::null );
}

