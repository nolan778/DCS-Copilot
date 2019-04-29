/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module:       aboutwindow.cpp
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

#include "aboutwindow.h"
#include "ui_aboutwindow.h"

#include "version.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

AboutWindow::AboutWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint)); //removes help (?) title bar button

    QString date = QStringLiteral("%1-%2-%3").arg(Version::YEAR).arg(Version::MONTH, 2, 10, QChar('0')).arg(Version::DAY, 2, 10, QChar('0'));
    QString version = QStringLiteral("%1.%2.%3.%4").arg(Version::MAJOR).arg(Version::MINOR).arg(Version::REVISION).arg(Version::BUILD);
    QString aboutText = "DCS Copilot\nBuild:  "+date+"  (v"+version+QString(")\nCopyright (c) %1 Cory Parks\nThis is a free software").arg(Version::YEAR);
    ui->label->setText(aboutText);
}

AboutWindow::~AboutWindow()
{
    delete ui;
}
