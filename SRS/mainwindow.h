#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QGroupBox;
class QLabel;
class QRadioButton;
class QCheckBox;
class QUdpSocket;
class QLineEdit;
class QBitArray;
QT_END_NAMESPACE

namespace Ui {
    class MainWindow;
}


class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    bool closeWidget;
    bool labelFromFile;
    QLabel *inputDataText;
    QWidget *ad;
    QWidget *ap;
    QWidget *pll;
    QWidget *APVAp;
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    int packageSizeGl;
    int reqIDCounter;
    quint32 reqId32, sudAdd32, cmd32, cmdInfo32, dataAdd32[15], data32[15];
    QGroupBox *groupBox;
    QRadioButton *radio[4];
    QRadioButton *command[5];
    QRadioButton *chipRegisterType[4];
    QCheckBox *channels[8];
    QCheckBox *SlaveChannels[8];
    QCheckBox *MasterChannels[8];
    QCheckBox *modifyAPVRegisters[16];
    QCheckBox *sendAPVRegisters[16];
    QLineEdit *APVAddresses[16];
    QLineEdit *APVValuesFromHW[16];
    QLineEdit *APVValues[16];
    QCheckBox *modifyADCRegisters[7];
    QCheckBox *sendADCRegisters[7];
    QLineEdit *ADCAddresses[7];
    QLineEdit *ADCValues[7];
    QLineEdit *ADCValuesFromHW[7];
    QCheckBox *modifyPLLRegisters[2];
    QCheckBox *sendPLLRegisters[2];
    QLineEdit *PLLAddresses[2];
    QLineEdit *PLLValues[2];
    QLineEdit *PLLValuesFromHW[2];
    QCheckBox *modifyAPVApplicationRegisters[11];
    QCheckBox *sendAPVApplicationRegisters[11];
    QLineEdit *addressesAPVApplicationRegisters[11];
    QLineEdit *valuesAPVApplicationRegisters[11];
    QLineEdit *valuesAPVApplicationRegistersFromHW[11];
    QUdpSocket *socket;
    QLineEdit *apvPort;
    QLineEdit *adcPort;
    QLineEdit *APVApplicationRegistersPort;
    bool bnd, isOk2Send;
    quint32 addr[16], vars[16];
    quint32 *lnEdt;
    quint16 APVRegPort;
    quint16 ADCRegPort;
    void on_pushButton_clicked();
    void writeDataToFile(QByteArray datatowrite, quint16 portToSave, quint16 identifierForFile);
    QString filename;
    int countAll;
    QTimer *timer;
    QTimer *timer2;

    //^^^^^^^^^^//

private slots:
    void EnableExpertMode();
    void on_saveLogPB_clicked();
    void on_clearFilePB_clicked();
    void on_forceReboot_clicked();
    void on_warmInit_clicked();
    void on_enableResets_clicked(bool checked);
    void on_resetAPVs_clicked();
    void on_offAcq_clicked();
    void on_onAcq_clicked();
    void on_externalTrg_clicked();
    void on_internalTrg_clicked();
    void on_selectAllChannels_clicked(bool checked);
    void on_clearLogPB_clicked();
    void APVConfigure();
    void on_connectPB_clicked();
    void Merger(bool source);
    void Sender(QByteArray datagram, quint16 port);
    void dataPending();
    void setAPVDefaults();
    void setADCDefaults();
    void setPLLDefaults();
    void RadioConfigureRegisters();
    void ADCRegisters();
    void PLLRegisters();
    void setAPVApplicationRegistersDefaults();
    void APVApplicationRegistersConfig();
    void configurePB();
    void openFile();
    void saveFile();


public slots:
    void ReadValuesFromHardware();
    void PrintValuesFromHardware();
    void SendConfFileFunction();
    void editMask();
    void editMaskBox();
    void sleep();
    void sleep2();
    void applyApv();
    void applyPLL();
    void applyAdc();
    void applyApvForFile();
    void applyPLLForFile();
    void applyAdcForFile();
    void Connect();
    void Disconnect();
    void applyAPVApplicationRegisters();
    void applyAPVApplicationRegistersForFile();
    void loadRecipeFromFile();
    void EnableMasks();
    void applyThreshold();
    void startPedestal();
    void bypassOn();
    void bypassOff();
    void resetAPVFunction();
    void selectAPVForBypassFunction();
protected:
    void changeEvent(QEvent *e);
    void createRadioGroupMode();
    void createRadioGroupPeripheral();
    void createRadioGroupChannel();
    void createRadioGroupCommand();
    void createResetButtons();
    int packageHandler(QByteArray package);
    QBitArray toBitConverter(QByteArray wordsQBA);
    void Listener();
    void frameDropped();
    void nominalReply();

private:
    QString words[150];
    QByteArray wordsQBA[150];
    bool initDone;
    int maxIter;
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
