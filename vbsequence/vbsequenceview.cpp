#include <QtGui>
#include <iostream>
#include <string>
#include <sstream>
#include <typeinfo>

#include "vbsequenceview.h"
#include "vbsequencescene.h"
#include "vbsequenceitem.h"

using namespace VB;
using namespace QtVB;
using namespace std;
using namespace std::tr1;

typedef QPair<int,int> intPair;
// ... because sometimes I use Qt's vbforeach, which is a macro, and the compiler
// whines if I put a comma in it.

/*
 * SequenceView::SequenceView(QWidget *parent)
 * 
 * Create a new SequenceView widget.
 */
SequenceView::SequenceView(QWidget *parent)
  : QGraphicsView(parent)
{
  setMouseTracking(true);
  setDragMode(QGraphicsView::RubberBandDrag);
/*  
  selection = 0;
  hovertion = 0;
  dragation = 0;
  
  dragMode = MOVE_MODE;
  connectStyle = LINE_STYLE;
  
  dragJob = dragRes = dragArg = dragDat = false;
  scrollOffset = QPoint(0,0);
  
  updateRect = QRect(0,0,0,0);
  dataset = 0;
  dataVisible = true;
  dataTextVisible = true;
  resStubsVisible = false;
  
  clear();*/
  
  createActions();
}

void SequenceView::createActions()
{
  deleteExecAct = new QExecItemAction("Delete this executable", this);
  connect(deleteExecAct, SIGNAL(triggered()), this, SLOT(deleteClickedExec()));
  
  toggleDetailAct = new QExecItemAction("Toggle block detail", this);
  toggleDetailAct->setCheckable(true);
  connect(toggleDetailAct, SIGNAL(toggled(bool)), this, SLOT(toggleClickedBlockDetail(bool)));
  
  beginPointAct = new QExecItemAction("Set as begin point", this);
  connect(beginPointAct, SIGNAL(triggered()), this, SLOT(beginFromClickedExec()));
  
  endPointAct = new QExecItemAction("Set as end point", this);
  connect(endPointAct, SIGNAL(triggered()), this, SLOT(endAtClickedExec()));
  
  noPointAct = new QExecItemAction("Unset checkpoint", this);
  connect(noPointAct, SIGNAL(triggered()), this, SLOT(noPointAtClickedExec()));  
  
  makeBlockFromSelectedAct = new QExecItemAction("Make block from selected executables", this);
  connect(makeBlockFromSelectedAct, SIGNAL(triggered()), this, SLOT(makeBlockFromSelectedExecs()));
}

SequenceScene* SequenceView::scene() const
{
	return static_cast<SequenceScene*>(QGraphicsView::scene());
}

/*
 * void SequenceView::dragEnterEvent(QDragEnterEvent* event)
 *
 * What to do when data is dragged into this widget's viewport()'s space.
 */
void SequenceView::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->mimeData()->hasFormat("image/x-voxbo-jobtype"))
  {
    event->accept();
  }
  else
  {
    event->ignore();
  }
}

void SequenceView::dragMoveEvent(QDragMoveEvent *event)
{
  if (event->mimeData()->hasFormat("image/x-voxbo-jobtype")) 
  {
    event->setDropAction(Qt::CopyAction);
    event->accept();
  } 
  else 
  {
    event->ignore();
  }
}
 
 /*
 * void SequenceView::dragLeaveEvent(QDragLeaveEvent *event)
 *
 * What to do when data is dragged out of this widget's viewport()'s space.
 */
void SequenceView::dragLeaveEvent(QDragLeaveEvent *event)
{
  event->accept();
}

/*
 * void SequenceView::dropEvent(QDropEvent *event)
 * 
 * What to do when data is dropped into this widget's viewport()'s space.
 */
void SequenceView::dropEvent(QDropEvent *event)
{
  QPointF point = mapToScene(event->pos());
  
  if (event->mimeData()->hasFormat("image/x-voxbo-jobtype")) {

    QByteArray jobData = event->mimeData()->data("image/x-voxbo-jobtype");
    QDataStream stream(&jobData, QIODevice::ReadOnly);
    QPixmap pixmap;
    QPointF location;
    JobType* jobtype;
    int pointer_val;
    
    stream >> pixmap >> pointer_val;
    jobtype = (JobType*)pointer_val;
    
    location = point;
    
    JobItem* ji = scene()->addJob(jobtype);
    ji->setPos(location);
    
    QRectF sr = scene()->sceneRect();
    
    event->setDropAction(Qt::CopyAction);
    event->accept();
    
    emit jobAdded(const_pointer_cast<Job>(ji->job()));

  } 
  else 
    event->ignore();
  
  scene()->update(scene()->sceneRect());
}

void SequenceView::mousePressEvent(QMouseEvent* event)
{
  scene()->supress_settler();
  wdi = 0;
  
  if (event->button() == Qt::LeftButton &&
      event->modifiers() == Qt::ShiftModifier)
  {
    setInteractive(false);
    
    ExecItem* ei = 0;
    vbforeach (QGraphicsItem* item_at, items(event->pos()))
      if ((ei = qgraphicsitem_cast<ExecItem*>(item_at)) != 0) break;
    
    if (ei)
    {
      wdi = new TempWaitforDragItem(ei);
    }
  }
  
  QGraphicsView::mousePressEvent(event);
}

void SequenceView::mouseMoveEvent(QMouseEvent* event)
{
  if (wdi)
  {
    wdi->setToEnd(wdi->mapFromScene(mapToScene(event->pos())));
    wdi->update();
  }
  
  QGraphicsView::mouseMoveEvent(event);
}

void SequenceView::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton && wdi)
  {
    ExecItem* ei = 0;
    vbforeach (QGraphicsItem* item_at, items(event->pos()))
    {
      ei = dynamic_cast<ExecItem*>(item_at);
      if (ei) break;
    }
    
    if (ei && 
        ei->parentItem() == wdi->parentItem())
    {
      shared_ptr<Exec> e1 = wdi->fromItem()->exec();
      shared_ptr<Exec> e2 = ei->exec();
      if (e1 != e2 && !(e1->depends_on(e2)) &&
          std::find(e2->depends.begin(), e2->depends.end(), e1) == e2->depends.end())
      {
        e2->depends.push_back(e1);
        new WaitforItem(wdi->fromItem(), ei);
      }
    }
    
    scene()->removeItem(wdi);
    delete wdi;
    wdi = 0;
  }
  
  setInteractive(true);
  scene()->supress_settler(false);
  scene()->arrange();
  
  QGraphicsView::mouseReleaseEvent(event);
}

void SequenceView::showDataText(bool vis)
{
  dataTextVisible = vis;
}

bool SequenceView::isDataTextVisible()
{
  return dataTextVisible;
}

void SequenceView::deleteClickedExec()
{
  QExecItemAction *action = qobject_cast<QExecItemAction*>(sender());
  scene()->removeExecItem(action->exec_item);
}

void SequenceView::disconnectClickedExecExec()
{
  QExecItemAction *action = qobject_cast<QExecItemAction*>(sender());
  QString act_string = action->data().toString();
  
  if (action)
  {
    shared_ptr<VB::Exec> exec1 = action->exec_item->exec();
    vbforeach (shared_ptr<VB::Exec> exec2, exec1->depends)
    {
      if (exec1 && exec2 && exec2->name == act_string.toStdString())
      {
        /* We don't know which depends on which, but it's probably quicker to just
         * handle both directions then to check dependency.
         */
        exec1->depends.remove(exec2);
        exec2->depends.remove(exec1);
        
        vbforeach (QGraphicsItem* gi, scene()->items())
        {
          SequenceItem* si = qgraphicsitem_cast<SequenceItem*>(gi);
          WaitforItem* wi = 0;
          if (si->sequenceItemType() == "waitfor" &&
              (wi = qgraphicsitem_cast<WaitforItem*>(gi)) &&
              (wi->fromItem()->exec() == exec1 || wi->toItem()->exec() == exec1) &&
              (wi->fromItem()->exec() == exec2 || wi->toItem()->exec() == exec2))
          {
            scene()->removeItem(wi);
            break;
          }
        }
        scene()->update(scene()->sceneRect());
      }
    }
  }
}

void SequenceView::beginFromClickedExec()
{
  QExecItemAction *action = qobject_cast<QExecItemAction*>(sender());
  ExecItem* ei = action->exec_item;
  
  scene()->setBeginPointItem(ei);
}

void SequenceView::endAtClickedExec()
{
  QExecItemAction *action = qobject_cast<QExecItemAction*>(sender());
  ExecItem* ei = action->exec_item;
  
  scene()->setEndPointItem(ei);
}

void SequenceView::noPointAtClickedExec()
{
  QExecItemAction *action = qobject_cast<QExecItemAction*>(sender());
  ExecItem* ei = action->exec_item;
  
  if (scene()->beginPointItem() == ei) scene()->setBeginPointItem(0);
  if (scene()->endPointItem() == ei)   scene()->setEndPointItem(0);
}

void SequenceView::makeBlockFromSelectedExecs()
{
  // QAction* action = qobject_cast<QAction*>(sender());
  QList<QGraphicsItem*> items = scene()->selectedItems();
  
  QGraphicsItem* parent_bi = 0;
  bool in_same_block = true;
  bool first = true;
  vbforeach (QGraphicsItem* gi, items)
  {
    if (first)
    {
      parent_bi = gi->parentItem();
      in_same_block = true;
      first = false;
    }
    else
    {
      if (gi->parentItem() != parent_bi)
      {
        in_same_block = false;
        break;
      }
    }
  }
  
  if (!in_same_block)
  {
    QMessageBox::critical(this, "Executables aren't members of the same block", 
      "The selected executables do not share the same parent block.");
  }
}

void SequenceView::toggleClickedBlockDetail(bool checked)
{
  QExecItemAction *action = qobject_cast<QExecItemAction*>(sender());
  BlockItem* bi = qgraphicsitem_cast<BlockItem*>(action->exec_item);
  
  if (bi)
  {
    if (checked)
      bi->showExecs();
    
    else
      bi->hideExecs();
    
    bi->calcBoundingRectSize();
    scene()->arrange();
  }
}

void SequenceView::makeDisconnectExecActions(QMenu* disconnect_menu, const list<shared_ptr<VB::Exec> >& connected_jobs)
{
  for (list<shared_ptr<VB::Exec> >::const_iterator iter = connected_jobs.begin(); 
       iter != connected_jobs.end(); ++iter)
  {
    const shared_ptr<VB::Exec>& e = *iter;
    
    QExecItemAction* act = new QExecItemAction(e->name.c_str(), this);
    act->setData(QString(e->name.c_str()));
    connect(act, SIGNAL(triggered()), this, SLOT(disconnectClickedExecExec()));
    disconnectExecActs.push_back(act);
    disconnect_menu->addAction(act);
  }
}

void SequenceView::singleExecContextMenu(QMenu* m, ExecItem* ei)
{
  /* Connect To ...
     Disconnect From ...
     -------------------
     Set as begin
     Set as end
     Unset 
     -------------------
     Delete Executable */
  vbforeach (QExecItemAction* act, disconnectExecActs) delete act;
  disconnectExecActs.clear();
  
  // Put disconnect actions in a submenu
  makeDisconnectExecActions(m, ei->exec()->depends);
  vbforeach (QExecItemAction* eiact, disconnectExecActs) eiact->exec_item = ei;
  
  m->addSeparator();
  beginPointAct->exec_item = ei;
  endPointAct->exec_item = ei;
  noPointAct->exec_item = ei;
  
  m->addAction(beginPointAct);
  m->addAction(endPointAct);
  m->addAction(noPointAct);
  
  beginPointAct->setEnabled(true);
  endPointAct->setEnabled(true);
  noPointAct->setEnabled(true);
  
  if (scene()->beginPointItem() == ei)    beginPointAct->setEnabled(false);
  else if (scene()->endPointItem() == ei) endPointAct->setEnabled(false);
  else                                    noPointAct->setEnabled(false);
  
  m->addSeparator();
  deleteExecAct->exec_item = ei;
  m->addAction(deleteExecAct);
}

void SequenceView::singleJobContextMenu(QMenu* m, JobItem* ji)
{
  singleExecContextMenu(m, ji);
}

void SequenceView::singleBlockContextMenu(QMenu* m, BlockItem* bi)
{
  /* Show/Hide Detail
     -----------------
     Connect To...
     Disconnect From...
     ------------------
     Delete Executable */
  toggleDetailAct->exec_item = bi;
  toggleDetailAct->setChecked(bi->execsShown());
  m->addAction(toggleDetailAct);
  m->addSeparator();
  singleExecContextMenu(m, bi);
}

void SequenceView::contextMenuEvent(QContextMenuEvent* e)
{
  if (scene())
  {
    QGraphicsItem* gi = 0;
    QList<QGraphicsItem*> gis_list;
    
    switch (e->reason())
    {
      case QGraphicsSceneContextMenuEvent::Mouse:
        gis_list = items(e->pos());
        break;

      case QGraphicsSceneContextMenuEvent::Keyboard:
        gis_list = scene()->selectedItems();;
        break;

      default:
        return;
    }
    
    vbforeach (QGraphicsItem* gi_item, gis_list)
    {
      if (qgraphicsitem_cast<ExecItem*>(gi_item))
      {
        gi = gi_item;
        break;
      }
    }
          
    if (gi)
    {
      scene()->setFocusItem(gi, Qt::MouseFocusReason);
      QMenu popup(this);
      
      if (JobItem* ji = dynamic_cast<JobItem*>(gi))
        singleJobContextMenu(&popup, ji);
      
      else if (BlockItem* bi = dynamic_cast<BlockItem*>(gi))
        singleBlockContextMenu(&popup, bi);
        
      popup.exec(QCursor::pos());
    }
  }
  
  scene()->supress_settler(false);
  scene()->arrange();
}

void SequenceView::updateSceneRect(const QRectF & rect)
{
  float FACTOR = 1.0;
  setSceneRect(QRectF(rect.topLeft()*FACTOR, rect.size()*FACTOR  + QSizeF(width(), 0)));
  
  QGraphicsView::updateSceneRect(rect);
}

