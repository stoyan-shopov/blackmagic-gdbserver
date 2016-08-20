#include "mainwindow.hxx"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	gdbserver.listen(QHostAddress::Any, 2331);
	connect(& gdbserver, SIGNAL(newConnection()), this, SLOT(newConnection()));
	gdbserver_socket = 0;
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::newConnection()
{
	gdbserver_socket = gdbserver.nextPendingConnection();
	connect(gdbserver_socket, SIGNAL(readyRead()), this, SLOT(gdbsocketReadyRead()));
}

void MainWindow::gdbsocketReadyRead()
{
}
