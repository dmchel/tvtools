#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class LedRecordModel;
class CustomRecordDelegate;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void setConnectionStatus(bool fConnected);
    void setConnectionInfo(const QJsonObject &info);

signals:
    void requestConnection(const QString &name);
    void dropConnection();

private slots:
    void onConnectionBoxCheck(bool fChecked);

private:
    void initConnectionTab();
    void initLedControlTab();

private:
    Ui::MainWindow *ui;
    LedRecordModel *model;
};

#endif // MAINWINDOW_H
