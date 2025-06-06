#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QByteArray>
#include <QFile>
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
    void onBinDataRead(QByteArray data);

private:
    Communicator *communicator = nullptr;
    Ui::MainWindow *ui = nullptr;
    QFile *file = nullptr;
    int fileId = 0;
    int startId = 0;
    int endId = 0;
};

#endif // DOWNLOADER_H
