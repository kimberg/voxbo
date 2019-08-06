
// qdecimalspinbox.cpp
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
// original version written by Tom King

#include "qdecimalspinbox.h"

/*****************************************************************************
 * QDecimalSpinBox implementation
 *
 * Remember, when using a QDecimalSpinBox, that the value that you set publicly
 * and that which is returned by the control, notwithstanding what is displayed
 * textually in the line edit box, is an integer value equal to the displayed
 * value (in base 10) with the decimal place shifted to the left mDecPlaces
 * places (i.e. (public_value)  = (display_value) * 10^mDecPlaces).
 *****************************************************************************/

QDecimalSpinBox::QDecimalSpinBox(int aDecPlaces, QWidget *parent,
                                 const char *name)
    : QSpinBox(parent, name), mDecPlaces(aDecPlaces) {
  setLineStep(100);

  mDivFactor = 1.0;
  while (aDecPlaces-- > 0) mDivFactor *= 10.0;
}

QString QDecimalSpinBox::mapValueToText(int value) {
  double dvalue = double(value) / mDivFactor;
  return QString::number(dvalue, 'f', mDecPlaces);
}

int QDecimalSpinBox::mapTextToValue(bool *ok) {
  *ok = true;
  return (int)(mDivFactor * text().toDouble());  // 0 to 100
}
