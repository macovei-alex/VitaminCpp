﻿#include "canvaspaint.h"
#include "mainwindow.h"
#include "ui_canvaspaint.h"

#include <QGuiApplication>
#include <QScreen>
#include <QObject>

const int32_t MyLine::DRAWING_COLOR_INT{ 0x000000 };
const int32_t MyLine::ERASING_COLOR_INT{ 0xFFFFFF };

#ifdef ONLINE
const int32_t MyLine::INVALID_COLOR_INT{ 0xFFFFFE };
#endif

#ifdef ONLINE
MyLine::MyLine(std::vector<common::img::Point>&& commonPoints, uint32_t color) :
	drawState{ color == ERASING_COLOR_INT ? DrawingState::ERASING : DrawingState::DRAWING },
	points{ static_cast<qsizetype>(commonPoints.size()), QPoint() }
{
	size_t pos = 0;
	for (const auto& commonPoint : commonPoints)
		points[pos++] = QPoint{ commonPoint.x, commonPoint.y };
}

std::vector<common::img::Point> MyLine::ToCommonPoints() const
{
	std::vector<common::img::Point> commonPoints{ static_cast<size_t>(points.size()), common::img::Point{} };
	size_t pos = 0;

	for (const auto& point : points)
	{
		commonPoints[pos++] = {
			point.x(),
			point.y(),
			(drawState == DrawingState::DRAWING ? DRAWING_COLOR_INT : ERASING_COLOR_INT) };
	}

	return commonPoints;
}

ImageReceiver::ImageReceiver(uint64_t roomID, bool& keepGoing, QWidget* parent) :
	QThread{ parent },
	keepGoing{ keepGoing },
	roomID{ roomID }
{
	/* empty */
}

void ImageReceiver::run()
{
	using std::chrono_literals::operator""s;
	try
	{
		while (keepGoing)
		{
			auto commonPoints{ std::move(services::ReceiveImageUpdates(roomID)) };

			if (commonPoints.empty())
			{
				std::this_thread::sleep_for(0.25s);
				continue;
			}

			qDebug() << "Received points: " << commonPoints.size();

			QList<MyLine>* newLines{ new QList<MyLine> };
			MyLine* line;

			for (size_t i = 0; i < commonPoints.size(); i++)
			{
				line = new MyLine;
				line->drawState = (commonPoints[i].color == MyLine::ERASING_COLOR_INT ? DrawingState::ERASING : DrawingState::DRAWING);

				while (i < commonPoints.size() && commonPoints[i].color != MyLine::INVALID_COLOR_INT)
				{
					line->points.emplace_back(commonPoints[i].x, commonPoints[i].y);
					i++;
				}

				newLines->emplace_back(std::move(*line));
				i++;
			}

			qDebug() << "Received lines: " << newLines->size();

			emit LinesReceivedSignal(newLines);

			std::this_thread::sleep_for(0.25s);
		}
	}
	catch (const std::exception& e)
	{
		qDebug() << e.what() << '\n';
	}
}
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
CanvasPaint::CanvasPaint(uint64_t roomID, QWidget* parent) :
	QDialog{ parent },
	ui{ new Ui::CanvasPaint },
	drawState{ DrawingState::DRAWING },
	keepGoing{ true },
	roomID{ roomID },
	imageReceiver{ new ImageReceiver(roomID, keepGoing, this) }
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

	connect(imageReceiver, &ImageReceiver::LinesReceivedSignal, this, &CanvasPaint::HandleAddLines);
	connect(imageReceiver, &ImageReceiver::finished, imageReceiver, &QObject::deleteLater);
	imageReceiver->start();
}
#endif

CanvasPaint::~CanvasPaint()
{

#ifdef ONLINE
	keepGoing = false;
	imageReceiver->quit();
	imageReceiver->wait();
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

void CanvasPaint::closeEvent(QCloseEvent* event)
{
	QCoreApplication::quit();
	event->accept();
}
