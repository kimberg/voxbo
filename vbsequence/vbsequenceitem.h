#ifndef VBSEQUENCEITEM_H
#define VBSEQUENCEITEM_H

#include <QGraphicsItem>

#include <tr1/memory>

#include "vbsequence.h"

using std::tr1::shared_ptr;

namespace QtVB
{
  class PipelineArranger;
  class SequenceScene;
	class SequenceView;
	
	class SequenceItem;
	class ExecItem;
	class JobItem;
	class BlockItem;
	class WaitforItem;
	
  class SequenceItem : public QGraphicsItem//, public QObject
  {
  	//Q_OBJECT
  	
    public:
      SequenceItem(QGraphicsItem * parent = 0, QGraphicsScene * scene = 0);
      
      bool isSelected() const { return _selected; }
      bool isHovered() const { return _hovered; }
      bool isPressed() const { return _pressed; }
      
      bool detailed() const;
      void detailed(bool);
      
      virtual QString sequenceItemType() const = 0;
    
    /*signals:
    	void moved();
    	void selected();
    	void deselected();
    	void hovered();
    	void pressed();
    	void dragged();
    	void released();
    	void clicked();
    	void doubleClicked();
    	void menuRequested();
    	
    	void detailChanged();
    	
    	virtual void painted();*/
    	
    protected:
    	bool _selected;
    	bool _hovered;
    	bool _pressed;
    	bool _dragged;
    	
    	virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
    	virtual void hoverLeaveEvent ( QGraphicsSceneHoverEvent * event );
			virtual void hoverMoveEvent ( QGraphicsSceneHoverEvent * event ) ;
			
      virtual void mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * event );
			virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * event );
			virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
			virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );
			
      virtual void contextMenuEvent ( QGraphicsSceneContextMenuEvent * event );
      
      virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
      
      bool _detailed;
      
      SequenceScene* seqScene;
  };
  
  class ExecItem : public SequenceItem
  {
  	friend class WaitforItem;
  	
  	protected:
  	  ExecItem(shared_ptr<VB::Exec> ei, SequenceItem * parent, SequenceScene * scene);
  	
  	public:
      virtual QRectF boundingRect() const = 0;
      virtual QPainterPath shape() const = 0;
      virtual void calcBoundingRectSize(QPainter *painter = 0) = 0;
      virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) = 0;
      
      QPoint sourcePoint() const;
      QPoint sinkPoint() const;
      shared_ptr<VB::Exec> exec() const;
      
      
      
    protected:
      virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
      
      shared_ptr<VB::Exec> _exec;
      QRect _boundingRect;
      QList<WaitforItem*> _connections;
  };
  
  class BlockItem : public ExecItem
  {
  	public:
  		BlockItem(shared_ptr<VB::Block> j, SequenceItem * parent, SequenceScene * scene);
      
      shared_ptr<VB::Block> block() const;
      virtual QRectF boundingRect() const;
      virtual QPainterPath shape() const;
      virtual void calcBoundingRectSize(QPainter *painter = 0);
      virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
      
      virtual QString sequenceItemType() const { return "block"; }
      
      void createExecs();
      void showExecs();
      void hideExecs();
      bool execsShown();
      
    protected:
      bool _detailed;
  };
  
  class JobItem : public ExecItem
  {
    public:
      JobItem(shared_ptr<VB::Job> j, SequenceItem * parent, SequenceScene * scene);
      
      shared_ptr<VB::Job> job() const;
      virtual QRectF boundingRect() const;
      virtual QPainterPath shape() const;
      virtual void calcBoundingRectSize(QPainter *painter = 0);
      virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
      
      virtual QString sequenceItemType() const { return "job"; }
      
    protected:
  };
  
  class WaitforItem : public SequenceItem
  {
  	//Q_OBJECT
  	
  	friend class SequenceScene;
  	friend class PipelineArranger;
  	
    public:
      WaitforItem(ExecItem* a_fromItem, ExecItem* a_toItem);
      virtual ~WaitforItem();
                  
      QRectF boundingRect() const;
      QPainterPath shape() const;
      virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
      
      virtual QPointF fromEnd() const;
      virtual QPointF toEnd() const;
      
      ExecItem* fromItem() const;
      ExecItem* toItem() const;
      
      virtual QString sequenceItemType() const { return "waitfor"; }
      
    protected:
    	ExecItem* _fromItem;
    	ExecItem* _toItem;
      bool _needsConversion;
  };
  
  class TempWaitforDragItem : public WaitforItem
  {
    friend class SequenceScene;
    friend class PipelineArranger;
    
    public:
      TempWaitforDragItem(ExecItem* a_fromItem);
      
      virtual QPointF toEnd() const;
      virtual void setToEnd(const QPointF& p);
      
      virtual QString sequenceItemType() const { return "waitdrag"; }
      
    protected:
      QPointF _toPoint;
  };
}

#endif
