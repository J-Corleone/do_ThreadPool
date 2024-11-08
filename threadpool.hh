#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>


// 任务抽象基类
class Task {
public:
    // 用户可以自定义任意任务类型，从Task继承，重写run方法, 实现自定义任务处理
    virtual void run() = 0;
private:
};

// 线程池支持的模式
enum class PoolMode {
    MODE_FIXED, // 固定数量的线程
    MODE_CACHED,// 线程数量可动态增长
};

// 线程类
/**
 * 线程函数 没法写在thread类中，因为线程相关的变量全在 threadpool 里(而且是private, 更不能写成全局函数)
 */
class Thread {
public:
    using ThreadFunc = std::function<void()>;

    Thread(ThreadFunc func);
    ~Thread();

    // start thread
    void start();
private:
    ThreadFunc func_;
};

// 线程池类型
class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();

    // 设置线程池的工作模式
    void setMode(PoolMode mode);

    // 设置任务队列上限阈值
    void setTaskqueMaxThreshHold(int threshhold);
    
    // 给线程池提交任务
    void submitTask(std::shared_ptr<Task> sptr);
    
    // 启动 线程池
    void start(int initThreshSize=4);

    // 防止对线程池本身copy
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    /** 
     * 在线程池中定义 线程执行函数
     * 线程 执行什么样的函数由 线程池 指定
     * 将来线程函数访问的变量也都在该线程池对象中（一切都理所当然）
     */
    void threadFunc();

private:
    std::vector<Thread*> threads_; // 线程列表
    std::size_t init_thread_size_;     // 初始的线程数量
    // 防止用户传递临时对象，用智能指针延长生命周期
    std::queue<std::shared_ptr<Task>> taskque_; // 任务队列
    std::atomic_uint task_size_;    // 任务数量
    int taskque_max_threshhold_;    // 任务队列数量上限阈值

    std::mutex taskque_mutx_;   // 保证任务队列的线程安全
    std::condition_variable not_full_;  // 表示任务队列不满
    std::condition_variable not_empty_; // 表示任务队列不空

    PoolMode pool_mode_;    // 当前线程池的工作模式
};