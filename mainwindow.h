#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QByteArray>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <string>
#include <QMap>
#include <iterator>
#include <QSettings>
#include <QVariant>
#include <QIcon>
#include <QJsonObject>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private:
    Ui::MainWindow *ui;
    QTcpSocket * socket;
    QString chatMessage, chatterName;
    QStringList readLineList, chatMessageList;
    QString channelName;
    QString chatChannel;
    QString msgChannelText;
    QString msgFieldText;
    QMap <QString, QVariant> commands;
    QVariant response;
    QString m_sSettingsFile;

    QString m_com, m_resp;
    QString nick, pass;

    bool isMod, isOwner, isSub;



    void loadSettings();
    void saveSettings();
    void printMap();
    void addCommand(QString, QVariant);
    void delCommand(QString);
    QString replaceSomething(QString);


private slots:
    void readData();
    void connectToServer();
    void joinChannel();
    void disconnectFromServer();
    void on_channelNameBox_editingFinished();
    void on_sendMsgButton_clicked();
    void on_msgChannel_editingFinished();
    void on_msgField_editingFinished();
    void on_msgField_returnPressed();
    void on_channelNameBox_returnPressed();
    void on_leaveChannel_clicked();
    void on_addComButton_clicked();
    void on_delComButton_clicked();
    void on_delCommandLine_returnPressed();
    void on_addCommandResponseLine_returnPressed();
    void on_loginButton_clicked();
};

#endif // MAINWINDOW_H
