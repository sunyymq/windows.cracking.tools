#ifndef BROWSEDIALOG_H
#define BROWSEDIALOG_H

#include <QDialog>

namespace Ui
{
    class BrowseDialog;
}

class BrowseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BrowseDialog(QWidget* parent, const QString & title, const QString & text, const QString & filter, const QString & defaultPath, bool save);
    ~BrowseDialog();

    QString path;
public slots:
    void on_browse_clicked();
    void on_ok_clicked();

private:
    Ui::BrowseDialog* ui;
    QString mFilter;
    bool mSave;
};

#endif // BROWSEDIALOG_H
