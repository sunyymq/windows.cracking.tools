#ifndef REFERENCEVIEW_H
#define REFERENCEVIEW_H

#include <QProgressBar>
#include <QLabel>
#include "SearchListView.h"

class ReferenceView : public SearchListView
{
    Q_OBJECT

public:
    ReferenceView(bool sourceView = false, QWidget* parent = nullptr);
    void setupContextMenu();
    void connectBridge();
    void disconnectBridge();

public slots:
    void addColumnAt(int width, QString title);
    void setRowCount(dsint count);
    void setCellContent(int r, int c, QString s);
    void reloadData();
    void setSingleSelection(int index, bool scroll);
    void setSearchStartCol(int col);
    void referenceContextMenu(QMenu* wMenu);
    void followAddress();
    void followDumpAddress();
    void followApiAddress();
    void followGenericAddress();
    void toggleBreakpoint();
    void setBreakpointOnAllCommands();
    void removeBreakpointOnAllCommands();
    void setBreakpointOnAllApiCalls();
    void removeBreakpointOnAllApiCalls();
    void toggleBookmark();
    void refreshShortcutsSlot();
    void referenceSetProgressSlot(int progress);
    void referenceSetCurrentTaskProgressSlot(int progress, QString taskTitle);
    void searchSelectionChanged(int index);

signals:
    void showCpu();

private:
    QProgressBar* mSearchTotalProgress;
    QProgressBar* mSearchCurrentTaskProgress;
    QAction* mFollowAddress;
    QAction* mFollowDumpAddress;
    QAction* mFollowApiAddress;
    QAction* mToggleBreakpoint;
    QAction* mToggleBookmark;
    QAction* mSetBreakpointOnAllCommands;
    QAction* mRemoveBreakpointOnAllCommands;
    QAction* mSetBreakpointOnAllApiCalls;
    QAction* mRemoveBreakpointOnAllApiCalls;
    QLabel* mCountTotalLabel;

    enum BPSetAction
    {
        Enable,
        Disable,
        Toggle,
        Remove
    };

    void setBreakpointAt(int row, BPSetAction action);
    dsint apiAddressFromString(const QString & s);
};

#endif // REFERENCEVIEW_H
