/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module:       mainwindow.cpp
Author:       Cory Parks
Date started: 10/2016
Purpose:

See LICENSE file for copyright and license information

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "NetworkLocal.h"
#include "Network.h"
#include "NetworkTypes.h"

#include <QCloseEvent>
#include <QTimer>
#include <QLabel>
#include <QDateTime>
#include <QSettings>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

Network::NetworkLocal* netLocal = nullptr;
Network::Network* net = nullptr;

QTimer* netLocalTimer = nullptr;
QTimer* netTimer = nullptr;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    netLocalTimer = new QTimer(this);
    netTimer = new QTimer(this);

    netLocalTimer->setInterval(1);
    netTimer->setInterval(10);

    connect(netLocalTimer, SIGNAL(timeout()), this, SLOT(updateLocalNetwork()));
    connect(netTimer, SIGNAL(timeout()), this, SLOT(updateNetwork()));


    //setup widgets
    QLabel* Listener_label = new QLabel("Listener:");
    QLabel* DCS_label = new QLabel("DCS:");
    QLabel* Server_label = new QLabel("Server:");

    Listener_status_label = new QLabel("NOT RUNNING");
    DCS_status_label = new QLabel("NOT RUNNING");
    Server_status_label = new QLabel("NOT CONNECTED");

    QFont status_label_font("Arial", 10, QFont::Bold);

    Listener_label->setFont(status_label_font);
    DCS_label->setFont(status_label_font);
    Server_label->setFont(status_label_font);

    Listener_label->setAlignment(Qt::AlignRight);
    DCS_label->setAlignment(Qt::AlignRight);
    Server_label->setAlignment(Qt::AlignRight);

    Listener_status_label->setFont(status_label_font);
    DCS_status_label->setFont(status_label_font);
    Server_status_label->setFont(status_label_font);

    int statusbar_qframe_style = QFrame::Panel | QFrame::Sunken;
    Listener_label->setFrameStyle(statusbar_qframe_style);
    DCS_label->setFrameStyle(statusbar_qframe_style);
    Server_label->setFrameStyle(statusbar_qframe_style);
    Listener_status_label->setFrameStyle(statusbar_qframe_style);
    DCS_status_label->setFrameStyle(statusbar_qframe_style);
    Server_status_label->setFrameStyle(statusbar_qframe_style);

    statusBar()->addPermanentWidget(Listener_label, 1);
    statusBar()->addPermanentWidget(Listener_status_label, 2);

    statusBar()->addPermanentWidget(DCS_label, 1);
    statusBar()->addPermanentWidget(DCS_status_label, 2);

    statusBar()->addPermanentWidget(Server_label,1);
    statusBar()->addPermanentWidget(Server_status_label,2);

    changeLabelColor(Listener_status_label,"darkred");
    changeLabelColor(DCS_status_label,"darkred");
    changeLabelColor(Server_status_label,"darkred");

    QStringList clientListHeaderLabels;
    clientListHeaderLabels << "Client Name" << "Seat" << "Ping";
    ui->tableWidget->setHorizontalHeaderLabels(clientListHeaderLabels);
    ui->tableWidget->setColumnWidth(0,170);
    ui->tableWidget->setColumnWidth(1,82);
    ui->tableWidget->setColumnWidth(2,82);
    ui->tableWidget->setColumnHidden(3, true);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    QPalette p = ui->textEdit->palette();
    p.setColor(QPalette::Base, QColor(240, 240, 240));
    ui->textEdit->setPalette(p);
    //ui->textEdit->setTextColor(QColor(120,120,120));
    //ui->textEdit->setEnabled(false);

    //Settings window
    settingsWindow = new SettingsWindow();

    //Server Start window
    serverStart = new ServerStart();
    connect(serverStart, SIGNAL(accepted()), this, SLOT(startServer()));

    //Connection window
    connectionWindow = new ConnectionWindow();
    connect(connectionWindow, SIGNAL(accepted()), this, SLOT(connectToServer()));

    //About window
    aboutWindow = new AboutWindow();

    //
    netLocal = new Network::NetworkLocal(this);
    net = new Network::Network(this);

    connect(netLocal, SIGNAL(localConnected(void)), net, SLOT(handleLocalConnected(void)));
    connect(netLocal, SIGNAL(receivedLocalCommand(int)), net, SLOT(handleReceivedLocalCommand(int)));
    connect(netLocal, SIGNAL(receivedLocalCommandValue(int,float,bool)), net, SLOT(handleReceivedLocalCommandValue(int,float,bool)));

    connect(net, SIGNAL(receivedSeatChange(int)), netLocal, SLOT(handleReceivedSeatChange(int)));
    connect(net, SIGNAL(receivedNetCommand(int)), netLocal, SLOT(handleReceivedNetCommand(int)));
    connect(net, SIGNAL(receivedNetCommandValue(int,float,bool)), netLocal, SLOT(handleReceivedNetCommandValue(int,float,bool)));

    updateListenerStatus(false);
    updateDCSStatus(false);
    updateServerStatus(Network::SS_NOT_CONNECTED);

    QSettings settings;
    bool startListenerOnStartup = settings.value("startListenerOnStartup", true).toBool();
    if (startListenerOnStartup)
        startLocalServer();
    //addClient("ClientTest1");
    //addClient("ClientTest2");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addClient(const QString& id, const QString& clientName, unsigned char seatNumber)
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setRowHeight(row, 21);

    QTableWidgetItem *twi = new QTableWidgetItem;
    QTableWidgetItem *twi2 = new QTableWidgetItem;
    QTableWidgetItem *twi3 = new QTableWidgetItem;
    QTableWidgetItem *twi4 = new QTableWidgetItem;

    twi->setText(clientName);
    twi2->setText(QString::number(seatNumber));
    twi3->setText("0");
    twi4->setText(id);

    ui->tableWidget->setItem(row, 0, twi);
    ui->tableWidget->setItem(row, 1, twi2);
    ui->tableWidget->setItem(row, 2, twi3);
    ui->tableWidget->setItem(row, 3, twi4);
}

void MainWindow::setSeat(const QString& id, unsigned char seatNumber)
{
    int rows = ui->tableWidget->rowCount();
    for (int i=0; i < rows; i++)
    {
        if (ui->tableWidget->item(i, 3)->text() == id)
        {
            ui->tableWidget->item(i, 1)->setText(QString::number(seatNumber));
            break;
        }
    }
}


void MainWindow::setPing(const QString& id, int ping)
{
    int rows = ui->tableWidget->rowCount();
    for (int i=0; i < rows; i++)
    {
        if (ui->tableWidget->item(i, 3)->text() == id)
        {
            ui->tableWidget->item(i, 2)->setText(QString::number(ping));
            break;
        }
    }
}

void MainWindow::setMaxSeats(unsigned char seatNumber)
{
    ui->spinBox->setMaximum(seatNumber);
}

void MainWindow::removeClient(const QString& id)
{
    int rows = ui->tableWidget->rowCount();
    for (int i=0; i < rows; i++)
    {
        if (ui->tableWidget->item(i, 3)->text() == id)
        {
            ui->tableWidget->removeRow(i);
            break;
        }
    }
}

void MainWindow::clearClients()
{
    ui->tableWidget->setRowCount(0);
}

void MainWindow::changeLabelColor(QLabel* label, const QString& color)
{
    //label->setText(QString("<font color='%1'>%2</font>").arg(color, label->text()));
    QString styleString = QString("QLabel { color : %1; }").arg(color);
    label->setStyleSheet(styleString);
}

void MainWindow::logMessage(const QString& logMsg)
{
    QString nextLine = QDateTime::currentDateTime().toString("hh:mm:ss") + "   " + logMsg;
    ui->textEdit->append(nextLine);
}

void MainWindow::updateListenerStatus(bool running)
{
    if (running) {
        Listener_status_label->setText("RUNNING");
        changeLabelColor(Listener_status_label, "green");
        findChild<QAction*>("actionStart_Listener")->setEnabled(false);
        findChild<QAction*>("actionStop_Listener")->setEnabled(true);
    } else {
        Listener_status_label->setText("NOT RUNNING");
        changeLabelColor(Listener_status_label, "darkred");
        findChild<QAction*>("actionStart_Listener")->setEnabled(true);
        findChild<QAction*>("actionStop_Listener")->setEnabled(false);
    }
}

void MainWindow::updateDCSStatus(bool running)
{
    if (running) {
        DCS_status_label->setText("RUNNING");
        changeLabelColor(DCS_status_label, "green");
    } else {
        DCS_status_label->setText("NOT RUNNING");
        changeLabelColor(DCS_status_label, "darkred");
    }
}

void MainWindow::updateServerStatus(int status)
{
    if (status == Network::SS_IS_CONNECTING)
    {
        Server_status_label->setText("CONNECTING");
        changeLabelColor(Server_status_label, "orange");
        findChild<QAction*>("actionStart_Server")->setEnabled(false);
        findChild<QAction*>("actionConnect")->setEnabled(false);
        findChild<QAction*>("actionDisconnect")->setEnabled(false);
    }
    else if (status == Network::SS_NOT_CONNECTED)
    {
        netTimer->stop();
        Server_status_label->setText("NOT CONNECTED");
        changeLabelColor(Server_status_label, "darkred");
        findChild<QAction*>("actionStart_Server")->setEnabled(true);
        findChild<QAction*>("actionConnect")->setEnabled(true);
        findChild<QAction*>("actionDisconnect")->setEnabled(false);
    }
    else if (status == Network::SS_CONNECTED)
    {
        Server_status_label->setText("CONNECTED");
        changeLabelColor(Server_status_label, "green");
        findChild<QAction*>("actionStart_Server")->setEnabled(false);
        findChild<QAction*>("actionConnect")->setEnabled(false);
        findChild<QAction*>("actionDisconnect")->setEnabled(true);
    }
    else if (status == Network::SS_HOSTING)
    {
        Server_status_label->setText("HOSTING");
        changeLabelColor(Server_status_label, "green");
        findChild<QAction*>("actionStart_Server")->setEnabled(false);
        findChild<QAction*>("actionConnect")->setEnabled(false);
        findChild<QAction*>("actionDisconnect")->setEnabled(true);
    }
}

void MainWindow::startLocalServer()
{
    netLocal->startServer();
    netLocalTimer->start();
}

void MainWindow::stopLocalServer()
{
    netLocalTimer->stop();
    netLocal->disconnect();
}

void MainWindow::startServer()
{;
    ServerStartAttempt attempt = serverStart->getServerStartAttempt();
    net->setTimeoutTimeMS(attempt.timeoutTimeMS.toInt());
    net->setMaxClients(attempt.maxClients);
    net->startServer(attempt.port.toUShort(), attempt.clientName.toStdString(), attempt.password.toStdString());
    netTimer->start();
}

void MainWindow::connectToServer(/*const QString& clientName, const QString& ip, const QString& port, const QString& password*/)
{
    ServerConnectionAttempt attempt = connectionWindow->getServerConnectionAttempt();    
    net->connect(attempt.ip.toStdString().c_str(), attempt.port.toUShort(), attempt.clientName.toStdString(), attempt.password.toStdString());
    connectionWindow->setRecentServer(attempt.ip+":"+attempt.port);
    netTimer->start();

}

void MainWindow::stopServer()
{
    netTimer->stop();
    net->disconnect();
}

void MainWindow::updateLocalNetwork()
{
    netLocal->update();
}

void MainWindow::updateNetwork()
{
    net->update();
}

void MainWindow::closeProgram()
{
    stopLocalServer();
    stopServer();
    QTimer::singleShot( 1000, qApp, SLOT(quit()) );
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    closeProgram();
}

void MainWindow::on_actionExit_triggered()
{
    closeProgram();
}

void MainWindow::on_actionStart_Listener_triggered()
{
    startLocalServer();
}

void MainWindow::on_actionStop_Listener_triggered()
{
    stopLocalServer();
}

void MainWindow::on_actionSettings_triggered()
{
    if (settingsWindow->isHidden())
        settingsWindow->show();
    else {
        settingsWindow->activateWindow();
    }
}

void MainWindow::on_actionStart_Server_triggered()
{
    if (serverStart->isHidden())
        serverStart->show();
    else {
        serverStart->activateWindow();
    }
}

void MainWindow::on_actionConnect_triggered()
{
    if (connectionWindow->isHidden())
        connectionWindow->show();
    else {
        connectionWindow->activateWindow();
    }
}

void MainWindow::on_actionDisconnect_triggered()
{
    stopServer();
}

void MainWindow::on_actionAbout_triggered()
{
    if (aboutWindow->isHidden())
        aboutWindow->show();
    else {
        aboutWindow->activateWindow();
    }
}

void MainWindow::on_pushButton_clicked()
{
    int seatNumber = ui->spinBox->value();
    net->requestSeat(seatNumber);
}

void MainWindow::on_pushButton_2_clicked()
{
    int command = ui->spinBox_2->value();
    net->handleReceivedLocalCommand(command);
}

void MainWindow::on_pushButton_3_clicked()
{
    int command = ui->spinBox_2->value();
    float value = ui->doubleSpinBox->value();
    net->handleReceivedLocalCommandValue(command, value, true);
}
