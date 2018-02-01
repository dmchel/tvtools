#include "logrecordmodel.h"

#include <QColor>

LogRecordItem::LogRecordItem() : num(""), time(QDateTime()), event(""),
                                 type("")
{

}

LogRecordItem::LogRecordItem(QString num_, QDateTime time_, QString event_, QString _state,
                             QString type_) {
    num = num_;
    time = time_;
    event = event_;
    state = _state;
    type = type_;
}

LogRecordModel::LogRecordModel(QObject *parent) : QAbstractTableModel(parent)
{
    records.clear();
    dateTimeMap[0] = QDateTime::currentDateTime();
    baseDt = QDateTime::currentDateTime();
}

void LogRecordModel::addRecord(const LogRecordItem &record) {
    beginResetModel();
    records.append(record);
    endResetModel();
}

void LogRecordModel::removeRecord(const LogRecordItem &record) {
    beginResetModel();
    int index = records.indexOf(record, 0);
    if(index >= 0) {
        records.removeAt(index);
    }
    endResetModel();
}

void LogRecordModel::rewriteRecord(int index, const LogRecordItem &rwRecord)
{
    beginResetModel();
    if((index >= 0) && (index < records.size())) {
        records[index] = rwRecord;
    }
    endResetModel();
}

void LogRecordModel::addDateTimeMark(int index, const QDateTime &value)
{
    dateTimeMap[index] = value;
    beginResetModel();
    calculateTimes(index);
    endResetModel();
}

void LogRecordModel::removeDateTimeMark(int index)
{
    dateTimeMap.remove(index);
    beginResetModel();
    calculateTimes(index);
    endResetModel();
}

void LogRecordModel::removeAllDataTimeMarks()
{
    dateTimeMap.clear();
}

void LogRecordModel::setBaseTime(const QDateTime &value)
{
    if(value.isValid()) {
        baseDt = value;
    }
}

QDateTime LogRecordModel::baseTime()
{
    return baseDt;
}

void LogRecordModel::removeAll() {
    beginResetModel();
    records.clear();
    dateTimeMap.clear();
    dateTimeMap[0] = QDateTime::currentDateTime();
    endResetModel();
}

Qt::ItemFlags LogRecordModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags theFlags = QAbstractTableModel::flags(index);
    if(index.isValid()) {
        theFlags |= Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
    return theFlags;
}

int LogRecordModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : records.count();
}

int LogRecordModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : (MAX_COLUMN_NUM_LOG + 1);
}

QVariant LogRecordModel::data(const QModelIndex &index, int role) const {
    if((!index.isValid()) || (index.column() < 0) || (index.row() < 0) || (index.row() > records.count())
            || (index.column() > MAX_COLUMN_NUM_LOG)) {
        return QVariant();
    }
    if(records.isEmpty()) {
        return QVariant();
    }
    if(role == Qt::DisplayRole) {
        const LogRecordItem &record = records.at(index.row());
        switch (index.column()) {
        case 0:
            return record.num;
        case 1:
            return record.time.toString("dd.MM.yyyy hh:mm:ss");
        case 2:
            return record.event;
        case 3:
            return record.state;
        case 4:
            return record.type;
        }
    }
    else if(role == Qt::DecorationRole) {
        if((index.column() == 1) && dateTimeMap.contains(index.row())) {
            return "marked";
        }
    }
    return QVariant();
}

QVariant LogRecordModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if(role != Qt::DisplayRole) {
        return QVariant();
    }
    if(orientation == Qt::Horizontal) {
        switch(section) {
        case 0:
            return tr("Номер");
        case 1:
            return tr("Время");
        case 2:
            return tr("Описание события");
        case 3:
            return tr("Состояние");
        case 4:
            return tr("Тип события");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

bool  LogRecordModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }
    if(index.isValid()) {
        auto row = index.row();
        if((index.column() == 1) && (row < records.size())) {
            QDateTime dt = QDateTime::fromString(value.toString(), "dd.MM.yyyy hh:mm:ss");
            if(dt.isValid()) {
                dateTimeMap[row] = dt;
                calculateTimes(row);
                emit dataChanged(index, index);
                return true;
            }
        }
    }
    return false;
}

/**
  Private methods
 */

/**
 * @brief LogRecordModel::calculateTimes
 * @param keyNum ключевое значение в карте меток
 */
void LogRecordModel::calculateTimes(int keyNum)
{
    if(records.isEmpty()) {
        return;
    }
    auto validKeyNum = getClosestValidDateRecord(keyNum);
    if(validKeyNum == -1) {
        return;
    }
    QList<int> keyList = dateTimeMap.keys();
    auto i = keyList.indexOf(keyNum);
    if(i != -1) {
        auto end = records.size();
        if((i + 1) < keyList.size()) {
            end = qMin(keyList[i + 1], records.size());
        }
        QDateTime baseTime = records[validKeyNum].time;
        if(baseTime.isValid()) {
            records[validKeyNum].time = dateTimeMap[keyNum];
            for(auto num = (validKeyNum + 1); num < end; num++) {
                if(records[num].time.isValid()) {
                    auto diffSec = baseTime.secsTo(records[num].time);
                    QDateTime dt = dateTimeMap[keyNum].addSecs(diffSec);
                    if(dt.isValid()) {
                        records[num].time = dt;
                    }
                }
            }
        }
    }
}

/**
 * @brief LogRecordModel::getClosestValidDateRecord
 * @param num номер записи
 * @return ближайший номер записи с валидным значением даты и времени,
 * либо -1, в случае ошибки поиска
 */
int LogRecordModel::getClosestValidDateRecord(int num)
{
    int res = -1;
    if(num < records.size()) {
        for(int i = num; i < records.size(); i++) {
            if(records[i].time.isValid()) {
                res = i;
                break;
            }
        }
    }
    return res;
}
