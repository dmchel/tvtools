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

    void setLedModel(LedRecordModel *lm);

public slots:
    void setConnectionStatus(bool fConnected);
    void setConnectionInfo(const QJsonObject &info);
    void setDebugMessage(const QString &mess);
    void setDebugData(const QByteArray &data);

signals:
    void requestConnection(const QString &name);
    void dropConnection();

    void play();
    void stop();
    void pause();
    void brightness(int value);
    void demo(int num, int step);

protected:
    virtual void contextMenuEvent(QContextMenuEvent *e);

private slots:
    void onConnectionBoxCheck(bool fChecked);
    void onDemoRadioButtonCheck(bool fChecked);
    void onDemoPeriodChanged(int period);
    void onPlayButtonClick();
    void onAddTaskTableAction();
    void onRemoveTaskTableAction();
    void onClearTableAction();

private:
    void initConnectionTab();
    void initLedControlTab();
    void initDemoTab();

private:
    Ui::MainWindow *ui;
    QMenu *tableMenu;
    LedRecordModel *model = Q_NULLPTR;

    int currentDemoNum = 0;
};

#endif // MAINWINDOW_H
