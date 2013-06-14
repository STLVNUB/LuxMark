 /***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#include <QTextEdit>
#include <QGraphicsSceneMouseEvent>
#include <QDialogButtonBox>
#include <QDateTime>
#include <QTextStream>
#include <QGraphicsItem>

#include "slg/renderconfig.h"

#include "mainwindow.h"
#include "aboutdialog.h"
#include "luxmarkdefs.h"
#include "luxmarkapp.h"
#include "luxmarkdefs.h"

MainWindow *LogWindow = NULL;

int EVT_LUX_LOG_MESSAGE = QEvent::registerEventType();
int EVT_LUX_ERR_MESSAGE = QEvent::registerEventType();

LuxLogEvent::LuxLogEvent(QString msg) : QEvent((QEvent::Type)EVT_LUX_LOG_MESSAGE), message(msg) {
	setAccepted(false);
}

LuxErrorEvent::LuxErrorEvent(QString msg) : QEvent((QEvent::Type)EVT_LUX_ERR_MESSAGE), message(msg) {
	setAccepted(false);
}

static void LuxRenderErrorHandler(int code, int severity, const char *msg) {
	switch (severity) {
		case LUX_ERROR:
		case LUX_SEVERE:
			LM_LOG_LUX_ERROR(msg);
			break;
		case LUX_WARNING:
			LM_LOG_LUX_WARNING(msg);
			break;
		default:
		case LUX_DEBUG:
		case LUX_INFO:
			LM_LOG_LUX(msg);
			break;
	}
}

//------------------------------------------------------------------------------

LuxFrameBuffer::LuxFrameBuffer(const QPixmap &pixmap) : QGraphicsPixmapItem(pixmap) {
	luxApp = NULL;

	setAcceptHoverEvents(true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);
}

//------------------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags) : QMainWindow(parent, flags),
		ui(new Ui::MainWindow) {
	ui->setupUi(this);

	// Setup rendering area
	renderScene = new QGraphicsScene();
	renderScene->setBackgroundBrush(QColor(127,127,127));
	luxLogo = renderScene->addPixmap(QPixmap(":/images/resources/luxlogo_bg.png"));
	luxFrameBuffer = new LuxFrameBuffer(QPixmap(":/images/resources/luxlogo_bg.png"));
	renderScene->addItem(luxFrameBuffer);

	authorLabelBack = new QGraphicsSimpleTextItem(QString("Scene designed by LuxRender project"));
	renderScene->addItem(authorLabelBack);
	authorLabelBack->setBrush(Qt::black);
	authorLabel = new QGraphicsSimpleTextItem(authorLabelBack->text());
	authorLabel->setBrush(Qt::blue);
	renderScene->addItem(authorLabel);

	screenLabelBack = renderScene->addRect(0.f, 0.f, 1.f, 1.f, QPen(), Qt::lightGray);
	screenLabel = new QGraphicsSimpleTextItem(QString("[Time: 0secs (Wait)]"));
	renderScene->addItem(screenLabel);

	ui->RenderView->setScene(renderScene);

	frameBuffer = NULL;
	fbWidth = 0;
	fbHeight = 0;

	// Setup status bar
	statusbarLabel = new QLabel(ui->statusbar);
	statusbarLabel->setText("");
	ui->statusbar->addWidget(statusbarLabel);

	// Disable the Complex benchmark if we are running with 32bit address space
	if (sizeof(size_t) < 8) {
		ui->action_Room->setDisabled(true);
		ui->action_Room->setText(QString("Complex benchmark not available on 32bit platform"));
	}

	ShowLogo();

	luxErrorHandler(&::LuxRenderErrorHandler);
}

MainWindow::~MainWindow() {
	delete ui;
	delete statusbarLabel;
	delete authorLabel;
	delete authorLabelBack;
	delete screenLabel;
	delete screenLabelBack;
	delete luxFrameBuffer;
	delete luxLogo;
	delete renderScene;
	delete frameBuffer;

}

//------------------------------------------------------------------------------
// Qt Slots
//------------------------------------------------------------------------------

void MainWindow::exitApp() {
	// Win32 refuses to exit without the following line
	((LuxMarkApp *)qApp)->Stop();

	exit(0);
}

void MainWindow::showAbout() {
	AboutDialog *dialog = new AboutDialog();

	dialog->exec();
}

void MainWindow::setLuxBallScene() {
	LM_LOG("Set LuxBall scene");
	authorLabelBack->setText(QString("Scene designed by LuxRender project"));
	authorLabel->setText(authorLabelBack->text());
	((LuxMarkApp *)qApp)->SetScene(SCENE_LUXBALL);
}

void MainWindow::setLuxBallHDRScene() {
	LM_LOG("Set LuxBall HDR scene");
	authorLabelBack->setText(QString("Scene designed by LuxRender project"));
	authorLabel->setText(authorLabelBack->text());
	((LuxMarkApp *)qApp)->SetScene(SCENE_LUXBALL_HDR);
}

void MainWindow::setLuxBallSkyScene() {
	LM_LOG("Set LuxBall Sky scene");
	authorLabelBack->setText(QString("Scene designed by LuxRender project"));
	authorLabel->setText(authorLabelBack->text());
	((LuxMarkApp *)qApp)->SetScene(SCENE_LUXBALL_SKY);
}

void MainWindow::setRoomScene() {
	LM_LOG("Set Room scene");
	authorLabelBack->setText(QString("Scene modeling/texturing by Mourelas Konstantinos \"Moure\" (http://moure-portfolio.blogspot.com/) for LuxMark"));
	authorLabel->setText(authorLabelBack->text());
	((LuxMarkApp *)qApp)->SetScene(SCENE_ROOM);
}

void MainWindow::setSalaScene() {
	LM_LOG("Set Sala scene");
	authorLabelBack->setText(QString("Scene designed by Daniel \"ZanQdo\" Salazar (http://www.3developer.com)\nand adapted for SLG by Michael \"neo2068\" Klemm"));
	authorLabel->setText(authorLabelBack->text());
	((LuxMarkApp *)qApp)->SetScene(SCENE_SALA);
}

void MainWindow::setMode_BENCHMARK_NOSPECTRAL_OCL_GPU() {
	LM_LOG("Set mode: BENCHMARK_NOSPECTRAL_OCL_GPU");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_NOSPECTRAL_OCL_GPU);
}

void MainWindow::setMode_BENCHMARK_NOSPECTRAL_OCL_CPUGPU() {
	LM_LOG("Set mode: BENCHMARK_NOSPECTRAL_OCL_CPUGPU");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_NOSPECTRAL_OCL_CPUGPU);
}

void MainWindow::setMode_BENCHMARK_NOSPECTRAL_OCL_CPU() {
	LM_LOG("Set mode: BENCHMARK_NOSPECTRAL_OCL_CPU");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_NOSPECTRAL_OCL_CPU);
}

void MainWindow::setMode_BENCHMARK_NOSPECTRAL_OCL_CUSTOM() {
	LM_LOG("Set mode: BENCHMARK_NOSPECTRAL_OCL_CUSTOM");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_NOSPECTRAL_OCL_CUSTOM);
}

void MainWindow::setMode_BENCHMARK_NOSPECTRAL_HYBRID_GPU() {
	LM_LOG("Set mode: BENCHMARK_NOSPECTRAL_HYBRID_GPU");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_NOSPECTRAL_HYBRID_GPU);
}

void MainWindow::setMode_BENCHMARK_NOSPECTRAL_HYBRID_CUSTOM() {
	LM_LOG("Set mode: BENCHMARK_NOSPECTRAL_HYBRID_CUSTOM");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_NOSPECTRAL_HYBRID_CUSTOM);
}

void MainWindow::setMode_BENCHMARK_NOSPECTRAL_NATIVE_PATH() {
	LM_LOG("Set mode: BENCHMARK_SPECTRAL_NATIVE_PATH");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_NOSPECTRAL_NATIVE_PATH);
}

void MainWindow::setMode_BENCHMARK_SPECTRAL_HYBRID_GPU() {
	LM_LOG("Set mode: BENCHMARK_SPECTRAL_HYBRID_GPU");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_SPECTRAL_HYBRID_GPU);
}

void MainWindow::setMode_BENCHMARK_SPECTRAL_HYBRID_CUSTOM() {
	LM_LOG("Set mode: BENCHMARK_SPECTRAL_HYBRID_CUSTOM");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_SPECTRAL_HYBRID_CUSTOM);
}

void MainWindow::setMode_BENCHMARK_SPECTRAL_NATIVE_PATH() {
	LM_LOG("Set mode: BENCHMARK_SPECTRAL_NATIVE_PATH");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_SPECTRAL_NATIVE_PATH);
}

void MainWindow::setMode_BENCHMARK_SPECTRAL_NATIVE_BIDIR() {
	LM_LOG("Set mode: BENCHMARK_SPECTRAL_NATIVE_BIDIR");
	((LuxMarkApp *)qApp)->SetMode(BENCHMARK_SPECTRAL_NATIVE_BIDIR);
}

void MainWindow::setMode_STRESSTEST_NOSPECTRAL_OCL_GPU() {
	LM_LOG("Set mode: STRESSTEST_NOSPECTRAL_OCL_GPU");
	((LuxMarkApp *)qApp)->SetMode(STRESSTEST_NOSPECTRAL_OCL_GPU);
}

void MainWindow::setMode_STRESSTEST_NOSPECTRAL_OCL_CPUGPU() {
	LM_LOG("Set mode: STRESSTEST_NOSPECTRAL_OCL_CPUGPU");
	((LuxMarkApp *)qApp)->SetMode(STRESSTEST_NOSPECTRAL_OCL_CPUGPU);
}

void MainWindow::setMode_STRESSTEST_NOSPECTRAL_OCL_CPU() {
	LM_LOG("Set mode: STRESSTEST_NOSPECTRAL_OCL_CPU");
	((LuxMarkApp *)qApp)->SetMode(STRESSTEST_NOSPECTRAL_OCL_CPU);
}

void MainWindow::setMode_STRESSTEST_SPECTRAL_NATIVE_BIDIR() {
	LM_LOG("Set mode: STRESSTEST_SPECTRAL_NATIVE_BIDIR");
	((LuxMarkApp *)qApp)->SetMode(STRESSTEST_SPECTRAL_NATIVE_BIDIR);
}

void MainWindow::setMode_DEMO_LUXVR() {
	LM_LOG("Set mode: DEMO_LUXVR");
	((LuxMarkApp *)qApp)->SetMode(DEMO_LUXVR);
}

void MainWindow::setMode_PAUSE() {
	LM_LOG("Set Pause mode");
	((LuxMarkApp *)qApp)->SetMode(PAUSE);
}

//------------------------------------------------------------------------------

void MainWindow::Pause() {
	setMode_PAUSE();
	SetModeCheck(PAUSE);
}

void MainWindow::ShowLogo() {
	if (luxFrameBuffer->isVisible()) {
		luxFrameBuffer->hide();
		authorLabel->hide();
		authorLabelBack->hide();
		screenLabelBack->hide();
		screenLabel->hide();
	}

	if (!luxLogo->isVisible()) {
		luxLogo->show();
		renderScene->setSceneRect(0.f, 0.f, 416.f, 389.f);
		ui->RenderView->centerOn(luxLogo);
	}

	statusbarLabel->setText("");
	ui->RenderView->setInteractive(false);
}

void MainWindow::SetModeCheck(const LuxMarkAppMode mode) {
	ui->action_NoSpectral_OpenCL_GPUs->setChecked(mode == BENCHMARK_NOSPECTRAL_OCL_GPU);
	ui->action_NoSpectral_OpenCL_CPUs_GPUs->setChecked(mode == BENCHMARK_NOSPECTRAL_OCL_CPUGPU);
	ui->action_NoSpectral_OpenCL_CPUs->setChecked(mode == BENCHMARK_NOSPECTRAL_OCL_CPU);
	ui->action_NoSpectral_OpenCL_Custom->setChecked(mode == BENCHMARK_NOSPECTRAL_OCL_CUSTOM);
	ui->action_NoSpectral_Hybrid_GPUs->setChecked(mode == BENCHMARK_NOSPECTRAL_HYBRID_GPU);
	ui->action_NoSpectral_Hybrid_Custom->setChecked(mode == BENCHMARK_NOSPECTRAL_HYBRID_CUSTOM);
	ui->action_NoSpectral_Path->setChecked(mode == BENCHMARK_NOSPECTRAL_NATIVE_PATH);

	ui->action_Spectral_Hybrid_GPUs->setChecked(mode == BENCHMARK_SPECTRAL_HYBRID_GPU);
	ui->action_Spectral_Hybrid_Custom->setChecked(mode == BENCHMARK_SPECTRAL_HYBRID_CUSTOM);
	ui->action_Spectral_Path->setChecked(mode == BENCHMARK_SPECTRAL_NATIVE_PATH);

	ui->action_Spectral_BiDir->setChecked(mode == BENCHMARK_SPECTRAL_NATIVE_BIDIR);

	ui->action_StressTest_NoSpectral_OpenCL_GPUs->setChecked(mode == STRESSTEST_NOSPECTRAL_OCL_GPU);
	ui->action_StressTest_NoSpectral_OpenCL_CPUs_GPUs->setChecked(mode == STRESSTEST_NOSPECTRAL_OCL_CPUGPU);
	ui->action_StressTest_NoSpectral_OpenCL_CPUs->setChecked(mode == STRESSTEST_NOSPECTRAL_OCL_CPU);
	ui->action_StressTest_Spectral_BiDir->setChecked(mode == STRESSTEST_SPECTRAL_NATIVE_BIDIR);

	ui->action_LuxVR->setChecked(mode == DEMO_LUXVR);

	ui->action_Pause->setChecked(mode == PAUSE);
}

void MainWindow::SetSceneCheck(const int index) {
	if (index == 0) {
		ui->action_Room->setChecked(true);
		ui->action_Sala->setChecked(false);
		ui->action_LuxBall_HDR->setChecked(false);
		ui->action_LuxBall->setChecked(false);
		ui->action_LuxBall_Sky->setChecked(false);
	} else if (index == 1) {
		ui->action_Room->setChecked(false);
		ui->action_Sala->setChecked(true);
		ui->action_LuxBall_HDR->setChecked(false);
		ui->action_LuxBall->setChecked(false);
		ui->action_LuxBall_Sky->setChecked(false);
	} else if (index == 2) {
		ui->action_Room->setChecked(false);
		ui->action_Sala->setChecked(false);
		ui->action_LuxBall_HDR->setChecked(true);
		ui->action_LuxBall->setChecked(false);
		ui->action_LuxBall_Sky->setChecked(false);
	} else if (index == 3) {
		ui->action_Room->setChecked(false);
		ui->action_Sala->setChecked(false);
		ui->action_LuxBall_HDR->setChecked(false);
		ui->action_LuxBall->setChecked(true);
		ui->action_LuxBall_Sky->setChecked(false);
	} else if (index == 4) {
		ui->action_Room->setChecked(false);
		ui->action_Sala->setChecked(false);
		ui->action_LuxBall_HDR->setChecked(false);
		ui->action_LuxBall->setChecked(false);
		ui->action_LuxBall_Sky->setChecked(true);
	} else
		assert(false);

}

bool MainWindow::IsShowingLogo() const {
	return luxLogo->isVisible();
}

void MainWindow::ShowFrameBuffer(const unsigned char *frameBufferSrc,
		const unsigned  int width, const unsigned  int height) {
	if (luxLogo->isVisible())
		luxLogo->hide();

	// Check if I have to allocate a new frame buffer
	if (!frameBuffer || (width != fbWidth) || (height != fbHeight)) {
		delete frameBuffer;
		fbWidth = width;
		fbHeight = height;
		frameBuffer = new unsigned char[fbWidth * fbHeight * 3];
	}

	memcpy(frameBuffer, frameBufferSrc, width * height * 3);

	luxFrameBuffer->setPixmap(QPixmap::fromImage(QImage(
		frameBuffer, fbWidth, fbHeight, width * 3, QImage::Format_RGB888)));

	if (!luxFrameBuffer->isVisible()) {
		luxFrameBuffer->show();
		authorLabelBack->show();
		authorLabel->show();

		qreal w = Max<qreal>(fbWidth, screenLabel->boundingRect().width());
		qreal h = fbHeight + screenLabel->boundingRect().height();
		renderScene->setSceneRect(0.f, 0.f, w, h);
		luxFrameBuffer->setPos(Max<qreal>(0.f, (w - fbWidth) / 2), 0.f);
		ui->RenderView->centerOn(luxFrameBuffer);

		screenLabelBack->setRect(0.f, fbHeight, w, screenLabel->boundingRect().height());
		screenLabelBack->show();
		screenLabel->show();
	}

	ui->RenderView->setDragMode(QGraphicsView::ScrollHandDrag);
	ui->RenderView->setInteractive(true);

	//LM_LOG("Screen updated");
}

void MainWindow::SetHardwareTreeModel(HardwareTreeModel *treeModel) {
	if (!ui->HardwareView->model()) {
		ui->HardwareView->setModel(treeModel);
		ui->HardwareView->expandAll();
	}
}

bool MainWindow::event(QEvent *event) {
	bool retval = FALSE;
	int eventtype = event->type();

	// Check if it's one of "our" events
	if (eventtype == EVT_LUX_LOG_MESSAGE) {
		QString buf;
		QTextStream ss(&buf);
		ss << QDateTime::currentDateTime().toString(tr("yyyy-MM-dd hh:mm:ss")) << " - " <<
				((LuxLogEvent *)event)->getMessage();

		ui->LogView->append(ss.readAll());
	} else if (eventtype == EVT_LUX_ERR_MESSAGE) {
		QString buf;
		QTextStream ss(&buf);
		ss << "<FONT COLOR=\"#ff0000\">" << QDateTime::currentDateTime().toString(tr("yyyy-MM-dd hh:mm:ss")) << " - " <<
				((LuxLogEvent *)event)->getMessage() << "</FONT>";

		ui->LogView->append(ss.readAll());

		((LuxMarkApp *)qApp)->SetMode(PAUSE);

		// If I'm in single-run mode, I have to exit
		if ((((LuxMarkApp *)qApp))->IsSingleRun()) {
			cout << "Error: " << ((LuxLogEvent *)event)->getMessage().toStdString() << endl;
			exit(EXIT_SUCCESS);
		}
	}

	if (retval) {
		// Was our event, stop the event propagation
		event->accept();
	}

	return QMainWindow::event(event);
}

void MainWindow::UpdateScreenLabel(const char *msg, const bool valid) {
	screenLabel->setText(QString(msg));
	//screenLabel->setBrush(valid ? Qt::green : Qt::red);
	screenLabel->setBrush(Qt::blue);
	screenLabel->setPos(0.f, fbHeight);

	// Update scene size
	const qreal w = Max<qreal>(fbWidth, screenLabel->boundingRect().width());
	const qreal h = fbHeight + screenLabel->boundingRect().height();
	renderScene->setSceneRect(0.f, 0.f, w, h);
	luxFrameBuffer->setPos(Max<qreal>(0.f, (w - fbWidth) / 2), 0.f);
	screenLabelBack->setRect(0.f, fbHeight, w, screenLabel->boundingRect().height());

	// Update author label
	const qreal alh = Max<qreal>(0.f, (w - fbWidth) / 2);
	authorLabelBack->setPos(alh + 1.f, 1.f);
	authorLabel->setPos(alh, 0.f);

	// Update status bar with the first line of the message
	QString qMsg(msg);
	statusbarLabel->setText(qMsg.split(QString("\n"))[0]);
}
