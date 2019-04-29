/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module:       clickableimage.cpp
Author:       Cory Parks
Date started: 10/2016
Purpose:

See LICENSE file for copyright and license information

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "clickableimage.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

ClickableImage::ClickableImage(QWidget *parent) : QLabel(parent)
{
}

void ClickableImage::mousePressEvent(QMouseEvent *event )
{

}

void ClickableImage::mouseReleaseEvent(QMouseEvent *event )
{
    if ( event->button() == Qt::LeftButton )
        emit clicked(this);
}



ClickableTableImage::ClickableTableImage(int r, int c, QWidget *parent) : ClickableImage(parent)
{
    row = r;
    col = c;

    QObject::connect(this, SIGNAL(clicked(ClickableImage*)), this, SLOT( onClick()));
}

void ClickableTableImage::onClick()
{
    emit cellClicked (row, col);
}


FavoriteTableIcon::FavoriteTableIcon(int r, QWidget *parent) : ClickableTableImage(r, 0, parent)
{
    state = true;
    QObject::connect(this, SIGNAL(cellClicked(int, int)), this, SLOT( toggleState()));
}

void FavoriteTableIcon::toggleState()
{
    state = !state;
}

