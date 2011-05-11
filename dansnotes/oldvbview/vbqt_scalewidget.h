
#include <qimage.h>
#include <qpainter.h>
#include <stdlib.h>

class VBScalewidget : public QFrame {
  Q_OBJECT
public:
  VBScalewidget(QWidget *parent=0,const char *name=0,WFlags f=0);
  void drawContents(QPainter *);
public slots:
  void setscale(float low,float high,QColor negwcolor1,QColor negcolor2,
              QColor poscolor1,QColor poscolor2);
protected:
  void drawContents(QPainter *p,int clipx,int clipy,int clipw,int cliph);
private slots:
signals:
  void newcolors(QColor,QColor,QColor,QColor);
private:
  void contextmenu();
  void drawcolorbars(QPainter *,QRect,int);
  void mousePressEvent(QMouseEvent *);
  void mouseDoubleClickEvent(QMouseEvent *);
  QColor q_negcolor1,q_negcolor2,q_poscolor1,q_poscolor2;
  float q_low,q_high;
  int q_lowleft,q_lowright,q_highleft,q_highright,q_top,q_bottom;
};
