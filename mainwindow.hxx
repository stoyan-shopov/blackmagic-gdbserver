#ifndef MAINWINDOW_HXX
#define MAINWINDOW_HXX

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtSerialPort/QSerialPort>

enum
{
	GDB_SERVER_PORT		= 5000,
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
	struct ram_area { uint32_t start_address, length; };
	struct flash_area { uint32_t start_address, length, block_size; };

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
protected:
	void closeEvent(QCloseEvent * e);
signals:
	void probeConnected(void);
	void targetScanSuccessfull(void);
	void targetAttached(void);
	
private:
	void extractGdbPacket(void);
	void extractBlackmagicResponsePacket(void);
	void handleBlackmagicResponsePacket(QByteArray packet);

private slots:
	void newGdbServerConnection(void);
	void gdbsocketReadyRead(void);
	void bmGdbPortReadyRead(void);
	void bmDebugPortReadyRead(void);
	
	void blackmagicError(QSerialPort::SerialPortError error);
	
	void handleLogVisibility(void);

	void on_pushButton_2_clicked();
	
	void on_pushButtonSWDPScan_clicked();
	void on_pushButtonAttach_clicked();
	void on_pushButtonReset_clicked();

	void on_pushButtonReadTest_clicked();
	
	void on_pushButtonWriteTest_clicked();
	
private:
	Ui::MainWindow *ui;
	QTcpServer	gdbserver;
	QTcpSocket	* gdbserver_socket;
	/* blackmagic usb serial ports */
	QSerialPort	bm_gdb_port;
	QSerialPort	bm_debug_port;
	
	QVector<struct ram_area> ram_areas;
	QVector<struct flash_area> flash_areas;
	
	int		packet_size;
	
	QByteArray	gdb_incoming_bytestream_data;
	QByteArray	bm_incoming_bytestream_data;
	bool		is_gdb_connected;
	enum
	{
		IDLE	= 0,
		WAITING_FOR_PROBE_CONNECT,
		WAITING_FOR_MEMORY_MAP,
		WAITING_SWDP_SCAN_RESPONSE,
		WAITING_SWDP_ATTACH_RESPONSE,
		WAITING_FEATURES_RESPONSE,
		WAITING_SWDP_RESET_RESPONSE,
	}
	blackmagic_state;
	
};

#endif // MAINWINDOW_HXX
