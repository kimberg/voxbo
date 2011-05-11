#include <QList>
#include <QHash>
#include <QGraphicsSceneMouseEvent>

#include "vbsequencescene.h"
#include "vbsequenceitem.h"

using namespace QtVB;
using namespace VB;
using namespace std::tr1;

SequenceScene::SequenceScene(QObject * parent)
  : QGraphicsScene(parent), _sequence(VB::Sequence(new BlockDef())), is_settling(false), is_supressed(false)
{
  _root_bd = static_cast<BlockDef*>(_sequence.root_block()->def);
}

SequenceScene::SequenceScene(const QRectF & sceneRect, QObject * parent)
	: QGraphicsScene(sceneRect, parent), _sequence(VB::Sequence(new BlockDef())), is_settling(false), is_supressed(false)
{
  _root_bd = static_cast<BlockDef*>(_sequence.root_block()->def);
}

SequenceScene::SequenceScene(qreal x, qreal y, qreal width, qreal height, QObject * parent)
	: QGraphicsScene(x, y, width, height, parent), _sequence(VB::Sequence(new BlockDef())),  is_settling(false), is_supressed(false)
{
  _root_bd = static_cast<BlockDef*>(_sequence.root_block()->def);
}

SequenceScene::~SequenceScene()
{
  delete _root_bd;
}

void SequenceScene::fitToPipeline()
{
//  QSize size = _arranger->size();
//  setSceneRect(0,0,size.width(), size.height());
}  

void SequenceScene::itemMoved(SequenceItem* item)
{
  QRectF bounds = item->boundingRect();
  QPointF bottomRight = item->mapToScene(bounds.bottomRight());
  
	qreal w = width();
	qreal h = height();
	if (w < bottomRight.x())
		w = bottomRight.x();
	if (h < bottomRight.y())
		h = bottomRight.y();
	
//	setSceneRect(0,0,0,0);
	setSceneRect(0,0,w,h);
}

JobItem* SequenceScene::addJob(shared_ptr<VB::Job> j, BlockItem* parent)
{
  JobItem* ji = new JobItem(j, parent, this);
//  connect(ji, SIGNAL(moved()),
//  			this, SLOT(itemMoved()));
  emit execItemAdded(ji);
  emit jobItemAdded(ji);
//  arrange();
  
  return ji;
}

JobItem* SequenceScene::addJob(VB::JobType* jt, BlockItem* parent)
{
	shared_ptr<Job> j = static_pointer_cast<Job>(jt->declare());
	_root_bd->execs().push_back(j);
	rootBlockDefChanged();
	
	return addJob(j, parent);
}

BlockItem* SequenceScene::addBlock(shared_ptr<VB::Block> b, BlockItem* parent)
{
	BlockItem* bi = new BlockItem(b, parent, this);
//	connect(bi, SIGNAL(moved()),
//	        this, SLOT(itemMoved()));
	emit execItemAdded(bi);
	emit blockItemAdded(bi);
//	arrange();
	
	return bi;
}

BlockItem* SequenceScene::addBlock(VB::BlockDef* bd, BlockItem* parent)
{
	shared_ptr<Block> b = static_pointer_cast<Block>(bd->declare());
	_root_bd->execs().push_back(b);
	rootBlockDefChanged();
	
	return addBlock(b, parent);
}

void SequenceScene::removeExecItem(ExecItem* ei)
{
	_root_bd->execs().remove(ei->exec());
	for (list<shared_ptr<Exec> >::iterator iter = _root_bd->execs().begin(); 
	     iter != _root_bd->execs().end(); ++iter)
	{
	  shared_ptr<Exec>& exec = *iter;
	  exec->depends.remove(ei->exec());
	}
	rootBlockDefChanged();
	
	vbforeach (QGraphicsItem* gi, items())
	{
	  if (qgraphicsitem_cast<SequenceItem*>(gi)->sequenceItemType() == "waitfor")
	  {
	    WaitforItem* wi = qgraphicsitem_cast<WaitforItem*>(gi);
	    if (wi->fromItem() == ei) removeItem(wi);
	    else if (wi->toItem() == ei) removeItem(wi);
	  }
	}
	
  removeItem(ei);
  emit execItemRemoved(ei);
//  arrange();
}

ExecItem* SequenceScene::selectedExecItem()
{
	QList<QGraphicsItem*> sel_list = selectedItems();
	if (sel_list.empty())
		return 0;
	
	// With dynamic_cast, if the object is not of type
	// JobItem, it will just return 0.
	return dynamic_cast<ExecItem*>(sel_list.first());
}

ExecItem* SequenceScene::beginPointItem()
{
  return _beginPointItem;
}

ExecItem* SequenceScene::endPointItem()
{
  return _endPointItem;
}

void SequenceScene::setBeginPointItem(ExecItem* ei)
{
  _beginPointItem = ei;
}

void SequenceScene::setEndPointItem(ExecItem* ei)
{
  _endPointItem = ei;
}

/*Job* SequenceScene::selectedJob()
{
	JobItem* sel_jp = selectedJobItem();
	if (sel_jp)
		return sel_jp->job();
	
	return 0;
}*/

void SequenceScene::clear()
{
	clearFocus();
	clearSelection();
	vbforeach (QGraphicsItem* item, items())
		removeItem(item);
}

ExecItem* SequenceScene::itemByExec(shared_ptr<VB::Exec> ei)
{
	vbforeach (QGraphicsItem* item, items())
	{
		if (ExecItem* test_ei = qgraphicsitem_cast<ExecItem*>(item))
			if (test_ei->exec() == ei) return test_ei;
	}
	return 0;
}

void SequenceScene::loadSequence(VB::Sequence& seq)
{
  clear();
  
  _sequence = seq;
  
  QList<ExecItem*> exec_item_list;
  BlockDef* bd = static_cast<BlockDef*>(_sequence.root_block()->def);
  std::list<shared_ptr<Exec> > exec_list = bd->execs();
  vbforeach(shared_ptr<Exec> ei, exec_list)
  {
    ExecItem* ei_item=NULL;
    if (shared_ptr<Job> j = dynamic_pointer_cast<Job>(ei))
    {
      ei_item = addJob(j);
    }
    else if (shared_ptr<Block> b = dynamic_pointer_cast<Block>(ei))
    {
      ei_item = addBlock(b);
    }
    
  	if (ei_item) exec_item_list.push_back(ei_item);
  }
  
  BlockDef* _old_root_bd = _root_bd;
  _root_bd = bd;
  rootBlockDefChanged();
  if (_old_root_bd) delete _old_root_bd;

  QHash<ExecItem*, QPair<int,int> > placement; // think of this as a grid indexed by contents.
  int num_jobs = exec_item_list.size();
  ExecItem* grid[num_jobs][num_jobs];
  for (int i = 0; i < num_jobs; ++i)
  for (int j = 0; j < num_jobs; ++j)
    grid[i][j] = 0;
  
  ExecItem* prev_job_item = 0;
  vbforeach (ExecItem* job_item, exec_item_list)
  {
    list<shared_ptr<Exec> > wait_set(job_item->exec()->depends);
    // if the job depends on the previous job in job_list and only that job
    // then place this job in the grid space to the right of the previous
    int row = 0, col = 0;
    
    if ((wait_set.size() == 1) && (*(wait_set.begin()) == prev_job_item->exec()))
    {
      col = placement[prev_job_item].second + 1;
      row = placement[prev_job_item].first;
      
      if (prev_job_item) new WaitforItem(prev_job_item, job_item);
    }
    else
    {
      vbforeach(shared_ptr<Exec> wait_job, wait_set)
      {
      	ExecItem* wait_job_item = itemByExec(wait_job);
        col = max(col, placement[wait_job_item].second + 1);
        row = max(row, placement[wait_job_item].first);
        
        if (wait_job_item) new WaitforItem(wait_job_item, job_item);
      }
    }
    
    // if there's already an item in this space, move down until we find an
    // empty cell.
    while (grid[row][col])
      ++row;
    
    placement[job_item] = QPair<int,int>(row, col);
    grid[row][col] = job_item;
    job_item->calcBoundingRectSize();
    
    prev_job_item = job_item;
  }
  
  int colstart[num_jobs];
  int colwidth[num_jobs];
  colstart[0] = 20;
  for (int c = 0; c < num_jobs; ++c)
  {
  	if (c > 0) colstart[c] = colstart[c-1] + colwidth[c-1];
  	
  	colwidth[c] = 0;
  	for (int r = 0; r < num_jobs; ++r)
  		if (grid[r][c])
  		{
  			colwidth[c] = max(colwidth[c], grid[r][c]->boundingRect().width()+50);
  		}
	}
  
  int rowstart[num_jobs];
  int rowheight[num_jobs];
  rowstart[0] = 20;
  for (int r = 0; r < num_jobs; ++r)
  {
  	if (r > 0) rowstart[r] = rowstart[r-1] + rowheight[r-1];
  	
  	rowheight[r] = 0;
  	for (int c = 0; c < num_jobs; ++c)
  		if (grid[r][c])
  		{
  			rowheight[r] = max(rowheight[r], grid[r][c]->boundingRect().height()+10);
  		}
	}
  
  for (int i = 0; i < num_jobs; ++i)
  for (int j = 0; j < num_jobs; ++j)
    if (grid[i][j])
    {
//    	cerr << "added job '" << grid[i][j]->job()->get_name() << "' at (" << 
//    		colstart[j] << ", " << rowstart[i] << ")" << endl;
      grid[i][j]->setPos(colstart[j], rowstart[i]);
    }
  
//  arrange();
} 

void SequenceScene::saveSequence(VB::Sequence& seq)
{
//  seq->root_block_def(_root_bd);
  seq = _sequence;
}

/*
Job* SequenceView::selectedJob()
{
  if (isJob(selection))
  {
    return reinterpret_cast<Job*>(selection);
  }
  else
  {
    return 0;
  }
}*/

void SequenceScene::rootBlockDefChanged()
{
  _sequence.root_block_def(_root_bd);
}

void SequenceScene::setDataSet(DataSet*/* ds*/)
{
/*  dataset = ds;
  vbforeach (const Job* job, jobs)
  {
    const_cast<Job*>(job)->set_dataset(ds);
  }*/
}

DataSet* SequenceScene::getDataSet()
{
//  return dataset;
//	return const_cast<DataSet*>(_sequence->get_dataset());
  return 0;
}

void SequenceScene::arrange()
{
  if (!is_settling && !is_supressed)
  {
    is_settling = true;
    settle_helper();
	}
}

void SequenceScene::supress_settler(bool s)
{
  if (s) is_settling = false;
  is_supressed = s;
}

bool SequenceScene::settler_supressed() const
{
  return is_supressed;
}

void SequenceScene::occupy_settler()
{
  is_settling = true;
}

void SequenceScene::free_settler()
{
  is_settling = false;
}

void SequenceScene::settle_helper()
{
  if (/*!is_settling && */!is_supressed) {
    qreal CALLS_PER_SECOND = 20;
    
    //		occupy_settler();
    //    QTimer::singleShot(int(1000 / CALLS_PER_SECOND), this, SLOT(free_setler()));
	
  	qreal ITERATIONS_PER_CALL = 10;
  	qreal STEPFACTOR = 1/CALLS_PER_SECOND/ITERATIONS_PER_CALL;
    qreal MOVETHRESHOLD = 0.01;
    
    qreal CRITICALDISTANCE = 100;
    QPointF WAITVECTOR = QPointF(CRITICALDISTANCE, 0);
    QPointF NOWAITVECTOR = QPointF(0, CRITICALDISTANCE);
    
    qreal max_movement_sq = 0.0;
    QMap<ExecItem*, QPointF> displacement; // should be one for each jobitem in item_list
    
    QList<QGraphicsItem*> item_list = items();
    for (int i = 0; i < ITERATIONS_PER_CALL; ++i) {
      for (QList<QGraphicsItem*>::iterator iter = item_list.begin();
           iter != item_list.end();
           ++iter) {
        SequenceItem* si = static_cast<SequenceItem*>(*iter);
        
        // Two jobs that are not connected prefer to be on top of each other, if they are within a given distance.
        if (si->isVisible() &&
            (si->sequenceItemType() == "job" || si->sequenceItemType() == "block")) {
          ExecItem* ei1 = static_cast<ExecItem*>(si);
          
          QList<QGraphicsItem*>::iterator iter2 = iter;
          for (++iter2;
               iter2 != item_list.end();
               ++iter2) {
            SequenceItem* si2 = static_cast<SequenceItem*>(*iter2);
            if (si2->isVisible() && si2->parentItem() == si->parentItem() &&
                (si2->sequenceItemType() == "job" || si2->sequenceItemType() == "block")) {
              ExecItem* ei2 = static_cast<ExecItem*>(si2);
              
              if (find(ei1->exec()->depends.begin(), ei1->exec()->depends.end(), ei2->exec()) == ei1->exec()->depends.end() && 
                  find(ei2->exec()->depends.begin(), ei2->exec()->depends.end(), ei1->exec()) == ei2->exec()->depends.end()) {
                QPointF dist_vector = ei2->pos() - ei1->pos();
                
                QPointF MY_NOWAITVECTOR = NOWAITVECTOR;
                if (dist_vector.y() < 0) MY_NOWAITVECTOR *= -1;
                
                qreal d_sq = dist_vector.x()*dist_vector.x() + dist_vector.y()*dist_vector.y();
                if (d_sq == 0) d_sq = 0.0000000001; // <-- never zero, but "very small"
                //                qreal d = sqrt(d_sq);
                
                //                if (d < CRITICALDISTANCE)
                if (fabs(dist_vector.x()) < CRITICALDISTANCE/2 && fabs(dist_vector.y()) < CRITICALDISTANCE)
                  {
                    displacement[ei1] += STEPFACTOR * (dist_vector - MY_NOWAITVECTOR) / 2;
                    displacement[ei2] -= STEPFACTOR * (dist_vector - MY_NOWAITVECTOR) / 2;
                  }
              }
            }
          }
        }
        
        // Waitfor lines prefer a perfectly horizontal orientation with legth LENGTH
        else if (si->isVisible() && si->sequenceItemType() == "waitfor")
          {
            WaitforItem* wi = static_cast<WaitforItem*>(si);
            ExecItem* ei1 = wi->_fromItem;
            ExecItem* ei2 = wi->_toItem;
            
            QPointF dist_vector = wi->toEnd() - wi->fromEnd();
            displacement[ei1] += STEPFACTOR * (dist_vector - WAITVECTOR) / 2;
            displacement[ei2] -= STEPFACTOR * (dist_vector - WAITVECTOR) / 2;
          }
      }
      
      for (QMap<ExecItem*, QPointF>::iterator iter = displacement.begin();
           iter != displacement.end();
           ++iter)
        {
          ExecItem* ei = iter.key();
          QPointF disp = iter.value();
          
          QPointF oldp, newp;
          oldp = ei->pos();
          newp = oldp + disp;
          if (ei->exec()->depends.empty())
            {
              newp.setX(10.0);
            }
          if (newp.y() < 10.0)
            {
              newp.setY(10.0);
            }
          
          ei->setPos(newp);
          
          QPointF diff = newp - oldp;
          qreal movement_sq = diff.x()*diff.x() + diff.y()*diff.y();
          if (movement_sq > max_movement_sq) max_movement_sq = movement_sq;
        }
      
    }
    
    qreal max_movement = sqrt(max_movement_sq);
    //  	if (max_movement > 0) std::cerr << "max movement (distance^2): " << max_movement << std::endl;
  	
    bool disequilibriated;
    if (max_movement > MOVETHRESHOLD) disequilibriated = true;
    else disequilibriated = false;
    
    if (!disequilibriated)
    {
      is_settling = false;
    }
    
    if (disequilibriated)
      {
    	// accelerate the movement at small movements.
        float time_factor = 1.0 + 1.0/max_movement;
        QTimer::singleShot(int(1000 / (CALLS_PER_SECOND * time_factor)), this, SLOT(settle_helper()));
      }
    
  }
}

