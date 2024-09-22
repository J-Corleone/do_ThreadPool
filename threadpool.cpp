#include "threadpool.hh"

#include <functional>

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

void ThreadPool::setInitThreadSize(int size) {
    init_thread_size_ = size;
}

void ThreadPool::setTaskqueMaxThreshHold(int threshhold) {
    taskque_max_threshhold_ = threshhold;
}

void ThreadPool::submitTask(std::shared_ptr<Task> sptr) {

}

void ThreadPool::start(int initThreshSize) {
    init_thread_size_ = initThreshSize;

    // 创建线程对象
    for (int i=0; i<init_thread_size_; i++) {
        // 创建thread线程对象的时候，把线程函数给到thread线程对象
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
    }
    // 启动所有线程
    for (int i=0; i<init_thread_size_; i++) {
        threads_[i]->start();   // 会去执行一个线程函数
    }
}

void ThreadPool::threadFunc() {}
