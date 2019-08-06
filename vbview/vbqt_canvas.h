
// vbqt_canvas.h
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
// original version written by Dan Kimberg

#include <qimage.h>
#include <qpainter.h>
#include <QMouseEvent>
#include <QPaintEvent>

enum { vbqt_prevslice, vbqt_nextslice, vbqt_prevvolume, vbqt_nextvolume };

class VBCanvas : public QWidget {
  Q_OBJECT
 public:
  VBCanvas(QWidget *parent = 0, const char *name = 0, Qt::WFlags f = 0);
  void SetImage(QImage *im);
  QImage *GetImage();
  void updateRegion(int x, int y, int width, int height);
  void updateVisibleNow();
  void updateVisibleNow(QRect &r);
 public slots:
 protected:
  void mouseMoveEvent(QMouseEvent *me);
  void mousePressEvent(QMouseEvent *me);
  void moveEvent(QMoveEvent *me);
  void leaveEvent(QEvent *me);
  // void keyPressEvent(QKeyEvent *ke);
  void drawContents(QPainter *p, int clipx, int clipy, int clipw, int cliph);
 private slots:
 signals:
  void mousepress(QMouseEvent me);
  void mousemove(QMouseEvent me);
  void leftcanvas();
  // void keypress(QKeyEvent ke);
 private:
  void paintEvent(QPaintEvent *);
  QImage *cim;
};
