#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>

#include "ui_MainWindow.h"

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(Ui::MainWindow *ui, QObject *parent = nullptr);

signals:

private:
    Ui::MainWindow *ui = nullptr;
};

#endif // LOGGER_H
