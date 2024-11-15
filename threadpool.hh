#pragma once

#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

// Any类：接收任意类型的数据
class Any {
public:
    Any() = default;
    ~Any() = default;
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;
    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    // 该 ctor 让 Any 接收任意类型的其它数据
    template<typename T>
    Any(T data) : base_(std::make_unique<Derive<T>>(data)) {}
    
    // 该函数把 Any 对象存储的 data 提取出来
    template <typename T>
    T cast_() {
        // 从 base_ 找到它指向的 Derive 类对象，从它里面取出 data 成员变量
        // 基类指针 -> 派生类指针 RTTI
        Derive<T> *pd = dynamic_cast<Derive<T>*>(base_.get());
        
        // 如果类型不对，抛出异常
        if (pd == nullptr)
            throw "type is unmatched!";
        
        // 类型对，用派生类指针调用派生类成员
        return pd->data_;
    }

private:
    // 基类类型
    class Base {
    public:
        virtual ~Base() = default;
    };

    // 派生类类型
    template<typename T>
    class Derive : public Base {
    public:
        Derive(T data) : data_(data) {}
        
        T data_;
    };

private:
    // 定义一个基类指针
    std::unique_ptr<Base> base_;
};

// 信号量类
class Semaphore {
public:
    Semaphore(int limit = 0) : resLimit_(limit) {}
    ~Semaphore() = default;

    // 消耗一个信号量资源
    void wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待信号量有资源，没有则 block
        cond_.wait(lock, [&]()->bool { return resLimit_ > 0; });
        resLimit_--;
    }

    // 增加一个信号量资源
    void post() {
        std::unique_lock<std::mutex> lock(mtx_);
        resLimit_++;
        // 有资源了需要通知
        cond_.notify_all();
    }

private:
    int resLimit_;
    std::mutex mtx_;
    std::condition_variable cond_;
};

class Task;
// 实现接收 提交到线程池的task执行完后的 返回值类型 Result
class Result {
public:
    Result(std::shared_ptr<Task> task, bool is_valid = true);
    ~Result() = default;

    // Q1 - get_val方法，获取 task 执行完的返回值
    void get_val(Any any);
    // Q2 - get方法，用户调用这个方法获取 task 的返回值
    Any get();

private:
    Any any_;       // 存储任务的返回值
    Semaphore sem_; // 线程通信信号量
    std::shared_ptr<Task> task_; // 指向获取返回值的task对象
    std::atomic_bool is_valid_;  // 返回值是否有效
};

// 任务抽象基类
class Task {
public:
    Task();
    ~Task() = default;

    // 用户可以自定义任意任务类型，从Task继承，重写run方法, 实现自定义任务处理
    virtual Any run() = 0;

    void set_result(Result *res);
    void exec();

private:
    // 不能放Result对象，因为它的生命周期要  > task
    Result *result_; // 不能用智能指针，会导致交叉引用
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
    using ThreadFunc = std::function<void(int)>;

    Thread(ThreadFunc func);
    ~Thread();

    // start thread
    void start();

    int getId() const;

private:
    ThreadFunc func_;
    static int genert_id_;
    int thread_id_; // 保存线程id
};

/*
e.g.
ThreadPool pool;
pool.start();

class MyTask: public Task {
public:
    void run() {}
};

pool.submitTask(std::make_shared<MyTask>());

*/

// 线程池类型
class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();

    // 设置线程池的工作模式
    void setMode(PoolMode mode);

    // 设置任务队列上限阈值
    void setTaskqueMaxThreshHold(int threshhold);

    // 设置线程池cached模式下 线程的阈值（让用户设置：有的服务器内存大，有的小）
    void setThreadThreshHold(int threshhold);
    
    // 给线程池提交任务     用户调用该接口，传入任务对象，"生产任务"
    Result submitTask(std::shared_ptr<Task> sptr);
    
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
    void threadFunc(int thread_id);

    // 检查 pool 的运行状态
    bool check_running_state() const;

private:
    // std::vector<Thread*> threads_;                   // 线程列表
    // std::vector<std::unique_ptr<Thread>> threads_;   // 线程列表
    std::unordered_map<int, std::unique_ptr<Thread>> threads_; // 线程列表
    
    std::size_t init_thread_size_;      // 初始的线程数量
    uint thread_max_threshhold_;         // cached模式下 线程阈值（资源不是无限的）
    std::atomic_uint cur_thread_size_;   // 记录线程池的实际线程数量
    std::atomic_uint idle_thread_num_;   // 记录空闲线程的数量

    // 防止用户传递临时对象，用智能指针延长生命周期
    std::queue<std::shared_ptr<Task>> taskque_; // 任务队列
    std::atomic_uint task_size_;        // 任务数量
    uint taskque_max_threshhold_;        // 任务队列数量上限阈值

    std::mutex taskque_mutx_;           // 保证任务队列的线程安全
    std::condition_variable not_full_;  // 表示任务队列不满
    std::condition_variable not_empty_; // 表示任务队列不空

    PoolMode pool_mode_;                // 当前线程池的工作模式
    std::atomic_bool is_pool_running_;  // 表示线程池当前的启动状态
};