#include "mainwindow.hxx"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QSettings>
#include "gdbpacket.hxx"

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{
	QCoreApplication::setOrganizationName("shopov instruments");
	QCoreApplication::setApplicationName("blackmagic gdbserver");
	QSettings s("bm-gdbserver.rc", QSettings::IniFormat);
	ui->setupUi(this);
	blackmagic_state = IDLE;
	if (!gdbserver.listen(QHostAddress::Any, GDB_SERVER_PORT))
	{
		QMessageBox::critical(0, "error", "cannot open gdb server port");
		exit(1);
	}
	restoreGeometry(s.value("window-geometry", QByteArray()).toByteArray());
	restoreGeometry(s.value("window-state", QByteArray()).toByteArray());
	ui->spinBoxGdbSerialPort->setValue(s.value("bm-gdb-serial-port-number", 1).toInt());
	ui->spinBoxDebugSerialPort->setValue(s.value("bm-debug-serial-port-number", 2).toInt());
	
	connect(& gdbserver, SIGNAL(newConnection()), this, SLOT(newGdbServerConnection()));
	gdbserver_socket = 0;
	bm_gdb_port.setPortName(QString("com%1").arg(ui->spinBoxGdbSerialPort->value()));
	bm_debug_port.setPortName(QString("com%1").arg(ui->spinBoxDebugSerialPort->value()));
	
	if (!bm_gdb_port.open(QIODevice::ReadWrite))
		QMessageBox::critical(0, "error", "could not open blackmagic gdb port");
	else
	{
		QMessageBox::information(0, "success", "blackmagic gdb port opened successfully");
		bm_gdb_port.setDataTerminalReady(true);
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

void MainWindow::closeEvent(QCloseEvent *e)
{
	QSettings s("bm-gdbserver.rc", QSettings::IniFormat);
	s.setValue("bm-gdb-serial-port-number", ui->spinBoxGdbSerialPort->value());
	s.setValue("bm-debug-serial-port-number", ui->spinBoxDebugSerialPort->value());
	s.setValue("window-geometry", saveGeometry());
	s.setValue("window-state", saveState());
	QMainWindow::closeEvent(e);
}

void MainWindow::extractGdbPacket()
{
QPair<QByteArray, QByteArray> x;

	while ((x = GdbPacket::extract_packet(gdb_incoming_bytestream_data)).first.length())
	{
		ui->plainTextEditInternalDebugLog->appendPlainText(QString("detected gdb packet: ")
				+ x.first);
		gdb_incoming_bytestream_data = x.second;
		
		ui->plainTextEditGdbLog->appendPlainText(QString("\t") + GdbPacket::decode_request_packet(x.first));
		
		ui->plainTextEditInternalDebugLog->appendPlainText(GdbPacket::make_complete_packet(x.first));
		ui->plainTextEditInternalDebugLog->appendPlainText(GdbPacket::make_complete_packet(x.first.append(0x80)));
	}
	ui->plainTextEditInternalDebugLog->appendPlainText(GdbPacket::make_complete_packet("."));
}

void MainWindow::extractBlackmagicResponsePacket()
{
QPair<QByteArray, QByteArray> x;

	while ((x = GdbPacket::extract_packet(bm_incoming_bytestream_data)).first.length())
	{
		ui->plainTextEditInternalDebugLog->appendPlainText(QString("detected bm response packet: ")
				+ x.first);
		handleBlackmagicResponsePacket(x.first);
		bm_incoming_bytestream_data = x.second;
	}
}

void MainWindow::handleBlackmagicResponsePacket(QByteArray packet)
{

	switch (blackmagic_state)
	{
		default:
		case IDLE:
			break;
		case WAITING_SWDP_SCAN_RESPONSE:
		{
			static bool waiting_target_list = false;
			if (packet.startsWith("OK"))
			{
				/* last packet */
				waiting_target_list = false;
				blackmagic_state = IDLE;
			}
			else switch (packet[0])
			{
				case 'O': /* monitor output packet */
					
					packet = QByteArray::fromHex(packet.mid(1));
					ui->plainTextEditInternalDebugLog->appendPlainText(QString("monitor: ") + packet);
					packet.replace("\n", "");
					if (waiting_target_list)
					{
						if (packet.startsWith("No usable targets found"))
							ui->comboBoxDetecteTargets->clear();
						else
							ui->comboBoxDetecteTargets->addItem(packet);
					}
					if (packet.startsWith("No. Att Driver"))
						waiting_target_list = true;
					break;
			}
			bm_gdb_port.write("+");
			break;
		}
		case WAITING_SWDP_ATTACH_RESPONSE:
		{
			bm_gdb_port.write("+");
			bm_gdb_port.flush();
			if (packet.startsWith("T"))
				QMessageBox::information(0, "success", "target attached");
			else
				QMessageBox::critical(0, "error", "cannot attach to target");
			blackmagic_state = IDLE;
			break;
		}
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
	ui->plainTextEditGdbLog->appendPlainText(QString("gdb> ") + (ba = gdbserver_socket->readAll()));
	bm_gdb_port.write(ba);
	gdb_incoming_bytestream_data += ba;
	extractGdbPacket();
}

void MainWindow::bmGdbPortReadyRead()
{
QByteArray ba;
	ba = bm_gdb_port.readAll();
	if (gdbserver_socket && blackmagic_state == IDLE)
		gdbserver_socket->write(ba);
	ui->plainTextEditGdbLog->appendPlainText(QString("bm > ") + ba);
	bm_incoming_bytestream_data += ba;
	extractBlackmagicResponsePacket();
}

void MainWindow::bmDebugPortReadyRead()
{
	ui->plainTextEditBmDebugLog->appendPlainText(bm_debug_port.readAll());
}

void MainWindow::on_pushButton_2_clicked()
{
	bm_gdb_port.write(GdbPacket::make_complete_packet("."));
}

void MainWindow::on_pushButtonSWDPScan_clicked()
{
	blackmagic_state = WAITING_SWDP_SCAN_RESPONSE;
	bm_gdb_port.write(GdbPacket::make_complete_packet(GdbPacket::format_monitor_packet("swdp_scan")));
}

void MainWindow::on_pushButtonAttach_clicked()
{
	blackmagic_state = WAITING_SWDP_ATTACH_RESPONSE;
	bm_gdb_port.write(GdbPacket::make_complete_packet("vAttach;1"));
}
