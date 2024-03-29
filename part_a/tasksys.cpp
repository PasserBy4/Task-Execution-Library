#include "tasksys.h"
#include <mutex>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this -> num_threads_ = num_threads;
    threads_pool_ = new std::thread[num_threads];
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {
    delete[] threads_pool_;
}

void TaskSystemParallelSpawn::threadRun(IRunnable* runnable, int num_total_tasks, std::mutex* mtx, int* curr_task){
    int rem_task = -1;
    while(rem_task < num_total_tasks){
        mtx -> lock();
        int rem_task = *curr_task;
        (*curr_task)++;
        mtx -> unlock();
        if(rem_task >= num_total_tasks){
            break;
        }
        runnable->runTask(rem_task, num_total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {

    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    // for (int i = 0; i < num_total_tasks; i++) {
    //     runnable->runTask(i, num_total_tasks);
    // }
    std::mutex* mtx = new std::mutex();
    int* curr_task = new int;
    *curr_task = 0;
    for (int i = 0; i < num_threads_; i++){
        threads_pool_[i] = std::thread(&TaskSystemParallelSpawn::threadRun, this, runnable, num_total_tasks, mtx, curr_task);
    }
    for (int i = 0; i < num_threads_; i++){
        threads_pool_[i].join();
    }
    delete mtx;
    delete curr_task;
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

TaskState::TaskState(){
    mutex_ = new std::mutex();
    finished_ = new std::condition_variable();
    finished_mutex_ = new std::mutex();
    runnable_ = nullptr;
    finished_tasks_ = -1;
    left_tasks_ = -1;
    num_total_tasks_ = -1;
}

TaskState::~TaskState(){
    delete mutex_;
    delete finished_;
    delete finished_mutex_;
}

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    state_ = new TaskState;
    killed = false;
    threads_pool_ = new std::thread[num_threads];
    num_threads_ = num_threads;
    for(int i = 0; i < num_threads; i++){
        threads_pool_[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::spinningThread, this);
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    killed = true;
    for(int i = 0; i < num_threads_; i++){
        threads_pool_[i].join();
    }
    delete[] threads_pool_;
    delete state_;
}

void TaskSystemParallelThreadPoolSpinning::spinningThread(){
    int id;
    int total;
    while(true){
        if(killed) break;

        state_ -> mutex_ -> lock();
        total = state_ -> num_total_tasks_;
        id = state_ -> num_total_tasks_ - state_ -> left_tasks_;
        if (id < total)
            state_ -> left_tasks_ --;
        state_ -> mutex_ -> unlock();

        if (id < total){
            state_ -> runnable_ -> runTask(id, total);

            state_ -> mutex_ -> lock();
            state_ -> finished_tasks_ ++;
            if(state_ -> finished_tasks_ == total){
                state_ -> mutex_ -> unlock();

                state_ -> finished_mutex_ -> lock();
                state_ -> finished_mutex_ -> unlock();
                state_ -> finished_ -> notify_all();
            } else {
                state_ -> mutex_ -> unlock();
            }
        }

    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> lk(*(state_ -> finished_mutex_));

    state_ -> mutex_ -> lock();
    state_ -> finished_tasks_ = 0;
    state_ -> left_tasks_ = num_total_tasks;
    state_ -> num_total_tasks_  = num_total_tasks;
    state_ -> runnable_ = runnable;
    state_ -> mutex_ -> unlock();

    state_ -> finished_ -> wait(lk);
    lk.unlock();
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    killed = false;
    state_ = new TaskState;
    has_task_cv_ = new std::condition_variable;
    has_task_mutex_ = new std::mutex;
    num_threads_ = num_threads;
    threads_pool_ = new std::thread[num_threads];
    for(int i = 0; i < num_threads; i++){
        threads_pool_[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::sleepingThread, this);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    killed = true;
    has_task_cv_ -> notify_all();
    for(int i = 0; i < num_threads_; i++){
        threads_pool_[i].join();
    }
    delete state_;
    delete has_task_cv_;
    delete has_task_mutex_;
    delete[] threads_pool_;
}

void TaskSystemParallelThreadPoolSleeping::sleepingThread(){
    int id;
    int total;
    while(true){
        if (killed == true) break;
        
        state_ -> mutex_ -> lock();
        total = state_ -> num_total_tasks_;
        id = state_ -> num_total_tasks_ - state_ -> left_tasks_;
        if (id < total) state_->left_tasks_--;
        state_ -> mutex_ -> unlock();

        if (id < total){
            state_ -> runnable_ -> runTask(id, total);

            state_ -> mutex_ -> lock();
            state_ -> finished_tasks_ ++;
            if(state_ -> finished_tasks_ == total){
                state_ -> mutex_ -> unlock();
                state_ -> finished_mutex_ -> lock();
                state_ -> finished_mutex_ -> unlock();
                state_ -> finished_ -> notify_all();
            } else {
                state_ -> mutex_ -> unlock();
            }
        } else {
            std::unique_lock<std::mutex> lk(*(has_task_mutex_));
            has_task_cv_ -> wait(lk);
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> lk(*(state_ -> finished_mutex_));

    state_ -> mutex_ -> lock();
    state_ -> finished_tasks_ = 0;
    state_ -> left_tasks_ = num_total_tasks;
    state_ -> num_total_tasks_ = num_total_tasks;
    state_ -> runnable_ = runnable;
    state_ -> mutex_ -> unlock();

    has_task_cv_ -> notify_all();

    state_ -> finished_ -> wait(lk);
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
