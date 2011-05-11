#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QColor>
#include <QHash>

#include "vbsequenceitem.h"
#include "vbsequencescene.h"

using namespace QtVB;
using namespace VB;
using namespace std;
using namespace std::tr1;

  int ARG_WIDTH = 10;
  int ARG_HEIGHT = ARG_WIDTH;
  int ARG_SPACING = 0;
  int DS_HEIGHT = 15;
  int DS_WIDTH = 30;
  int DS_MARGIN = 3;
  int DATA_WIDTH = 50;
  int DATA_HEIGHT = 15;
  int SOURCE_RAD = 5;
  int SINK_RAD = 3;
  int CONN_RAD = 3;
  int JOB_WIDTH = 100;
  int JOB_PADDING = 6;
  int STUB_LENGTH = 12;
  int ARG_RADIUS = 3;
  int RES_RADIUS = ARG_RADIUS + 1;

  QColor BG_FILL_COLOR = QColor("#f8f8f8");

  QColor JOB_LINE_COLOR = QColor("#000000");
  QColor JOB_FILL_COLOR = QColor("#fff0f0");
  QColor JOB_TEXT_COLOR = QColor("#000000");
  QColor JOB_HOVER_LINE_COLOR = QColor("#800000");
  QColor JOB_HOVER_FILL_COLOR = QColor("#ff8080");
  QColor JOB_HOVER_TEXT_COLOR = QColor("#800000");
  QColor JOB_SELECT_LINE_COLOR = QColor("#fff0f0");
  QColor JOB_SELECT_FILL_COLOR = QColor("#ff0000");
  QColor JOB_SELECT_TEXT_COLOR = QColor("#fff0f0");

  QColor BLOCK_LINE_COLOR = QColor("#000000");
  QColor BLOCK_FILL_COLOR = QColor("#f0f0ff");
  QColor BLOCK_TEXT_COLOR = QColor("#000000");
  QColor BLOCK_HOVER_LINE_COLOR = QColor("#000080");
  QColor BLOCK_HOVER_FILL_COLOR = QColor("#8080ff");
  QColor BLOCK_HOVER_TEXT_COLOR = QColor("#000080");
  QColor BLOCK_SELECT_LINE_COLOR = QColor("#f0f0ff");
  QColor BLOCK_SELECT_FILL_COLOR = QColor("#0000ff");
  QColor BLOCK_SELECT_TEXT_COLOR = QColor("#f0f0ff");

  QColor DATA_LINE_COLOR = QColor("#000000");
  QColor DATA_FILL_COLOR = QColor("#e0f0ff");
  QColor DATA_TEXT_COLOR = QColor("#000000");
  QColor DATA_HOVER_LINE_COLOR = QColor("#004080");
  QColor DATA_HOVER_FILL_COLOR = QColor("#80c0ff");
  QColor DATA_HOVER_TEXT_COLOR = QColor("#004080");
  QColor DATA_SELECT_LINE_COLOR = QColor("#e0f0ff");
  QColor DATA_SELECT_FILL_COLOR = QColor("#0080ff");
  QColor DATA_SELECT_TEXT_COLOR = QColor("#e0f0ff");

  QColor CONN_LINE_COLOR = QColor("#000000");
  QColor CONN_FILL_COLOR = QColor("#f0ffe0");
  QColor CONN_TEXT_COLOR = QColor("#000000");
  QColor CONN_HOVER_LINE_COLOR = QColor("#408000");
  QColor CONN_HOVER_FILL_COLOR = QColor("#c0ff80");
  QColor CONN_HOVER_TEXT_COLOR = QColor("#408000");
  QColor CONN_SELECT_LINE_COLOR = QColor("#f0ffe0");
  QColor CONN_SELECT_FILL_COLOR = QColor("#80ff00");
  QColor CONN_SELECT_TEXT_COLOR = QColor("#f0ffe0");

  QColor DS_LINE_COLOR = QColor("#000000");
  QColor DS_FILL_COLOR = QColor("#ffffff");
  QColor DS_TEXT_COLOR = QColor("#000000");
  QColor DS_HOVER_LINE_COLOR = QColor("#000080");
  QColor DS_HOVER_FILL_COLOR = QColor("#8080ff");
  QColor DS_HOVER_TEXT_COLOR = QColor("#000080");
  QColor DS_SELECT_LINE_COLOR = QColor("#ffffff");
  QColor DS_SELECT_FILL_COLOR = QColor("#0000ff");
  QColor DS_SELECT_TEXT_COLOR = QColor("#ffffff");
  
  const int JOB_ZVALUE = 0;
  const int WAITFOR_ZVALUE = 1;

/* Constructors */

SequenceItem::SequenceItem(QGraphicsItem * parent, QGraphicsScene * scene)
  : QGraphicsItem(parent, scene), 
  	_selected(false), _hovered(false), _pressed(false), _dragged(false), 
  	seqScene(qobject_cast<SequenceScene*>(scene))
{}

ExecItem::ExecItem(shared_ptr<VB::Exec> ei, SequenceItem * parent, SequenceScene * scene)
  : SequenceItem(parent, scene), _exec(ei), _boundingRect(0,0,50,20)
{}

BlockItem::BlockItem(shared_ptr<VB::Block> b, SequenceItem * parent, SequenceScene * scene) 
  : ExecItem(b, parent, scene), _detailed(false)
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptsHoverEvents(true);
	setZValue(JOB_ZVALUE);
	createExecs();
}

JobItem::JobItem(shared_ptr<VB::Job> j, SequenceItem * parent, SequenceScene * scene) 
  : ExecItem(j, parent, scene)
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptsHoverEvents(true);
	setZValue(JOB_ZVALUE);
}

WaitforItem::WaitforItem(ExecItem* a_fromItem, ExecItem* a_toItem)
  : SequenceItem(0,0), _fromItem(a_fromItem), _toItem(a_toItem) 
{
  if (_fromItem) setParentItem(_fromItem->parentItem());
  if (_fromItem) _fromItem->scene()->addItem(this);
  
  if (_fromItem) _fromItem->_connections.push_back(this);
  if (_toItem)   _toItem->_connections.push_back(this);
//	setZValue(WAITFOR_ZVALUE);
}

TempWaitforDragItem::TempWaitforDragItem(ExecItem* a_fromItem)
  : WaitforItem(a_fromItem, 0)
{
  _toPoint = fromEnd();
}

WaitforItem::~WaitforItem()
{
  if (_fromItem) _fromItem->_connections.removeAll(this);
  if (_toItem)   _toItem->_connections.removeAll(this);
}

/* SequenceItem ***************************************************************/

/* detailed()
 */
bool SequenceItem::detailed() const
{
	return _detailed;
}

void SequenceItem::detailed(bool d)
{
	_detailed = d;
//	emit detailChanged();
}

/* WaitforItem ***************************************************************/

/* repaint (overloaded)
 */
//void WaitforItem::repaint()
//{
//	update(boundingRect());
//}

/* paint (virtual)
 */
void WaitforItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  painter->setPen(CONN_LINE_COLOR);
  painter->setBrush(CONN_FILL_COLOR);
  
  QPointF src_point = fromEnd();
  QPointF snk_point = toEnd();
  QPoint offset(0,3);
  
  if (toItem())
  {
    if (fromItem()->exec()->path == toItem()->exec()->path &&
        fromItem()->exec()->path.find("*") != string::npos)
    {
      painter->drawLine(src_point + offset, snk_point + offset);
      painter->drawLine(src_point - offset, snk_point - offset);
    }
    else if (fromItem()->exec()->path != toItem()->exec()->path &&
        fromItem()->exec()->path.find("*") != string::npos &&
        toItem()->exec()->path.find("*") == string::npos)
    {
      painter->drawLine(src_point + offset, snk_point);
      painter->drawLine(src_point - offset, snk_point);
    }
    else if (fromItem()->exec()->path != toItem()->exec()->path &&
        fromItem()->exec()->path.find("*") == string::npos &&
        toItem()->exec()->path.find("*") != string::npos)
    {
      painter->drawLine(src_point, snk_point + offset);
      painter->drawLine(src_point, snk_point - offset);
    }
    else if (fromItem()->exec()->path != toItem()->exec()->path &&
        fromItem()->exec()->path.find("*") != string::npos &&
        toItem()->exec()->path.find("*") != string::npos)
    {
      painter->drawLine(src_point - offset, snk_point + offset);
      painter->drawLine(src_point + offset, snk_point - offset);
    }
  }
  else if (fromItem() && fromItem()->exec()->path.find("*") != string::npos)
  {
    painter->drawLine(src_point + offset, snk_point);
    painter->drawLine(src_point - offset, snk_point);
  }
  painter->drawLine(src_point, snk_point);
  
  painter->drawEllipse(int(src_point.x()) - CONN_RAD, int(src_point.y()) - CONN_RAD,
                      CONN_RAD*2, CONN_RAD*2);
  painter->drawEllipse(int(snk_point.x()) - CONN_RAD, int(snk_point.y()) - CONN_RAD,
                      CONN_RAD*2, CONN_RAD*2);
  
}

/* boundingRect (virtual)
 */
QRectF WaitforItem::boundingRect() const
{
	QPointF fromPoint = fromEnd();
	QPointF toPoint = toEnd();
	
	double nearx = min(fromPoint.x(), toPoint.x());
	double neary = min(fromPoint.y(), toPoint.y());
	double width = fabs(fromPoint.x() - toPoint.x());
	double height = fabs(fromPoint.y() - toPoint.y());
	QRectF rect(nearx, neary, width, height);
	return rect.adjusted(-CONN_RAD, -CONN_RAD, CONN_RAD, CONN_RAD);//.adjusted(-10,-10,10,10);
}

QPointF WaitforItem::fromEnd() const
{
  return mapFromItem(_fromItem, _fromItem->sinkPoint());
}

QPointF WaitforItem::toEnd() const
{
  return mapFromItem(_toItem, _toItem->sourcePoint());
}

/* shape (virtual)
 */
QPainterPath WaitforItem::shape() const
{
//	QPointF _fromPoint = mapFromItem(_fromItem, _fromItem->sinkPoint());
//	QPointF _toPoint = mapFromItem(_toItem, _toItem->sourcePoint());
//	QPointF _offset = QPoint(2,2);
	
	QPainterPath path;
	/*path.moveTo(_fromPoint + _offset);
	path.lineTo(_toPoint + _offset);
	path.lineTo(_toPoint - _offset);
	path.lineTo(_fromPoint - _offset);
	path.closeSubpath();*/
	path.addRect(boundingRect());
	return path;
}

/* TempWaitforDragItem ***************************************************************/

QPointF TempWaitforDragItem::toEnd() const
{
  return _toPoint;
}

void TempWaitforDragItem::setToEnd(const QPointF& p)
{
  _toPoint = p;
  update();
}

/* BlockItem *****************************************************************/
 
/* block
 */
shared_ptr<VB::Block> BlockItem::block() const
{
	return static_pointer_cast<Block>(_exec);
}

/* boundingRect (virtual)
 */
QRectF BlockItem::boundingRect() const
{
	return _boundingRect.adjusted(0,0,5,5);
}

/* shape (virtual)
 */
QPainterPath BlockItem::shape() const
{
	QPainterPath path;
	path.addRect(boundingRect());
	return path;
}

void BlockItem::calcBoundingRectSize(QPainter *painter)
{
	QRect _textBounds;
//  int x = 0;
//  int y = 0;
  int h = 0;//_boundingRect.height();
  int w = 0;//_boundingRect.width();
  
  // find out how much space the name takes up
  if (painter)
  {
  //	  _textBounds = painter->boundingRect(x,y,w,h, Qt::AlignHCenter|Qt::AlignVCenter, job()->get_name().c_str());
    _textBounds = painter->boundingRect(0,0,0,0, Qt::AlignHCenter|Qt::AlignVCenter, block()->name.c_str());
  }
  else
  {
  	int CHAR_WIDTH = 10; // <-- what???
  	int CHAR_HEIGHT = 12;
  	_textBounds = QRect(0, 0, CHAR_WIDTH * block()->name.length(), CHAR_HEIGHT);
  }
  
  if (!_detailed)
  {
    h = max(h, _textBounds.height() + JOB_PADDING);
    w = max(w, _textBounds.width() + JOB_PADDING);
  }
  else if (_detailed)
  {
  	QSizeF size = childrenBoundingRect().adjusted(-10,-10,10,10).size();
  	h = size.height();
  	w = size.width();
  }
  
  _boundingRect.setHeight(h);
  _boundingRect.setWidth(w);
}

/* paint (virtual)
 */
void BlockItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  calcBoundingRectSize(painter);
  
  int x = 0;
  int y = 0;
  int h = _boundingRect.height();
  int w = _boundingRect.width();
  
  bool once = (_exec->path.find("*") == string::npos);
  
  bool selected = isSelected();
  bool hovered = isHovered();
  
  painter->save();
  painter->setPen(BLOCK_LINE_COLOR);
  painter->setBrush(BLOCK_FILL_COLOR);

  if (selected)
  {
    painter->setPen(BLOCK_SELECT_LINE_COLOR);
    painter->setBrush(BLOCK_SELECT_FILL_COLOR);
  }
  else if (hovered)
  {
    painter->setPen(BLOCK_HOVER_LINE_COLOR);
    painter->setBrush(BLOCK_HOVER_FILL_COLOR);
  }
	
  if (!once)
  {
//    w -= 4;
//    h -= 4;
    painter->drawRect(x+4, y+4, w, h);
    painter->drawRect(x+2, y+2, w, h);
  }
  painter->drawRect(x,y,w,h);
  
  painter->setPen(Qt::NoPen);
  painter->setBrush(BLOCK_TEXT_COLOR);
  if (selected)     painter->setBrush(BLOCK_SELECT_TEXT_COLOR);
  else if (hovered) painter->setBrush(BLOCK_HOVER_TEXT_COLOR);
  
  painter->setPen(BLOCK_TEXT_COLOR);
  if (selected)     painter->setPen(BLOCK_SELECT_TEXT_COLOR);
  else if (hovered) painter->setPen(BLOCK_HOVER_TEXT_COLOR);
	
  painter->drawText(x,y,w,h, Qt::AlignHCenter|Qt::AlignVCenter, block()->name.c_str());
	
  painter->restore();
  
//  emit painted();
}

void BlockItem::createExecs()
{
  vbforeach (QGraphicsItem* gi, QGraphicsItem::children())
  {
    scene()->removeItem(gi);
  }
  
  QList<ExecItem*> exec_item_list;
  std::list<shared_ptr<Exec> > exec_list = block()->execs;
  vbforeach (shared_ptr<Exec> ei, exec_list)
  {
    ExecItem* ei_item=NULL;
    if (shared_ptr<Job> j = dynamic_pointer_cast<Job>(ei))
    {
      ei_item = static_cast<SequenceScene*>(scene())->addJob(j, this);
    }
    else if (shared_ptr<Block> b = dynamic_pointer_cast<Block>(ei))
    {
      ei_item = static_cast<SequenceScene*>(scene())->addBlock(b, this);
    }
    
  	if (ei_item) exec_item_list.push_back(ei_item);
  }
  
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
      vbforeach (shared_ptr<Exec> wait_job, wait_set)
      {
      	ExecItem* wait_job_item = static_cast<SequenceScene*>(scene())->itemByExec(wait_job);
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
  
  if (!_detailed)
    hideExecs();
}

void BlockItem::showExecs()
{
  vbforeach (QGraphicsItem* gi, QGraphicsItem::children())
  {
    _detailed = true;
    gi->show();
  }
  update();
}

void BlockItem::hideExecs()
{
  vbforeach (QGraphicsItem* gi, QGraphicsItem::children())
  {
    _detailed = false;
    gi->hide();
  }
  update();
}

bool BlockItem::execsShown()
{
  return _detailed;
}

/* ExecItem ******************************************************************/

shared_ptr<VB::Exec> ExecItem::exec() const
{
  return _exec;
}

/* sourcePoint
 *
 * Determine where the "centerpoint" of the arguments should be.  
 */
QPoint ExecItem::sourcePoint() const
{
  int x,y;
  QRect rect;
  
  // Get the argument's rect.
  rect = _boundingRect;//.toRect();
  x = rect.left() - CONN_RAD;
  y = rect.top() + rect.height()/2;
  
  return QPoint(x,y);
}

/* sinkPoint
 *
 * Determine where the "centerpoint" of the resources should be.  
 */
QPoint ExecItem::sinkPoint() const
{
  int x,y;
  QRect rect;
  
  rect = _boundingRect;//.toRect();
  x = rect.right() + CONN_RAD;
  y = rect.top() + rect.height()/2;
  
  return QPoint(x,y);
}

/* JobItem *******************************************************************/
 
/* job
 */
shared_ptr<VB::Job> JobItem::job() const
{
	return static_pointer_cast<Job>(_exec);
}

/* boundingRect (virtual)
 */
QRectF JobItem::boundingRect() const
{
	return _boundingRect.adjusted(0,0,5,5);
}

/* shape (virtual)
 */
QPainterPath JobItem::shape() const
{
	QPainterPath path;
	path.addRect(boundingRect());
	return path;
}

void JobItem::calcBoundingRectSize(QPainter *painter)
{
	QRect _textBounds;
//  int x = 0;
//  int y = 0;
  int h = _boundingRect.height();
  int w = _boundingRect.width();
	if (painter)
	{
//	  _textBounds = painter->boundingRect(x,y,w,h, Qt::AlignHCenter|Qt::AlignVCenter, job()->get_name().c_str());
	  _textBounds = painter->boundingRect(0,0,0,0, Qt::AlignHCenter|Qt::AlignVCenter, job()->name.c_str());
	}
	else
	{
  	int CHAR_WIDTH = 10; // <-- what???
  	int CHAR_HEIGHT = 12;
  	_textBounds = QRect(0, 0, CHAR_WIDTH * job()->name.length(), CHAR_HEIGHT);
  }
//  h = max(h, _textBounds.height() + JOB_PADDING);
//  w = max(w, _textBounds.width() + JOB_PADDING);
  h = _textBounds.height() + JOB_PADDING;
  w = _textBounds.width() + JOB_PADDING;
  
  _boundingRect.setHeight(h);
  _boundingRect.setWidth(w);
}

/* paint (virtual)
 */
void JobItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  int x = 0;
  int y = 0;
  int h = _boundingRect.height();
  int w = _boundingRect.width();
  
  calcBoundingRectSize(painter);
  
  bool once = (_exec->path.find("*") == string::npos);
  
  bool selected = isSelected();
  bool hovered = isHovered();
  
  painter->save();
  painter->setPen(JOB_LINE_COLOR);
  painter->setBrush(JOB_FILL_COLOR);

  if (selected)
  {
    painter->setPen(JOB_SELECT_LINE_COLOR);
    painter->setBrush(JOB_SELECT_FILL_COLOR);
  }
  else if (hovered)
  {
    painter->setPen(JOB_HOVER_LINE_COLOR);
    painter->setBrush(JOB_HOVER_FILL_COLOR);
  }
	
  if (!once)
  {
//    w -= 4;
//    h -= 4;
    painter->drawRoundRect(x+4, y+4, w, h);
    painter->drawRoundRect(x+2, y+2, w, h);
  }
  painter->drawRoundRect(x,y,w,h);
  
  painter->setPen(Qt::NoPen);
  painter->setBrush(JOB_TEXT_COLOR);
  if (selected)     painter->setBrush(JOB_SELECT_TEXT_COLOR);
  else if (hovered) painter->setBrush(JOB_HOVER_TEXT_COLOR);
  
  painter->setPen(JOB_TEXT_COLOR);
  if (selected)     painter->setPen(JOB_SELECT_TEXT_COLOR);
  else if (hovered) painter->setPen(JOB_HOVER_TEXT_COLOR);
	
  painter->drawText(x,y,w,h, Qt::AlignHCenter|Qt::AlignVCenter, job()->name.c_str());
	
  painter->restore();

//  emit painted();
}

/* 
 * itemChange
 */
QVariant SequenceItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (change == ItemPositionChange) 
  {
    if (seqScene) seqScene->itemMoved(this);
	}
	else if (change == ItemSelectedChange) 
	{
		if (!value.toBool()) _selected = false;
		else                 _selected = true;
		qobject_cast<SequenceScene*>(scene())->emitSelectionChanged();
	}
	
  return QGraphicsItem::itemChange(change, value);
}

QVariant ExecItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (change == ItemPositionChange)
  {
    vbforeach (WaitforItem* wi, _connections) wi->update();
  }
  
  return SequenceItem::itemChange(change, value);
}

/* events (hover, click, move, ...)
 */
void SequenceItem::hoverEnterEvent ( QGraphicsSceneHoverEvent * event )
{
	_hovered = true;
	QGraphicsItem::hoverEnterEvent(event);
}

void SequenceItem::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event )
{
	_hovered = false;
	QGraphicsItem::hoverLeaveEvent(event);
}

void SequenceItem::hoverMoveEvent ( QGraphicsSceneHoverEvent * event ) 
{
	QGraphicsItem::hoverMoveEvent(event);
}

void SequenceItem::mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * event )
{
	QGraphicsItem::mouseDoubleClickEvent(event);
}

void SequenceItem::mouseMoveEvent ( QGraphicsSceneMouseEvent * event )
{
	if (_pressed)
	{
		_dragged = true;
	}
	
	QGraphicsItem::mouseMoveEvent(event);
}	

void SequenceItem::mousePressEvent ( QGraphicsSceneMouseEvent * event )
{
	_pressed = true;
	QGraphicsItem::mousePressEvent(event);
}

void SequenceItem::mouseReleaseEvent ( QGraphicsSceneMouseEvent * event )
{
	_pressed = false;
	if (!_dragged)
	{
//		emit clicked();
	}
	_dragged = false;
//	emit released();
	
	QGraphicsItem::mouseReleaseEvent(event);
}

void SequenceItem::contextMenuEvent ( QGraphicsSceneContextMenuEvent * event )
{
	// For now we're going to stick to using the menu on only one item at a time.
	if (!isSelected() || scene()->selectedItems().size() > 1)
	{
		scene()->clearSelection();
		setSelected(true);
	}
//	emit menuRequested();
	
	QGraphicsItem::contextMenuEvent(event);
}

ExecItem* WaitforItem::fromItem() const
{
  return _fromItem;
}

ExecItem* WaitforItem::toItem() const
{
  return _toItem;
}


