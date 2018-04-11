#include "tabtask.h"

#include <QStringList>

TabTask::TabTask(int ts, int dur, const QString &colorStr, const QString &tabStr)
{
    memset(data, 0, sizeof(data));

    timestamp = ts;
    if(timestamp > 0x00FFFFFF) {
        timestamp = 0;
    }
    duration = dur;
    if(duration > 0x0000FFFF) {
        duration = 0xFFFF;
    }
    color = colorStr;

    parseData(tabStr);
}

void TabTask::parseData(const QString &tabStr)
{
    if(tabStr.isEmpty()) {
        return;
    }
    //prepare task data
    for(int i = 0; i < TAB_TASK_MAX_SIZE; i++) {
        data[i].fretNum = 0xFF;
        memset(data[i].leds, 0, sizeof(data[i].leds));
    }
    QStringList tabList = tabStr.split(" ", QString::SkipEmptyParts);
    if(!tabList.isEmpty()) {
        LedState ls = parseColor();
        int fretCount = 0;
        for(auto str : tabList) {
            //too much frets
            if(fretCount == TAB_TASK_MAX_SIZE) {
                break;
            }
            int stringNum = 0;
            while(!str.isEmpty() && (stringNum < 6)) {
                auto sym = str.left(1);
                str.remove(0, 1);
                if(sym == "x" || sym == "X") {
                    //skip;
                    stringNum++;
                    continue;
                }
                else {
                    bool fConvert = false;
                    int fretNum = sym.toInt(&fConvert);
                    if(fConvert) {
                       for(int i = 0; i < TAB_TASK_MAX_SIZE; i++) {
                           if(data[i].fretNum == 0xFF) {
                               data[i].fretNum = fretNum;
                               data[i].leds[stringNum] = ls;
                               fretCount++;
                               break;
                           }
                           else if(data[i].fretNum == fretNum) {
                               data[i].leds[stringNum] = ls;
                               break;
                           }
                       }
                    }
                    stringNum++;
                }
            }
        }
    }
}

/**
 * Private methods
 */

TabTask::LedState TabTask::parseColor()
{
    LedState state = LED_OFF;
    if(color.compare("red", Qt::CaseInsensitive) == 0) {
        state = LED_RED;
    }
    else if(color.compare("yellow", Qt::CaseInsensitive) == 0) {
        state = LED_YELLOW;
    }
    else if(color.compare("both", Qt::CaseInsensitive) == 0) {
        state = LED_BOTH;
    }
    return state;
}
