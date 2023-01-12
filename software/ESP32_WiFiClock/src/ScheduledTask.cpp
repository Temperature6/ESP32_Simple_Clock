#include "ScheduledTask.h"

void ScheduledTask::AddTask(void (*callback)(void), int interval)
{
    Task tsk;
    tsk.callback = callback;
    tsk.inter = interval;
    tsk.old_time = *now;
    task_list.push_back(tsk);
}

void ScheduledTask::Check()
{
    for (int i = 0; i < task_list.size(); i++)
    {
        if (*now - task_list[i].old_time > task_list[i].inter)
        {
            task_list[i].old_time = *now;
            task_list[i].callback();
        }
    }
}

void ScheduledTask::SetTime(time_t* t)
{
    now = t;
}

ScheduledTask::ScheduledTask(time_t* t)
{
    now = t;
}

ScheduledTask::~ScheduledTask()
{
}
