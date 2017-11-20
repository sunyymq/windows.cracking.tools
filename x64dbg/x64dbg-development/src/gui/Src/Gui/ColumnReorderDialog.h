#ifndef COLUMNREORDERDIALOG_H
#define COLUMNREORDERDIALOG_H

#include <QDialog>

class AbstractTableView;

namespace Ui
{
    class ColumnReorderDialog;
}

class ColumnReorderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ColumnReorderDialog(AbstractTableView* parent = 0);
    ~ColumnReorderDialog();

private slots:
    void on_upButton_clicked();
    void on_downButton_clicked();
    void on_addButton_clicked();
    void on_hideButton_clicked();
    void on_addAllButton_clicked();
    void on_okButton_clicked();

private:
    Ui::ColumnReorderDialog* ui;
    AbstractTableView* mParent;
};

#endif // COLUMNREORDERDIALOG_H
