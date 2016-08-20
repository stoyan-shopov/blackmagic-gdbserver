#include "mainwindow.hxx"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QRegExp>

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	gdbserver.listen(QHostAddress::Any, 2331);
	connect(& gdbserver, SIGNAL(newConnection()), this, SLOT(newGdbServerConnection()));
	gdbserver_socket = 0;
	bm_gdb_port.setPortName("com6");
	bm_debug_port.setPortName("com7");
	
	if (!bm_gdb_port.open(QIODevice::ReadWrite))
		QMessageBox::critical(0, "error", "could not open blackmagic gdb port");
	else
	{
		QMessageBox::information(0, "success", "blackmagic gdb port opened successfully");
		connect(& bm_gdb_port, SIGNAL(readyRead()), this, SLOT(bmGdbPortReadyRead()));
	}
	if (!bm_debug_port.open(QIODevice::ReadOnly))
		QMessageBox::critical(0, "error", "could not open blackmagic debug port");
	else
	{
		QMessageBox::information(0, "success", "blackmagic debug port opened successfully");
		connect(& bm_debug_port, SIGNAL(readyRead()), this, SLOT(bmDebugPortReadyRead()));
	}
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::extractGdbPacket()
{
QRegExp	rx("\\$(.+)#..");
QByteArray unzeroed_gdb_data = gdb_incoming_stream_data;
int x;
	/* literal zero bytes in byte arrays do not work with qt regular expressions,
	 * so do this replacement hack as a workaround */
	unzeroed_gdb_data.replace(0, QString("s"));
	if ((x = rx.indexIn(unzeroed_gdb_data)) != -1)
	{
		ui->plainTextEditInternalDebugLog->appendPlainText(QString("detected gdb packet: "
			+ gdb_incoming_stream_data.mid(x, rx.cap(1).length())));
		gdb_incoming_stream_data = gdb_incoming_stream_data.right(gdb_incoming_stream_data.length() - x - rx.matchedLength());
	}
}

void MainWindow::newGdbServerConnection()
{
	gdbserver_socket = gdbserver.nextPendingConnection();
	connect(gdbserver_socket, SIGNAL(readyRead()), this, SLOT(gdbsocketReadyRead()));
}

void MainWindow::gdbsocketReadyRead()
{
QByteArray ba;
	ui->plainTextEditGdbLog->appendPlainText(ba = gdbserver_socket->readAll());
	bm_gdb_port.write(ba);
	gdb_incoming_stream_data += ba;
	extractGdbPacket();
}

void MainWindow::bmGdbPortReadyRead()
{
QByteArray ba;
	ba = bm_gdb_port.readAll();
	if (gdbserver_socket)
		gdbserver_socket->write(ba);
	ui->plainTextEditBmGdbLog->appendPlainText(ba);
}

void MainWindow::bmDebugPortReadyRead()
{
	ui->plainTextEditBmDebugLog->appendPlainText(bm_debug_port.readAll());
}
