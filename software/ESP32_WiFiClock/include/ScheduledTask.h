#ifndef _SCHEDULEDTASK_H_
#define _SCHEDULEDTASK_H_

#include <Arduino.h>
#include <vector>

class ScheduledTask
{
private:
    time_t* now;
    typedef struct TASK
    {
        void (*callback)(void);
        int inter;
        time_t old_time;
    }Task;
    std::vector <Task> task_list;
public:
    void AddTask(void (*callback)(void), int interval);
    void Check();
    void SetTime(time_t* t);
    ScheduledTask(time_t* t);
    ~ScheduledTask();
};

#endif //_SCHEDULEDTASK_H_
