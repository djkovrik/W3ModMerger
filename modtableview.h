#ifndef MODTABLEVIEW_H
#define MODTABLEVIEW_H

#include <QTableView>

class ModTableView : public QTableView
{
    Q_OBJECT

public:
    ModTableView(QWidget* parent = 0);

signals:
    void itemDropped(int source, int destination);

private:
    int source;
    int destination;

    void mousePressEvent(QMouseEvent* event);
    void dropEvent(QDropEvent* event);
};

#endif // MODTABLEVIEW_H
