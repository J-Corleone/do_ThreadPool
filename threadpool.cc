#include "threadpool.hh"
#include <thread>


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

}

void ThreadPool::start(int initThreshSize) {
    init_thread_size_ = initThreshSize;

    /** 
     * 创建线程对象
     * 保证线程启动的公平性，先集中创建，后边再启动所有线程
     */
    for (int i=0; i<init_thread_size_; i++) {
        // 在线程池创建thread线程对象的时候，把线程函数给它
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
    }

    // 启动所有线程: std::vector<Thread*> threads_;
    for (int i=0; i<init_thread_size_; i++) {
        threads_[i]->start();   // 会去执行一个线程函数
    }
}

void ThreadPool::threadFunc() {}


/*** 线程方法实现 **************************************/

Thread::Thread(ThreadFunc func):
    func_(func) {}

Thread::~Thread() {}

void Thread::start() {
    /**
    * To execute a thread func
    */
    std::thread t(func_);   // 创建线程对象，去执行线程函数
    t.detach();     // 分离线程，让线程函数自己去执行
}
