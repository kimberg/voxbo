#ifndef VBSEQUENCESCENE_H
#define VBSEQUENCESCENE_H

#include <QGraphicsScene>
#include <QTimer>

#include "vbsequence.h"
#include "vbsequenceitem.h"

namespace QtVB {
class JobItem;
class BlockItem;
class ExecItem;
class WaitforItem;

class SequenceScene : public QGraphicsScene  //, public PipelineArranger
{
  Q_OBJECT

 public:
  SequenceScene(QObject* parent = 0);
  SequenceScene(const QRectF& sceneRect, QObject* parent = 0);
  SequenceScene(qreal x, qreal y, qreal width, qreal height,
                QObject* parent = 0);
  virtual ~SequenceScene();

  JobItem* addJob(VB::JobType* jt, BlockItem* parent = 0);
  BlockItem* addBlock(VB::BlockDef* bd, BlockItem* parent = 0);
  JobItem* addJob(shared_ptr<VB::Job> j, BlockItem* parent = 0);
  BlockItem* addBlock(shared_ptr<VB::Block> b, BlockItem* parent = 0);
  void removeExecItem(ExecItem* ei);

  ExecItem* selectedExecItem();
  ExecItem* beginPointItem();
  ExecItem* endPointItem();

  void setBeginPointItem(ExecItem*);
  void setEndPointItem(ExecItem*);

  ExecItem* itemByExec(shared_ptr<VB::Exec> ei);

  void loadSequence(VB::Sequence& seq);
  void saveSequence(VB::Sequence& seq);
  void clear();

  void setDataSet(VB::DataSet* ds);
  VB::DataSet* getDataSet();

  VB::Sequence& sequence() { return _sequence; }
  const VB::Sequence& sequence() const { return _sequence; }

 public slots:
  void arrange();
  void itemMoved(SequenceItem* item);
  void fitToPipeline();

  void supress_settler(bool s = true);
  bool settler_supressed() const;

 public:
  // ...cause signals are protected
  void emitSelectionChanged() { emit selectionChanged(); }

 signals:
  void jobItemAdded(JobItem* j);
  void jobItemRemoved(JobItem* j);
  void blockItemAdded(BlockItem* j);
  void blockItemRemoved(BlockItem* j);
  void execItemAdded(ExecItem* j);
  void execItemRemoved(ExecItem* j);

  void settleStepped();
  void settleCompleted();
  // The selectionChanged signal is a part of the QGraphicsScene interface
  // as of Qt 4.3.  Since, at this point, I am not using 4.3, and it would
  // be a hassle to go that route, I will reimplement it.
  void selectionChanged();

 protected:
  PipelineArranger* _arranger;
  VB::Sequence _sequence;
  VB::BlockDef* _root_bd;

  ExecItem* _beginPointItem;
  ExecItem* _endPointItem;

 protected slots:
  void rootBlockDefChanged();
  void occupy_settler();
  void free_settler();
  void settle_helper();

 protected:
  bool is_settling;
  bool is_supressed;
};
}  // namespace QtVB

#endif
