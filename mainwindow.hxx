#ifndef MAINWINDOW_HXX
#define MAINWINDOW_HXX

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtSerialPort/QSerialPort>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void newGdbServerConnection(void);
	void gdbsocketReadyRead(void);
	void bmGdbPortReadyRead(void);
	void bmDebugPortReadyRead(void);

private:
	Ui::MainWindow *ui;
	QTcpServer	gdbserver;
	QTcpSocket	* gdbserver_socket;
	/* blackmagic usb serial ports */
	QSerialPort	bm_gdb_port;
	QSerialPort	bm_debug_port;
};

#endif // MAINWINDOW_HXX
