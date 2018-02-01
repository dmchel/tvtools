#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "recordModel/ledrecordmodel.h"
#include "recordModel/datetimedelegate.h"

#include <QSerialPortInfo>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initConnectionTab();
    initLedControlTab();
    initDemoTab();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setConnectionStatus(bool fConnected)
{
    ui->connectCheckBox->setChecked(fConnected);
    if(!fConnected) {
        ui->onlineLabel->setText(tr("TabVision OFFLINE"));
    }
}

void MainWindow::setConnectionInfo(const QJsonObject &info)
{
    if(info.isEmpty()) {
        return;
    }

    if(info.contains("fOnline") && info["fOnline"].isBool()) {
        bool fOnline = info["fOnline"].toBool();
        if(fOnline) {
            ui->onlineLabel->setText(tr("TabVision ONLINE"));
        }
        else {
            ui->onlineLabel->setText(tr("TabVision OFFLINE"));
        }
    }

    if(info.contains("txPacks") && info["txPacks"].isDouble()) {
        quint32 txPacks = info["txPacks"].toInt();
        ui->txPackCounter->setText(tr("txPacks: ") + QString().setNum(txPacks));
    }

    if(info.contains("rxPacks") && info["rxPacks"].isDouble()) {
        quint32 rxPacks = info["rxPacks"].toInt();
        ui->rxPackCounter->setText(tr("rxPacks: ") + QString().setNum(rxPacks));
    }

    if(info.contains("txBytes") && info["txBytes"].isDouble()) {
        quint32 txBytes = info["txBytes"].toInt();
        ui->txByteCounter->setText(tr("txBytes: ") + QString().setNum(txBytes));
    }

    if(info.contains("rxBytes") && info["rxBytes"].isDouble()) {
        quint32 rxBytes = info["rxBytes"].toInt();
        ui->rxByteCounter->setText(tr("rxBytes: ") + QString().setNum(rxBytes));
    }

    if(info.contains("errors") && info["errors"].isDouble()) {
        quint32 errors = info["errors"].toInt();
        ui->errorCounter->setText(tr("Errors: ") + QString().setNum(errors));
    }
}

/**
 * Private methods
 */

void MainWindow::onConnectionBoxCheck(bool fChecked)
{
    ui->connectCheckBox->setChecked(false);
    if(ui->portComboBox->currentText().isEmpty()) {
        return;
    }
    if(fChecked) {
        emit requestConnection(ui->portComboBox->currentText());
    }
    else {
        emit dropConnection();
    }
}

void MainWindow::onDemoRadioButtonCheck(bool fChecked)
{
    if(fChecked) {
        if(ui->randomRadioButton->isChecked()) {
            emit demo(0, ui->randomSpinBox->value());
        }
        else if(ui->stringRadioButton->isChecked()) {
            emit demo(1, ui->stringSpinBox->value());
        }
        else if(ui->fretRadioButton->isChecked()) {
            emit demo(2, ui->fretSpinBox->value());
        }
    }
}

void MainWindow::onPlayButtonClick()
{
    if(ui->playButton->text().startsWith('P')) {
        emit play();
        ui->playButton->setText("Stop");
    }
    else if(ui->playButton->text().startsWith('S')) {
        emit stop();
        ui->playButton->setText("Play");
    }
}

void MainWindow::initConnectionTab()
{
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    for(auto item : portList) {
        ui->portComboBox->addItem(item.portName());
    }
    connect(ui->connectCheckBox, &QCheckBox::clicked, this, &MainWindow::onConnectionBoxCheck);
}

void MainWindow::initLedControlTab()
{
    CustomRecordDelegate *myDelegate = new CustomRecordDelegate(this);
    ui->ledTableView->setItemDelegate(myDelegate);

    model = new LedRecordModel(this);

    LedRecordItem item;
    item.num = 0;
    item.timestamp = 1000;
    item.duration = 500;
    item.ledColor = "red";
    item.ledData = "0xxx";

    for(int i = 0; i < 10; i++) {
        item.num++;
        model->addRecord(item);
    }

    ui->ledTableView->setModel(model);

    connect(ui->playButton, &QPushButton::clicked, this, &MainWindow::onPlayButtonClick);
    connect(ui->pauseButton, &QPushButton::clicked, this, &MainWindow::pause);
    connect(ui->brightSlider, &QSlider::sliderMoved, this, &MainWindow::brightness);
}

void MainWindow::initDemoTab()
{
    connect(ui->randomRadioButton, &QRadioButton::clicked, this, &MainWindow::onDemoRadioButtonCheck);
    connect(ui->stringRadioButton, &QRadioButton::clicked, this, &MainWindow::onDemoRadioButtonCheck);
    connect(ui->fretRadioButton, &QRadioButton::clicked, this, &MainWindow::onDemoRadioButtonCheck);
}
