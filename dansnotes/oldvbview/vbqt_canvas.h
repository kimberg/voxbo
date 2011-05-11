
#include <qimage.h>
#include <qpainter.h>

enum {vbqt_prevslice,vbqt_nextslice,vbqt_prevvolume,vbqt_nextvolume};

class VBCanvas : public QWidget {
  Q_OBJECT
public:
  VBCanvas(QWidget *parent=0,const char *name=0,WFlags f=0);
  void SetImage(QImage *im);
  QImage *GetImage();
  void updateRegion(int x,int y,int width,int height);
  void updateVisibleNow();
  void updateVisibleNow(QRect &r);
public slots:
protected:
 void mouseMoveEvent(QMouseEvent *me);
 void mousePressEvent(QMouseEvent *me);
 //void keyPressEvent(QKeyEvent *ke);
 void drawContents(QPainter *p,int clipx,int clipy,int clipw,int cliph);
private slots:
signals:
 void mousepress(QMouseEvent me);
 void mousemove(QMouseEvent me);
 //void keypress(QKeyEvent ke);
private:
  void paintEvent(QPaintEvent *);
  QImage *cim;
};
