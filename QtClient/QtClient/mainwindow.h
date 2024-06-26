#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "macro.h"

#include <QMainWindow>

class CanvasWindow;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent *event) override;

private slots:
	void on_loginButton_clicked();
	void on_joinRoomButton_clicked();
	void on_createRoomButton_clicked();

private:
	Ui::MainWindow *ui;
	CanvasWindow *canvasWindow;
	uint64_t roomID;
	bool m_isConnected;
};
#endif // MAINWINDOW_H
