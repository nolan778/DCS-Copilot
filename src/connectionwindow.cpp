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
#include <QMenu>

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

ConnectionWindow::ConnectionWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectionWindow),
    contextMenuRowAction(-1)
{
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint)); //removes help (?) title bar button
    //this->setFixedSize(window()->sizeHint());

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Connect");

    ui->lineEdit_2->setValidator(new QIntValidator(1024,65535, this));
    ui->lineEdit_5->setValidator(new IP4Validator());
    QFont f("Courier New");
    f.setStyleHint(QFont::Monospace);
    ui->lineEdit_5->setFont(f);

    connect(ui->tableWidget, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(useHistoryServer(int)));
    connect(ui->tableWidget_2, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(useFavoriteServer(int)));

    QStringList serverHistoryHeaderLabels;
    serverHistoryHeaderLabels << "" << "IP : Port" << "Favorite Name";
    ui->tableWidget->setHorizontalHeaderLabels(serverHistoryHeaderLabels);

    QStringList serverFavoritesHeaderLabels;
    serverFavoritesHeaderLabels << "" << "IP : Port" << "Favorite Name";
    ui->tableWidget_2->setHorizontalHeaderLabels(serverFavoritesHeaderLabels);

    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->tableWidget->setColumnWidth(0,25);
    ui->tableWidget->setColumnWidth(1,153);
    ui->tableWidget->setColumnWidth(2,180);

    ui->tableWidget_2->setColumnWidth(0,25);
    ui->tableWidget_2->setColumnWidth(1,153);
    ui->tableWidget_2->setColumnWidth(2,180);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget_2->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget_2->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->tableWidget_2->setSelectionMode(QAbstractItemView::SingleSelection);

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

// Capture single click of the Port Lock icon
void ConnectionWindow::togglePortLock()
{
    // Port Lock is Off
    if (ui->lineEdit_2->isEnabled())
    {
        // Turn Port Lock On
        ui->lineEdit_2->setEnabled(false);
        portLock->setPixmap(*lockOffPix);
        portLock->setToolTip("Unlock to change change the server port.");
    }
    // Port Lock is On
    else
    {
        // Turn Port Lock Off
        ui->lineEdit_2->setEnabled(true);
        portLock->setPixmap(*lockOnPix);
        portLock->setToolTip("Lock to prevent port change.");
    }
}

// Update recent server list with the given "ip:port" server
void ConnectionWindow::setRecentServer(const QString &ipPort)
{
    QSettings settings;
    QStringList servers = settings.value("recentServerList").toStringList();
    int maxServerHistorySize = settings.value("maxServerHistorySize", 10).toInt();

    // Remove any instance of this server in the recent server list already
    servers.removeAll(ipPort);
    // Add or re-add the server to the beginning of the recent server list
    servers.prepend(ipPort);

    // If the server list grows larger than the max size, remove servers until the max size is reached.
    while (servers.size() > maxServerHistorySize) {
        servers.removeLast();
    }

    // Update settings for recent server list changes
    settings.setValue("recentServerList", servers);
}

// Capture single click of a Favorite Star Icon from the History tab server list
// Will remove the server on this row from favorites list if it is already a favorite
// Will add the server on this row to the favorites list if it is not already a favorite
void ConnectionWindow::setFavoriteFromRow(int row)
{
    ui->tableWidget->selectRow(row);
    QString ipPort = ui->tableWidget->item(row, 1)->text();

    QSettings settings;
    QStringList favorites = settings.value("favoriteServerList").toStringList();
    QStringList favoriteNames = settings.value("favoriteServerNamesList").toStringList();

    // Server is already a favorite
    if (favoriteStars.at(row)->getState())
    {
        // Remove server from favorites

        // Turn off the favorite star for this server's row in the History tab server list
        favoriteStars.at(row)->setPixmap(*starOffPix);

        // Find its index in favorites list
        int favoriteToRemove = -1;
        for (int i = 0; i < favorites.size(); i++)
        {
            // Found favorite
            if (QString::compare(favorites.at(i), ipPort) == 0)
            {
                // First, inform all favorites in favorite stars list after this removal of upcoming row number change!!
                for (int j = i + 1; j < favorites.size(); j++) {
                    favoriteStars_Favorites.at(j)->updateRow(favoriteStars_Favorites.at(j)->getRow()-1);
                }
                // Now safe to remove favorite from favorite stars list
                favoriteStars_Favorites.remove(i);
                // Remove this favorite row from the Favorites tab server list
                ui->tableWidget_2->removeRow(i);
                favoriteToRemove = i;
                break;
            }
        }
        // Remove favorite (and any favorite name given) from settings
        favorites.removeAll(ipPort);
        if (favoriteToRemove >= 0 && favoriteToRemove < favoriteNames.size())
        {
            favoriteNames.removeAt(favoriteToRemove);
        }

        // Reset the text on the favorite name column to empty
        auto model = ui->tableWidget->model();
        model->setData(model->index(row, 2),QStringLiteral(""));
    }
    // Server is not already a favorite
    else
    {
        // Add server to favorites

        // Turn on the favorite star for this server's row in the History tab server list
        favoriteStars.at(row)->setPixmap(*starOnPix);

        // Check existing favorites list to prevent a duplicate from being added
        for (int i = 0; i < favorites.size(); i++)
        {
            if (QString::compare(favorites.at(i), ipPort) == 0)
                return;
        }

        // Add server (and empty favorite name) to favorites list
        favorites.append(ipPort);
        favoriteNames.append(" ");

        // Add row to Favorites tab server list
        int rowToInsert = ui->tableWidget_2->rowCount();
        ui->tableWidget_2->insertRow(rowToInsert);

        // Add favorite star icon for this favorites tab server list row (in column 0), turn it on, and connect a single cell click signal to a slot
        FavoriteTableIcon* favoriteStar = new FavoriteTableIcon(rowToInsert, this);
        favoriteStars_Favorites.append(favoriteStar);
        favoriteStar->toggleState();
        favoriteStar->setPixmap(*starOnPix);
        favoriteStar->resize(15, 14);
        favoriteStar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableWidget_2->setCellWidget(rowToInsert, 0, favoriteStar);
        connect(favoriteStar, SIGNAL(cellClicked(int, int)), this, SLOT(setFavoriteFromRow_Favorites(int)));

        // Add Table Widget Items to columns 1 and 2 to this new row
        // Column 1 is non-editable ip:port
        QTableWidgetItem *twi = new QTableWidgetItem("non editable");
        QFont f("Courier New");
        f.setStyleHint(QFont::Monospace);
        twi->setFont(f);
        twi->setText(favorites.at(rowToInsert));
        twi->setFlags(twi->flags() & ~Qt::ItemIsEditable); // non editable
        ui->tableWidget_2->setItem(rowToInsert, 1, twi);
        // Column 2 is editable favorite server name
        QTableWidgetItem *twi2 = new QTableWidgetItem("editable");
        twi2->setText(favoriteNames.at(rowToInsert));
        ui->tableWidget_2->setItem(rowToInsert, 2, twi2);

        ui->tableWidget_2->setRowHeight(rowToInsert, 21);
    }

    // Update settings for favorites changes
    settings.setValue("favoriteServerList", favorites);
    settings.setValue("favoriteServerNamesList", favoriteNames);
}

// Capture single click of a Favorite Star Icon from the Favorites tab server list
// Will always remove the server on this row from favorites list
void ConnectionWindow::setFavoriteFromRow_Favorites(int row)
{
    ui->tableWidget_2->selectRow(row);
    QString ipPort = ui->tableWidget_2->item(row, 1)->text();

    QSettings settings;
    QStringList servers = settings.value("recentServerList").toStringList();
    QStringList favorites = settings.value("favoriteServerList").toStringList();
    QStringList favoriteNames = settings.value("favoriteServerNamesList").toStringList();

    // Server is already a favorite (should always be true)
    if (favoriteStars_Favorites.at(row)->getState())
    {
        // Remove server from favorites

        // First, inform all favorites in favorite stars list after this removal of upcoming row number change!!
        for (int i = row + 1; i < favorites.size(); i++) {
            favoriteStars_Favorites.at(i)->updateRow(favoriteStars_Favorites.at(i)->getRow()-1);
        }
        // Now safe to remove favorite from favorite stars list
        favoriteStars_Favorites.remove(row);
        // Remove this favorite row from the Favorites tab server list
        ui->tableWidget_2->removeRow(row);

        // Find the row of this favorite server in the History tab server list
        for (int i = 0; i < servers.size(); i++)
        {
            // Server found
            if (QString::compare(servers.at(i), ipPort) == 0)
            {
                // Turn off the favorite star
                favoriteStars.at(i)->setPixmap(*starOffPix);
                favoriteStars.at(i)->toggleState();

                // Reset the text on the favorite name column to empty for this server's row in the History tab server list
                auto model = ui->tableWidget->model();
                model->setData(model->index(i, 2),QStringLiteral(""));
                break;
            }
        }

        // Remove favorite (and any favorite name given) from settings
        favorites.removeAll(ipPort);
        if (row < favoriteNames.size())
        {
            favoriteNames.removeAt(row);
        }
    }

    // Update settings for favorites changes
    settings.setValue("favoriteServerList", favorites);
    settings.setValue("favoriteServerNamesList", favoriteNames);
}

// Capture double click of a server row from the History tab server list (except Favorite Name column, which is editable)
// Fill in the connection information (ip:port) with server info from this history server row.
void ConnectionWindow::useHistoryServer(int row)
{
    QSettings settings;
    QStringList servers = settings.value("recentServerList").toStringList();
    QString defaultServerPort = settings.value("serverPort", "39640").toString();

    if (servers.size() > 0 && row < servers.size())
    {
        QString ipPort = servers.at(row);
        QStringList ipPortStrList = ipPort.split(":"); // "ip:port" -> ["ip", "port"]
        ui->lineEdit_5->setText(ipPortStrList.at(0));
        if (ipPortStrList.size() > 1) {
            ui->lineEdit_2->setText(ipPortStrList.at(1));
        }
        else {
            ui->lineEdit_2->setText(defaultServerPort);
        }
    }
    else
    {
        ui->lineEdit_5->setText("127.0.0.1");
        ui->lineEdit_2->setText(defaultServerPort);
    }
}

// Capture double click of a server row from the Favorites tab server list (except Favorite Name column, which is editable)
// Fill in the connection information (ip:port) with server info from this favorites server row.
void ConnectionWindow::useFavoriteServer(int row)
{
    QSettings settings;
    QStringList favorites = settings.value("favoriteServerList").toStringList();
    QString defaultServerPort = settings.value("serverPort", "39640").toString();

    if (favorites.size() > 0 && row < favorites.size())
    {
        QString ipPort = favorites.at(row);
        QStringList ipPortStrList = ipPort.split(":"); // "ip:port" -> ["ip", "port"]
        ui->lineEdit_5->setText(ipPortStrList.at(0));
        if (ipPortStrList.size() > 1) {
            ui->lineEdit_2->setText(ipPortStrList.at(1));
        }
        else {
            ui->lineEdit_2->setText(defaultServerPort);
        }
    }
    else
    {
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
    QStringList favoriteNames = settings.value("favoriteServerNamesList").toStringList();
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


    for (int row = 0; row < numRecentServers; row++)
    {
        ui->tableWidget->insertRow(row);

        FavoriteTableIcon* favoriteStar = new FavoriteTableIcon(row, this);
        favoriteStars.append(favoriteStar);
        favoriteStar->setPixmap(*starOffPix);
        favoriteStar->resize(15, 14);
        favoriteStar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        int favoriteIndex = -1;
        for (int i = 0; i < favorites.size(); i++)
        {
            if (QString::compare(servers.at(row), favorites.at(i)) == 0)
            {
                favoriteIndex = i;
                break;
            }
        }
        if (favoriteIndex >= 0)
        {
            favoriteStar->setPixmap(*starOnPix);
            favoriteStar->toggleState();
            if (favoriteIndex < favoriteNames.size())
            {
                QTableWidgetItem *twi2 = new QTableWidgetItem("editable");
                twi2->setText(favoriteNames.at(favoriteIndex));
                ui->tableWidget->setItem(row, 2, twi2);
            }
        }

        ui->tableWidget->setCellWidget(row, 0, favoriteStar);
        connect(favoriteStar, SIGNAL(cellClicked(int, int)), this, SLOT(setFavoriteFromRow(int)));

        QTableWidgetItem *twi = new QTableWidgetItem("non editable");
        QFont f("Courier New");
        f.setStyleHint(QFont::Monospace);
        twi->setFont(f);
        twi->setText(servers.at(row));
        twi->setFlags(twi->flags() & ~Qt::ItemIsEditable); // non editable
        ui->tableWidget->setItem(row, 1, twi);
        ui->tableWidget->setRowHeight(row, 21);
    }

    for (int row = 0; row < favorites.size(); row++)
    {
        ui->tableWidget_2->insertRow(row);

        FavoriteTableIcon* favoriteStar = new FavoriteTableIcon(row, this);
        favoriteStars_Favorites.append(favoriteStar);
        favoriteStar->toggleState();
        favoriteStar->setPixmap(*starOnPix);
        favoriteStar->resize(15, 14);
        favoriteStar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableWidget_2->setCellWidget(row, 0, favoriteStar);
        connect(favoriteStar, SIGNAL(cellClicked(int, int)), this, SLOT(setFavoriteFromRow_Favorites(int)));

        for (int col = 1; col < 3; col++)
        {
            QTableWidgetItem *twi = 0;
            if (col==1)
            {
                twi = new QTableWidgetItem("non editable");
                twi->setFlags(twi->flags() & ~Qt::ItemIsEditable); // non editable
                QFont f("Courier New");
                f.setStyleHint(QFont::Monospace);
                twi->setFont(f);
                twi->setText(favorites.at(row));
            }
            else if (col==2)
            {
                twi = new QTableWidgetItem("editable");
                if (row >= favoriteNames.size())
                {
                    favoriteNames.append(" ");
                    settings.setValue("favoriteServerNamesList", favoriteNames);
                }
                twi->setText(favoriteNames.at(row));
            }
            ui->tableWidget_2->setItem(row, col, twi);
            ui->tableWidget_2->setRowHeight(row, 21);
        }
    }
}

// Capture connection information after Connection Window closes from pressing "Connect" (normally "OK")
void ConnectionWindow::on_ConnectionWindow_accepted()
{
    serverConnectionAttempt.clientName = ui->lineEdit_3->text();
    serverConnectionAttempt.ip = ui->lineEdit_5->text();
    serverConnectionAttempt.port = ui->lineEdit_2->text();
    serverConnectionAttempt.password = ui->lineEdit_4->text();
}

// Capture single cell clicks on the History tab server list
void ConnectionWindow::on_tableWidget_cellClicked(int row, int col)
{
    // Clicked cell is in Favorite Name column
    if (col == 2 && row >= 0)
    {
        QSettings settings;
        QStringList servers = settings.value("recentServerList").toStringList();
        QStringList favorites = settings.value("favoriteServerList").toStringList();

        // Determine if server on row clicked is actually a favorite and if so, find its index
        int favoriteIndex = -1;
        for (int i = 0; i < favorites.size(); i++)
        {
            if (QString::compare(servers.at(row), favorites.at(i)) == 0)
            {
                favoriteIndex = i;
                break;
            }
        }

        // Server is a favorite
        if (favoriteIndex >= 0)
        {
            // Force entrance to edit mode after a single click
            ui->tableWidget->edit(ui->tableWidget->currentIndex());
        }
    }
}

// Capture single cell clicks on the Favorites tab server list
void ConnectionWindow::on_tableWidget_2_cellClicked(int row, int col)
{
    // Clicked cell is in Favorite Name column
    if (col == 2 && row >= 0)
    {
        // Force entrance to edit mode after a single click
        ui->tableWidget_2->edit(ui->tableWidget_2->currentIndex());
    }
}

// Capture cell edit changes on the History tab server list
void ConnectionWindow::on_tableWidget_cellChanged(int row, int col)
{
    // Changed cell is in Favorite Name column
    if (col == 2 && row >= 0)
    {
        QSettings settings;
        QStringList servers = settings.value("recentServerList").toStringList();
        QStringList favorites = settings.value("favoriteServerList").toStringList();

        // Determine if server name changed is actually a favorite and if so, find its index
        int favoriteIndex = -1;
        for (int i = 0; i < favorites.size(); i++)
        {
            if (QString::compare(servers.at(row), favorites.at(i)) == 0)
            {
                favoriteIndex = i;
                break;
            }
        }

        // Server is a favorite
        if (favoriteIndex >= 0)
        {
            QSettings settings;
            QStringList favoriteNames = settings.value("favoriteServerNamesList").toStringList();

            if (favoriteIndex < favoriteNames.size())
            {
                QString newFavoriteName = ui->tableWidget->item(row, col)->text();
                // Prevent empty string names
                if (newFavoriteName.length() == 0) {
                    newFavoriteName = " ";
                    ui->tableWidget->item(row, col)->setText(newFavoriteName);
                }

                // Update server favorite name in settings and carry over the name change to the History tab server list
                favoriteNames[favoriteIndex] = newFavoriteName;

                auto model = ui->tableWidget_2->model();
                model->setData(model->index(favoriteIndex, 2), newFavoriteName);

                settings.setValue("favoriteServerNamesList", favoriteNames);
            }
        }
        else
        {
            // Not a favorite, erase changes to cell text
            auto model = ui->tableWidget->model();
            model->setData(model->index(row, 2), QString(""));
        }
    }
}

// Capture cell edit changes on the Favorites tab server list
void ConnectionWindow::on_tableWidget_2_cellChanged(int row, int col)
{
    // Changed cell is in Favorite Name column
    if (col == 2 && row >= 0)
    {
        QSettings settings;
        QStringList favoriteNames = settings.value("favoriteServerNamesList").toStringList();

        if (row < favoriteNames.size())
        {
            QStringList servers = settings.value("recentServerList").toStringList();
            QStringList favorites = settings.value("favoriteServerList").toStringList();
            if (row < favorites.size())
            {
                QString ipPort = favorites.at(row);

                QString newFavoriteName = ui->tableWidget_2->item(row, col)->text();
                // Prevent empty string names
                if (newFavoriteName.length() == 0) {
                    newFavoriteName = " ";
                    ui->tableWidget_2->item(row, col)->setText(newFavoriteName);
                }

                // Update server favorite name in settings and carry over the name change to the Favorites tab server list
                favoriteNames[row] = newFavoriteName;

                for (int i = 0; i < servers.size(); i++)
                {
                    if (QString::compare(servers.at(i), ipPort) == 0)
                    {
                        auto model = ui->tableWidget->model();
                        model->setData(model->index(i, 2), newFavoriteName);
                        break;
                    }
                }

                settings.setValue("favoriteServerNamesList", favoriteNames);
            }
        }
    }
}

void ConnectionWindow::removeServerFromHistory()
{
    if (contextMenuRowAction >= 0 && contextMenuRowAction < ui->tableWidget->rowCount())
    {
        QSettings settings;
        QStringList servers = settings.value("recentServerList").toStringList();
        servers.removeAt(contextMenuRowAction);

        ui->tableWidget->removeRow(contextMenuRowAction);

        // Save changes to History to settings
        if (servers.size() == 0) {
            settings.remove("recentServerList");
        }
        else {
            settings.setValue("recentServerList", servers);
        }
    }
    contextMenuRowAction = -1;
}

void ConnectionWindow::clearHistory()
{
    // Clear History Server List contents
    int rowsToRemove = ui->tableWidget->rowCount();
    for (int i = 0; i < rowsToRemove; i++) {
        ui->tableWidget->removeRow(0);
    }

    // Save changes to History to settings
    QSettings settings;
    settings.remove("recentServerList");

    contextMenuRowAction = -1;
}
void ConnectionWindow::on_tableWidget_customContextMenuRequested(const QPoint &pos)
{
    int row = ui->tableWidget->indexAt(pos).row();
    if (row >= 0)
    {
        contextMenuRowAction = row;

        QMenu contextMenu;

        QAction action1("Remove from History", this);
        connect(&action1, SIGNAL(triggered()), this, SLOT(removeServerFromHistory()));
        contextMenu.addAction(&action1);
        contextMenu.addSeparator();

        QAction action2("Clear History", this);
        connect(&action2, SIGNAL(triggered()), this, SLOT(clearHistory()));
        contextMenu.addAction(&action2);

        contextMenu.exec(QCursor::pos());
    }
    else
    {
        QMenu contextMenu;

        QAction action2("Clear History", this);
        connect(&action2, SIGNAL(triggered()), this, SLOT(clearHistory()));
        contextMenu.addAction(&action2);

        contextMenu.exec(QCursor::pos());
    }
}

