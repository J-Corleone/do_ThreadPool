#include "threadpool.hh"

#include <chrono>
#include <thread>

#include <iostream>

using namespace std;

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

using uLong = unsigned long long;

class MyTask : public Task {
public:
    MyTask(int begin, int end) : begin_(begin), end_(end) {}

    /**
     *  Q1 - 怎么设计 run() 的返回值，可以表示任意类型？
     *  模板用不了，因为 虚函数 和 模板 不能一起用（编译时没有类型实例，没有真正的函数，抽象基类的虚表 就放不了 虚函数地址）
     * 
     *  c++17: any
     */
    Any run() {
        std::cout << "tid: " << std::this_thread::get_id()
                  << " begin!\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        uLong sum = 0;
        for (int i = begin_; i <= end_; i++)
            sum += (uLong)i;

        std::cout << "tid: " << std::this_thread::get_id()
                  << " end!\n";

        return sum;
    }
private:
    int begin_;
    int end_;
};

int main() {
    {
        ThreadPool p;
        p.setMode(PoolMode::MODE_CACHED);
        p.start(2);
        Result res1 = p.submitTask(std::make_shared<MyTask>(1, 100000000));
        p.submitTask(std::make_shared<MyTask>(100000001, 200000000));
        p.submitTask(std::make_shared<MyTask>(100000001, 200000000));
        p.submitTask(std::make_shared<MyTask>(100000001, 200000000));
        
        uLong sum1 = res1.get().cast_<uLong>(); 
        
        std::cout << sum1 << endl;
    }
    std::cout << "========== Main Over! ===========\n";

#if 0
    {
        // *Q - ThreadPool对象析构后，怎么回收线程相关的资源？
        ThreadPool p;
        /**
         *  *用户自己设置线程池的工作模式(*代表第二轮思考)
         *  保证启动后不允许用户设置模式？
         */
        p.setMode(PoolMode::MODE_CACHED);
        p.start(3);

        // 由于 线程函数 是分离执行，main线程不能执行的太快，否则看不到东西
        // std::this_thread::sleep_for(std::chrono::seconds(3));


        /**
         *  Q2 - 如何设计这里的 Result 机制？
         *  Result res = p.submitTask(std::make_shared<MyTask>());
         *  
         *  + 线程没执行完时，阻塞在这
         *  res.get();
         *  + 执行完时，拿到返回值，怎么转为具体类型？
         *  auto val = res.get().cast_<T>();  这个类型要用户自己指定（因为是用户传的任务）
         */ 
        Result res1 = p.submitTask(std::make_shared<MyTask>(1, 100000000));
        Result res2 = p.submitTask(std::make_shared<MyTask>(100000001, 200000000));
        Result res3 = p.submitTask(std::make_shared<MyTask>(200000001, 300000000));
        p.submitTask(std::make_shared<MyTask>(100000001, 200000000));
        p.submitTask(std::make_shared<MyTask>(100000001, 200000000));

        uLong sum1 = res1.get().cast_<uLong>();
        uLong sum2 = res2.get().cast_<uLong>();
        uLong sum3 = res3.get().cast_<uLong>();
        
        cout << sum1 + sum2 + sum3 << endl;

        // uLong sum = 0;
        // for (int i=0; i <= 300000000; i++)
        //     sum += i;
        // cout << sum << endl;
    }
#endif
    
    getchar();
}