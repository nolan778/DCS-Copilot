/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header:       mainwindow.h
Author:       Cory Parks
Date started: 10/2016

See LICENSE file for copyright and license information

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <QMainWindow>
#include <QLabel>

#include "banlistwindow.h"
#include "settingswindow.h"
#include "serverstart.h"
#include "connectionwindow.h"
#include "aboutwindow.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

enum class TimeStringFormats {
    S = 0,		//seconds only - always increasing
    MMSS,		//mm::ss - minutes can go higher than 59
    HHMMSS,		//HH:mm::ss - hours can go higher than 23
    DHHMMSS,	//d HH:mm:ss - days increment at 24 hours and always increasing
    YDHHMMSS,	//y d HH:mm::ss - years increment at 365 days

    S_FLOAT1,	//seconds "X.Y"
    S_FLOAT2,	//seconds "X.YY"
    S_FLOAT3,	//seconds "X.YYY"
};

enum class BandwidthStringFormats {
    bps = 0,	//bits/sec
    kbps,		//kilobits/sec
    mbps,		//megabits/sec
    gbps,		//gigabits/sec
    Bps,		//bytes/sec
    KBps,		//Kilobytes/sec
    MBps,		//Megabytes/sec
    GBps,		//Gigabytes/sec
    bytes,		//bytes
    KB,			//Kilobytes
    MB,			//Megabytes
    GB,			//Gigabytes
    rateAdaptive, //bps, kbps, mbps, or gbps depending on rate
    totalAdaptive, //bytes, KB, MB, or GB depending on amount
};

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
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void closeEvent(QCloseEvent *event);
    ~MainWindow();

    void changeLabelColor(QLabel* label, const QString& color);
    void logMessage(const QString& logMsg);

    void updateListenerStatus(bool running);
    void updateDCSStatus(bool running);
    void updateServerStatus(int status);

    void addClient(const QString& id, const QString& clientName, unsigned char seatNumber = 0);
    void removeClient(const QString& id);
    void setSeat(const QString& id, unsigned char seatNumber);
    void setServerIP(const QString& ip);
    void setPing(const QString& id, int ping);
    void setMyPing(int ping);
    void setMaxSeats(unsigned char seatNumber);
    void setStatistics(int numClients,
                       uint64_t bandwidthSendRate,
                       uint64_t bandwidthReceiveRate,
                       uint64_t bandwidthSentTotal,
                       uint64_t bandwidthReceivedTotal,
                       uint64_t connectionTime,
                       float myPacketLoss);
    void resetStatistics();
    void clearClients();

    QLabel* Listener_status_label = nullptr;
    QLabel* DCS_status_label = nullptr;
    QLabel* Server_status_label = nullptr;

private slots:
    void on_actionExit_triggered();
    void updateLocalNetwork();
    void updateNetwork();
    void startServer();
    void connectToServer();
    void kickClientFromSeat();
    void kickClientFromServer();
    void banClientFromServer();
    void HandleIndicatorChanged(int logicalIndex, Qt::SortOrder eSort);

    void on_actionStart_Listener_triggered();
    void on_actionStop_Listener_triggered();
    void on_actionBan_List_triggered();
    void on_actionSettings_triggered();
    void on_actionStart_Server_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionAbout_triggered();

    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();

    void on_tableWidget_customContextMenuRequested(const QPoint &pos);

private:
    Ui::MainWindow* ui;
    SettingsWindow* settingsWindow;
    BanListWindow* banListWindow;
    ServerStart* serverStart;
    ConnectionWindow* connectionWindow;
    AboutWindow* aboutWindow;
    unsigned int clientConnectionIndex;
    int contextMenuRowAction;
    int prevClientSortIndex;
    Qt::SortOrder prevClientSortOrder;
    void closeProgram();
    void startLocalServer();
    void stopLocalServer();
    void stopServer();
};

#endif // MAINWINDOW_H
