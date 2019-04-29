/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module:       settingswindow.cpp
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#else
#include <stdlib.h>
#include <pwd.h>
#include <stdio.h>
#endif

#include "settingswindow.h"
#include "ui_settingswindow.h"

#include <QSettings>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

QString getUserAccountName()
{
#ifdef _WIN32
    const char* username = getenv("USERNAME");
    return QString(username);
#else
    register struct passwd *pw;
    register uid_t uid;
    int c;

    uid = geteuid ();
    pw = getpwuid (uid);
    if (pw)
        return QString(pw->pw_name);
#endif
    return QString("UnknownUser");
}

SettingsWindow::SettingsWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint)); //removes help (?) title bar button


    ui->lineEdit_2->setValidator(new QIntValidator(1024,65535, this));
    ui->lineEdit_4->setValidator(new QIntValidator(1,999999999, this));

    //set initial client name to registry
    QSettings settings;
    QString clientName = settings.value("clientName", getUserAccountName()).toString();
    settings.setValue("clientName", clientName);
}

void SettingsWindow::showEvent( QShowEvent* event )
{
    QDialog::showEvent( event );

    QSettings settings;
    bool startListenerOnStartup = settings.value("startListenerOnStartup", true).toBool();
    QString clientName = settings.value("clientName", getUserAccountName()).toString();
    QString serverPort = settings.value("serverPort", "39640").toString();
    int maxServerHistorySize = settings.value("maxServerHistorySize", 10).toInt();
    QString timeoutTimeMS = settings.value("timeoutTimeMS", "10000").toString();
    int maxClients = settings.value("maxClients", 8).toInt();

    ui->checkBox->setChecked(startListenerOnStartup);
    ui->lineEdit->setText(clientName);
    ui->spinBox->setValue(maxServerHistorySize);
    ui->lineEdit_2->setText(serverPort);
    ui->lineEdit_4->setText(timeoutTimeMS);
    ui->spinBox_2->setValue(maxClients);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::on_SettingsWindow_accepted()
{
    QSettings settings;
    settings.setValue("clientName", ui->lineEdit->text());
    settings.setValue("serverPort", ui->lineEdit_2->text());
    settings.setValue("startListenerOnStartup", ui->checkBox->isChecked());
    settings.setValue("maxServerHistorySize", ui->spinBox->value());
    settings.setValue("timeoutTimeMS", ui->lineEdit_4->text());
    settings.setValue("maxClients", ui->spinBox_2->value());
}
