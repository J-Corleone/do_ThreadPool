#include "threadpool.hh"

#include <chrono>
#include <thread>

#include <iostream>

/* 
需求 - 有些场景，希望获得线程执行任务的返回值
e.g.    1 + ... + 30000 的和
thread1: 1 + ... + 10000
thread2: 10001 + ... + 20000
thread1: 20001 + ... + 30000

main thread: 
+ 给每个线程分配计算区间；
+ 等待他们算完返回结果，合并最终结果
*/

class MyTask : public Task {
public:
    /**
     *  Q1 - 怎么设计 run() 的返回值，可以表示任意类型？
     *  模板用不了，因为 虚函数 和 模板 不能一起用（编译时没有类型实例，没有真正的函数，抽象基类的虚表 就放不了 虚函数地址）
     * 
     *  c++17: any
     */
    Any run() {
        std::cout << "tid: " << std::this_thread::get_id()
                  << " begin!\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "tid: " << std::this_thread::get_id()
                  << " end!\n";
    }
};

int main() {
    ThreadPool p;
    p.start(3);

    // 由于 线程函数 是分离执行，main线程不能执行的太快，否则看不到东西
    // std::this_thread::sleep_for(std::chrono::seconds(3));


    /**
     *  Q2 - 如何设计这里的 Result 机制？
     *  Result res = p.submitTask(std::make_shared<MyTask>());
     *  
     *  + 线程没执行完时，阻塞在这
     *  res.get();  
     *  + 执行完时，拿到返回值，需要转为具体类型
     *  auto val = res.get.cast_<T>();  这个类型要用户自己指定（因为是用户传的任务）
     */ 

    p.submitTask(std::make_shared<MyTask>());
    p.submitTask(std::make_shared<MyTask>());
    p.submitTask(std::make_shared<MyTask>());
    p.submitTask(std::make_shared<MyTask>());
    p.submitTask(std::make_shared<MyTask>());
    p.submitTask(std::make_shared<MyTask>());
    p.submitTask(std::make_shared<MyTask>());
    p.submitTask(std::make_shared<MyTask>());

    getchar();
}