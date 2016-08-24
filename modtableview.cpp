#include "modtableview.h"

#include <QMouseEvent>

ModTableView::ModTableView(QWidget* parent) : QTableView(parent)
{

}

void ModTableView::mousePressEvent(QMouseEvent* event)
{
    QModelIndex start = this->indexAt(event->pos());
    source = start.row();
    QTableView::mousePressEvent(event);
}

void ModTableView::dropEvent(QDropEvent* event)
{
    QModelIndex end = this->indexAt(event->pos());
    destination = end.row();
    QTableView::dropEvent(event);
    emit itemDropped(source, destination);
}
