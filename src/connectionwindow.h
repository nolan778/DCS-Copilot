/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header:       connectionwindow.h
Author:       Cory Parks
Date started: 10/2016

See LICENSE file for copyright and license information

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef CONNECTIONWINDOW_H
#define CONNECTIONWINDOW_H


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <QDialog>
#include <QVector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/



/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/



/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/



/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace Ui {
class ConnectionWindow;
}

class ClickableImage;
class FavoriteTableIcon;

struct ServerConnectionAttempt
{
    QString clientName = "DefaultClientName";
    QString ip = "127.0.0.1";
    QString port = "39640";
    QString password;
};

class ConnectionWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionWindow(QWidget *parent = 0);
    ~ConnectionWindow();

    void showEvent( QShowEvent* event );
    void setRecentServer(const QString &ipPort);

    ServerConnectionAttempt getServerConnectionAttempt() { return serverConnectionAttempt; }

private slots:
    void on_ConnectionWindow_accepted();
    void togglePortLock();
    void setFavoriteFromRow(int row);
    void setFavoriteFromRow_Favorites(int row);
    void useHistoryServer(int row);
    void useFavoriteServer(int row);

    void on_tableWidget_cellClicked(int row, int col);
    void on_tableWidget_2_cellClicked(int row, int col);
    void on_tableWidget_cellChanged(int row, int col);
    void on_tableWidget_2_cellChanged(int row, int col);

    void removeServerFromHistory();
    void clearHistory();
    void on_tableWidget_customContextMenuRequested(const QPoint &pos);

private:
    Ui::ConnectionWindow *ui;
    ServerConnectionAttempt serverConnectionAttempt;


    QPixmap *lockOnPix = nullptr;
    QPixmap *lockOffPix = nullptr;
    QPixmap *starOnPix = nullptr;
    QPixmap *starOffPix = nullptr;
    QVector<FavoriteTableIcon*> favoriteStars;
    QVector<FavoriteTableIcon*> favoriteStars_Favorites;
    ClickableImage *portLock = nullptr;
    int contextMenuRowAction;
};

#endif // CONNECTIONWINDOW_H
