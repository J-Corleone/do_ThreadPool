#include "threadpool.hh"

#include <thread>

#include <iostream>

const int Task_max_threshhold = 1024;

ThreadPool::ThreadPool():
    init_thread_size_(0),
    task_size_(0),
    taskque_max_threshhold_(Task_max_threshhold),
    pool_mode_(PoolMode::MODE_FIXED) {

}

ThreadPool::~ThreadPool() {}

void ThreadPool::setMode(PoolMode mode) {
    pool_mode_ = mode;
}


void ThreadPool::setTaskqueMaxThreshHold(int threshhold) {
    taskque_max_threshhold_ = threshhold;
}

void ThreadPool::submitTask(std::shared_ptr<Task> sptr) {
    // 获取锁
    std::unique_lock<std::mutex> lock(taskque_mutx_);

    // 线程通信  等待Taskque有空余
    not_full_.wait(lock, [&]()->bool { return taskque_.size() < taskque_max_threshhold_; });

    // 如果有空余，把任务放入Taskque中
    taskque_.emplace(sptr);
    task_size_++;

    // 因为新放了任务，任务队列肯定不空，在notEmpty上进行通知，赶快分配线程执行任务
    not_empty_.notify_all();
}

void ThreadPool::start(int initThreshSize) {
    init_thread_size_ = initThreshSize;

    /** 
     * 创建线程对象
     * 保证线程启动的公平性，先集中创建，后边再启动所有线程
     */
    for (int i=0; i<init_thread_size_; i++) {
        // 在线程池创建thread线程对象的时候，把线程函数给它
        // threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
        // threads_使用智能指针，避免出现new/delete
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
        threads_.emplace_back(std::move(ptr));      // unique_ptr 不允许copy, 所以要用 移动语义，传右值
    }

    // 启动所有线程: std::vector<Thread*> threads_;
    for (int i=0; i<init_thread_size_; i++) {
        threads_[i]->start();   // 会去执行一个线程函数
    }
}

/**
 * 线程池的所有线程从任务队列消费任务
 */
void ThreadPool::threadFunc() {
    std::cout << "begin threadFunc tid: "
              << std::this_thread::get_id() 
              << std::endl;

    std::cout << "end threadFunc tid: "
              << std::this_thread::get_id() 
              << std::endl;
}


/*** 线程方法实现 **************************************/

Thread::Thread(ThreadFunc func):
    func_(func) {}

Thread::~Thread() {}

void Thread::start() {
    /**
    * To execute a thread func
    */
    std::thread t(func_);   // 创建线程对象，去执行线程函数
    t.detach();     // 分离线程，让线程函数自己去执行, (start一结束这个对象就没了)
}
