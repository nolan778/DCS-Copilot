/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module:       serverstart.cpp
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

#include "serverstart.h"
#include "ui_serverstart.h"
#include "clickableimage.h"

#include <QPushButton>
#include <QSettings>
#include <QPixmap>
#include <QResource>
#include <QIntValidator>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

ServerStart::ServerStart(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ServerStart)
{
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint)); //removes help (?) title bar button

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Start");

    ui->lineEdit_2->setValidator(new QIntValidator(1024,65535, this));

    lockOnPix = new QPixmap(":/res/img/lock_on_icon.png");
    lockOffPix = new QPixmap(":/res/img/lock_off_icon.png");

    portLock = new ClickableImage(this);
    portLock->resize(19, 20);
    portLock->move(144, 40);
    portLock->setToolTip("Unlock to change change the server port.");
    connect(portLock, SIGNAL(clicked(ClickableImage*)), this, SLOT(togglePortLock()));
}

ServerStart::~ServerStart()
{
    delete ui;
}

void ServerStart::togglePortLock()
{
    if (ui->lineEdit_2->isEnabled()) {
        ui->lineEdit_2->setEnabled(false);
        portLock->setPixmap(*lockOffPix);
        portLock->setToolTip("Unlock to change change the server port.");
    }
    else {
        ui->lineEdit_2->setEnabled(true);
        portLock->setPixmap(*lockOnPix);
        portLock->setToolTip("Lock to prevent port change.");
    }
}

void ServerStart::showEvent( QShowEvent* event )
{
    QDialog::showEvent( event );

    QSettings settings;
    QString clientName = settings.value("clientName", "UnknownUser").toString();
    QString serverPort = settings.value("serverPort", "39640").toString();

    ui->lineEdit_3->setText(clientName);
    ui->lineEdit_2->setText(serverPort);
    ui->lineEdit_4->clear();


    ui->lineEdit_2->setEnabled(false);
    portLock->setPixmap(*lockOffPix);
}

void ServerStart::on_ServerStart_accepted()
{
    serverStartAttempt.clientName = ui->lineEdit_3->text();
    serverStartAttempt.port = ui->lineEdit_2->text();
    serverStartAttempt.password = ui->lineEdit_4->text();

    QSettings settings;
    QString timeoutTimeMS = settings.value("timeoutTimeMS", "10000").toString();
    int maxClients = settings.value("maxClients", 8).toInt();

    serverStartAttempt.timeoutTimeMS = timeoutTimeMS;
    serverStartAttempt.maxClients = maxClients;
}
