#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>
#include <iostream>
#include <QDebug>
#include <QtDebug>
#include <QAction>
#include <QMenuBar>
#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QHostAddress>
#include <QUdpSocket>
#include <QIODevice>
#include <QDataStream>
#include <QAbstractSocket>
#include <QNetworkProxy>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QWaitCondition>
#include <QMutex>
#include <QDateTime>
#include <QSizePolicy>
#include <qwaitcondition.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    closeWidget=1;
    reqIDCounter = 0;
    ui->setupUi(this);
    ui->baseAddressTF->setText("10.0.0.2");
    createRadioGroupMode();
    createRadioGroupChannel();
    createRadioGroupCommand();
    createRadioGroupPeripheral();
    on_selectAllChannels_clicked(true);
    bnd = false;
    isOk2Send = false;
    ui->sendPB->setVisible(false);
    connect( ui->configureRegisters, SIGNAL(clicked()), this, SLOT(RadioConfigureRegisters()) );
    connect( ui->connectPB, SIGNAL(clicked()), this, SLOT(Connect()) );
    connect( ui->loadButton, SIGNAL(clicked()), this, SLOT(openFile()) );
    connect( ui->disconnectPB, SIGNAL(clicked()), this, SLOT(Disconnect()));
    connect( ui->sendPB, SIGNAL(clicked(bool)), this, SLOT(Merger(bool)));
    connect( ui->SendConfFile, SIGNAL(clicked(bool)), this, SLOT(sleep()));
    connect( ui->setThresholdButton, SIGNAL(clicked(bool)), this, SLOT(applyThreshold()));
    connect( ui->startPedestalButton, SIGNAL(clicked(bool)), this, SLOT(startPedestal()));
    connect( ui->bypassAPVOn, SIGNAL(clicked(bool)), this, SLOT(bypassOn()));
    connect( ui->bypassAPVOff, SIGNAL(clicked(bool)), this, SLOT(bypassOff()));
    connect( ui->resetAPVProcessor, SIGNAL(clicked(bool)), this, SLOT(resetAPVFunction()));
    connect( ui->selectAPVforBypass, SIGNAL(clicked(bool)), this, SLOT(selectAPVForBypassFunction()));

    connect(chipRegisterType[3],SIGNAL(clicked(bool)),radio[0],SLOT(setChecked(bool)));
    connect(chipRegisterType[0],SIGNAL(clicked(bool)),radio[3],SLOT(setChecked(bool)));
    connect(chipRegisterType[1],SIGNAL(clicked(bool)),radio[3],SLOT(setChecked(bool)));
    connect(chipRegisterType[2],SIGNAL(clicked(bool)),radio[3],SLOT(setChecked(bool)));
    Connect();
    connect(ui->enableExperModeButton, SIGNAL(clicked()), this, SLOT(EnableExpertMode()));
    ui->link->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    ui->link->setOpenExternalLinks(true);
    ui->link->setText("<a href=\"https://twiki.cern.ch/twiki/bin/view/AtlasPublic/SDC\">Help</a>");
    ui->link->show();
    connect( ui->clearLogPB, SIGNAL(clicked()), ui->replyText, SLOT(clear()));
    ui->replyText->setText( "  \tWaiting for data...\t\n" );
    this->setFixedSize(258,620);
}
//_________________________________________________________________________________________________
void MainWindow::Connect(){
    if (bnd) return;
    socket = new QUdpSocket(this);
    bnd = socket->bind(6007, QUdpSocket::ShareAddress);
    if(bnd){
        connect( socket, SIGNAL(readyRead()), this, SLOT(dataPending()) );
        ui->connectionLabel->setText("True");
        ui->connectionLabel->setStyleSheet("background-color: green");
        //        if (isOk2Send) ui->sendPB->setEnabled(true);

        isOk2Send = true;
    }
}
//_________________________________________________________________________________________________
void MainWindow::Disconnect(){
    ui->connectionLabel->setText("False");
    ui->connectionLabel->setStyleSheet("background-color: red");
    socket->disconnectFromHost();
    //    ui->sendPB->setEnabled(false);
    bnd = false;
}
////_________________________________________________________________________________________________
void MainWindow::RadioConfigureRegisters(){
    if (chipRegisterType[0]->isChecked())
        APVConfigure();
    else if(chipRegisterType[1]->isChecked())
        APVApplicationRegistersConfig();
    else if (chipRegisterType[2]->isChecked())
        ADCRegisters();
    else if (chipRegisterType[3]->isChecked()){
        PLLRegisters();
    }
}
////_________________________________________________________________________________________________
void MainWindow::Merger(bool source)
{
    quint16 identifierForFile = 0;
    reqIDCounter++;
    on_connectPB_clicked();
    configurePB();
    on_pushButton_clicked();
    quint16 port=0;
    if(ui->enableResets->isChecked())
        port = 6007;
    else {
        if (chipRegisterType[0]->isChecked()){
            port = 6263;
            identifierForFile = 2;
        }else if(chipRegisterType[3]->isChecked()){
            port = 6263;
            identifierForFile = 4;
        }
        else if(chipRegisterType[1]->isChecked()){
            port = 6039;
            identifierForFile = 3;
        }
        else if (chipRegisterType[2]->isChecked()){
            port = 6519;
            identifierForFile = 1;
        }
    }
    QByteArray datagramAll;
    QDataStream out (&datagramAll, QIODevice::WriteOnly);
    out.setVersion (QDataStream::Qt_4_7);
    out << reqId32 << sudAdd32 << cmd32 ;

    if (command[0]->isChecked() ||command[2]->isChecked()){
        out << (quint32)0;
        for (int i = 0; i<maxIter; i++)
            out << addr[i] << vars[i];
    }// END if Pairs
    if (command[1]->isChecked() ||command[3]->isChecked()) {
        out << addr[0];
        for (int i = 0; i<maxIter; i++)
            out << vars[i];
    }// END if Burst
    if(!source)
        Sender(datagramAll, port);
    else
        writeDataToFile(datagramAll, port, identifierForFile);
}
//_________________________________________________________________________________________________
void MainWindow::Sender(QByteArray blockOfData, quint16 port)
{
    if(bnd){
        //        qDebug() <<"Start,";

        //        QThread::wait(1500);

        //        qDebug() <<"Continue...";
        QString address = ui->baseAddressTF->text();
        socket->writeDatagram(blockOfData,QHostAddress(address), port);
        //        ui->sendPB->setEnabled(false);
        qDebug()<<blockOfData.toHex();
    }else
        qDebug() <<"Error from socket: "<< socket->errorString()<<", Local Port = "<<socket->localPort()<< ", Bind Reply: "<<bnd;
}
//_________________________________________________________________________________________________
void MainWindow::dataPending()
{
    while( socket->hasPendingDatagrams() )
    {
        QByteArray buffer( socket->pendingDatagramSize(), 0 );
        socket->readDatagram( buffer.data(), buffer.size() );
        packageHandler(buffer);
        qDebug("size: %d", buffer.size());
        qDebug()<<"packageSize "<<packageSizeGl;
        ui->replyText->moveCursor(QTextCursor::End ,QTextCursor::MoveAnchor );
    }
}
//_________________________________________________________________________________________________
void MainWindow::createRadioGroupChannel()
{
    QGroupBox *groupBoxChannel8 = new QGroupBox(tr("Select Channel(s)"));
    QVBoxLayout *vbox8 = new QVBoxLayout;
    QString channelNames8[] = {"Channel 1", "Channel 2", "Channel 3", "Channel 4", "Channel 5", "Channel 6", "Channel 7","Channel 8"};
    for (int i = 0; i<8; i++){
        channels[i] = new QCheckBox(channelNames8[i],groupBoxChannel8);
        vbox8->addWidget(channels[i]);
    }
    vbox8->addStretch(1);
    groupBoxChannel8->setGeometry(QRect(115, 20, 120, 181));
    groupBoxChannel8->setLayout(vbox8);
    ui->gridLayoutCh->addWidget(groupBoxChannel8);
}
//_________________________________________________________________________________________________
void MainWindow::createRadioGroupCommand()
{
    groupBox = new QGroupBox(tr("Select Command Type"));
    QVBoxLayout *vbox = new QVBoxLayout;
    QString radioNames[] = {"Write Pairs", "Write Burst", "Read Pairs", "Read Burst", "Reset"};
    for (int i =0; i<5; i++){
        command[i] = new QRadioButton(radioNames[i],groupBox);
        vbox->addWidget(command[i]);
    }
    command[0]->setChecked(1);
    command[4]->setEnabled(false);
    vbox->addStretch(1);
    groupBox->setGeometry(QRect(10, 20, 121, 21));
    groupBox->setLayout(vbox);
    ui->gridLayoutCmd->addWidget(groupBox);
}
//_________________________________________________________________________________________________
void MainWindow::createRadioGroupPeripheral()
{
    QGroupBox *groupBoxPeripheral = new QGroupBox(tr("Select Peripheral"));
    QVBoxLayout *vboxPer = new QVBoxLayout;
    QString radioChipsString[] = {"APV Hybrid", "APV Application", "ADC Card","PLL"};
    for (int i =0; i<4; i++){
        chipRegisterType[i] = new QRadioButton(radioChipsString[i]);
        vboxPer->addWidget(chipRegisterType[i]);
    }
    chipRegisterType[2]->setChecked(1);
    vboxPer->addStretch(1);
    groupBoxPeripheral->setGeometry(QRect(0, 0, 40, 40));
    groupBoxPeripheral->setLayout(vboxPer);
    ui->gridLayoutPer->addWidget(groupBoxPeripheral);

}
//_________________________________________________________________________________________________
void MainWindow::createRadioGroupMode()
{
    groupBox = new QGroupBox(tr("Select Mode"));
    QVBoxLayout *vbox = new QVBoxLayout;
    QString radioNames[] = {"PLL", "Master APV", "Slave APV", "Both APVs"};
    for (int i =0; i<4; i++){
        radio[i] = new QRadioButton(radioNames[i],groupBox);
        vbox->addWidget(radio[i]);
    }
    radio[3]->setChecked(1);
    vbox->addStretch(1);
    groupBox->setGeometry(QRect(10, 20, 101, 21));
    groupBox->setLayout(vbox);
    ui->gridLayout->addWidget(groupBox);
}
//_________________________________________________________________________________________________
MainWindow::~MainWindow()
{
    delete ui;
}
//_________________________________________________________________________________________________
void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
//_________________________________________________________________________________________________
void MainWindow::on_connectPB_clicked()
{
    ui->reqIdDisplay->display(reqIDCounter);
    reqId32 =(quint32)2147483648 + (quint32)reqIDCounter;

    QString addressSTR = ui->baseAddressTF->text();
    bool addressOk;
    addressOk = true;

    foreach (QChar c1, addressSTR) {
        if (c1 !='.')
            if (!c1.isNumber() ) {
                addressOk = false;
                std::cout<<"Not Valid address"<<std::endl;
            }
    }
    QPalette a(ui->baseAddressTF->palette());
    if (!addressOk) {
        a.setColor(QPalette::Base,Qt::red);
    } else a.setColor(QPalette::Base,Qt::transparent);
    ui->baseAddressTF->setPalette(a);
}
//_________________________________________________________________________________________________
void MainWindow::configurePB()
{
    int device = -1;
    for (int i =0; i<4; i++) {
        device = i;
        if (radio[i]->isChecked()) break;
    }
    //    int chan = 0;
    //    for (int i = 0; i<8; i++){
    //        int asus = 1;
    //        if (channels[i]->isChecked()) chan = chan + (asus<<i);

    //    }
    QString chMask = "0000000000000000";
    for (int ig = 0; ig<8; ig++)
        if (channels[ig]->isChecked()) chMask.replace(7-ig, 1, '1');
    bool oko;
    sudAdd32 = device + (chMask.toUInt(&oko, 2));
}
//_________________________________________________________________________________________________
void MainWindow::on_pushButton_clicked()
{
    quint32 cmdQUInt32Types[] = {2863333375, 2864447487, 3149660159, 3148546047};
    for (int i =0; i<4; i++)
        if (command[i]->isChecked())
            cmd32 = cmdQUInt32Types[i];
    cmdInfo32 = 0;
}
//_________________________________________________________________________________________
int MainWindow::packageHandler(QByteArray package)
{
    QString packQS = package.toHex();
    //    qDebug()<<"Packet Received: "<< packQS;
    int posCounter = 0;
    //    QByteArray wordsQBA[150];
    for (QString::Iterator itr = packQS.begin(); itr != packQS.end(); ++itr, ++posCounter){
        //                qDebug()<< *itr<<" --> "<<posCounter<<"  "<<posCounter/8<<" / "<<posCounter%8;
        wordsQBA[posCounter/8].insert(posCounter%8,*itr);
        words[posCounter/8].append(*itr);
    }
    packageSizeGl = posCounter/8;
    qDebug()<<wordsQBA[4]<<"  "<<words[4];
    if (posCounter/8 == 2) {
        frameDropped();
        qDebug()<<"~frameDropped~";
    }
    else {
        nominalReply();
        qDebug()<<"~nominalReply~";
    }
    //    for (int i = 0; i<posCounter/8; i++){
    //        qDebug()<<"Word  "<< i<<" is "<<words[i]<<" or "<<wordsQBA[i];
    //        QBitArray bits(32);
    //        bits = toBitConverter(wordsQBA[i]);
    //    }
    for (int i = 0; i<posCounter/8; i++)
    {
        wordsQBA[i] = "";
        words[i] = "";

    }

    return posCounter/8;
}
//_________________________________________________________________________________________________
QBitArray MainWindow::toBitConverter(QByteArray wordsQBA)
{
    bool ok;
    ulong fromHex = wordsQBA.toULong (&ok, 16);
    //    qDebug() <<"Decimal value: " << fromHex << " "<<ok;
    QString binar;
    binar = QString::number(fromHex, 2);
    //    qDebug() <<"Binary " << binar<<" size " << sizeof(binar);
    QBitArray bits(32);
    int posBit = 0;
    for (QString::Iterator itr = binar.begin(); itr!=binar.end(); ++itr, ++posBit){
        if (*itr == '1') bits[posBit] = true;
        else bits[posBit] = false;
    }
    return bits;
}
//_________________________________________________________________________________________________
void MainWindow::APVConfigure()
{
    ap = new QWidget;
    ap->setGeometry(400,800,500,550);
    QPushButton *applyValues = new QPushButton(ap);
    applyValues->setText("Send");
    applyValues->move(290,490);
    QPushButton *ReadAPVValues = new QPushButton(ap);
    ReadAPVValues->setText("Read From H/W");
    ReadAPVValues->move(380,490);
    ReadAPVValues->setEnabled(0);
    QPushButton *closeValues = new QPushButton(ap);
    closeValues->setText("Close");
    closeValues->move(200,490);
    QPushButton *defaultValues = new QPushButton(ap);
    defaultValues->setText("Factory Settings");
    defaultValues->move(130,520);
    defaultValues->setEnabled(0);
    QPushButton *saveToFile = new QPushButton(ap);
    saveToFile->setText("Save To File");
    //    saveToFile->hide();
    saveToFile->move(0,490);
    QPushButton *loadFromFile = new QPushButton(ap);
    loadFromFile->setText("Load From File");
    loadFromFile->move(0,520);

    QCheckBox *apvPortCheck = new QCheckBox(ap);
    apvPortCheck->setChecked(1);
    apvPortCheck->setText("APV Port");
    apvPortCheck->move(10,20);

    apvPort =new QLineEdit(ap);
    apvPort->setText("6263");
    apvPort->setAlignment(Qt::AlignRight);
    apvPort->setEnabled(0);
    apvPort->move(100,20);
    apvPort->resize(80,20);

    inputDataText = new QLabel(ap);
    inputDataText->setText("That's the Default Settings");
    inputDataText->move(250,25);

    QGroupBox *groupModify = new QGroupBox(tr("APV Registers"),ap);
    QGridLayout *grid = new QGridLayout(ap);
    grid->setHorizontalSpacing(200);
    QString sendnamesAPV[] = {"MODE", "LATENCY", "MUXGAIN", "IPRE","IPCARC","IPSF","ISHA","ISSF","IPSP","IMUXIN","ICAL","VPSP","VFS","VFP","CDRV","CSEL"};
    QString addressesAPV[] = {"0000001","0000010","0000011","0010000","0010001","0010010","0010011","0010100","0010101","0010110","0011000","0011011","0011010","0011001","0011100","0011101"};
    QString valuesAPV[] = {"11001","105","100","98","52","34","34","34","55","16","100","40","60","30","11101111","11110111"};
    for (int i = 0; i<16; i++){
        modifyAPVRegisters[i] = new QCheckBox(sendnamesAPV[i],groupModify);

        sendAPVRegisters[i] = new QCheckBox(groupModify);
        sendAPVRegisters[i]->move(180,80);
        sendAPVRegisters[i]->setChecked(1);

        APVAddresses[i] = new QLineEdit(addressesAPV[i],groupModify);
        APVAddresses[i]->resize(80,20);
        APVAddresses[i]->move(100,30+i*23);
        APVAddresses[i]->setEnabled(0);
        APVAddresses[i]->setAlignment(Qt::AlignRight);

        APVValuesFromHW[i] = new QLineEdit("",groupModify);
        APVValuesFromHW[i]->resize(80,20);
        APVValuesFromHW[i]->move(380,30+i*23);
        APVValuesFromHW[i]->setEnabled(0);
        APVValuesFromHW[i]->setAlignment(Qt::AlignRight);


        APVValues[i] = new QLineEdit(valuesAPV[i],groupModify);
        APVValues[i]->resize(80,20);
        APVValues[i]->move(200,30+i*23);
        APVValues[i]->setEnabled(0);
        APVValues[i]->setAlignment(Qt::AlignRight);


        connect( modifyAPVRegisters[i], SIGNAL(clicked(bool)), APVValues[i], SLOT(setEnabled(bool)) );

        grid->addWidget(modifyAPVRegisters[i],i,1);
        grid->addWidget(sendAPVRegisters[i],i,2);
    }
    QString textFile = ui->confFile->text();
    if((textFile!="")&&(textFile!="Error")){
        loadRecipeFromFile();
        inputDataText->setText("Data Settings From Recipe File");
    }
    groupModify->setGeometry(QRect(10, 50, 480, 420));
    groupModify->setLayout(grid);
    ap->show();
    connect( closeValues, SIGNAL(clicked()), ap, SLOT(close()) );
    connect( defaultValues, SIGNAL(clicked()), this, SLOT(setAPVDefaults()) );
    connect( applyValues, SIGNAL(clicked()), this, SLOT(applyApv()) );
    //    connect( applyValues, SIGNAL(released()), saveToFile, SLOT(show() ));
    connect( saveToFile, SIGNAL(clicked()), this, SLOT(saveFile()) );
    connect( loadFromFile, SIGNAL(clicked()), this, SLOT(loadRecipeFromFile()) );
    connect( ReadAPVValues, SIGNAL(clicked()), this, SLOT(ReadValuesFromHardware()) );
}
//_________________________________________________________________________________________________
void MainWindow::setAPVDefaults()
{
    QString valuesAPV[] = {"11001","128","100","98","52","34","34","34","55","16","100","40","60","30","11101111","11110111"};
    for (int i = 0; i<16; i++){
        APVValues[i]->setText(valuesAPV[i]);
    }
}
//_________________________________________________________________________________________________
void MainWindow::applyApv()
{    
    APVRegPort = (quint16)apvPort->text().toUInt();
    isOk2Send = true;
    if (bnd) ui->sendPB->setEnabled(true);
    /// to include maxIter

    bool ok[32];
    maxIter = 0;
    for (int i = 0; i<16; i++){
        if(sendAPVRegisters[i]->isChecked()){
            addr[maxIter] = (quint32)APVAddresses[i]->text().toUInt(&ok[i],2);
            if((i==0)||(i==2)||(i==14)||(i==15))
                vars[maxIter] = (quint32)APVValues[i]->text().toUInt(&ok[i*2],2);
            else
                vars[maxIter] = (quint32)APVValues[i]->text().toUInt(&ok[i*2]);
            maxIter++;
        }
    }
    Merger(false);
    if(closeWidget)
        ap->close();
    //    qDebug()<<" maxIter "<<maxIter;
}
//_________________________________________________________________________________________________
void MainWindow::applyApvForFile()
{
    APVRegPort = (quint16)apvPort->text().toUInt();
    isOk2Send = true;
    if (bnd) ui->sendPB->setEnabled(true);
    /// to include maxIter

    bool ok[32];
    maxIter = 0;
    for (int i = 0; i<16; i++){
        if(sendAPVRegisters[i]->isChecked()){
            addr[maxIter] = (quint32)APVAddresses[i]->text().toUInt(&ok[i],2);
            if((i==0)||(i==2)||(i==14)||(i==15))
                vars[maxIter] = (quint32)APVValues[i]->text().toUInt(&ok[i*2],2);
            else
                vars[maxIter] = (quint32)APVValues[i]->text().toUInt(&ok[i*2]);
            maxIter++;
        }
    }
    //    Merger(false);
    //    if(closeWidget)
    //        ap->close();
    //    //    qDebug()<<" maxIter "<<maxIter;
}
//_________________________________________________________________________________________________
void MainWindow::on_clearLogPB_clicked()
{
    //    ui->replyText->clear();
}
//_________________________________________________________________________________________________
void MainWindow::ADCRegisters()
{
    ad = new QWidget;
    ad->setGeometry(400,400,530,380);
    QPushButton *applyValues = new QPushButton(ad);
    applyValues->setText("Send");
    applyValues->move(320,260);
    QPushButton *ReadADCValues = new QPushButton(ad);
    ReadADCValues->setText("Read From H/W");
    ReadADCValues->move(410,260);
    ReadADCValues->setEnabled(0);
    QPushButton *closeValues = new QPushButton(ad);
    closeValues->setText("Close");
    closeValues->move(230,260);
    QPushButton *defaultValues = new QPushButton(ad);
    defaultValues->setText("Factory Settings");
    defaultValues->move(140,290);
    defaultValues->setEnabled(0);
    QPushButton *saveToFile = new QPushButton(ad);
    saveToFile->setText("Save To File");
    //    saveToFile->hide();
    saveToFile->move(0,260);
    QPushButton *loadFromFile = new QPushButton(ad);
    loadFromFile->setText("Load From File");
    loadFromFile->move(0,290);

    QCheckBox *adcPortCheck = new QCheckBox(ad);
    adcPortCheck->setChecked(1);
    adcPortCheck->setText("ADC Port");
    adcPortCheck->move(10,20);

    adcPort =new QLineEdit(ad);
    adcPort->setText("6519");
    adcPort->setAlignment(Qt::AlignRight);
    adcPort->setEnabled(0);
    adcPort->move(100,20);
    adcPort->resize(80,20);

    inputDataText = new QLabel(ad);
    inputDataText->setText("That's the Default Settings");
    inputDataText->move(250,25);

    QGroupBox *groupModify = new QGroupBox(tr("ADC Registers (hex)"),ad);
    QGridLayout *grid = new QGridLayout(ad);
    grid->setHorizontalSpacing(200);
    QString sendnamesADC[] = {"HYBRID RST N", "PWRDOWN CH0", "PWRDOWN CH1", "EQ LEVEL 0", "EQ LEVEL 1","TRGOUT ENABLE","BCLK ENABLE"};
    QString addressesADC[] = {"00","01","02","03","04","05","06"};
    QString valuesADC[] = {"FF","00","00","00","00","00","FF"};
    for (int i = 0; i<7; i++){
        modifyADCRegisters[i] = new QCheckBox(sendnamesADC[i],groupModify);

        sendADCRegisters[i] = new QCheckBox(groupModify);
        sendADCRegisters[i]->move(200,80);
        sendADCRegisters[i]->setChecked(1);

        ADCAddresses[i] = new QLineEdit(addressesADC[i],groupModify);
        ADCAddresses[i]->resize(80,20);
        ADCAddresses[i]->move(150,30+i*23);
        ADCAddresses[i]->setEnabled(0);
        ADCAddresses[i]->setAlignment(Qt::AlignRight);

        ADCValues[i] = new QLineEdit(valuesADC[i],groupModify);
        ADCValues[i]->resize(80,20);
        ADCValues[i]->move(250,30+i*23);
        ADCValues[i]->setEnabled(0);
        ADCValues[i]->setAlignment(Qt::AlignRight);

        ADCValuesFromHW[i] = new QLineEdit("",groupModify);
        ADCValuesFromHW[i]->resize(80,20);
        ADCValuesFromHW[i]->move(390,30+i*23);
        ADCValuesFromHW[i]->setEnabled(0);
        ADCValuesFromHW[i]->setAlignment(Qt::AlignRight);

        connect( modifyADCRegisters[i], SIGNAL(clicked(bool)), ADCValues[i], SLOT(setEnabled(bool)) );

        grid->addWidget(modifyADCRegisters[i],i,1);
        grid->addWidget(sendADCRegisters[i],i,2);
    }
    QString textFile = ui->confFile->text();
    if((textFile!="")&&(textFile!="Error")){
        loadRecipeFromFile();
        inputDataText->setText("Data Settings From Recipe File");
    }
    groupModify->setGeometry(QRect(10, 50, 500, 200));
    groupModify->setLayout(grid);
    ad->show();
    connect( closeValues, SIGNAL(clicked()), ad, SLOT(close()));
    connect( defaultValues, SIGNAL(clicked()), this, SLOT(setADCDefaults()) );
    connect( applyValues, SIGNAL(clicked()), this, SLOT(applyAdc()));
    //    connect( applyValues, SIGNAL(released()), saveToFile, SLOT(show()));
    connect( saveToFile, SIGNAL(clicked()), this, SLOT(saveFile()));
    connect( loadFromFile, SIGNAL(clicked()), this, SLOT(loadRecipeFromFile()) );
    connect( ReadADCValues, SIGNAL(clicked()), this, SLOT(ReadValuesFromHardware()) );
}
//_________________________________________________________________________________________________
void MainWindow::setADCDefaults()
{
    QString valuesADC[] = {"FF","00","00","00","00","00","FF"};
    for (int i = 0; i<7; i++){
        ADCValues[i]->setText(valuesADC[i]);

    }
}
//_________________________________________________________________________________________________
void MainWindow::applyAdc()
{
    ADCRegPort = (quint16)adcPort->text().toUInt();
    isOk2Send = true;
    if (bnd) ui->sendPB->setEnabled(true);
    /// to include maxIter

    bool ok[14];
    maxIter = 0;
    for (int i = 0; i<7; i++){
        if(sendADCRegisters[i]->isChecked()){
            addr[maxIter] = (quint32)ADCAddresses[i]->text().toUInt(&ok[i],16);
            vars[maxIter] = (quint32)ADCValues[i]->text().toUInt(&ok[i*2],16);
            maxIter++;
        }
    }
    Merger(false);
    if(closeWidget)
        ad->close();
}
//_________________________________________________________________________________________________
void MainWindow::applyAdcForFile()
{
    ADCRegPort = (quint16)adcPort->text().toUInt();
    isOk2Send = true;
    if (bnd) ui->sendPB->setEnabled(true);
    /// to include maxIter

    bool ok[14];
    maxIter = 0;
    for (int i = 0; i<7; i++){
        if(sendADCRegisters[i]->isChecked()){
            addr[maxIter] = (quint32)ADCAddresses[i]->text().toUInt(&ok[i],16);
            vars[maxIter] = (quint32)ADCValues[i]->text().toUInt(&ok[i*2],16);
            maxIter++;
        }
    }
    //    Merger(false);
    //    if(closeWidget)
    //        ad->close();
}
//_________________________________________________________________________________________________
void MainWindow::PLLRegisters()
{
    pll = new QWidget;
    pll->setGeometry(400,400,520,250);
    QPushButton *applyValues = new QPushButton(pll);
    applyValues->setText("Send");
    applyValues->move(310,160);
    QPushButton *ReadPLLValues = new QPushButton(pll);
    ReadPLLValues->setText("Read From H/W");
    ReadPLLValues->move(400,160);
    ReadPLLValues->setEnabled(0);
    QPushButton *closeValues = new QPushButton(pll);
    closeValues->setText("Close");
    closeValues->move(220,160);
    QPushButton *defaultValues = new QPushButton(pll);
    defaultValues->setText("Factory Settings");
    defaultValues->move(140,190);
    defaultValues->setEnabled(0);
    QPushButton *saveToFile = new QPushButton(pll);
    saveToFile->setText("Save To File");
    //    saveToFile->hide();
    saveToFile->move(0,160);
    QPushButton *loadFromFile = new QPushButton(pll);
    loadFromFile->setText("Load From File");
    loadFromFile->move(0,190);

    QCheckBox *pllPortCheck = new QCheckBox(pll);
    pllPortCheck->setChecked(1);
    pllPortCheck->setText("PLL Port");
    pllPortCheck->move(10,20);

    apvPort =new QLineEdit(pll);
    apvPort->setText("6263");
    apvPort->setAlignment(Qt::AlignRight);
    apvPort->setEnabled(0);
    apvPort->move(100,20);
    apvPort->resize(80,20);

    inputDataText = new QLabel(pll);
    inputDataText->setText("That's the Default Settings");
    inputDataText->move(250,25);

    QGroupBox *groupModify = new QGroupBox(tr("PLL Registers (hex)"),pll);
    QGridLayout *grid = new QGridLayout(pll);
    grid->setHorizontalSpacing(200);
    QString sendnamesPLL[] = {"CSR1 FINEDELAY","TRG DELAY"};
    QString addressesPLL[] = {"01","03"};
    QString valuesPLL[] = {"10","00"};
    for (int i = 0; i<2; i++){
        modifyPLLRegisters[i] = new QCheckBox(sendnamesPLL[i],groupModify);

        sendPLLRegisters[i] = new QCheckBox(groupModify);
        sendPLLRegisters[i]->move(200,80);
        sendPLLRegisters[i]->setChecked(1);

        PLLAddresses[i] = new QLineEdit(addressesPLL[i],groupModify);
        PLLAddresses[i]->resize(80,20);
        PLLAddresses[i]->move(150,30+i*23);
        PLLAddresses[i]->setEnabled(0);
        PLLAddresses[i]->setAlignment(Qt::AlignRight);

        PLLValues[i] = new QLineEdit(valuesPLL[i],groupModify);
        PLLValues[i]->resize(80,20);
        PLLValues[i]->move(250,30+i*23);
        PLLValues[i]->setEnabled(0);
        PLLValues[i]->setAlignment(Qt::AlignRight);

        PLLValuesFromHW[i] = new QLineEdit("",groupModify);
        PLLValuesFromHW[i]->resize(80,20);
        PLLValuesFromHW[i]->move(380,30+i*23);
        PLLValuesFromHW[i]->setEnabled(0);
        PLLValuesFromHW[i]->setAlignment(Qt::AlignRight);

        connect( modifyPLLRegisters[i], SIGNAL(clicked(bool)), PLLValues[i], SLOT(setEnabled(bool)) );

        grid->addWidget(modifyPLLRegisters[i],i,1);
        grid->addWidget(sendPLLRegisters[i],i,2);
    }
    QString textFile = ui->confFile->text();
    if((textFile!="")&&(textFile!="Error")){
        loadRecipeFromFile();
        inputDataText->setText("Data Settings From Recipe File");
    }
    groupModify->setGeometry(QRect(10, 50, 500, 90));
    groupModify->setLayout(grid);
    pll->show();
    connect( closeValues, SIGNAL(clicked()), pll, SLOT(close()) );
    connect( defaultValues, SIGNAL(clicked()), this, SLOT(setPLLDefaults()) );
    connect( applyValues, SIGNAL(clicked()), this, SLOT(applyPLL()) );
    //    connect( applyValues, SIGNAL(released()), saveToFile, SLOT(show() ));
    connect( saveToFile, SIGNAL(clicked()), this, SLOT(saveFile()) );
    connect( loadFromFile, SIGNAL(clicked()), this, SLOT(loadRecipeFromFile()) );
    connect( ReadPLLValues, SIGNAL(clicked()), this, SLOT(ReadValuesFromHardware()) );
}
//_________________________________________________________________________________________________
void MainWindow::setPLLDefaults()
{
    QString valuesPLL[] = {"10","00"};
    for (int i = 0; i<2; i++){
        PLLValues[i]->setText(valuesPLL[i]);
    }
}
//_________________________________________________________________________________________________
void MainWindow::applyPLL()
{
    APVRegPort = (quint16)apvPort->text().toUInt();
    isOk2Send = true;
    if (bnd) ui->sendPB->setEnabled(true);
    /// to include maxIter

    bool ok[4];
    maxIter = 0;
    for (int i = 0; i<2; i++){
        if(sendPLLRegisters[i]->isChecked()){
            addr[maxIter] = (quint32)PLLAddresses[i]->text().toUInt(&ok[i],16);
            vars[maxIter] = (quint32)PLLValues[i]->text().toUInt(&ok[i*2],16);
            //            qDebug()<<addr[maxIter]<<vars[maxIter];

            maxIter++;
        }
    }
    Merger(false);
    if(closeWidget)
        pll->close();
    //    qDebug()<<" maxIter "<<maxIter;
}
//_________________________________________________________________________________________________
void MainWindow::applyPLLForFile()
{
    APVRegPort = (quint16)apvPort->text().toUInt();
    isOk2Send = true;
    if (bnd) ui->sendPB->setEnabled(true);
    /// to include maxIter

    bool ok[4];
    maxIter = 0;
    for (int i = 0; i<2; i++){
        if(sendPLLRegisters[i]->isChecked()){
            addr[maxIter] = (quint32)PLLAddresses[i]->text().toUInt(&ok[i],16);
            vars[maxIter] = (quint32)PLLValues[i]->text().toUInt(&ok[i*2],16);
            //            qDebug()<<addr[maxIter]<<vars[maxIter];

            maxIter++;
        }
    }
    //    Merger(false);
    //    if(closeWidget)
    //        pll->close();
    //    //    qDebug()<<" maxIter "<<maxIter;
}
//_________________________________________________________________________________________________
void MainWindow::APVApplicationRegistersConfig()
{
    APVAp = new QWidget;
    APVAp->setGeometry(410,800,600,680);
    QPushButton *applyValues = new QPushButton(APVAp);
    applyValues->setText("Send");
    applyValues->move(360,600);
    QPushButton *ReadAPVAValues = new QPushButton(APVAp);
    ReadAPVAValues->setText("Read From H/W");
    ReadAPVAValues->move(450,600);
    ReadAPVAValues->setEnabled(0);
    QPushButton *closeValues = new QPushButton(APVAp);
    closeValues->setText("Close");
    closeValues->move(270,600);
    QPushButton *defaultValues = new QPushButton(APVAp);
    defaultValues->setText("Factory Settings");
    defaultValues->move(150,630);
    defaultValues->setEnabled(0);
    QPushButton *saveToFile = new QPushButton(APVAp);
    saveToFile->setText("Save To File");
    //    saveToFile->hide();
    saveToFile->move(0,600);
    QPushButton *loadFromFile = new QPushButton(APVAp);
    loadFromFile->setText("Load From File");
    loadFromFile->move(0,630);


    QCheckBox *apvPortCheck = new QCheckBox(APVAp);
    apvPortCheck->setChecked(1);
    apvPortCheck->setText("APV Application Registers Port");
    apvPortCheck->move(10,20);

    APVApplicationRegistersPort =new QLineEdit(APVAp);
    APVApplicationRegistersPort->setText("6039");
    APVApplicationRegistersPort->setAlignment(Qt::AlignRight);
    APVApplicationRegistersPort->setEnabled(0);
    APVApplicationRegistersPort->move(260,20);
    APVApplicationRegistersPort->resize(80,20);

    inputDataText = new QLabel(APVAp);
    inputDataText->setText("That's the Default Settings");
    inputDataText->move(350,25);

    QGroupBox *groupModify = new QGroupBox(tr("APV Registers"),APVAp);
    QGridLayout *grid = new QGridLayout(APVAp);
    grid->setHorizontalSpacing(220);
    QString sendnamesAPV[] = {"BCLK_MODE", "BCLK_TRGBURST", "BCLK_FREQ", "BCLK_TRGDELAY", "BCLK_TPDELAY","BCLK_ROSYNC","EVBLD_CHMASK","EVBLD_DATALENGTH","EVBLD_MODE","EVBLD_EVENTINFOTYPE","EVBLD_EVENTINFODATA"};
    QString addressesAPV[] = {"00000000","00000001","00000010","00000011","00000100","0000101",  "00001000","00001001","00001010","00001011","00001100"};
    //    QString valuesAPV[] = {"00000111","4","40000","256","128","300","65535","3000","0","0","2864384952"};
    //    QString valuesAPV[] = {"00000111","4","40000","256","128","300","1111111111111111","3000","0","0","2864384952"};
    QString valuesAPV[] = {"00000100","8","4000","128","128","200","0000001101010101","4000","0","0","2864384952"};

    for (int i = 0; i<11; i++){
        modifyAPVApplicationRegisters[i] = new QCheckBox(sendnamesAPV[i],groupModify);

        sendAPVApplicationRegisters[i] = new QCheckBox(groupModify);
        sendAPVApplicationRegisters[i]->move(190,75);
        sendAPVApplicationRegisters[i]->setChecked(1);

        addressesAPVApplicationRegisters[i] = new QLineEdit(addressesAPV[i],groupModify);
        addressesAPVApplicationRegisters[i]->resize(80,20);
        addressesAPVApplicationRegisters[i]->move(190,40+i*35.1);
        addressesAPVApplicationRegisters[i]->setEnabled(0);
        addressesAPVApplicationRegisters[i]->setAlignment(Qt::AlignRight);

        valuesAPVApplicationRegisters[i] = new QLineEdit(valuesAPV[i],groupModify);
        valuesAPVApplicationRegisters[i]->resize(120,20);
        valuesAPVApplicationRegisters[i]->move(278,40+i*35.1);
        valuesAPVApplicationRegisters[i]->setEnabled(0);
        valuesAPVApplicationRegisters[i]->setAlignment(Qt::AlignRight);

        valuesAPVApplicationRegistersFromHW[i] = new QLineEdit("",groupModify);
        valuesAPVApplicationRegistersFromHW[i]->resize(120,20);
        valuesAPVApplicationRegistersFromHW[i]->move(430,40+i*35.1);
        valuesAPVApplicationRegistersFromHW[i]->setEnabled(0);
        valuesAPVApplicationRegistersFromHW[i]->setAlignment(Qt::AlignRight);

        connect( modifyAPVApplicationRegisters[i], SIGNAL(clicked(bool)), valuesAPVApplicationRegisters[i], SLOT(setEnabled(bool)) );

        grid->addWidget(modifyAPVApplicationRegisters[i],i,1);
        grid->addWidget(sendAPVApplicationRegisters[i],i,2);
    }
    QGroupBox *chMaskModifyM = new QGroupBox(tr("Channel Mask Masters"),APVAp);
    QGroupBox *chMaskModifyS = new QGroupBox(tr("Channel Mask Slaves"),APVAp);

    QHBoxLayout *hbox8Master = new QHBoxLayout;
    QHBoxLayout *hbox8Slaves = new QHBoxLayout;
    QString channelMasters[] = {"0","2", "4", "6", "8", "10", "12", "14"};
    QString channelSlaves[] = {"1", "3", "5", "7", "9", "11", "13","15"};
    for (int i = 0; i<8; i++){
        MasterChannels[i] = new QCheckBox(channelMasters[i],chMaskModifyM);
        SlaveChannels[i] = new QCheckBox(channelSlaves[i],chMaskModifyS);
        hbox8Master->addWidget(MasterChannels[i]);
        hbox8Slaves->addWidget(SlaveChannels[i]);
        MasterChannels[i]->setEnabled(false);
        SlaveChannels[i]->setEnabled(false);
    }
    chMaskModifyM->setLayout(hbox8Master);
    chMaskModifyS->setLayout(hbox8Slaves);
    QString textFile = ui->confFile->text();
    if((textFile!="")&&(textFile!="Error")){
        loadRecipeFromFile();
        inputDataText->setText("Data Settings From Recipe File");
    }
    groupModify->setGeometry(QRect(10, 50, 580, 440));
    chMaskModifyM->setGeometry(QRect(10, 490, 580, 57));
    chMaskModifyS->setGeometry(QRect(10, 540, 580, 57));

    groupModify->setLayout(grid);
    APVAp->show();
    for (int i = 0; i<8; i++){
        connect(MasterChannels[i],SIGNAL(clicked(bool)),this,SLOT(editMask()));
        connect(SlaveChannels[i],SIGNAL(clicked(bool)),this,SLOT(editMask()));
    }
    connect( modifyAPVApplicationRegisters[6], SIGNAL(clicked()), this, SLOT(EnableMasks()) );

    connect(valuesAPVApplicationRegisters[6],SIGNAL(returnPressed()),this,SLOT(editMaskBox()));
    connect( closeValues, SIGNAL(clicked()), APVAp, SLOT(close()) );
    connect( defaultValues, SIGNAL(clicked()), this, SLOT(setAPVApplicationRegistersDefaults()) );
    connect( applyValues, SIGNAL(clicked()), this, SLOT(applyAPVApplicationRegisters()) );
    //    connect( applyValues, SIGNAL(released()), saveToFile, SLOT(show() ));
    connect( saveToFile, SIGNAL(clicked()), this, SLOT(saveFile()) );
    connect( loadFromFile, SIGNAL(clicked()), this, SLOT(loadRecipeFromFile()) );
    connect( ReadAPVAValues, SIGNAL(clicked()), this, SLOT(ReadValuesFromHardware()) );
    editMaskBox();
}
//_________________________________________________________________________________________________
void MainWindow::EnableMasks(){
    if(modifyAPVApplicationRegisters[6]->isChecked()){
        for (int i = 0; i<8; i++){
            MasterChannels[i]->setEnabled(true);
            SlaveChannels[i]->setEnabled(true);
        }
    }else{
        for (int i = 0; i<8; i++){
            MasterChannels[i]->setEnabled(false);
            SlaveChannels[i]->setEnabled(false);
        }
    }
}
//_________________________________________________________________________________________________
void MainWindow::editMask(){
    QString chanMask = valuesAPVApplicationRegisters[6]->text();
    int ln = chanMask.length();
    if(ln<16){
        for(int i=0;i<16-ln;i++){
            chanMask.insert(ln+i,'0');
        }
    }

    for (int i = 0; i<8; i++){
        if (MasterChannels[i]->isChecked()) chanMask.replace(15-2*i, 1, '1');
        else chanMask.replace(15-2*i, 1, '0');
        if (SlaveChannels[i]->isChecked()) chanMask.replace(14-2*i, 1, '1');
        else chanMask.replace(14-2*i, 1, '0');
        valuesAPVApplicationRegisters[6]->setText(chanMask);
    }
}
//_________________________________________________________________________________________________
void MainWindow::editMaskBox(){
    QString chanMask = valuesAPVApplicationRegisters[6]->text();
    int ln = chanMask.length();
    if(ln<16){
        for(int i=0;i<16-ln;i++){
            chanMask.insert(i,'0');
        }
    }
    valuesAPVApplicationRegisters[6]->setText(chanMask);
    ln = chanMask.length();
    for (int i = 1; i<=ln/2;i++){
        if (chanMask.mid(ln-(2*i),1)==QChar('1')) SlaveChannels[i-1]->setChecked(true);
        else SlaveChannels[i-1]->setChecked(false);
        if (chanMask.mid(ln-(2*i-1),1)==QChar('1')) MasterChannels[i-1]->setChecked(true);
        else MasterChannels[i-1]->setChecked(false);
    }
}
//_________________________________________________________________________________________________
void MainWindow::setAPVApplicationRegistersDefaults()
{
    QString valuesAPV[] = {"00000100","4","40000","256","128","300","1111111111111111","3000","0","0","2864384952"};
    for (int i = 0; i<11; i++)
        valuesAPVApplicationRegisters[i]->setText(valuesAPV[i]);
}
//_________________________________________________________________________________________________
void MainWindow::applyAPVApplicationRegisters()
{
    APVRegPort = (quint16)APVApplicationRegistersPort->text().toUInt();
    isOk2Send = true;
    if (bnd) ui->sendPB->setEnabled(true);
    /// to include maxIter

    bool ok[22];
    maxIter = 0;
    for (int i = 0; i<11; i++){
        if(sendAPVApplicationRegisters[i]->isChecked()){
            addr[maxIter] = (quint32)addressesAPVApplicationRegisters[i]->text().toUInt(&ok[i],2);
            if((i==0)||(i==6))
                vars[maxIter] = (quint32)valuesAPVApplicationRegisters[i]->text().toUInt(&ok[i*2],2);
            else
                vars[maxIter] = (quint32)valuesAPVApplicationRegisters[i]->text().toUInt(&ok[i*2]);
            maxIter++;
            //           qDebug()<<" addr[i] "<<addr[maxIter];
        }
    }
    //    qDebug()<<" maxIter from APVApplicationRegisters "<<maxIter;
    //    for (int i = 0; i<maxIter; i++) qDebug()<<i<<" addr "<<addr[i]<<" vars "<<vars[i];
    ui->internalTrg->setStyleSheet("background-color: gray");
    ui->externalTrg->setStyleSheet("background-color: green");
    Merger(false);
    if(closeWidget)
        APVAp->close();
}
//_________________________________________________________________________________________________
void MainWindow::applyAPVApplicationRegistersForFile()
{
    APVRegPort = (quint16)APVApplicationRegistersPort->text().toUInt();
    isOk2Send = true;
    if (bnd) ui->sendPB->setEnabled(true);
    /// to include maxIter

    bool ok[22];
    maxIter = 0;
    for (int i = 0; i<11; i++){
        if(sendAPVApplicationRegisters[i]->isChecked()){
            addr[maxIter] = (quint32)addressesAPVApplicationRegisters[i]->text().toUInt(&ok[i],2);
            if((i==0)||(i==6))
                vars[maxIter] = (quint32)valuesAPVApplicationRegisters[i]->text().toUInt(&ok[i*2],2);
            else
                vars[maxIter] = (quint32)valuesAPVApplicationRegisters[i]->text().toUInt(&ok[i*2]);
            maxIter++;
            //           qDebug()<<" addr[i] "<<addr[maxIter];
        }
    }
    //    qDebug()<<" maxIter from APVApplicationRegisters "<<maxIter;
    //    for (int i = 0; i<maxIter; i++) qDebug()<<i<<" addr "<<addr[i]<<" vars "<<vars[i];
    //    ui->internalTrg->setStyleSheet("background-color: gray");
    //    ui->externalTrg->setStyleSheet("background-color: green");
    //    Merger(false);
    //    if(closeWidget)
    //        APVAp->close();
}
//_________________________________________________________________________________________
void MainWindow::frameDropped()
{
    char ch[50];
    QString tempo;
    bool ok;
    ui->replyText->append("~~~~~~~~~~~~~~~~~~~~>> Frame Dropped!!! <<~~~~~~~~~~~~~~~~~~~~");
    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("hh:mm:ss   of   yyyy.MM.dd");
    ui->replyText->append("Received at " + dateTimeString);
    ulong fromHex = wordsQBA[0].toULong (&ok, 16);
    if (!ok)
        sprintf(ch, "Request Id: %lu", fromHex);
    tempo.append(ch);
    ui->replyText->append(tempo);
    tempo = "";

    QBitArray bits(32);
    bits = toBitConverter(wordsQBA[1]);
    for (int iBits = 0; iBits<bits.size(); iBits++)
        if (bits.testBit(iBits)) tempo.append('1');
        else tempo.append('0');
    ui->replyText->append(tempo);
    tempo = "";

    QString errorMSGFrameDropped[] = {"destination port unavailable", "illegal source address", "buffer full",
                                      "illegal length (incomplete 32-bit word)", "illegal length (< 4 words)", "command unrecognized",
                                      "illformed command","checksum error"};
    if (bits.testBit(0)) ui->replyText->append(errorMSGFrameDropped[0]);
    if (bits.testBit(1)) ui->replyText->append(errorMSGFrameDropped[1]);
    if (bits.testBit(2)) ui->replyText->append(errorMSGFrameDropped[2]);
    if (bits.testBit(3)) ui->replyText->append(errorMSGFrameDropped[3]);
    if (bits.testBit(4)) ui->replyText->append(errorMSGFrameDropped[4]);
    if (bits.testBit(12)) ui->replyText->append(errorMSGFrameDropped[5]);
    if (bits.testBit(13)) ui->replyText->append(errorMSGFrameDropped[6]);
    if (bits.testBit(15)) ui->replyText->append(errorMSGFrameDropped[7]);
    ui->replyText->append("\t\t\t~.~\n");
}
//_________________________________________________________________________________________
void MainWindow::nominalReply()
{
    initDone = true;
    char ch[150];
    QString tempo;
    bool ok;
    ui->replyText->append("~~~~~~~~~~~~~~~~~~~~>> Nominal Reply <<~~~~~~~~~~~~~~~~~~~~");
    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("hh:mm:ss   of   yyyy.MM.dd");
    ui->replyText->append("Received at " + dateTimeString);
    ulong fromHex = wordsQBA[0].toULong (&ok, 16);
    ulong problematicReq = 0;
    if (!ok)
        qDebug() <<"NOT OK::Decimal value: " << fromHex << " "<<ok;
    sprintf(ch, "Request Id: %lu", fromHex);
    tempo.append(ch);
    tempo = tempo + "    Header: "+wordsQBA[1] +", "+wordsQBA[2] +", "+wordsQBA[3];
    ui->replyText->append(tempo);
    tempo = "";
    ui->replyText->append("\t     Error\t|          Data    \t\t     Error\t|          Data");
    for (int i = 4; i<packageSizeGl; ){
        tempo = tempo+"\t"+wordsQBA[i] +"\t|     "+wordsQBA[i+1];
        if (wordsQBA[i] != "00000000") {initDone = false; problematicReq = fromHex;}
        if ((i-2)%4 == 0 || i == (packageSizeGl-2)) {ui->replyText->append(tempo); tempo = "";}
        i=i+2;
    }
    ui->replyText->append("\t\t\t~.~\n");
    if (!initDone){
        ui->replyText->append("**********************************************************************");
        sprintf(ch, "*       Problem with initialization sent with request Id: %lu   \t*", fromHex);
        ui->replyText->append(ch);
        ui->replyText->append("**********************************************************************");

    }
}
//_________________________________________________________________________________________
void MainWindow::on_selectAllChannels_clicked(bool checked)
{
    for(int i=0; i<8; i++)
        channels[i]->setChecked(checked);
}
//_________________________________________________________________________________________
void MainWindow::on_internalTrg_clicked()
{
    if(bnd){
        bool oktrg=0;
        QString address = "00000000";
        QString value = "00000000";
        chipRegisterType[1]->setChecked(1);
        ui->internalTrg->setStyleSheet("background-color: green");
        ui->externalTrg->setStyleSheet("background-color: gray");
        addr[0] = (quint32)address.toUInt(&oktrg,2);
        vars[0] = (quint32)value.toUInt(&oktrg,2);
        maxIter=1;
        Merger(false);
    }
}
//_________________________________________________________________________________________
void MainWindow::on_externalTrg_clicked()
{
    if(bnd){
        bool oktrg=0;
        QString address = "00000000";
        QString value = "00000100";
        chipRegisterType[1]->setChecked(1);
        ui->externalTrg->setStyleSheet("background-color: green");
        ui->internalTrg->setStyleSheet("background-color: gray");
        addr[0] = (quint32)address.toUInt(&oktrg,2);
        vars[0] = (quint32)value.toUInt(&oktrg,2);
        maxIter=1;
        Merger(false);
    }
}
//_________________________________________________________________________________________
void MainWindow::on_onAcq_clicked()
{
    if(bnd){
        bool oktrg=0;
        QString address = "00001111";
        QString value = "00000001";
        chipRegisterType[1]->setChecked(1);
        ui->onAcq->setStyleSheet("background-color: green");
        ui->offAcq->setStyleSheet("background-color: gray");
        addr[0] = (quint32)address.toUInt(&oktrg,2);
        vars[0] = (quint32)value.toUInt(&oktrg,2);
        maxIter=1;
        Merger(false);
    }
}
//_________________________________________________________________________________________
void MainWindow::on_offAcq_clicked()
{
    if(bnd){
        bool oktrg=0;
        QString address = "00001111";
        QString value = "00000000";
        chipRegisterType[1]->setChecked(1);
        ui->offAcq->setStyleSheet("background-color: green");
        ui->onAcq->setStyleSheet("background-color: gray");
        addr[0] = (quint32)address.toUInt(&oktrg,2);
        vars[0] = (quint32)value.toUInt(&oktrg,2);
        maxIter=1;
        Merger(false);
    }
}
//_________________________________________________________________________________________
void MainWindow::on_resetAPVs_clicked()
{
    if(bnd){
        bool oktrg=0;
        QString address = "11111111111111111111111111111111";
        QString value = "00000001";
        chipRegisterType[1]->setChecked(1);
        addr[0] = (quint32)address.toUInt(&oktrg,2);
        vars[0] = (quint32)value.toUInt(&oktrg,2);
        maxIter=1;
        Merger(false);
    }
}
//_________________________________________________________________________________________
void MainWindow::openFile()
{
    quint16 outPort;
    quint16 outIdentifier;
    quint64 sizeOfFile=0;
    filename = QFileDialog::getOpenFileName(
                this,
                tr("Open Document"),
                QDir::currentPath(),
                tr("Document files (*.txt);;(*.*);;(*)"),0,QFileDialog::DontUseNativeDialog );
    if( !filename.isNull() )
    {
        QFile file(filename);
        sizeOfFile = file.size();
        if(sizeOfFile==368){
            if(file.open(QIODevice::ReadOnly)){
                QDataStream in(&file);
                in >> outPort;
                in >> outIdentifier;
                if ((outPort==6519)&&(outIdentifier==1)){
                    in.device()->seek(76);
                    in >> outPort;
                    in >> outIdentifier;
                    if ((outPort==6263)&&(outIdentifier==2)){
                        in.device()->seek(76+148);
                        in >> outPort;
                        in >> outIdentifier;
                        if ((outPort==6039)&&(outIdentifier==3)){
                            in.device()->seek(76+148+108);
                            in >> outPort;
                            in >> outIdentifier;
                            if ((outPort==6263)&&(outIdentifier==4)){
                                QMessageBox msgBox;
                                msgBox.setText("File Succsefully Loaded");
                                msgBox.exec();
                                ui->confFile->setText(filename);
                                file.close();
                            }
                        }
                    }
                }
            }
        }else{
            QMessageBox msgBox;
            msgBox.setText("Error: File seems not to be a Full SDC Recipe file");
            msgBox.setInformativeText("Load a file which is a full configuration file");
            msgBox.exec();
            ui->confFile->setText("Error");
        }
    }
}
//_________________________________________________________________________________________
void MainWindow::saveFile(){
    filename = QFileDialog::getSaveFileName(
                this,
                tr("Save Document"),
                QDir::currentPath(),
                tr("Documents (*.txt);;(*)") );
    if( !filename.isNull() )
    {
        if (chipRegisterType[0]->isChecked()){
            applyApvForFile();
        }else if(chipRegisterType[3]->isChecked()){
            applyPLLForFile();
        }
        else if(chipRegisterType[1]->isChecked()){
            applyAPVApplicationRegistersForFile();
        }
        else if (chipRegisterType[2]->isChecked()){
            applyAdcForFile();
        }
        Merger(true);
    }
}
//_________________________________________________________________________________________
void MainWindow::writeDataToFile(QByteArray dataToWrite, quint16 portToSave, quint16 identifierForFile){

    qDebug()<<dataToWrite.toHex();
    QFile file(filename);
    quint16 outPort;
    quint16 outIdentifier;
    quint64 sizeOfFile=0;
    if (file.exists(filename)){
        sizeOfFile = file.size();
        QDataStream in(&file);
        if(sizeOfFile>360){
            if(file.open(QIODevice::ReadWrite)){
                if (identifierForFile==1){
                    in.device()->seek(0);
                    in >> outPort;
                    in >> outIdentifier;
                    if ((outPort==portToSave)&&(outIdentifier==identifierForFile)){
                        in.device()->seek(0);
                        in<<dataToWrite;
                        in.device()->seek(0);
                        in<<portToSave<<identifierForFile;
                        QMessageBox msgBox;
                        msgBox.setText("ADC Configuration Saved");
                        msgBox.exec();
                        file.close();
                    }
                }else if (identifierForFile==2){
                    in.device()->seek(76);
                    in >> outPort;
                    in >> outIdentifier;
                    if ((outPort==portToSave)&&(outIdentifier==identifierForFile)){
                        in.device()->seek(76);
                        in<<dataToWrite;
                        in.device()->seek(76);
                        in<<portToSave<<identifierForFile;
                        QMessageBox msgBox;
                        msgBox.setText("APV Hybrid Configuration Saved");
                        msgBox.exec();
                        file.close();
                    }
                }else if (identifierForFile==3){
                    in.device()->seek(76+148);
                    in >> outPort;
                    in >> outIdentifier;
                    if ((outPort==portToSave)&&(outIdentifier==identifierForFile)){
                        in.device()->seek(76+148);
                        in<<dataToWrite;
                        in.device()->seek(76+148);
                        in<<portToSave<<identifierForFile;
                        QMessageBox msgBox;
                        msgBox.setText("APV Applications Configuration Saved");
                        msgBox.exec();
                        file.close();
                    }
                }else if (identifierForFile==4){
                    in.device()->seek(76+148+108);
                    in >> outPort;
                    in >> outIdentifier;
                    if ((outPort==portToSave)&&(outIdentifier==identifierForFile)){
                        in.device()->seek(76+148+108);
                        in<<dataToWrite;
                        in.device()->seek(76+148+108);
                        in<<portToSave<<identifierForFile;
                        file.close();
                        QMessageBox msgBox;
                        msgBox.setText("PLL Configuration Saved");
                        msgBox.exec();
                    }
                }else{
                    QMessageBox msgBox;
                    msgBox.setText("Error : Not a valid identifier");
                    qDebug() << outIdentifier;
                    msgBox.exec();
                    file.close();
                }
            }
        }else{
            if(file.open(QIODevice::ReadWrite | QIODevice::Append)){
                QDataStream in(&file);
                in<<portToSave<<identifierForFile;
                file.write(dataToWrite);
                file.close();
            }
        }
    }else{
        if(file.open(QIODevice::ReadWrite | QIODevice::Append)){
            QDataStream in(&file);
            in<<portToSave<<identifierForFile;
            file.write(dataToWrite);
            file.close();
        }
    }
}
//_________________________________________________________________________________________
void MainWindow::on_enableResets_clicked(bool checked)
{
    if(checked){
//        QMessageBox msgBox;
//        msgBox.setText("This will enable reset commands for the FEC");
//        msgBox.setInformativeText("Are you sure you want to continue?");
//        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
//        msgBox.setDefaultButton(QMessageBox::Cancel);
//        int ret =msgBox.exec();
//        switch (ret) {
//        case QMessageBox::Yes:
//            ui->warmInit->setEnabled(checked);
//            ui->forceReboot->setEnabled(checked);
//            break;
//        case QMessageBox::Cancel:
            ui->enableResets->setChecked(1);
            ui->warmInit->setEnabled(1);
            ui->forceReboot->setEnabled(1);
//            break;
//        }
    }else{
        ui->enableResets->setChecked(false);
        ui->warmInit->setEnabled(false);
        ui->forceReboot->setEnabled(false);
    }
}
//_________________________________________________________________________________________
void MainWindow::on_warmInit_clicked()
{
    if(bnd){
        bool okres=0;
        QString address = "FFFFFFFF";
        QString value = "FFFF0001";
        chipRegisterType[1]->setChecked(1);
        addr[0] = (quint32)address.toUInt(&okres,16);
        if (okres)
            vars[0] = (quint32)value.toUInt(&okres,16);
        if (okres)
            maxIter=1;
        Merger(false);
    }
    ui->enableResets->setChecked(false);
    ui->warmInit->setEnabled(false);
    ui->forceReboot->setEnabled(false);
}
//_________________________________________________________________________________________
void MainWindow::on_forceReboot_clicked()
{
    if(bnd){
        bool okres=0;
        QString address = "FFFFFFFF";
        QString value = "FFFF8000";
        chipRegisterType[1]->setChecked(1);
        addr[0] = (quint32)address.toUInt(&okres,16);
        if (okres)
            vars[0] = (quint32)value.toUInt(&okres,16);
        if(okres)
            maxIter=1;
        Merger(false);
    }
    ui->enableResets->setChecked(false);
    ui->warmInit->setEnabled(false);
    ui->forceReboot->setEnabled(false);
}
//_________________________________________________________________________________________
void MainWindow::SendConfFileFunction(){
    if(bnd){
        command[0]->setChecked(1);
        quint16 outPort;
        quint16 outIdentifier;
        quint32 intermediatePacket;
        QString LoadedFile = ui->confFile->text();
        if(!LoadedFile.isNull()){
            if(LoadedFile!="Error"){
                QFile file(LoadedFile);
                QDataStream in(&file);
                QByteArray datagramOut;
                QDataStream out(&datagramOut, QIODevice::WriteOnly);
                if(file.open(QIODevice::ReadOnly)){
                    in >> outPort;
                    in >> outIdentifier;
                    for(int i=1; i<=18; i++){
                        in >>intermediatePacket;
                        out<<intermediatePacket;
                    }
                    if(countAll==1)
                        Sender(datagramOut, outPort);
                    //***********************************************************************
                    in.device()->seek(76);
                    datagramOut.clear();
                    out.device()->seek(0);
                    in >> outPort;
                    in >> outIdentifier;
                    for(int i=1; i<=36; i++){
                        in>>intermediatePacket;
                        out<<intermediatePacket;
                    }
                    if(countAll==2)
                        Sender(datagramOut, outPort);
                    //***********************************************************************
                    in.device()->seek(76+148);
                    datagramOut.clear();
                    out.device()->seek(0);
                    in >> outPort;
                    in >> outIdentifier;
                    for(int i=1; i<=26; i++){
                        in>>intermediatePacket;
                        out<<intermediatePacket;
                    }
                    if(countAll==3)
                        on_resetAPVs_clicked();
                    if(countAll==4)
                        Sender(datagramOut, outPort);
                    //***********************************************************************
                    in.device()->seek(76+148+108);
                    datagramOut.clear();
                    out.device()->seek(0);
                    in >> outPort;
                    in >> outIdentifier;
                    for(int i=1; i<=8; i++){
                        in>>intermediatePacket;
                        out<<intermediatePacket;
                    }
                    if(countAll==5)
                        Sender(datagramOut, outPort);
                    file.close();
                    if(countAll>5){
                        QMessageBox msgBox;
                        msgBox.setText("Finished");
                        msgBox.exec();
                        timer->stop();
                    }
                }
            }
            countAll++;
        }
    }else {
        QMessageBox msgBox;
        msgBox.setText("Socket is not Open");
        msgBox.exec();
    }
}
//_________________________________________________________________________________________
void MainWindow::loadRecipeFromFile(){
    QString Peripheral, valToAdd;
    int maxNum = 0;
    quint64 seekNum = 0;
    quint32 tempAddress;
    quint32 tempValue;
    if (chipRegisterType[0]->isChecked()){
        Peripheral = "APVH";
        maxNum = 16;
        seekNum = 76+20;
    }else if(chipRegisterType[3]->isChecked()){
        Peripheral = "PLL";
        maxNum = 2;
        seekNum = 76+148+108+20;
    }
    else if(chipRegisterType[1]->isChecked()){
        Peripheral = "APVA";
        maxNum = 11;
        seekNum = 76+148+20;
    }
    else if (chipRegisterType[2]->isChecked()){
        Peripheral = "ADC";
        maxNum = 7;
        seekNum = 20;
    }
    QString LoadedFile = ui->confFile->text();
    if(LoadedFile!=""){
        if(LoadedFile!="Error"){
            QFile file(LoadedFile);
            QDataStream in(&file);
            if(file.open(QIODevice::ReadOnly)){
                in.device()->seek(seekNum);
                for(int i=0; i<maxNum; i++){
                    if(Peripheral=="ADC"){
                        in>>tempAddress;
                        in>>tempValue;
                        valToAdd = QString::number(tempValue,16);
                        ADCValues[i]->setText(valToAdd);
                        inputDataText->setText("Data Settings From Recipe File");
                    }else if(Peripheral=="PLL"){
                        in>>tempAddress;
                        in>>tempValue;
                        valToAdd = QString::number(tempValue,16);
                        PLLValues[i]->setText(valToAdd);
                        inputDataText->setText("Data Settings From Recipe File");
                    }else if(Peripheral=="APVA"){
                        in>>tempAddress;
                        in>>tempValue;
                        if((i==0)||(i==6))
                            valToAdd = QString::number(tempValue,2);
                        else
                            valToAdd = QString::number(tempValue);
                        valuesAPVApplicationRegisters[i]->setText(valToAdd);
                        inputDataText->setText("Data Settings From Recipe File");
                    }else if(Peripheral=="APVH"){
                        in>>tempAddress;
                        in>>tempValue;
                        if((i==0)||(i==2)||(i==14)||(i==15))
                            valToAdd = QString::number(tempValue,2);
                        else
                            valToAdd = QString::number(tempValue);
                        APVValues[i]->setText(valToAdd);
                        inputDataText->setText("Data Settings From Recipe File");
                    }else {
                        QMessageBox msgBox;
                        msgBox.setText("Didn't find such register type in file");
                        msgBox.exec();
                    }
                }
                labelFromFile=true;
            }
            file.close();
        }
    }else{
        QMessageBox msgBox1;
        msgBox1.setText("No file loaded");
        QPushButton *LButton = msgBox1.addButton(tr("Load"), QMessageBox::ActionRole);
        QPushButton *CButton = msgBox1.addButton(QMessageBox::Cancel);
        msgBox1.exec();
        if (msgBox1.clickedButton() == LButton) {
            openFile();
        }else{
            CButton->setFocus();
        }
    }
}
//_________________________________________________________________________________________
void MainWindow::on_clearFilePB_clicked()
{
    ui->confFile->setText("");
}
//_________________________________________________________________________________________
void MainWindow::on_saveLogPB_clicked()
{
    QString logFileStr = QFileDialog::getSaveFileName(
                this,
                tr("Save Document"),
                QDir::currentPath(),
                tr("Documents (*.txt);;(*.*);;(*)") );
    QFile logFile(logFileStr);
    QString replyLog = ui->replyText->toPlainText();
    if(logFile.open(QIODevice::ReadWrite | QIODevice::Text)){
        QTextStream input(&logFile);
        input<<replyLog;
    }
    logFile.close();

}
//_________________________________________________________________________________________
void MainWindow::sleep() {
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(SendConfFileFunction()));
    countAll=1;
    timer->start(2000);
}
//_________________________________________________________________________________________
void MainWindow::sleep2() {
    timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(PrintValuesFromHardware()));
    timer2->start(2000);
}
//_________________________________________________________________________________________
void MainWindow::EnableExpertMode()
{
    if(ui->enableExperModeButton->isChecked()){
//        QMessageBox msgBox;
//        msgBox.setText("Are you sure you want to enable the expert mode?");
//        msgBox.setInformativeText("This will enable advanced configuration control");
//        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
//        msgBox.setDefaultButton(QMessageBox::Cancel);
//        ui->sendPB->setEnabled(1);
//        int ret =msgBox.exec();
//        switch (ret) {
//        case QMessageBox::Yes:
            this->setFixedSize(833,643);
//            break;
//        case QMessageBox::Cancel:
//            ui->enableExperModeButton->setChecked(0);
//            break;
//        }
    }else{
        this->setFixedSize(258,620);
        command[0]->setChecked(1);
    }
}
//_________________________________________________________________________________________
void MainWindow::ReadValuesFromHardware(){
    command[2]->setChecked(1);
    closeWidget=0;
    if (chipRegisterType[0]->isChecked()){
        applyApv();
        sleep2();
    }else if(chipRegisterType[1]->isChecked()){
        applyAPVApplicationRegisters();
        sleep2();
    }
    else if(chipRegisterType[2]->isChecked()){
        applyAdc();
        sleep2();
    }
    else if (chipRegisterType[3]->isChecked()){
        applyPLL();
        sleep2();
    }
    command[0]->setChecked(1);
    closeWidget=1;
}
//_________________________________________________________________________________________
void MainWindow::PrintValuesFromHardware(){
    if(initDone){
        if (chipRegisterType[0]->isChecked()){
            for(int i=0;i<maxIter;i++){
                valuesAPVApplicationRegistersFromHW[maxIter]->setText(wordsQBA[(2*i)+5]);
            }
        }else if(chipRegisterType[1]->isChecked()){
            for(int i=0;i<maxIter;i++){
                APVValuesFromHW[maxIter]->setText(wordsQBA[(2*i)+5]);
            }
        }
        else if(chipRegisterType[2]->isChecked()){
            for(int i=0;i<maxIter;i++){
                ADCValuesFromHW[maxIter]->setText(wordsQBA[(2*i)+5]);
            }
        }
        else if (chipRegisterType[3]->isChecked()){
            for(int i=0;i<maxIter;i++){
                PLLValuesFromHW[maxIter]->setText(wordsQBA[(2*i)+5]);
            }
        }
    }else{
        QMessageBox msgBox;
        msgBox.setText("No Responce from the Hardware");
        msgBox.setStandardButtons(QMessageBox::Ok);
        int ret =msgBox.exec();
        switch (ret) {
        case QMessageBox::Ok:
            break;
        }
    }
    timer2->stop();
}
//_________________________________________________________________________________________
void MainWindow::applyThreshold(){
    chipRegisterType[1]->setChecked(1);
    uint threshold = ui->zeroSuppressionThreshold->value();
    QString forcePedZero,forceSignalDetection;
    bool okchange;
    QString thresholdString;
    thresholdString.setNum(threshold,2);
    if (ui->forcePedestalZeroCheck->isChecked()) forcePedZero = "1";
    else forcePedZero = "0";
    if(ui->forceSignalCheck->isChecked()) forceSignalDetection = "1";
    else forceSignalDetection = "0";

    QString finalThresValue =thresholdString;
    finalThresValue.insert(0,forcePedZero);
    finalThresValue.insert(0,forceSignalDetection);
    finalThresValue.insert(0,"00");
    if(bnd){
        QString address = "00010100";
        chipRegisterType[1]->setChecked(1);
        addr[0] = (quint32)address.toUInt(&okchange,2);
        if (okchange){
            vars[0] = (quint32)finalThresValue.toUInt(&okchange,2);
            maxIter=1;
        }
        if (okchange){
            Merger(false);
            QMessageBox msgBox;
            msgBox.setText("Done");
            msgBox.exec();
        }
    }

}
//_________________________________________________________________________________________
void MainWindow::startPedestal(){
    if(bnd){
        bool okres=0;
        QString address = "00011111";
        QString value = "00000000";
        chipRegisterType[1]->setChecked(1);
            addr[0] = (quint32)address.toUInt(&okres,2);
        if (okres)
            addr[1] = (quint32)address.toUInt(&okres,2);
        if (okres)
            addr[2] = (quint32)address.toUInt(&okres,2);
        if (okres)
            vars[0] = (quint32)value.toUInt(&okres,2);
        value = "00000010";
        if (okres)
            vars[1] = (quint32)value.toUInt(&okres,2);
        value = "00000000";
        if (okres)
            vars[2] = (quint32)value.toUInt(&okres,2);
        if(okres)
            maxIter=3;
        Merger(false);
    }
}
//_________________________________________________________________________________________
void MainWindow::bypassOn(){
    if(bnd){
        bool okres=0;
        QString address = "00011111";
        QString value = "00001111";
        chipRegisterType[1]->setChecked(1);
            addr[0] = (quint32)address.toUInt(&okres,2);
        if (okres)
            vars[0] = (quint32)value.toUInt(&okres,2);
        if (okres)
            maxIter=1;
        Merger(false);
    }
}
//_________________________________________________________________________________________
void MainWindow::bypassOff(){
    if(bnd){
        bool okres=0;
        QString address = "00011111";
        QString value = "00000000";
        chipRegisterType[1]->setChecked(1);
            addr[0] = (quint32)address.toUInt(&okres,2);
        if (okres)
            vars[0] = (quint32)value.toUInt(&okres,2);
        if (okres)
            maxIter=1;
        Merger(false);
    }
}
//_________________________________________________________________________________________
void MainWindow::resetAPVFunction(){
    if(bnd){
        bool okres=0;
        QString address = "00011111";
        QString value = "11111111";
        chipRegisterType[1]->setChecked(1);
            addr[0] = (quint32)address.toUInt(&okres,2);
        if (okres)
            vars[0] = (quint32)value.toUInt(&okres,2);
        if (okres)
            maxIter=1;
        Merger(false);
    }
}
//_________________________________________________________________________________________

void MainWindow::selectAPVForBypassFunction(){
    if(bnd){
        bool okres=0;
        QString address = "00010010";
        uint apvForbypass = ui->apvSelectForBypass->value();
        QString value;
        value.setNum(apvForbypass,2);
        chipRegisterType[1]->setChecked(1);
            addr[0] = (quint32)address.toUInt(&okres,2);
        if (okres)
            vars[0] = (quint32)value.toUInt(&okres,2);
        if (okres)
            maxIter=1;
        Merger(false);
    }
}
