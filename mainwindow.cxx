#include "mainwindow.hxx"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QXmlStreamReader>
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
	is_gdb_connected = false;
	packet_size = 0;
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
	bm_gdb_port.setPortName(QString("\\\\.\\com%1").arg(ui->spinBoxGdbSerialPort->value()));
	bm_debug_port.setPortName(QString("\\\\.\\com%1").arg(ui->spinBoxDebugSerialPort->value()));
	
	if (!bm_gdb_port.open(QIODevice::ReadWrite))
		QMessageBox::critical(0, "error", "could not open blackmagic gdb port");
	else
	{
		connect(& bm_gdb_port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(blackmagicError(QSerialPort::SerialPortError)));
		blackmagic_state = WAITING_FOR_PROBE_CONNECT;
		connect(& bm_gdb_port, SIGNAL(readyRead()), this, SLOT(bmGdbPortReadyRead()));
		QMessageBox::information(0, "success", "blackmagic gdb port opened successfully");
		bm_gdb_port.setDataTerminalReady(true);
	}
	if (!bm_debug_port.open(QIODevice::ReadOnly))
		QMessageBox::critical(0, "error", "could not open blackmagic debug port");
	else
	{
		connect(& bm_debug_port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(blackmagicError(QSerialPort::SerialPortError)));
		QMessageBox::information(0, "success", "blackmagic debug port opened successfully");
		connect(& bm_debug_port, SIGNAL(readyRead()), this, SLOT(bmDebugPortReadyRead()));
	}
	connect(ui->checkBoxShowLogs, SIGNAL(clicked(bool)), this, SLOT(handleLogVisibility()));
QString target_xml(
"<?xml version=\"1.0\"?><!DOCTYPE target SYSTEM \"gdb-target.dtd\"><target>  <architecture>arm</architecture>  <feature name=\"org.gnu.gdb.arm.m-profile\">    <reg name=\"r0\" bitsize=\"32\"/>    <reg name=\"r1\" bitsize=\"32\"/>    <reg name=\"r2\" bitsize=\"32\"/>    <reg name=\"r3\" bitsize=\"32\"/>    <reg name=\"r4\" bitsize=\"32\"/>    <reg name=\"r5\" bitsize=\"32\"/>    <reg name=\"r6\" bitsize=\"32\"/>    <reg name=\"r7\" bitsize=\"32\"/>    <reg name=\"r8\" bitsize=\"32\"/>    <reg name=\"r9\" bitsize=\"32\"/>    <reg name=\"r10\" bitsize=\"32\n"
"bm > \"/>    <reg name=\"r11\" bitsize=\"32\"/>    <reg name=\"r12\" bitsize=\"32\"/>    <reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>    <reg name=\"lr\" bitsize=\"32\" type=\"code_ptr\"/>    <reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>    <reg name=\"xpsr\" bitsize=\"32\"/>    <reg name=\"msp\" bitsize=\"32\" save-restore=\"no\" type=\"data_ptr\"/>    <reg name=\"psp\" bitsize=\"32\" save-restore=\"no\" type=\"data_ptr\"/>    <reg name=\"special\" bitsize=\"32\" save-restore=\"no\"/>  </feature></target>"
);
"<memory-map><memory type=\"ram\" start=\"0x20000000\" length=\"0x5000\"/><memory type=\"flash\" start=\"0x08000000\" length=\"0x20000\"><property name=\"blocksize\">0x800</property></memory></memory-map>";
QXmlStreamReader xml(target_xml);

while (!xml.atEnd())
{
	QXmlStreamAttributes a;
	xml.readNext();
	qDebug() << xml.tokenType() << xml.tokenString() << xml.text() << xml.name();
	if (xml.isStartElement())
	{
		qDebug() << "attributes:";
		int i;
		a = xml.attributes();
		for (i = 0; i < a.size(); i++)
		{
			qDebug() << a[i].name() << a[i].value();
		}
		if (xml.name() == "reg")
		{
			for (i = 0; i < a.size(); i++)
			{
				if (a[i].name() == "name")
					ui->comboBoxRegisters->addItem(a[i].value().toString());
			}
		}
	}
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
			if (!is_gdb_connected)
				bm_gdb_port.write("+");
			break;
		case WAITING_FOR_MEMORY_MAP:
			bm_gdb_port.write("+");
			if (packet.at(0) == 'm')
			{
				QXmlStreamReader xml(packet.right(packet.length() - 1));
				int i;
				ram_areas.clear();
				flash_areas.clear();
				uint32_t address, length;
				bool is_ram;

				while (!xml.atEnd())
				{
					QXmlStreamAttributes a;
					xml.readNext();
					qDebug() << xml.tokenType() << xml.tokenString() << xml.text() << xml.name();
					if (xml.isStartElement())
					{
						qDebug() << "attributes:";
						a = xml.attributes();
						for (i = 0; i < a.size(); i++)
						{
							qDebug() << a[i].name() << a[i].value();
						}
						if (xml.name() == "memory")
						{
							is_ram = false;
							for (i = 0; i < a.size(); i++)
							{
								if (a[i].name() == "type")
									is_ram = ((a[i].value() == "ram") ? true : false);
								else if (a[i].name() == "start")
									address = a[i].value().toUInt(0, 0);
								else if (a[i].name() == "length")
									length = a[i].value().toUInt(0, 0);
							}
							if (is_ram)
							{
								struct ram_area mem;
								mem.start_address = address;
								mem.length = length;
								ram_areas.push_back(mem);
							}
						}
						else if (xml.name() == "property")
						{
							for (i = 0; i < a.size(); i++)
							{
								if (a[i].name() == "name" && a[i].value() == "blocksize")
								{
									struct flash_area mem;
									xml.readNext();
									mem.start_address = address;
									mem.length = length;
									mem.block_size = xml.text().toUInt(0, 0);
									flash_areas.push_back(mem);
									break;
								}
							}
						}
					}
				}
				ui->tableWidgetMemoryAreas->clear();
				for (i = 0; i < ram_areas.length(); i ++)
				{
					ui->tableWidgetMemoryAreas->insertRow(ui->tableWidgetMemoryAreas->rowCount());
					ui->tableWidgetMemoryAreas->setItem(i, 0, new QTableWidgetItem("ram"));
					ui->tableWidgetMemoryAreas->setItem(i, 1, new QTableWidgetItem(QString("0x%1").arg(ram_areas[i].start_address, 0, 16)));
					ui->tableWidgetMemoryAreas->setItem(i, 2, new QTableWidgetItem(QString("0x%1").arg(ram_areas[i].length, 0, 16)));
					ui->tableWidgetMemoryAreas->setItem(i, 3, new QTableWidgetItem("n/a"));
				}
				for (i = 0; i < flash_areas.length(); i ++)
				{
					int n;
					ui->tableWidgetMemoryAreas->insertRow(n = ui->tableWidgetMemoryAreas->rowCount());
					ui->tableWidgetMemoryAreas->setItem(n, 0, new QTableWidgetItem("flash"));
					ui->tableWidgetMemoryAreas->setItem(n, 1, new QTableWidgetItem(QString("0x%1").arg(flash_areas[i].start_address, 0, 16)));
					ui->tableWidgetMemoryAreas->setItem(n, 2, new QTableWidgetItem(QString("0x%1").arg(flash_areas[i].length, 0, 16)));
					ui->tableWidgetMemoryAreas->setItem(n, 3, new QTableWidgetItem(QString("0x%1").arg(flash_areas[i].block_size, 0, 16)));
				}
			}
			blackmagic_state = IDLE;
			break;
		case WAITING_FEATURES_RESPONSE:
			bm_gdb_port.write("+");
			{
				QRegExp	rx("PacketSize=(.+);");
				rx.setMinimal(true);
				if (rx.indexIn(packet) != -1)
				{
					packet_size = rx.cap(1).toInt(0, 16);
					ui->plainTextEditInternalDebugLog->appendPlainText(QString("detected packet size: %1").arg(packet_size));
					bm_gdb_port.write(GdbPacket::make_complete_packet(QString("qXfer:memory-map:read::0,%1")
						  .arg(packet_size - /* reserve space for the start and end packet markers, the expected 'm' response packet type byte, and the two byte checksum */ - 5,0, 16).toLocal8Bit()));
					blackmagic_state = WAITING_FOR_MEMORY_MAP;
					break;
				}
			}
			blackmagic_state = IDLE;
		case WAITING_FOR_PROBE_CONNECT:
			bm_gdb_port.write("+");
			if (packet.startsWith("OK"))
			{
				/* probe connected */
				ui->groupBoxBlackmagicConnectionSettings->setEnabled(false);
				ui->groupBoxTargetControl->setEnabled(true);
				blackmagic_state = IDLE;
			}
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
		case WAITING_SWDP_RESET_RESPONSE:
		{
			bm_gdb_port.write("+");
			bm_gdb_port.flush();
			if (packet.startsWith("T"))
				QMessageBox::information(0, "success", "target reset");
			else
				QMessageBox::critical(0, "error", "cannot reset target");
			blackmagic_state = IDLE;
			break;
		}
	}
}

void MainWindow::newGdbServerConnection()
{
	gdbserver_socket = gdbserver.nextPendingConnection();
	is_gdb_connected = true;
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

void MainWindow::blackmagicError(QSerialPort::SerialPortError error)
{
	qDebug() << error;
}

void MainWindow::handleLogVisibility()
{
	if (ui->checkBoxShowLogs->isChecked())
	{
		ui->groupBoxLogs->show();
	}
	else
	{
		ui->groupBoxLogs->hide();
	}
	this->updateGeometry();
	this->resize(minimumSize());
}

void MainWindow::on_pushButton_2_clicked()
{
	//bm_gdb_port.write(GdbPacket::make_complete_packet("."));
	blackmagic_state = WAITING_FEATURES_RESPONSE;
	bm_gdb_port.write(GdbPacket::make_complete_packet("qSupported"));
}

void MainWindow::on_pushButtonSWDPScan_clicked()
{
	blackmagic_state = WAITING_SWDP_SCAN_RESPONSE;
	ui->comboBoxDetecteTargets->clear();
	bm_gdb_port.write(GdbPacket::make_complete_packet(GdbPacket::format_monitor_packet("swdp_scan")));
}

void MainWindow::on_pushButtonAttach_clicked()
{
	blackmagic_state = WAITING_SWDP_ATTACH_RESPONSE;
	bm_gdb_port.write(GdbPacket::make_complete_packet("vAttach;1"));
}

void MainWindow::on_pushButtonReset_clicked()
{
	blackmagic_state = WAITING_SWDP_RESET_RESPONSE;
	bm_gdb_port.write(GdbPacket::make_complete_packet("vRun;"));
}

void MainWindow::on_pushButtonReadTest_clicked()
{
	bm_gdb_port.write(GdbPacket::make_complete_packet("m0,400"));
}

void MainWindow::on_pushButtonWriteTest_clicked()
{
	
	bm_gdb_port.write(GdbPacket::make_complete_packet(QByteArray("X20000000,400:") + QByteArray(0x400, ' ')));
}
