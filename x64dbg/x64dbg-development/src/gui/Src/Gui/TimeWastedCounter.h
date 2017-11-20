#ifndef TIMEWASTEDCOUNTER_H
#define TIMEWASTEDCOUNTER_H

#include <QObject>

class QLabel;

class TimeWastedCounter : public QObject
{
    Q_OBJECT
public:
    explicit TimeWastedCounter(QObject* parent, QLabel* label);

private slots:
    void updateTimeWastedCounter();

private:
    QLabel* mLabel;
};

#endif // TIMEWASTEDCOUNTER_H
