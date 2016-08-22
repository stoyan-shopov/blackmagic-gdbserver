#include "mainwindow.hxx"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include "gdbpacket.hxx"

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	if (!gdbserver.listen(QHostAddress::Any, GDB_SERVER_PORT))
	{
		QMessageBox::critical(0, "error", "cannot open gdb server port");
		exit(1);
	}
	
	connect(& gdbserver, SIGNAL(newConnection()), this, SLOT(newGdbServerConnection()));
	gdbserver_socket = 0;
	bm_gdb_port.setPortName(QString("com%1").arg(ui->spinBoxGdbSerialPort->value()));
	bm_debug_port.setPortName(QString("com%1").arg(ui->spinBoxDebugSerialPort->value()));
	
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
QPair<QByteArray, QByteArray> x = GdbPacket::extract_packet(gdb_incoming_bytestream_data);

	if (x.first.length())
	{
		ui->plainTextEditInternalDebugLog->appendPlainText(QString("detected gdb packet: ")
				+ x.first);
		gdb_incoming_bytestream_data = x.second;
		
		ui->plainTextEditInternalDebugLog->appendPlainText(GdbPacket::make_complete_packet(x.first));
		ui->plainTextEditInternalDebugLog->appendPlainText(GdbPacket::make_complete_packet(x.first.append(0x80)));
	}
	ui->plainTextEditInternalDebugLog->appendPlainText(GdbPacket::make_complete_packet("."));
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
	gdb_incoming_bytestream_data += ba;
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

void MainWindow::on_pushButton_2_clicked()
{
	bm_gdb_port.write(GdbPacket::make_complete_packet("."));
}
