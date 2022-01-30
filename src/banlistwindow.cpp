/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module:       banlistwindow.cpp
Author:       Cory Parks
Date started: 01/2022
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

#include "banlistwindow.h"
#include "ui_banlistwindow.h"

#include <QSettings>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

BanListWindow::BanListWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BanListWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint)); //removes help (?) title bar button

    QStringList banListHeaderLabels;
    banListHeaderLabels << "IP";
    ui->tableWidget->setHorizontalHeaderLabels(banListHeaderLabels);
    ui->tableWidget->setColumnWidth(0,185);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

BanListWindow::~BanListWindow()
{
    delete ui;
}

void BanListWindow::showEvent( QShowEvent* event )
{
    QDialog::showEvent( event );

    // Clear Ban List contents
    int rowsToRemove = ui->tableWidget->rowCount();
    for (int i = 0; i < rowsToRemove; i++) {
        ui->tableWidget->removeRow(0);
    }

    QSettings settings;
    banList = settings.value("banList").toStringList();

    for (int i=0; i<banList.size(); i++)
    {
        ui->tableWidget->insertRow(i);
        QTableWidgetItem *twi = new QTableWidgetItem();
        QFont f("Courier New");
        f.setStyleHint(QFont::Monospace);
        twi->setFont(f);
        twi->setText(banList.at(i));
        ui->tableWidget->setItem(i, 0, twi);
    }
}

void BanListWindow::on_pushButton_clicked()
{
    QModelIndexList selection = ui->tableWidget->selectionModel()->selectedRows();

    // Only one row should be selected
    if (selection.size() > 0)
    {
        QModelIndex index = selection.at(0);

        banList.removeAt(index.row());
        ui->tableWidget->removeRow(index.row());

        // Save changes to Ban List to settings
        QSettings settings;
        if (banList.size() == 0) {
            settings.remove("banList");
        }
        else {
            settings.setValue("banList", banList);
        }
    }
}


void BanListWindow::on_pushButton_2_clicked()
{
    // Clear Ban List contents
    int rowsToRemove = ui->tableWidget->rowCount();
    for (int i = 0; i < rowsToRemove; i++) {
        ui->tableWidget->removeRow(0);
    }

    banList.clear();

    // Save changes to Ban List to settings
    QSettings settings;
    settings.remove("banList");
}

