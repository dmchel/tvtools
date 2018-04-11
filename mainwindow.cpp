#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "recordModel/ledrecordmodel.h"
#include "recordModel/datetimedelegate.h"

#include <QSerialPortInfo>
#include <QJsonObject>
#include <QMenu>
#include <QAction>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    tableMenu = new QMenu(ui->ledTableView);
    QAction *addTaskAction = new QAction(tr("Add new task"), ui->ledTableView);
    //addTaskAction->setShortcut(tr("Alt+N"));
    connect(addTaskAction, &QAction::triggered, this, &MainWindow::onAddTaskTableAction);
    tableMenu->addAction(addTaskAction);
    QAction *removeTaskAction = new QAction(tr("Remove task"), ui->ledTableView);
    //removeTaskAction->setShortcut(tr("Ctrl+D"));
    connect(removeTaskAction, &QAction::triggered, this, &MainWindow::onRemoveTaskTableAction);
    tableMenu->addAction(removeTaskAction);
    QAction *clearAction = new QAction(tr("Clear table"), ui->ledTableView);
    //clearAction->setShortcut(tr("Ctrl+L"));
    connect(clearAction, &QAction::triggered, this, &MainWindow::onClearTableAction);
    tableMenu->addAction(clearAction);

    initConnectionTab();
    initLedControlTab();
    initDemoTab();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setLedModel(LedRecordModel *lm)
{
    if(lm) {
        model = lm;
        ui->ledTableView->setModel(model);
    }
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

void MainWindow::contextMenuEvent(QContextMenuEvent *e)
{
    if(ui->tabWidget->currentIndex() == 1) {
        tableMenu->exec(e->globalPos());
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
            emit demo(1, ui->randomSpinBox->value());
        }
        else if(ui->stringRadioButton->isChecked()) {
            emit demo(2, ui->stringSpinBox->value());
        }
        else if(ui->fretRadioButton->isChecked()) {
            emit demo(3, ui->fretSpinBox->value());
        }
        else if(ui->redFillRadioButton->isChecked()) {
            emit demo(4, ui->redFillSpinBox->value());
        }
        else if(ui->yellowFillRadioButton->isChecked()) {
            emit demo(5, ui->yellowFillSpinBox->value());
        }
        else if(ui->snakeRadioButton->isChecked()) {
            emit demo(6, ui->snakeSpinBox->value());
        }
        else {
            emit demo(0, 0);
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

void MainWindow::onAddTaskTableAction()
{
    if(model) {
        model->addRecord(LedRecordItem(model->rowCount() + 1, 0, 0, "red", "xxxx"));
    }
}

void MainWindow::onRemoveTaskTableAction()
{
    if(model == Q_NULLPTR) {
        return;
    }
    QModelIndexList selectionList = ui->ledTableView->selectionModel()->selectedIndexes();
    if(!selectionList.isEmpty()) {
        for(auto select : selectionList) {
            model->removeRecord(select.row());
        }
    }
}

void MainWindow::onClearTableAction()
{
    if(model) {
        model->removeAll();
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
    ui->brightSlider->setTracking(true);

    connect(ui->playButton, &QPushButton::clicked, this, &MainWindow::onPlayButtonClick);
    connect(ui->pauseButton, &QPushButton::clicked, this, &MainWindow::pause);
    connect(ui->brightSlider, &QSlider::sliderMoved, this, &MainWindow::brightness);
    connect(ui->addTaskButton, &QPushButton::clicked, this, &MainWindow::onAddTaskTableAction);
}

void MainWindow::initDemoTab()
{
    connect(ui->randomRadioButton, &QRadioButton::clicked, this, &MainWindow::onDemoRadioButtonCheck);
    connect(ui->stringRadioButton, &QRadioButton::clicked, this, &MainWindow::onDemoRadioButtonCheck);
    connect(ui->fretRadioButton, &QRadioButton::clicked, this, &MainWindow::onDemoRadioButtonCheck);
    connect(ui->redFillRadioButton, &QRadioButton::clicked, this, &MainWindow::onDemoRadioButtonCheck);
    connect(ui->yellowFillRadioButton, &QRadioButton::clicked, this, &MainWindow::onDemoRadioButtonCheck);
    connect(ui->snakeRadioButton, &QRadioButton::clicked, this, &MainWindow::onDemoRadioButtonCheck);
    connect(ui->demoOffRadioButton, &QRadioButton::clicked, this, &MainWindow::onDemoRadioButtonCheck);
}
