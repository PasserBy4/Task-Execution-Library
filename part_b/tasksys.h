#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <map>
#include <set>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */

class Task {
    public:
        TaskID id;
        IRunnable* runnable;
        int num_total_tasks;

        Task(TaskID id, IRunnable* runnable, int num_total_tasks){
            this -> id = id;
            this -> runnable = runnable;
            this -> num_total_tasks = num_total_tasks;
        }

        Task(const Task &other){
            this -> id = other.id;
            this -> runnable = other.runnable;
            this -> num_total_tasks = other.num_total_tasks;
        }
};

class RunnableTask : public Task {
    public:
        int current_task_id;
    
        RunnableTask(TaskID id, TaskID curent_task_id, IRunnable* runnable, int num_total_tasks)
            : Task(id, runnable, num_total_tasks), current_task_id(current_task_id) {}
        
        RunnableTask(const RunnableTask &other)
            : Task(other.id, other.runnable, other.num_total_tasks), current_task_id(other.current_task_id) {}
};

class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        bool killed;
        int num_threads;
        int current_task_id;
        std::map<TaskID, std::set<TaskID>> tasks_dep;
        std::map<TaskID, Task*> task_id_to_task;
        std::map<TaskID, int> remaining_tasks;
        std::deque<RunnableTask*> runnable_tasks;
        std::deque<TaskID> finished_tasks;
        std::vector<std::thread> pool;
        std::mutex* task_run_mutex;
        std::mutex* finished_task_mutex;
        std::condition_variable* task_run_cr;
        std::condition_variable* finished_task_cr;

        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        void workThread(int thread_number);
        void scanForReadyTasks();
        void removeTaskIDFromDependency(TaskID i);
};

#endif
