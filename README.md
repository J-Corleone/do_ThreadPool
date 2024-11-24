## do_ThreadPool


```
        wait
等待状态 <-- 就绪状态
      \      ^ mutex
notify v    /
       阻塞状态
```

#### 死锁问题

当线程池析构时，线程有三种情况：
1. 线程刚好等待在条件变量上`not_empty_.wait(lock)`，唤醒后被回收
2. 线程正在执行`task->exec()`，结束后被回收
3. 线程刚开始，这时候tfunc会和dtor抢锁 `lock(taskque_mutx_);`
   - tfunc抢到锁：顺序执行到`not_empty_.wait`，释放锁并开始等待。dtor已经`notify`过了，拿到锁后直接在`exit_cond_`上等待，所以tfunc将无法被唤醒，造成**死锁**
   - dtor抢到锁：在条件变量`exit_cond_`上等待并释放锁。tfunc拿到锁后，顺序执行到`not_empty_.wait`开始等待。但由于dtor已经`notify`过了，所以tfunc将一直等待在该条件变量上，造成**死锁**


#### 完善
当线程池出作用域析构时，保证任务队列的任务执行完再结束
