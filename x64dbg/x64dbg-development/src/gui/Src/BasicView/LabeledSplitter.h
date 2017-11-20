#ifndef LABELEDSPLITTER_H
#define LABELEDSPLITTER_H

#include <QSplitter>

class LabeledSplitter;
class QMenu;
class QAction;
class LabeledSplitterDetachedWindow;

class LabeledSplitter : public QSplitter
{
    Q_OBJECT
public:
    LabeledSplitter(QWidget* parent);
    void addWidget(QWidget* widget, const QString & name);
    void insertWidget(int index, QWidget* widget, const QString & name);
    void collapseLowerTabs();
    void loadFromConfig(const QString & configName);
    QList<QString> names;
    QList<QWidget*> m_Windows;
public slots:
    void attachSlot(LabeledSplitterDetachedWindow* widget);
    void contextMenuEvent(QContextMenuEvent* event);
protected slots:
    void detachSlot();
    void collapseSlot();
    void closeSlot();
protected:
    QMenu* mMenu;
    QAction* mExpandCollapseAction;
    QString mConfigName;
    int currentIndex;
    void setOrientation(Qt::Orientation o); // LabeledSplitter is always vertical
    void addWidget(QWidget* widget);
    void insertWidget(int index, QWidget* widget);
    QSplitterHandle* createHandle() override;
    void setupContextMenu();

    friend class LabeledSplitterHandle;
};

#endif //LABELEDSPLITTER_H
