﻿#include "canvaspaint.h"
#include "mainwindow.h"
#include "ui_canvaspaint.h"
#include "choosewordwindow.h"

#include <QGuiApplication>
#include <QScreen>
#include <QObject>

#include "common.h"

#ifdef ONLINE
#include "services.h"
#endif


CanvasPaint::CanvasPaint(QWidget* parent) :
	QDialog{ parent },
	ui{ new Ui::CanvasPaint },
	m_drawState{ DrawingState::DRAWING }
{
	ui->setupUi(this);

	ui->roomLabel->setText("Room ID: none");
	ui->gameChatLabel->setStyleSheet("border: none;");
	ui->gameChat->setStyleSheet("QWidget { border: 1px solid black; }");

	QSize screenSize{ QGuiApplication::primaryScreen()->geometry().size() };
	setGeometry(0, 0, screenSize.width(), screenSize.height());
	setStyleSheet("QDialog { border: 2px solid black; }");

	canvasPixmap = QPixmap{ screenSize.width() * 3 / 4, screenSize.height() };
	canvasPixmap.fill(Qt::white);

	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

	roomLabel = new QLabel(this);
	roomLabel->setText(QString{ "Room ID: " } + QString::number(static_cast<qint64>(roomID)));
}

void CanvasPaint::setRoomID(uint64_t roomID)
{
	this->roomID = roomID;
	ui->roomLabel->setText(QString{ "Room ID: " } + QString::number(static_cast<qint64>(roomID)));
	ui->roomLabel->update();
}

#ifdef ONLINE
CanvasPaint::CanvasPaint(uint64_t roomID, const QString& username, QWidget* parent) :
	QDialog{ parent },
	ui{ new Ui::CanvasPaint },
	m_drawState{ DrawingState::DRAWING },
	m_keepGoing{ true },
	m_onlineData{ roomID, username },
	m_imageThread{ new ImageThread(roomID, m_keepGoing, this) },
	m_gameStateThread{ new GameStateThread(roomID, m_keepGoing, this) },
	m_chatThread{ new ChatThread(roomID, m_keepGoing, this) }
{
	ui->setupUi(this);

	ui->roomLabel->setText(QString{ "Room ID: " } + QString::number(static_cast<qint64>(roomID)));
	ui->roomLabel->update();
	ui->gameChatLabel->setStyleSheet("border: none;");
	ui->gameChat->setStyleSheet("QWidget { border: 1px solid black; }");

	QSize screenSize{ QGuiApplication::primaryScreen()->geometry().size() };
	setGeometry(0, 0, screenSize.width(), screenSize.height());
	setStyleSheet("QDialog { border: 2px solid black; }");

	canvasPixmap = QPixmap{ screenSize.width() * 3 / 4, screenSize.height() };
	canvasPixmap.fill(Qt::white);

	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

	connect(m_imageThread, &ImageThread::ImageSignal, this, &CanvasPaint::HandleImage);
	connect(m_imageThread, &ImageThread::finished, m_imageThread, &QObject::deleteLater);

	connect(m_gameStateThread, &GameStateThread::GameStateSignal, this, &CanvasPaint::HandleGameState);
	connect(m_gameStateThread, &GameStateThread::finished, m_gameStateThread, &QObject::deleteLater);

	connect(m_chatThread, &ChatThread::ChatSignal, this, &CanvasPaint::HandleChat);
	connect(m_chatThread, &ChatThread::finished, m_chatThread, &QObject::deleteLater);

	m_imageThread->start();
	m_gameStateThread->start();
	m_chatThread->start();

	m_imageThread->Pause();
	m_chatThread->Pause();
}
#endif

CanvasPaint::~CanvasPaint()
{
	delete ui;
}

void CanvasPaint::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setPen(kDRAWING_PEN);

	QRect canvasRect = rect();
	painter.drawRect(canvasRect);
	painter.drawPixmap(2, 2, canvasPixmap);
}

void CanvasPaint::mousePressEvent(QMouseEvent* event)
{

#ifdef ONLINE
	if (m_onlineData.playerRole != common::game::PlayerRole::DRAWING)
		return;
#endif

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

#ifdef ONLINE
	if (m_onlineData.playerRole != common::game::PlayerRole::DRAWING)
		return;
#endif

	if (!canvasPixmap.rect().contains(event->pos()))
		return;

	QPoint currentPos{ event->pos() };

	if (m_drawState == DrawingState::DRAWING)
	{
		QPainter painter{ &canvasPixmap };
		painter.setPen(kDRAWING_PEN);

		currentLine.points.emplace_back(currentPos);

		if (currentLine.points.size() > 1)
		{
			painter.drawLine(
				currentLine.points[currentLine.points.size() - 2],
				currentLine.points[currentLine.points.size() - 1]);
		}

		update();
	}

	else if (m_drawState == DrawingState::ERASING)
	{
		QPainter painter{ &canvasPixmap };
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.setPen(kERASING_PEN);

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

void CanvasPaint::mouseReleaseEvent(QMouseEvent* event)
{

#ifdef ONLINE
	if (m_onlineData.playerRole != common::game::PlayerRole::DRAWING)
		return;
#endif

	if (event->button() == Qt::LeftButton)
	{
		currentLine.drawState = m_drawState;

#ifdef ONLINE
		if (currentLine.points.size() > 1)
		{
			auto commonPoints{ std::move(currentLine.ToCommonPoints()) };
			commonPoints.emplace_back(common::img::Point{ -1, -1, Line::INVALID_COLOR_INT });
			services::SendImageUpdates(m_onlineData.roomID, commonPoints);
		}
#endif

		lines.emplace_back(std::move(currentLine));
	}
}

void CanvasPaint::resizeEvent(QResizeEvent* event)
{
	QPixmap newPixmap{ event->size().width() * 3 / 4, event->size().height() };
	newPixmap.fill(Qt::white);

	QPainter painter{ &newPixmap };
	painter.drawPixmap(QRect{ 0, 0, event->size().width() * 3 / 4, event->size().height() }, canvasPixmap);
	canvasPixmap = std::move(newPixmap);

	update();
}

void CanvasPaint::ClearCanvas()
{
	canvasPixmap.fill(Qt::white);
	lines.clear();
	update();
}

void CanvasPaint::on_resetCanvas_clicked()
{
	/*ClearCanvas();
	m_drawState = DrawingState::DRAWING;*/
}

void CanvasPaint::on_drawButton_clicked()
{
	m_drawState = DrawingState::DRAWING;
}

void CanvasPaint::on_eraseButton_clicked()
{
	m_drawState = DrawingState::ERASING;
}

void CanvasPaint::on_undoButton_clicked()
{
	/*if (!lines.isEmpty())
	{
		canvasPixmap.fill(Qt::white);
		QPainter painter{ &canvasPixmap };
		lines.pop_back();

		for (const auto& line : lines)
		{
			painter.setPen(line.drawState == DrawingState::DRAWING ? kDRAWING_PEN : kERASING_PEN);

			for (qsizetype i = 1; i < line.points.size(); i++)
				painter.drawLine(line.points[i - 1], line.points[i]);
		}

		update();
	}*/
}

void CanvasPaint::on_messageButton_clicked()
{
	QString formattedMessage{ QString{"[%1]: %2"}.arg(m_onlineData.username).arg(ui->messageBox->text()) };

	ui->gameChatLabel->setText(ui->gameChatLabel->text() + "\n" + formattedMessage);

#ifdef ONLINE
	services::SendNewMessage(m_onlineData.username.toStdString(),
		ui->messageBox->text().toStdString(), m_onlineData.roomID);
#endif

	ui->messageBox->clear();
}

void CanvasPaint::on_startGameButton_clicked()
{
	services::StartGame(m_onlineData.roomID);
	SetAllThreadsPauseStatus(false);
}

void CanvasPaint::closeEvent(QCloseEvent* event)
{

#ifdef ONLINE
	services::SignOut(m_onlineData.username.toStdString());
	m_keepGoing = false;

	m_imageThread->quit();
	m_gameStateThread->quit();
	m_chatThread->quit();

	m_imageThread->wait();
	m_gameStateThread->wait();
	m_chatThread->wait();

	delete m_imageThread;
	delete m_gameStateThread;
	delete m_chatThread;
#endif

	parentWidget()->show();
	event->accept();
}

#ifdef ONLINE
void CanvasPaint::HandleImage(QList<Line>* newLines)
{
	QPainter painter{ &canvasPixmap };

	for (auto& line : *newLines)
	{
		painter.setPen((line.drawState == DrawingState::DRAWING ? kDRAWING_PEN : kERASING_PEN));
		for (qsizetype i = 1; i < line.points.size(); i++)
			painter.drawLine(line.points[i - 1], line.points[i]);

		lines.emplace_back(std::move(line));
	}

	delete newLines;
	update();
}

void CanvasPaint::HandleGameState(const QPair<common::game::GameState, uint64_t>& gameStatePair)
{
	m_onlineData.gameState = gameStatePair.first;
	m_onlineData.playerRole = services::ReceivePlayerRole(m_onlineData.roomID, m_onlineData.username.toStdString());

	if (gameStatePair.second < UINT64_MAX)
		ui->timerLabel->setText(QString::number(gameStatePair.second));
	else
		ui->timerLabel->setText("");

	if (m_onlineData.gameState == common::game::GameState::PICK_WORD)
	{
		if (m_onlineData.playerRole != common::game::PlayerRole::DRAWING
			|| !m_onlineData.chosenWord.isEmpty())
		{
			return;
		}

		auto words{ services::ReceiveWordOptions(m_onlineData.roomID) };
		choosewordwindow* chooseWordWindow{ new choosewordwindow(this) };
		chooseWordWindow->setButtonNames(words);

		m_onlineData.chosenWord = "not empty";

		if (chooseWordWindow->exec() == QDialog::Accepted)
		{
			services::SendGuessingWord(m_onlineData.roomID, m_onlineData.chosenWord.toStdString());

			m_imageThread->Pause();
			m_chatThread->Unpause();
		}

		delete chooseWordWindow;
	}

	else if (m_onlineData.gameState == common::game::GameState::DRAW_AND_GUESS)
	{
		if (m_onlineData.playerRole == common::game::PlayerRole::DRAWING)
		{
			m_drawState = DrawingState::DRAWING;
		}
		else if (m_onlineData.playerRole == common::game::PlayerRole::GUESSING)
		{
			m_onlineData.chosenWord = "";
			m_imageThread->Unpause();
			m_chatThread->Unpause();
		}
	}

	else if (m_onlineData.gameState == common::game::GameState::NONE)
	{
		m_onlineData.chosenWord = "";
		m_imageThread->Pause();
		m_chatThread->Pause();
	}
}


void CanvasPaint::HandleChat(const QList<common::Message>& messages)
{
	for (const auto& message : messages)
	{
		qDebug() << "Received message: "
			<< QString::fromStdString(message.author) << " "
			<< QString::fromStdString(message.text);
	}
	for (const auto& message : messages)
	{
		ui->gameChatLabel->setText(ui->gameChatLabel->text() + "\n" +
			QString::fromStdString(message.author) +
			": " + QString::fromStdString(message.text));
	}
}

void CanvasPaint::SetAllThreadsPauseStatus(bool paused)
{
	if (paused == true)
	{
		m_imageThread->Pause();
		m_gameStateThread->Pause();
		m_chatThread->Pause();
	}
	else
	{
		m_imageThread->Unpause();
		m_gameStateThread->Unpause();
		m_chatThread->Unpause();
	}
}

void CanvasPaint::SetAllButtonsEnabled(bool enabled)
{
	ui->startGameButton->setEnabled(enabled);
	ui->resetCanvas->setEnabled(enabled);
	ui->drawButton->setEnabled(enabled);
	ui->eraseButton->setEnabled(enabled);
	ui->undoButton->setEnabled(enabled);
	ui->messageButton->setEnabled(enabled);
	ui->messageBox->setEnabled(enabled);
	ui->gameChat->setEnabled(enabled);
}

OnlineData& CanvasPaint::GetOnlineData()
{
	return m_onlineData;
}

void CanvasPaint::SetChosenWord(const QString& word)
{
	m_onlineData.chosenWord = word;
}
#endif
