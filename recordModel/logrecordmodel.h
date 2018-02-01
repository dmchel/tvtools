#ifndef LOGRECORDMODEL_H
#define LOGRECORDMODEL_H

#include <QAbstractTableModel>
#include <QString>
#include <QList>
#include <QDateTime>

#define MAX_COLUMN_NUM_LOG 4

class LogRecordItem
{
public:
    LogRecordItem();
    LogRecordItem(QString num_, QDateTime time_, QString event_,
                  QString _state, QString type_);

    QString num;            //номер записи
    QDateTime time;         //время создания записи
    QString event;          //событие
    QString state;          //состояние
    QString type;           //тип источника события

    inline bool operator ==(const LogRecordItem &other) {
        return (num == other.num) && (time == other.time) && (event == other.event) &&
               (state == other.state) && (type == other.type);
    }
};

class LogRecordModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit LogRecordModel(QObject* parent = 0);
    //~LogRecordModel();

    void addRecord(const LogRecordItem &record);
    void removeRecord(const LogRecordItem &record);
    void rewriteRecord(int index, const LogRecordItem &rwRecord);

    void addDateTimeMark(int index, const QDateTime &value);
    void removeDateTimeMark(int index);
    void removeAllDataTimeMarks();

    void setBaseTime(const QDateTime &value);
    QDateTime baseTime();

    void removeAll();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);

private:
    void calculateTimes(int keyNum);
    int getClosestValidDateRecord(int num);

private:
    QList<LogRecordItem> records;
    QMap<int, QDateTime> dateTimeMap;
    QDateTime baseDt;

};

#endif // LOGRECORDMODEL_H
