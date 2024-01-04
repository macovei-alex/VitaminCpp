﻿#include "canvaspaint.h"
#include "mainwindow.h"
#include "ui_canvaspaint.h"
#include <QGuiApplication>
#include <QScreen>

CanvasPaint::CanvasPaint(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::CanvasPaint),
	drawState{ DrawState::DRAWING }
{
	ui->setupUi(this);

	ui->gameChatLabel->setStyleSheet("border: none;");
	ui->gameChat->setStyleSheet("QWidget { border: 1px solid black; }");

	QScreen* primaryScreen = QGuiApplication::primaryScreen();
	QRect screenRect = primaryScreen->geometry();
	setGeometry(0, 0, screenRect.width(), screenRect.height());
	setStyleSheet("QDialog { border: 2px solid black; }");

	canvasPixmap = QPixmap(screenRect.size()); // Ajustează dimensiunile după necesități
	canvasPixmap.fill(Qt::white); // Umple canvas-ul cu culoarea albă

	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
	//  connect(ui->minimize, SIGNAL(clicked()), this, SLOT(minimizeButtonClicked()));
	  //connect(ui->minimize, &QPushButton::clicked, this, &CanvasPaint::minimizeButtonClicked);
	  /*  connect(ui->minimize, SIGNAL(clicked()), this, SLOT(minimizeButtonClicked()));
	  connect(ui->minimize, &QPushButton::clicked, this, &CanvasPaint::minimizeButtonClicked);*/

	connect(ui->messageButton, &QPushButton::clicked, this, &CanvasPaint::on_messageButton_clicked);
}

CanvasPaint::~CanvasPaint()
{
	delete ui;
}

void CanvasPaint::setDrawState(DrawState state)
{
	drawState = state;
}

void CanvasPaint::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	QPen pen(Qt::black, 2, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin);
	painter.setPen(pen);
	QRect canvasRect = rect();
	painter.drawRect(canvasRect);
	painter.drawPixmap(2, 2, canvasPixmap);
}

void CanvasPaint::on_button_clicked()
{
	QPainter painter(&canvasPixmap);
	painter.drawLine(10, 10, 100, 100);

	// Actualizează afișarea
	update();

}

// Actualizează metoda mousePressEvent
void CanvasPaint::mousePressEvent(QMouseEvent* event)
{
	// Verifică dacă evenimentul este un clic al mouse-ului
	if (event->button() == Qt::LeftButton)
	{
		if (event->x() < width() * 3 / 4)
		{
			lastPoint = event->pos();
		}
	}
}

// Adaugă o nouă metodă pentru gestionarea mișcării mouse-ului
void CanvasPaint::mouseMoveEvent(QMouseEvent* event)
{
	if (event->x() < width() * 3 / 4)
	{
		QPoint currentPoint = event->pos();

		if (drawState == DrawState::DRAWING)
		{
			QPainter painter(&canvasPixmap);
			painter.setPen(Qt::black);
			currentLine.isDrawing = true;
			currentLine.points.append(currentPoint);

			for (int i = 1; i < currentLine.points.size(); ++i)
			{
				painter.drawLine(currentLine.points[i - 1], currentLine.points[i]);
			}

			update();
		}

		else if (drawState == DrawState::ERASING)
		{
			QPainter painter(&canvasPixmap);
			painter.setCompositionMode(QPainter::CompositionMode_Source);
			painter.setPen(QPen(Qt::white, 20, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

			currentLine.isDrawing = true;
			currentLine.points.append(currentPoint);

			for (int i = 1; i < currentLine.points.size(); ++i)
			{
				painter.drawLine(currentLine.points[i - 1], currentLine.points[i]);
			}

			update();
		}
	}
}

// Adaugă o nouă metodă pentru gestionarea eliberării butonului mouse-ului
void CanvasPaint::mouseReleaseEvent(QMouseEvent* event)
{
	// Verifică dacă butonul stâng al mouse-ului este eliberat
	if (event->button() == Qt::LeftButton)
	{
		if (drawState == DrawState::DRAWING)
		{
			drawnLines.append(currentLine);
			currentLine.points.clear();
		}
	}
}

void CanvasPaint::resizeEvent(QResizeEvent* event)
{
	// Creează o nouă imagine cu dimensiunile actualizate
	QPixmap newPixmap(event->size());
	newPixmap.fill(Qt::white);
	QPainter painter(&newPixmap);
	painter.drawPixmap(QRect(0, 0, event->size().width(), event->size().height()), canvasPixmap);
	canvasPixmap = newPixmap;

	// Actualizează afișarea
	update();
}

void CanvasPaint::clearCanvas()
{
	// sterge ce s-a desenat
	canvasPixmap.fill(Qt::white);
	update();
}

void CanvasPaint::on_leaveServerButton_clicked()
{
	hide();
	obiect = new MainWindow(this);
	obiect->show();
}

void CanvasPaint::on_resetCanvas_clicked()
{
	clearCanvas();
	drawState = DrawState::DRAWING;
}

//void CanvasPaint::minimizeButtonClicked()
//{
//    qDebug() << "Minimize button clicked";
//	showMinimized();
//}

void CanvasPaint::on_drawButton_clicked()
{
	drawingOrErasing = true;
	drawState = DrawState::DRAWING;
}

void CanvasPaint::on_eraseButton_clicked()
{
	drawingOrErasing = false;
	drawState = DrawState::ERASING;
}

void CanvasPaint::on_undoButton_clicked()
{
	if (!drawnLines.isEmpty())
	{
		drawnLines.pop_back();
		// clearCanvas();
		QPixmap newPixmap(size());
		newPixmap.fill(Qt::white);


		QPainter painter(&newPixmap);
		for (const auto& line : drawnLines)
		{
			for (int i = 1; i < line.points.size(); ++i) {
				if (line.isDrawing)
					painter.setPen(Qt::black);
				else
					painter.setPen(QPen(Qt::white, 20, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

				painter.drawLine(line.points[i - 1], line.points[i]);
			}

		}
		canvasPixmap = newPixmap;

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
