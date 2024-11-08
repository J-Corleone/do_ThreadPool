#include "threadpool.hh"

#include <chrono>
#include <thread>

int main() {
    ThreadPool p;
    p.start();

    // 由于 线程函数 是分离执行，main线程不能执行的太快，否则看不到东西
    std::this_thread::sleep_for(std::chrono::seconds(3));

}