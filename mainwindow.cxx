#include "mainwindow.hxx"
#include "ui_mainwindow.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	gdbserver.listen(QHostAddress::Any, 2331);
	connect(& gdbserver, SIGNAL(newConnection()), this, SLOT(newConnection()));
	gdbserver_socket = 0;
	bm_gdb_port.setPortName("com6");
	bm_debug_port.setPortName("com7");
	
	if (!bm_gdb_port.open(QIODevice::ReadWrite))
		QMessageBox::critical(0, "error", "could not open blackmagic gdb port");
	else
		QMessageBox::information(0, "success", "blackmagic gdb port opened successfully");
	if (!bm_debug_port.open(QIODevice::ReadWrite))
		QMessageBox::critical(0, "error", "could not open blackmagic debug port");
	else
		QMessageBox::information(0, "success", "blackmagic debug port opened successfully");
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
	ui->plainTextEditGdbserverLog->appendPlainText(gdbserver_socket->readAll());
}
