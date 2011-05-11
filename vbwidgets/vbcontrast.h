
// vbcontrast.h
// Copyright (c) 2010 by The VoxBo Development Team

// This file is part of VoxBo
// 
// VoxBo is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VoxBo is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VoxBo.  If not, see <http://www.gnu.org/licenses/>.
// 
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
// 
// original version written by Tom King?

#include <qspinbox.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <q3listbox.h>
#include <q3scrollview.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qdialog.h>
#include <qcheckbox.h>
#include <QLineEdit>
#include <q3filedialog.h>
#include <q3groupbox.h>
#include <QBoxLayout>

#include <cmath>
#include <cstdlib>
#include <vector>
#include <list>
#include <map>
#include <iostream>

#include "glmutil.h"
#include "covariates.h"
#include "qdecimalspinbox.h"

namespace VB
{

  class VBContrastParamScalingWidget : public QDialog
  {
    Q_OBJECT
  public:
    VBContrastParamScalingWidget(QWidget *parent=0,const char *name=0);
    ~VBContrastParamScalingWidget();

  protected:
    std::string mFileStem;
    std::vector<std::string> mScalesText;
    std::map<std::string, int> mScalesIndex;
    
    VB::ContrastsView *mContrastList;
    QPushButton *mNewContrast;
    QPushButton *mDupContrast;
    QPushButton *mDelContrast;
    
    QComboBox *mScaleByCombo; // get from vbstatmap:
    
    VB::ContParamsView *mContrastParamList;
    QDecimalSpinBox *mParamWeight;
    QCheckBox *mShowInterestOnly;
    QPushButton *mTargetVectorButton;
    QPushButton *mZeroAllButton;
    
    QPushButton *mOpenButton;
    QPushButton *mSaveButton;
    QPushButton *mCancelButton;
    
    QBoxLayout *mContrastParamsLayout;
    
    ///////////////////////////////////////////////////////////////////////////
    
    GLMInfo* mGLMInfo;
    bool mWriteOnAccept;
  
  public:
    void LoadContrastInfo( std::string stemname );
    void WriteContrastInfo( std::string stemname );
    
    void showBrowseButton(bool s = true);
    void writeFilesOnExit(bool s = true);
    
    VBContrast* selectedContrast();
    
    void initialize();
    void clearContrastParamProps();
    
  protected slots:
    void onContrastVectorSelected( );
    void onContrastVectorDoubleClicked( Q3ListViewItem *, const QPoint &, int );
    void onContrastParamsSelected( );
    void onContrastScaleChanged( int );
    void onBrowseForParamFile();
//    void onBrowseForGFile();
    
    void onNewContrast();
    void onDupContrast();
    void onDelContrast();
    
    void zeroAll( float value = 0.0 );
    
    void changeType( int );
    void changeWeight( int );
    
    void diagnostics( int );
    
    virtual void accept();
    virtual void reject();
    
//    void syncCurrentContrast( int );
  signals:
    void contrastAccepted(VBContrast*);
  };
}
