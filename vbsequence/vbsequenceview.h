#ifndef VBSEQUENCEVIEW_H
#define VBSEQUENCEVIEW_H

#include <QListView>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QMap>
#include <QPoint>
#include <QMap>
#include <QSet>
#include <QPicture>
#include <QAction>

#include "vbsequence.h"
#include "vbsequencescene.h"

namespace QtVB
{
  class SequenceItem;
  class ExecItem;
    class BlockItem;
    class JobItem;
  class WaitforItem;
  class SequenceScene;
  class SequenceView;
  
  class QExecItemAction : public QAction
  {
    Q_OBJECT
    
    public:
      QExecItemAction(const QString & text, QObject * parent)
        : QAction(text, parent), exec_item(0) {}
        
      ExecItem* exec_item;
  };
  
  class SequenceView : public QGraphicsView
  {
    Q_OBJECT
    
    public:
      SequenceView(QWidget* parent = 0);
      
      SequenceScene* scene() const;
      
    signals:
      void jobAdded(shared_ptr<VB::Job>);
      void jobSelected(shared_ptr<VB::Job>);
      void jobDeselected(shared_ptr<VB::Job>);
      
      void blockAdded(shared_ptr<VB::Block>);
      void blockSelected(shared_ptr<VB::Block>);
      void blockDeselected(shared_ptr<VB::Block>);
      
      void execMoved(shared_ptr<VB::Exec>, QRect const& oldr, QRect const& newr);
      void execMovedBy(shared_ptr<VB::Exec>, QPoint const& moved_by);
      
      void jobClicked(shared_ptr<VB::Job>, QMouseEvent*);
      void blockClicked(shared_ptr<VB::Block>, QMouseEvent*);
      
    public slots:
      void disconnectClickedExecExec();
      void deleteClickedExec();
      void toggleClickedBlockDetail(bool);
      void beginFromClickedExec();
      void endAtClickedExec();
      void noPointAtClickedExec();
      void makeBlockFromSelectedExecs();
      void updateSceneRect(const QRectF & rect);
            
    protected:
      void createActions();
      
      virtual void dragEnterEvent(QDragEnterEvent* event);
      virtual void dragLeaveEvent(QDragLeaveEvent* event);
      virtual void dragMoveEvent(QDragMoveEvent *event);
      virtual void dropEvent(QDropEvent* event);
      
      virtual void mousePressEvent(QMouseEvent* event);
      virtual void mouseMoveEvent(QMouseEvent* event);
      virtual void mouseReleaseEvent(QMouseEvent* event);
      
      virtual void contextMenuEvent(QContextMenuEvent* e);
      void singleExecContextMenu(QMenu* m, ExecItem* ei);
      void singleJobContextMenu(QMenu* m, JobItem* ji);
      void singleBlockContextMenu(QMenu* m, BlockItem* bi);
      
      void makeDisconnectExecActions(QMenu* disconnect_menu, const std::list<shared_ptr<VB::Exec> >& connected_jobs);
    
    public:
      void showDataText(bool vis = true);
      bool isDataTextVisible();
      
    protected:
      VB::DataSet* dataset;
			bool dataTextVisible;
			
			QList<QExecItemAction*> disconnectExecActs;
			QExecItemAction* deleteExecAct;
			QExecItemAction* toggleDetailAct;
			QExecItemAction* beginPointAct;
			QExecItemAction* endPointAct;
			QExecItemAction* noPointAct;
			
			QAction* makeBlockFromSelectedAct;
			
			ExecItem* beginPointExec;
			ExecItem* endPointExec;
			
			TempWaitforDragItem* wdi;
  };
}

inline uint qHash(QPair<int,int> &key)
{
  return qHash(key.first)*100 + qHash(key.second);
}

inline uint qHash(const void* &key)
{
  return qHash(reinterpret_cast<long>(key));
}

/*template <typename T>
inline uint qHash(const smart_pointer<T> &key)
{
  return qHash(reinterpret_cast<void*>(key->c_pointer()));
}*/

#endif

