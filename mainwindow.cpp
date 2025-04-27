#include "mainwindow.h"
#include "serialport.h"
#include "ui_mainwindow.h"

namespace
{
SerialPort *serialPort = nullptr;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    serialPort = new SerialPort(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
