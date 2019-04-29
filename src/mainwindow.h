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

#include "settingswindow.h"
#include "serverstart.h"
#include "connectionwindow.h"
#include "aboutwindow.h"

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
    void setPing(const QString& id, int ping);
    void setMaxSeats(unsigned char seatNumber);
    void clearClients();

    QLabel* Listener_status_label = nullptr;
    QLabel* DCS_status_label = nullptr;
    QLabel* Server_status_label = nullptr;

private slots:
    void on_actionExit_triggered();
    void updateLocalNetwork();
    void updateNetwork();
    void startServer();
    void connectToServer(/*const QString& clientName, const QString& ip, const QString& port, const QString& password*/);

    void on_actionStart_Listener_triggered();
    void on_actionStop_Listener_triggered();
    void on_actionSettings_triggered();
    void on_actionStart_Server_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionAbout_triggered();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::MainWindow* ui;
    SettingsWindow* settingsWindow;
    ServerStart* serverStart;
    ConnectionWindow* connectionWindow;
    AboutWindow* aboutWindow;
    void closeProgram();
    void startLocalServer();
    void stopLocalServer();
    void stopServer();
};

#endif // MAINWINDOW_H
