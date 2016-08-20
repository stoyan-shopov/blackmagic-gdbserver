#ifndef MAINWINDOW_HXX
#define MAINWINDOW_HXX

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

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
	void newConnection(void);
	void gdbsocketReadyRead(void);

private:
	Ui::MainWindow *ui;
	QTcpServer	gdbserver;
	QTcpSocket	* gdbserver_socket;
};

#endif // MAINWINDOW_HXX
