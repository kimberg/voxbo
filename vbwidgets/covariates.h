
// covariates.h
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

#ifndef COVARIATES_H
#define COVARIATES_H

#include "plotscreen.h"
#include "glmutil.h"

#include <string>
#include <vector>
#include <q3mainwindow.h>
#include <q3listview.h>
#include <qstring.h>

#include <string>
#include <vector>
#include <list>

namespace VB
{
  class Covariate;

  class Contrast;
//  class Contrast::Parameter;

  class CovariatesView;
  class ContrastsView;
  class ContParamsView;

  /*---------------------------------------------------------------------------*/

  class Covariate : public QObject
  {
    Q_OBJECT
    
    public:
      enum InterestType {
        INTEREST_ERR = -1, INTEREST_I, INTEREST_N,
        INTEREST_K, INTEREST_U, INTEREST_D 
      };
      
      Covariate(QObject * parent = 0, const char * name = 0);
      Covariate(const Covariate& aCov);
      
      static InterestType str2type(const std::string& str);
      static std::string type2str(InterestType inter);
    
      void setName(const std::string& aName);
      void setType(InterestType aType);
      
      const std::string& getName() const;
      const InterestType& getType() const;
      
      Covariate& operator=(const Covariate& aCov);
      
    private:
      std::string mName; // name inlcudes group hierarchy position
      InterestType mInterest;
      
     signals:
      void nameChanged( const std::string& );
      void typeChanged( const InterestType& );
  };

  class Contrast : QObject
  {
    Q_OBJECT
    
    public:
      typedef double Parameter;
      enum ScaleType {
        SCALE_ERR = -1, SCALE_BY_SCERR, SCALE_BY_INTERCEPT, 
        SCALE_BY_RATIO, SCALE_BY_RAWERR, SCALE_NONE
      };
      
    public:
      Contrast(std::vector<Covariate>& aCovList,
               QObject * parent = 0, const char * name = 0);
      // This is bad data-hiding (or lack thereof) practice, but i'm gonna trust
      // the programmer to not shoot themselves in the foot and not change the 
      // covariates when they shouldn't or something.  One thing to keep in mind
      // is that "covariates" is a reference (saves some space), so changing it
      // here will change it in any other Contrast objects that share the 
      // reference.  This can be a good thing; if a Covariate name actually 
      // does legitimately change, then it only has to be modified in one place
      // and all the Contrasts will be notified.  Just don't shoot yourself in
      // the foot.
      
      std::string Name;
      
      // - should be -
    //VB::Vector<Parameter> Weights;
      // - but for now -
      VB_Vector Weights;
      
      std::vector<Covariate>& Covariates;
      
      // - should be - 
    //ScaleType Scale; // from vbstatmap
      // - but for now - 
      std::string Scale;
      
      // When accessing a single weight, one should really use one of the
      // following methods, not the Weights variable directly.  And one should
      // [pretty much] never use the Covariates variable for anything; I'd make
      // it protected if I were absolutely certain of that.
      Parameter& operator[](int aIndex);
      Parameter& operator[](std::string aCovName);
  };

  /*-------------------------------------------------------------------------*/

  /***************************************************************************
   *       CovariatesView class: written as an abstraction of other 
   *                             covariate-containing QListViews
   ***************************************************************************/
  class CovariatesView : public Q3ListView
  {
      Q_OBJECT
    public:
      static const char* NAME_COL;
      static const char* ID_COL;
      static const char* TYPE_COL;
      
    protected:
      std::vector<Covariate>* mCovariates;
      std::list<Q3ListViewItem*> mSelection;
      std::list<int> mSelectionIDs;
        
    public:
      CovariatesView(QWidget *parent = 0, const char *name = 0);
      
      virtual void buildTree(GLMInfo* aGLMInfo, bool aShowAll = true);
      virtual void buildTree(std::vector<Covariate>& aCovList, bool aShowAll = true);
      virtual void buildTree(std::vector<std::string>& aNameList,
                             std::vector<std::string>& aTypeList, bool aShowAll = true);
      virtual void copyTree(const CovariatesView* aCovView, bool aShowAll = true);

      Q3ListViewItem* firstChild(const Q3ListViewItem* parent = 0) const;
      Q3ListViewItem* lastChild(const Q3ListViewItem* parent = 0) const;
      Q3ListViewItem* findChild(const Q3ListViewItem* parent,
                               const QString& text, int column) const;
      Q3ListViewItem* findChild(const QString& text, int column) const;
      Q3ListViewItem* findGroup(const Q3ListViewItem* parent,
                               const QString& text) const;
      Q3ListViewItem* findGroup(const QString& text) const;

      Q3ListViewItem* findParent(Q3ListViewItem *inputItem) const;
      
      int columnNumber(const QString& text);
      void setColumnText(int column, const QStringList& textList);
      void setColumnText(int column, const QString& text);
      void setColumnText(const QString& column, const QStringList& textList);
      void setColumnText(const QString& column, const QString& text);
      
      void setSelectedColumnText(int column, const QString& text);
      void setSelectedColumnText(const QString& column, const QString& text);
      
      int itemIndex(const Q3ListViewItem* item);
      std::list<Q3ListViewItem*>& selectedItems();
      std::list<int>& selectedItemIDs();
      
    private:
      void setupColumns();
      
    public slots:
      void showInterestOnly(bool aInterestOnly = true);
      virtual void clear();
      
    protected slots:
      void onSelectionChanged( );
  };

  class ContrastsView : public Q3ListView
  {
      Q_OBJECT
    protected:
      // eventually - 
    //std::vector<Contrast>* mContrasts;
      // for now -
      std::vector<VBContrast*> mContrasts;
      VBContrast* mCurrentContrast;
    
    public:
      ContrastsView(QWidget *parent = 0, const char *name = 0);
      
      virtual void buildList(GLMInfo* aGLMInfo);
      virtual void buildList(std::vector<Contrast>& aContList);
      virtual void buildList(std::vector<std::string> aNameList,
                             std::vector<std::string> aScaleList,
                             std::vector<VB_Vector> aWeightList);
      virtual void buildList(std::vector<VBContrast*>& aContrastList);
      
      VBContrast* selectedContrast() const;
      VBContrast* contrastAt(const Q3ListViewItem* item, bool diag = false);
      
      void insertContrast(VBContrast* aContrast);
      void takeContrast(VBContrast* aContrast);
      
      int itemIndex(const Q3ListViewItem* item);
      
    protected slots:
      void onSelectionChanged();
      void onContrastRenamed(Q3ListViewItem * item, int col, const QString & text);
    
  };

  class ContParamsView : public CovariatesView
  {
      Q_OBJECT
    public:
      static const char* WEIGHT_COL;
    
    public:
      ContParamsView(QWidget *parent = 0, const char *name = 0);
      
      virtual void buildTree(GLMInfo* aGLMInfo, bool aShowAll = true);
      virtual void buildTree(std::vector<Covariate>& aCovList, bool aShowAll = true);
      virtual void buildTree(std::vector<std::string>& aNameList,
                             std::vector<std::string>& aTypeList, bool aShowAll = true);
                             
      void setContrast(const VBContrast& aContrast);
      void setContrast(const VB_Vector& aContrast);
      void clearContrast();
  };
  
};

#endif
