#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>

#include "communicator.h"
#include "ui_MainWindow.h"

class Downloader : public QObject
{
    Q_OBJECT
public:
    explicit Downloader(Ui::MainWindow *ui, Communicator *communicator, QObject *parent = nullptr);
    ~Downloader();

signals:

private slots:
    bool download();

private:
    Communicator *communicator = nullptr;
    Ui::MainWindow *ui = nullptr;
};

#endif // DOWNLOADER_H
