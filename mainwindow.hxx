#ifndef MAINWINDOW_HXX
#define MAINWINDOW_HXX

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtSerialPort/QSerialPort>

enum
{
	GDB_SERVER_PORT		= 2331,
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
private:
	void extractGdbPacket(void);
	void extractBlackmagicResponsePacket(void);
	void handleBlackmagicResponsePacket(void);

private slots:
	void newGdbServerConnection(void);
	void gdbsocketReadyRead(void);
	void bmGdbPortReadyRead(void);
	void bmDebugPortReadyRead(void);

	void on_pushButton_2_clicked();
	
private:
	Ui::MainWindow *ui;
	QTcpServer	gdbserver;
	QTcpSocket	* gdbserver_socket;
	/* blackmagic usb serial ports */
	QSerialPort	bm_gdb_port;
	QSerialPort	bm_debug_port;
	
	QByteArray	gdb_incoming_bytestream_data;
	QByteArray	bm_incoming_bytestream_data;
};

#endif // MAINWINDOW_HXX
