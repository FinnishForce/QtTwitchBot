#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    // Load settings (commands and login details)
    loadSettings();


    // Create socket
    socket = new QTcpSocket(this);

    // Connect signals and slots!
    connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(connectToServer()));
    connect(ui->disconnectButton, SIGNAL(clicked()), this, SLOT(disconnectFromServer()));
    connect(ui->joinChannel, SIGNAL(clicked()), this, SLOT(joinChannel()));
}

void MainWindow::connectToServer() {
    socket->connectToHost(QString("irc.chat.twitch.tv"), 6667);

    // PASS and NICK commands (they are read from identities.conf when program starts (check loadSettings() for more info)
    socket->write(QString("PASS " + pass + "\r\n").toUtf8());
    socket->write(QString("NICK " + nick + "\r\n").toUtf8());
    ui->textEdit->append("Logged in as " + nick);
    socket->write(QString("CAP REQ :twitch.tv/tags\r\n").toUtf8());
    // Now we are successfully connected to IRC SERVER... hopefully
}

void MainWindow::readData() {
    // Read a line from chat

    QString readLine = socket->readLine();



    readLineList = readLine.split(";");

    qDebug() << readLine;

    if (readLineList.size() == 11){

        //Find out who sent the message
        chatterName = readLineList.at(2);
        chatterName.remove("display-name=");

        //Find out if mod sent the message
        QString s_mod = readLineList.at(5);
        if (s_mod == "mod=1" | chatterName == nick) { isMod = 1; }

        //Find out if subscriber sent the message
        QString s_sub = readLineList.at(7);
        if (s_sub == "subscriber=1") { isSub = 1; }

        //Find the message
        QString s_msg = readLineList.at(10);
        QStringList s_msgList = s_msg.split("PRIVMSG #");
        s_msgList.removeFirst();
        s_msgList = s_msgList.at(0).split(" :");
        qDebug() << s_msgList;
        chatMessage = s_msgList.at(1).trimmed();

        /*else if (s_msgList.size() > 2) {
            s_msgList.removeFirst();
            chatMessage = s_msgList.join(":").trimmed();
        }*/

        //What channel the message was sent on
        QString s_chan = readLineList.at(10);
        chatChannel = s_chan.split("#").at(1).split(" :").at(0);

    }
    /*
    // Lets split the data received from server into a list that we can use later
    if (readLine.contains(":")) {
        readLineList = readLine.split(':', QString::SkipEmptyParts);
    }

    // Check if the split above worked so we don't get errors
    if (readLineList.size() >= 2) {
        readLineList.removeFirst();
        chatMessageList = readLineList;
        chatMessage = readLineList.replaceInStrings("\r\n", "").join(":");
        chatMessage = chatMessage.trimmed();
    }

    // If the line is a chat message then lets dig out what channel it was sent from and who sent it
    if (readLine.contains("PRIVMSG")) {
        readLineList = readLine.split("PRIVMSG #");
        chatChannel = readLineList.at(1).split(" :").at(0).trimmed();

        //Lets dig out who sent the message
        if (readLineList.at(0).contains("!")) {
            readLineList = readLine.split('!', QString::SkipEmptyParts);
        }

        //Now we have :<chattername> left at readLineList[0]
        if (readLineList.at(0).contains(":")) {
            readLineList = readLineList.at(0).split(':', QString::SkipEmptyParts);
        }
        //And now we have only the chatter name at readLineList[0]
        //We trim the name so it's only "name" instead of "name\r\n"
        chatterName = readLineList.at(0).trimmed();
    } //END readLine.contains("PRIVMSG")

    */
    /* Example: How to make a command
    if(readLine.contains("End of /NAMES")) {
        QString tempString = QString("PRIVMSG #" + chatChannel + ":WutFace\r\n");
        socket->write(tempString.toUtf8());
    }*/

    //You must answer the PINGs or you will get DC'd kid
    if (readLine.contains("PING"))
    {
        socket->write("PONG :tmi.twitch.tv\r\n");
    }

    // Lets check if the chat message was a command to the bot
    if (commands.contains(chatMessage.toLower()))
    {
        response = commands.value(chatMessage.toLower());
    }
    else {
        // We didn't get a response, so we try if it was a command like "slap someone"
        // where slap is the command and someone is not used there
        QStringList chatMessageList = chatMessage.split(" ");
        QString tempChatMessage = chatMessageList.at(0);
        response = commands.value(tempChatMessage.toLower());
    }

    // If we got no response from both of the previous checks, we might as well skip this as it does nothing then
    if (response != QVariant()) {

        // response is QVariant, lets make a QString version of it
        QString s_response = response.toString();

        // If response contains replaceable text, we replace them
        if (s_response.contains("$chatterName$") | s_response.contains("$1$")) {
            s_response = replaceSomething(s_response);
        }

        // And then we response
        socket->write(QString("PRIVMSG #" + chatChannel + " :" + s_response + "\r\n").toUtf8());
    } // END if (response != QVariant()) {

    if (isMod && ui->allowAddcomChat->isChecked()) {

        // Command for adding commands
        if (chatMessage.startsWith("!addcom ") && isMod)
        {
            // Lets remove !addcom so we can easily get the command we need to add
            m_com = chatMessage.remove("!addcom ").split(" ").at(0);

            //Now we dig out the response we need to add
            QStringList tempList = chatMessage.remove("!addcom ").split(" ");
            tempList.removeAt(0);
            m_resp = tempList.join(" ");

            //Lets not add same command twice...
            if (!commands.contains(m_com) && m_resp != "") {
                addCommand(m_com, m_resp);
                socket->write(QString("PRIVMSG #" + chatChannel + " :" + "Added <" + m_com + "> successfully!\r\n").toUtf8());
            }
        } // END !addcom


        // Command for removing commands
        if (chatMessage.startsWith("!delcom ") && isMod)
        {
            // Lets remove !delcom so we can easily get the command we need to remove
            m_com = chatMessage.remove("!delcom ").split(" ").at(0);

            //Lets not try to delete something that doesn't exist...
            if (commands.contains(m_com)) {
                delCommand(m_com);
                socket->write(QString("PRIVMSG #" + chatChannel + " : Deleted <" + m_com + "> successfully!\r\n").toUtf8());
            }
        } // END !delcom
    }

    if (ui->printChatMsgCheckbox->isChecked()) {
        // Add received chat messages to ouput
        ui->textEdit->append("(#" + chatChannel + ")\t" + chatterName + ": " + chatMessage);
    }

    // Next data??
    if(socket->canReadLine()) readData();

}

void MainWindow::joinChannel() {
    // Here we join a channel (not twitch server but a channel like #finnishforce_)
    QString tempString = QString("JOIN #" + channelName + "\r\n");
    socket->write(tempString.toUtf8());
    ui->textEdit->append("Joined channel \"" + channelName + "\"");
}

void MainWindow::disconnectFromServer() {
    // Disconnect from IRC server
    //socket->write("QUIT Good bye \r\n"); // Good bye is optional message
    socket->flush();
    socket->disconnect(); // Now we can try it :-)
    //ui->textEdit->append("This button doesn't do anything except print this, sorry");
}



MainWindow::~MainWindow()
{
    // When the program is closed from 'X', we save settings and close the program
    saveSettings();
    delete ui;
}

void MainWindow::on_channelNameBox_editingFinished()
{
    // After editing is finished, we read text from the field
    channelName = ui->channelNameBox->text();
}

void MainWindow::on_sendMsgButton_clicked()
{
    // When "Send message" button is clicked, we use text from the 2 fields to send a message to the right channel
    // Then we empty the message field so it actually looks like a chat
    QString tempMsgString = QString("PRIVMSG #" + msgChannelText + " :" + msgFieldText + "\r\n");
    socket->write(tempMsgString.toUtf8());
    ui->msgField->setText("");
    ui->textEdit->append("#" + msgChannelText+" - You: " + msgFieldText);
}

void MainWindow::on_msgChannel_editingFinished()
{
    // After editing is finished, we read text from the field
    msgChannelText = ui->msgChannel->text();
}

void MainWindow::on_msgField_editingFinished()
{
    // After editing is finished, we read text from the field
    msgFieldText = ui->msgField->text();
}

void MainWindow::on_msgField_returnPressed()
{
    // Make pressing return do the same as pressing button

    msgFieldText = ui->msgField->text();
    on_sendMsgButton_clicked();
}

void MainWindow::on_channelNameBox_returnPressed()
{
    // Make pressing return do the same as pressing button
    channelName = ui->channelNameBox->text();
    joinChannel();
}

void MainWindow::on_leaveChannel_clicked()
{
    // When Leave channel button is clicked, we PART the channel
    QString tempString = QString("PART #" + channelName + "\r\n");
    socket->write(tempString.toUtf8());
    ui->textEdit->append("Left channel \"" + channelName + "\"");
}

void MainWindow::loadSettings()
{

    // Here we load commands and login details
    QString path = QFileInfo(".").absoluteFilePath();
    path = path + "/settings.conf";

    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup("dictionary");
    QVariant qv;
    qv=settings.value("identities");
    commands = qv.toMap();
    printMap();

    settings.beginGroup("logindetails");
    nick = settings.value("nick").toString();
    pass = settings.value("pass").toString();
    ui->channelNameBox->setText(nick);
    ui->msgChannel->setText(nick);
    channelName = nick;
    msgChannelText = nick;

    settings.beginGroup("uisettings");
    bool checkState = settings.value("allowAddcomChat").toBool();
    ui->allowAddcomChat->setChecked(checkState);


}

void MainWindow::saveSettings()
{
    // It's a good idea to save things. We save commands and login details.
    QString path = QFileInfo(".").absoluteFilePath();
    path = path + "/settings.conf";

    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup("dictionary");
    settings.setValue("identities",commands);
    settings.beginGroup("logindetails");
    settings.setValue("nick", nick);
    settings.setValue("pass", pass);

    settings.beginGroup("uisettings");
    settings.setValue("allowAddcomChat", ui->allowAddcomChat->checkState());
}

void MainWindow::printMap() {
    ui->commandBox->setText("");
    QMap<QString, QVariant>::const_iterator i = commands.constBegin();
     while (i != commands.constEnd()) {
         ui->commandBox->append("<" + i.key() + "> " + i.value().toString());
         ++i;
     }
}

void MainWindow::addCommand(QString command, QVariant command_response) {
    if (ui->allowAddcomChat) {
        commands.insert(command.trimmed().toLower(), command_response);
        printMap();
    }
}

void MainWindow::delCommand(QString command) {
    if (ui->allowAddcomChat) {
        commands.remove(command.toLower());
        printMap();
    }
}

void MainWindow::on_addComButton_clicked()
{
    QString command = ui->addCommandLine->text();
    QVariant command_response = QVariant(ui->addCommandResponseLine->text());
    if (command != "" && command_response != "" && !commands.contains(command)) {
       addCommand(command, command_response);
    }
    ui->addCommandLine->setText("");
    ui->addCommandResponseLine->setText("");
}

void MainWindow::on_delComButton_clicked()
{
    QString command = ui->delCommandLine->text();
    if (command != "" && commands.contains(command.toLower())) {
      delCommand(command);
      ui->delCommandLine->setText("");
    }
}

QString MainWindow::replaceSomething(QString thisString)
{
    if (thisString.contains("$chatterName$")) { thisString.replace("$chatterName$", chatterName); }

    if (thisString.contains("$oneWord$")) {
        QStringList tMsgList = chatMessageList;
        tMsgList = chatMessage.split(" ", QString::SkipEmptyParts);
        tMsgList.removeFirst();
        thisString.replace("$oneWord$", tMsgList.at(0));
    }

    if (thisString.contains("$multiWord$")) {
        QStringList tMsgList = chatMessageList;
        tMsgList = chatMessage.split(" ", QString::SkipEmptyParts);
        tMsgList.removeFirst();
        QString tMsg = tMsgList.join(" ");
        thisString.replace("$multiWord$", tMsg);
    }

    return thisString;

}

void MainWindow::on_delCommandLine_returnPressed()
{
    on_delComButton_clicked();
}

void MainWindow::on_addCommandResponseLine_returnPressed()
{
    on_addComButton_clicked();
}

void MainWindow::on_loginButton_clicked()
{
    nick = ui->usernameField->text();
    pass = ui->passwordField->text();
    ui->usernameField->setText("");
    ui->passwordField->setText("");
    saveSettings();
    ui->channelNameBox->setText(nick);
    ui->msgChannel->setText(nick);

}
