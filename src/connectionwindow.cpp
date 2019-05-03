/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module:       connectionwindow.cpp
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

#include "connectionwindow.h"
#include "clickableimage.h"
#include "ui_connectionwindow.h"

#include <QPushButton>
#include <QSettings>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class IP4Validator : public QValidator {
public:
    IP4Validator(QObject *parent=0) : QValidator(parent){}

    void fixup(QString &input) const {}

    State validate(QString &input, int &pos) const
    {
        if(input.isEmpty()) return Acceptable;
        QStringList slist = input.split(".");
        int s = slist.size();
        if(s>4) return Invalid;
        bool emptyGroup = false;
        for(int i=0;i<s;i++){
            bool ok;
            if(slist[i].isEmpty())
            {
                emptyGroup = true;
                continue;
            }
            int val = slist[i].toInt(&ok);
            if(!ok || val<0 || val>255) return Invalid;
        }
        if(s<4 || emptyGroup) return Intermediate;
        return Acceptable;
    }
};

//void ConnectionWindow::addRecentServersActions()
//{
//    for (int i = 0; i < maxServerHistory; ++i) {
//        recentFileActs[i] = new QAction(this);
//        recentFileActs[i]->setVisible(false);
//        connect(recentFileActs[i], SIGNAL(triggered()),
//                this, SLOT(openRecentFile()));
//    }
//}

void ConnectionWindow::setRecentServer(const QString &ipPort)
{
    QSettings settings;
    QStringList servers = settings.value("recentServerList").toStringList();
    int maxServerHistorySize = settings.value("maxServerHistorySize", 10).toInt();
    servers.removeAll(ipPort);
    servers.prepend(ipPort);

    while (servers.size() > maxServerHistorySize)
        servers.removeLast();

    settings.setValue("recentServerList", servers);

    //foreach (QWidget *widget, QApplication::topLevelWidgets()) {
    //    MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
    //    if (mainWin)
    //        mainWin->updateRecentFileActions();
    //}
}

//void ConnectionWindow::useRecentServer()
//{
//    QAction *action = qobject_cast<QAction *>(sender());
//    if (action) {
//        openFile(action->data().toString());
//    }
//}

//void ConnectionWindow::addRecentServers()
//{
//    for (int i = 0; i < maxServerHistory; ++i)
//        ui->tableWidget->addAction(recentFileActs[i]);
//    updateRecentFileActions();
//}

//void ConnectionWindow::updateRecentServerActions()
//{
//    QSettings settings;
//    QStringList files = settings.value("recentFileList").toStringList();
//
//    int numRecentFiles = qMin(files.size(), (int)maxServerHistory);
//
//    for (int i = 0; i < numRecentFiles; ++i) {
//        QString text = tr("&%1:  %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
//        recentFileActs[i]->setText(text);
//        recentFileActs[i]->setData(files[i]);
//        recentFileActs[i]->setVisible(true);
//    }
//    for (int j = numRecentFiles; j < maxServerHistory; ++j)
//        recentFileActs[j]->setVisible(false);
//
//    //separatorAct->setVisible(numRecentFiles > 0);
//}

ConnectionWindow::ConnectionWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectionWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint)); //removes help (?) title bar button
    //this->setFixedSize(window()->sizeHint());

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Connect");

    ui->lineEdit_2->setValidator(new QIntValidator(1024,65535, this));
    ui->lineEdit_5->setValidator(new IP4Validator());

    connect(ui->tableWidget, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(useHistoryServer(int)));
    connect(ui->tableWidget_2, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(useFavoriteServer(int)));

    QStringList serverHistoryHeaderLabels;
    serverHistoryHeaderLabels << "" << "IP : Port";
    ui->tableWidget->setHorizontalHeaderLabels(serverHistoryHeaderLabels);

    QStringList serverFavoritesHeaderLabels;
    serverFavoritesHeaderLabels << "" << "IP : Port";
    ui->tableWidget_2->setHorizontalHeaderLabels(serverFavoritesHeaderLabels);

    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setColumnWidth(0,25);
    ui->tableWidget->setColumnWidth(1,130);

    ui->tableWidget_2->setColumnWidth(0,25);
    ui->tableWidget_2->setColumnWidth(1,130);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget_2->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->tableWidget_2->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_2->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->tableWidget_2->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //addRecentServersActions();
    //addRecentServers();

    lockOnPix = new QPixmap(":/res/img/lock_on_icon.png");
    lockOffPix = new QPixmap(":/res/img/lock_off_icon.png");
    starOnPix = new QPixmap(":/res/img/star.png");
    starOffPix = new QPixmap(":/res/img/star_up.png");

    portLock = new ClickableImage(this);
    portLock->resize(19, 20);
    portLock->move(144, 70);
    portLock->setToolTip("Unlock to change change the server port.");
    connect(portLock, SIGNAL(clicked(ClickableImage*)), this, SLOT(togglePortLock()));
}

ConnectionWindow::~ConnectionWindow()
{
    delete ui;
}

void ConnectionWindow::togglePortLock()
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

//void ConnectionWindow::updateFavoritesTable(const QString& ipPort, bool remove)
//{
//
//}

void ConnectionWindow::setFavoriteFromRow(int row)
{
    ui->tableWidget->selectRow(row);
    QString ipPort = ui->tableWidget->item(row, 1)->text();
    //std::cout << ipPort.toStdString().c_str() << std::endl;

    QSettings settings;
    QStringList favorites = settings.value("favoriteServerList").toStringList();

    if (favoriteStars.at(row)->getState())
    {
        //remove server from favorites
        favoriteStars.at(row)->setPixmap(*starOffPix);

        for (int i = 0; i < favorites.size(); i++) {
            if (QString::compare(favorites.at(i), ipPort) == 0) {
                //first, inform all favorites in list after this removal of upcoming row number change!!
                for (int j = i + 1; j < favorites.size(); j++) {
                    favoriteStars_Favorites.at(j)->updateRow(favoriteStars_Favorites.at(j)->getRow()-1);
                }
                //now safe to remove
                favoriteStars_Favorites.remove(i);
                ui->tableWidget_2->removeRow(i);
            }
        }
        favorites.removeAll(ipPort);
    }
    else
    {
        //add server to favorites

        favoriteStars.at(row)->setPixmap(*starOnPix);

        //check favorites list to prevent duplicate
        bool alreadyInFavorites = false;
        for (int i = 0; i < favorites.size(); i++)
        {
            if (QString::compare(favorites.at(i), ipPort) == 0)
                return;
        }

        favorites.append(ipPort);

        int rowToInsert = ui->tableWidget_2->rowCount();
        ui->tableWidget_2->insertRow(rowToInsert);
        FavoriteTableIcon* favoriteStar = new FavoriteTableIcon(rowToInsert, this);
        favoriteStars_Favorites.append(favoriteStar);
        favoriteStar->toggleState();
        favoriteStar->setPixmap(*starOnPix);
        favoriteStar->resize(15, 14);
        favoriteStar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableWidget_2->setCellWidget(rowToInsert, 0, favoriteStar);
        connect(favoriteStar, SIGNAL(cellClicked(int, int)), this, SLOT(setFavoriteFromRow_Favorites(int)));

        QTableWidgetItem *twi = new QTableWidgetItem;
        twi->setText(favorites.at(rowToInsert));
        ui->tableWidget_2->setItem(rowToInsert, 1, twi);
        ui->tableWidget_2->setRowHeight(rowToInsert, 21);



    }

    settings.setValue("favoriteServerList", favorites);
}

void ConnectionWindow::setFavoriteFromRow_Favorites(int row)
{
    ui->tableWidget_2->selectRow(row);
    QString ipPort = ui->tableWidget_2->item(row, 1)->text();

    QSettings settings;
    QStringList servers = settings.value("recentServerList").toStringList();
    QStringList favorites = settings.value("favoriteServerList").toStringList();

    if (favoriteStars_Favorites.at(row)->getState())
    {
        //remove server from favorites

        //first, inform all favorites in list after this removal of upcoming row number change!!
        for (int i = row + 1; i < favorites.size(); i++) {
            favoriteStars_Favorites.at(i)->updateRow(favoriteStars_Favorites.at(i)->getRow()-1);
        }
        //now safe to remove
        favoriteStars_Favorites.remove(row);
        ui->tableWidget_2->removeRow(row);

        for (int i = 0; i < servers.size(); i++) {
            if (QString::compare(servers.at(i), ipPort) == 0) {
                favoriteStars.at(i)->setPixmap(*starOffPix);
                favoriteStars.at(i)->toggleState();
            }
        }

        favorites.removeAll(ipPort);


    }
    else {
        //favoriteStars_Favorites.at(row)->setPixmap(*starOnPix);
    }

    settings.setValue("favoriteServerList", favorites);
}

void ConnectionWindow::useHistoryServer(int row)
{
    QSettings settings;
    QStringList servers = settings.value("recentServerList").toStringList();
    QString defaultServerPort = settings.value("serverPort", "39640").toString();

    if (servers.size() > 0)
    {
        QString ipPort = servers.at(row);
        QStringList ipPortStrList = ipPort.split(":");
        ui->lineEdit_5->setText(ipPortStrList.at(0));
        if (ipPortStrList.size() > 1) {
            ui->lineEdit_2->setText(ipPortStrList.at(1));
        }
        else {
            ui->lineEdit_2->setText(defaultServerPort);
        }
    }
    else {
        ui->lineEdit_5->setText("127.0.0.1");
        ui->lineEdit_2->setText(defaultServerPort);
    }
}

void ConnectionWindow::useFavoriteServer(int row)
{
    QSettings settings;
    QStringList favorites = settings.value("favoriteServerList").toStringList();
    QString defaultServerPort = settings.value("serverPort", "39640").toString();

    if (favorites.size() > 0)
    {
        QString ipPort = favorites.at(row);
        QStringList ipPortStrList = ipPort.split(":");
        ui->lineEdit_5->setText(ipPortStrList.at(0));
        if (ipPortStrList.size() > 1) {
            ui->lineEdit_2->setText(ipPortStrList.at(1));
        }
        else {
            ui->lineEdit_2->setText(defaultServerPort);
        }
    }
    else {
        ui->lineEdit_5->setText("127.0.0.1");
        ui->lineEdit_2->setText(defaultServerPort);
    }
}

void ConnectionWindow::showEvent( QShowEvent* event )
{
    QDialog::showEvent( event );

    QSettings settings;
    QString clientName = settings.value("clientName", "UnknownUser").toString();

    ui->lineEdit_3->setText(clientName);
    ui->lineEdit_4->clear();
    useHistoryServer(0);

    ui->lineEdit_2->setEnabled(false);
    portLock->setPixmap(*lockOffPix);

    QStringList servers = settings.value("recentServerList").toStringList();
    QStringList favorites = settings.value("favoriteServerList").toStringList();
    int maxServerHistorySize = settings.value("maxServerHistorySize", 10).toInt();
    int numRecentServers = qMin(servers.size(), maxServerHistorySize);

    //clear History contents
    int rowsToRemove = ui->tableWidget->rowCount();
    for (int i = 0; i < rowsToRemove; i++) {
        ui->tableWidget->removeRow(0);
    }
    favoriteStars.clear();

    //clear Favorite contents
    rowsToRemove = ui->tableWidget_2->rowCount();
    for (int i = 0; i < rowsToRemove; i++) {
        ui->tableWidget_2->removeRow(0);
    }
    favoriteStars_Favorites.clear();


    for (int row = 0; row < numRecentServers; row++) {
        ui->tableWidget->insertRow(row);

        FavoriteTableIcon* favoriteStar = new FavoriteTableIcon(row, this);
        favoriteStars.append(favoriteStar);
        favoriteStar->setPixmap(*starOffPix);
        favoriteStar->resize(15, 14);
        favoriteStar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        bool foundInFavorites = false;
        for (int i = 0; i < favorites.size(); i++) {
            if (QString::compare(servers.at(row), favorites.at(i)) == 0)
                foundInFavorites = true;
        }
        if (foundInFavorites) {
            favoriteStar->setPixmap(*starOnPix);
            favoriteStar->toggleState();
        }

        ui->tableWidget->setCellWidget(row, 0, favoriteStar);
        connect(favoriteStar, SIGNAL(cellClicked(int, int)), this, SLOT(setFavoriteFromRow(int)));

        QTableWidgetItem *twi = new QTableWidgetItem;
        twi->setText(servers.at(row));
        ui->tableWidget->setItem(row, 1, twi);
        ui->tableWidget->setRowHeight(row, 21);

    }

    for (int row = 0; row < favorites.size(); row++) {
        ui->tableWidget_2->insertRow(row);

        FavoriteTableIcon* favoriteStar = new FavoriteTableIcon(row, this);
        favoriteStars_Favorites.append(favoriteStar);
        favoriteStar->toggleState();
        favoriteStar->setPixmap(*starOnPix);
        favoriteStar->resize(15, 14);
        favoriteStar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableWidget_2->setCellWidget(row, 0, favoriteStar);
        connect(favoriteStar, SIGNAL(cellClicked(int, int)), this, SLOT(setFavoriteFromRow_Favorites(int)));

        for (int col = 1; col < 3; col++) {
            QTableWidgetItem *twi = new QTableWidgetItem;
            if (col==1) twi->setText(favorites.at(row));
            ui->tableWidget_2->setItem(row, col, twi);
            ui->tableWidget_2->setRowHeight(row, 21);
        }
    }
}

void ConnectionWindow::on_ConnectionWindow_accepted()
{
    serverConnectionAttempt.clientName = ui->lineEdit_3->text();
    serverConnectionAttempt.ip = ui->lineEdit_5->text();
    serverConnectionAttempt.port = ui->lineEdit_2->text();
    serverConnectionAttempt.password = ui->lineEdit_4->text();
}
