#include "threadpool.hh"

#include <thread>

#include <iostream>

const int Task_max_threshhold = INT32_MAX;
const int Thread_max_threshhold = 1024;
const int Thread_max_idle_time = 60;

ThreadPool::ThreadPool():
    init_thread_size_(0),
    taskque_max_threshhold_(Task_max_threshhold),
    cur_thread_size_(0),
    idle_thread_num_(0),
    task_size_(0),
    thread_max_threshhold_(Thread_max_threshhold),
    pool_mode_(PoolMode::MODE_FIXED),
    is_pool_running_(false) {}

ThreadPool::~ThreadPool() {}

void ThreadPool::setMode(PoolMode mode) {
    if (check_running_state()) return;

    pool_mode_ = mode;
}

void ThreadPool::setTaskqueMaxThreshHold(int threshhold) {
    if (check_running_state()) return;

    taskque_max_threshhold_ = threshhold;
}

void ThreadPool::setThreadThreshHold(int threshhold) {
    if (check_running_state()) return;
    if (pool_mode_ == PoolMode::MODE_FIXED) return;

    thread_max_threshhold_ = threshhold;
}

Result ThreadPool::submitTask(std::shared_ptr<Task> sptr) {
    // 1.获取锁
    std::unique_lock<std::mutex> lock(taskque_mutx_);

    // 2.1 线程通信  等待Taskque有空余
    /** 
     * 2.2 用户提交任务，最长不能超过1s，否则判提交任务失败，返回
     * 
     * wait       - 等待条件满足，等待期间自动 unlock
     * wait_for   - 等待一段时间
     * wait_until - 一直等到某个时刻
     */
    if(!not_full_.wait_for(lock, std::chrono::seconds(1),
        [&]()->bool { return taskque_.size() < taskque_max_threshhold_; })) {
        // 说明 not_full_ 等待1s，条件仍然没有满足
        std::cerr << "task queue is full, submit task failed." << std::endl;
        printf("taskque_.size(): %ld --- task_threshhold: %d\n", taskque_.size(), taskque_max_threshhold_);

        /**
         *  return task->getResult(); X
         *  不能用这种方法，因为 task对象 在线程函数中执行完就析构了
         */
        return Result(sptr, false); // 返回的临时对象，会自动匹配"移动copy和assign"(>=c++17)
    }

    // 3.如果有空余，把任务放入Taskque中
    taskque_.emplace(sptr);
    task_size_++;

    // 4. 因为新放了任务，任务队列肯定不空，在notEmpty上进行通知，赶快分配线程执行任务
    not_empty_.notify_all();

    // *cached 模式 任务处理比较紧急 场景：根据任务数量和空闲线程数量，判断是否需要创建新的线程？
    if (PoolMode::MODE_CACHED == pool_mode_
        && task_size_ > idle_thread_num_
        && cur_thread_size_ < thread_max_threshhold_) {

            std::cout << ">>> create new thread..." << std::endl;

            // 创建新线程
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
            int tid = ptr->getId();
            threads_.emplace(tid, std::move(ptr));
            // 启动线程
            threads_[tid]->start();
            // 修改线程个数相关的变量
            cur_thread_size_++; // 记得增加
            idle_thread_num_++; 
        }

    // 返回 Result 对象
    return Result(sptr);
}

void ThreadPool::start(int initThreadSize) {
    // 设置线程池运行状态
    is_pool_running_ = true;

    // 记录初始线程数
    init_thread_size_ = initThreadSize;
    cur_thread_size_ = initThreadSize;

    /** 
     * 创建线程对象
     * 保证线程启动的公平性，先集中创建，后边再启动所有线程
     */
    for (uint32_t i=0; i<init_thread_size_; i++) {
        // 在线程池创建thread线程对象的时候，把线程函数给它
        // threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
        // threads_使用智能指针，避免出现new/delete
        // threads_改用map
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        // threads_.emplace_back(std::move(ptr));      // unique_ptr 不允许copy, 所以要用 移动语义，传右值
        int tid = ptr->getId();
        threads_.emplace(tid, std::move(ptr));
    }

    // 启动所有线程: std::vector<Thread*> threads_;
    for (uint i=0; i < init_thread_size_; i++) {
        threads_[i]->start();   // 会去执行一个线程函数

        idle_thread_num_++;     // 启动一个增加一个空闲线程
                                /**
                                 * 感觉不对啊？start是执行线程函数去了，线程函数里面执行结束也会++
                                 * 线程函数会先--，好像又没问题？
                                 */
    }
}

/**
 * 线程池的所有线程从任务队列消费任务
 */
void ThreadPool::threadFunc(int thread_id) {
    auto last_time = std::chrono::high_resolution_clock::now();

    for (;;) {
        std::shared_ptr<Task> task;
        {
            // 1.先获取锁
            std::unique_lock<std::mutex> lock(taskque_mutx_);
            std::cout << "tid: " << std::this_thread::get_id()
                      << " 获取任务中...\n";

            // *cached模式下，可能已经创建了很多线程，但是空闲时间超过60s，应该把多余的(超过init_thread_size_数量)线程结束回收掉
            // 当前时间 - 线程上次执行结束的时间 > 60s
            if (PoolMode::MODE_CACHED == pool_mode_) {
                // 没有任务的时候等待，并检查
                while (taskque_.size() == 0) {
                    /** 每一秒检查一次
                     *  区分：超时返回 | 有任务等待返回
                     */
                    if (std::cv_status::timeout ==
                        not_empty_.wait_for(lock, std::chrono::seconds(1))) {
                        // 如果没拿到任务就检查空闲时间
                        auto now = std::chrono::high_resolution_clock::now();
                        auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - last_time);
                        if (dur.count() >= Thread_max_idle_time
                            && cur_thread_size_ > init_thread_size_) {
                            /** 开始回收当前线程
                             *  修改 记录线程数量的相关变量值
                             *  把线程对象从线程列表中删除    如何将 threadFunc <=> thread 对应起来？
                             *  thread_id => thread对象 => 删除
                             */
                            threads_.erase(thread_id);
                            cur_thread_size_--;
                            idle_thread_num_--;

                            std::cout << "thread: " << std::this_thread::get_id()
                                      << " exit." << std::endl;
                            return;
                        }
                    }
                }
            } else {
                // 2.等待任务队列不空, not_empty_ 条件
                not_empty_.wait(lock, [&]()->bool { return taskque_.size() > 0; }); // 有🔒, so size of task can be indicated by taskque_.size()
            }

            idle_thread_num_--;

            std::cout << "tid: " << std::this_thread::get_id()
                      << " 获取任务成功...\n";

            // 3.如果不空，从任务队列取一个任务
            task = taskque_.front();
            taskque_.pop();
            task_size_--;

            // 3.1 如果队列还有任务，继续通知其它的线程
            if (taskque_.size() > 0)
                not_empty_.notify_all();

            // 3.2 取出一个任务，在 not_full_ 上通知，可以继续提交生产任务
            not_full_.notify_all();

        }   // 4.弄个作用域，取出任务后就需要释放锁了

        // 5.当前线程执行这个任务
        if (task != nullptr)
            // task->run(); 执行任务，并把任务返回值get_val给到Result
            task->exec();
    
        idle_thread_num_++;
        
        last_time = std::chrono::high_resolution_clock::now();  // 线程结束，重新记录last_time
    }
}

bool ThreadPool::check_running_state() const {
    return is_pool_running_;
}

/*** 线程方法实现 **************************************/

Thread::Thread(ThreadFunc func):
    func_(func),
    thread_id_(genert_id_++) // 线程池创建一个线程，就会构造一次
     {}

Thread::~Thread() {}

void Thread::start() {
    // To execute a thread func
    std::thread t(func_, thread_id_);   // 创建线程对象，去执行线程函数
    t.detach();     // 分离线程，让线程函数自己去执行, (start一结束这个对象就没了)
}

int Thread::getId() const { return thread_id_; }

int Thread::genert_id_ = 0;
/*** Task方法实现 ******************************************/
Task::Task() : result_(nullptr) {}

void Task::set_result(Result* res) { // Result 构造时调用
    result_ = res;
}

void Task::exec() {                  // 线程函数调用
    if (result_ != nullptr)
        result_->get_val(run()); // run() 发生多态调用
}

/*** Result方法实现 ****************************************/
Result::Result(std::shared_ptr<Task> task, bool is_valid)
    : task_(task), is_valid_(is_valid) { // submitTask时调用
    
    task_->set_result(this);
}

void Result::get_val(Any any) { // Task执行完调用
    any_ = std::move(any);
    sem_.post();
}

Any Result::get() {             // 用户调用
    if (!is_valid_) return "";

    // task没执行完时会阻塞
    sem_.wait();
    return std::move(any_);
}
