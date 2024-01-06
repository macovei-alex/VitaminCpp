﻿#include "canvaspaint.h"
#include "mainwindow.h"
#include "ui_canvaspaint.h"

#include <QGuiApplication>
#include <QScreen>
#include <QObject>

#ifdef ONLINE
#include "services.h"
#include "common.h"
#endif

CanvasPaint::CanvasPaint(QWidget* parent) :
	QDialog{ parent },
	ui{ new Ui::CanvasPaint },
	drawState{ DrawingState::DRAWING }
{
	ui->setupUi(this);

	ui->gameChatLabel->setStyleSheet("border: none;");
	ui->gameChat->setStyleSheet("QWidget { border: 1px solid black; }");

	QSize screenSize{ QGuiApplication::primaryScreen()->geometry().size() };
	setGeometry(0, 0, screenSize.width(), screenSize.height());
	setStyleSheet("QDialog { border: 2px solid black; }");

	canvasPixmap = QPixmap{ screenSize.width() * 3 / 4, screenSize.height() };
	canvasPixmap.fill(Qt::white);

	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
}

#ifdef ONLINE
CanvasPaint::CanvasPaint(uint64_t roomID, const QString& username, QWidget* parent) :
	QDialog{ parent },
	ui{ new Ui::CanvasPaint },
	drawState{ DrawingState::DRAWING },
	keepGoing{ true },
	roomID{ roomID },
	username{ username },
	imageReceiverThread{ new ImageReceiverThread(roomID, keepGoing, this) },
	gameStateReceiverThread{ new GameStateReceiverThread(roomID, keepGoing, this) }
{
	ui->setupUi(this);

	ui->gameChatLabel->setStyleSheet("border: none;");
	ui->gameChat->setStyleSheet("QWidget { border: 1px solid black; }");

	QSize screenSize{ QGuiApplication::primaryScreen()->geometry().size() };
	setGeometry(0, 0, screenSize.width(), screenSize.height());
	setStyleSheet("QDialog { border: 2px solid black; }");

	canvasPixmap = QPixmap{ screenSize.width() * 3 / 4, screenSize.height() };
	canvasPixmap.fill(Qt::white);

	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

	connect(imageReceiverThread, &ImageReceiverThread::ImageReceivedSignal,
		this, &CanvasPaint::HandleAddLines);
	connect(imageReceiverThread, &ImageReceiverThread::finished,
		imageReceiverThread, &QObject::deleteLater);

	connect(gameStateReceiverThread, &GameStateReceiverThread::GameStateReceivedSignal,
		this, &CanvasPaint::HandleReceiveState);
	connect(gameStateReceiverThread, &GameStateReceiverThread::finished,
		gameStateReceiverThread, &QObject::deleteLater);

	imageReceiverThread->start();
	gameStateReceiverThread->start();
}
#endif

CanvasPaint::~CanvasPaint()
{

#ifdef ONLINE
	services::SignOut(username.toStdString());
	keepGoing = false;
	imageReceiverThread->quit();
	gameStateReceiverThread->quit();

	imageReceiverThread->wait();
	gameStateReceiverThread->wait();
#endif

	delete ui;
}

void CanvasPaint::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setPen(DRAWING_PEN);

	QRect canvasRect = rect();
	painter.drawRect(canvasRect);
	painter.drawPixmap(2, 2, canvasPixmap);
}

void CanvasPaint::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (canvasPixmap.rect().contains(event->pos()))
		{
			lastPoint = event->pos();
		}
	}
}

void CanvasPaint::mouseMoveEvent(QMouseEvent* event)
{
	if (canvasPixmap.rect().contains(event->pos()))
	{
		QPoint currentPos{ event->pos() };

		if (drawState == DrawingState::DRAWING)
		{
			QPainter painter{ &canvasPixmap };
			painter.setPen(DRAWING_PEN);

			currentLine.points.emplace_back(currentPos);

			if (currentLine.points.size() > 1)
			{
				painter.drawLine(
					currentLine.points[currentLine.points.size() - 2],
					currentLine.points[currentLine.points.size() - 1]);
			}

			update();
		}

		else if (drawState == DrawingState::ERASING)
		{
			QPainter painter{ &canvasPixmap };
			painter.setCompositionMode(QPainter::CompositionMode_Source);
			painter.setPen(ERASING_PEN);

			currentLine.points.emplace_back(currentPos);

			if (currentLine.points.size() > 1)
			{
				painter.drawLine(
					currentLine.points[currentLine.points.size() - 2],
					currentLine.points[currentLine.points.size() - 1]);
			}

			update();
		}
	}
}

void CanvasPaint::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		currentLine.drawState = drawState;

#ifdef ONLINE
		auto commonPoints{ std::move(currentLine.ToCommonPoints()) };
		commonPoints.emplace_back(common::img::Point{ -1, -1, MyLine::INVALID_COLOR_INT });
		services::SendImageUpdates(roomID, commonPoints);
#endif

		myLines.emplace_back(std::move(currentLine));
	}
}

void CanvasPaint::resizeEvent(QResizeEvent* event)
{
	QPixmap newPixmap{ event->size().width() * 3 / 4, event->size().height() };
	newPixmap.fill(Qt::white);
	QPainter painter(&newPixmap);
	painter.drawPixmap(QRect{ 0, 0, event->size().width() * 3 / 4, event->size().height() }, canvasPixmap);
	canvasPixmap = std::move(newPixmap);

	update();
}

void CanvasPaint::ClearCanvas()
{
	canvasPixmap.fill(Qt::white);
	myLines.clear();
	update();
}

void CanvasPaint::on_leaveServerButton_clicked()
{
	hide();
	signInWindow = new MainWindow(this);
	signInWindow->show();
}

void CanvasPaint::on_resetCanvas_clicked()
{
	ClearCanvas();
	drawState = DrawingState::DRAWING;
}

void CanvasPaint::on_drawButton_clicked()
{
	drawState = DrawingState::DRAWING;
}

void CanvasPaint::on_eraseButton_clicked()
{
	drawState = DrawingState::ERASING;
}

void CanvasPaint::on_undoButton_clicked()
{
	if (!myLines.isEmpty())
	{
		canvasPixmap.fill(Qt::white);
		QPainter painter{ &canvasPixmap };
		myLines.pop_back();

		for (const auto& line : myLines)
		{
			painter.setPen(line.drawState == DrawingState::DRAWING ? DRAWING_PEN : ERASING_PEN);

			for (qsizetype i = 1; i < line.points.size(); i++)
				painter.drawLine(line.points[i - 1], line.points[i]);
		}

		update();
	}
}

void CanvasPaint::on_messageButton_clicked()
{
	ui->messageBox->clear();
}

void CanvasPaint::closeEvent(QCloseEvent* event)
{
	QCoreApplication::quit();
	event->accept();
}

void CanvasPaint::HandleAddLines(QList<MyLine>* newLines)
{
	QPainter painter{ &canvasPixmap };

	for (auto& line : *newLines)
	{
		painter.setPen((line.drawState == DrawingState::DRAWING ? DRAWING_PEN : ERASING_PEN));
		for (qsizetype i = 1; i < line.points.size(); i++)
			painter.drawLine(line.points[i - 1], line.points[i]);

		myLines.emplace_back(std::move(line));
	}

	delete newLines;
	update();
}

void CanvasPaint::HandleReceiveState(const std::pair<common::game::GameState, uint64_t>& gameStatePair)
{
	qDebug() << "Received game state: "
		<< static_cast<uint16_t>(gameStatePair.first) << " "
		<< gameStatePair.second;
}
