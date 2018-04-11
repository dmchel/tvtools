#ifndef TABTASK_H
#define TABTASK_H

#include <QString>

#define TAB_TASK_MAX_SIZE 16

/**
 * @brief The TabTask class
 *  Tab descriptrion in apropriate form for TabVision device.
 */
class TabTask
{
public:
    enum LedState {
        LED_OFF,
        LED_RED,
        LED_YELLOW,
        LED_BOTH
    };

    explicit TabTask(int ts = 0, int dur = 0, const QString &colorStr = "red", const QString &tabStr = "");

    void parseData(const QString &tabStr);

    struct TabData {
        quint8 fretNum;
        LedState leds[6];
    };

    int timestamp = 0;
    int duration = 0;
    QString color = "red";

    TabData data[TAB_TASK_MAX_SIZE];

private:
    LedState parseColor();

private:


};

#endif // TABTASK_H
