#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#include <future> // 异步
#include <memory> // make_shared


using std::function;
using std::vector;
using std::thread;
using std::queue;
using std::mutex;
using std::condition_variable;
using std::unique_lock;
using std::move;
using std::future;
using std::packaged_task;
using std::runtime_error;
using std::invalid_argument;
using std::make_shared;

class ThreadPool {
public:
    ThreadPool(size_t threads) :stop(false) {
        if (threads == 0) {
            return;
        }
        for (size_t i = 0;i < threads;i++) {
            workers.emplace_back([this] {
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(this->queue_mutex);
                        // 若条件为真，则持有锁并继续执行；否则释放锁并进入阻塞态
                        this->condition.wait(lock, [this] {return this->stop || !this->tasks.empty();});
                        if (this->stop && this->tasks.empty()) {
                            return;
                        }

                        if (!this->tasks.empty()) {
                            task = move(this->tasks.front());
                            this->tasks.pop();
                        }
                        // else {
                        //     continue;
                        // }
                    }
                    if (task) {
                        task();
                    }
                }
            });
        }
    }
    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (thread &worker : workers) {
            if (worker.joinable()) { // 检查线程是否可以被 join
                worker.join(); // 等待线程结束
            }
        }
    }
    // 任务函数无返回值版本
    void enqueue(function<void()> task_function) {
        if (!task_function) {
            return;
        }
        {
            unique_lock<mutex>lock(queue_mutex);
            if (stop) {
                return;
            }
            tasks.emplace(move(task_function));
        }
        condition.notify_one();
    }
    
    // 任务函数有返回值的异步版本
    template<typename R>
    future<R> enqueue(function<R()> task_function) {
        if (!task_function) {
            throw invalid_argument("Cannot enqueue an empty std::function");
        }
        // 1. 从function创建packaged_task
        // packaged_task<R()> pack_task(move(task_function));
        auto pack_task_ptr = make_shared<packaged_task<R()>>(move(task_function));
        // 2. 从packaged_task获取future
        // future<R> result_future = pack_task.get_future();
        future<R> result_future = pack_task_ptr->get_future();

        {
            unique_lock<mutex> lock(queue_mutex);
            if (stop) {
                throw runtime_error("enqueue on stopped ThreadPool");
            }
            // packaged_task不支持拷贝构造，因此不支持按值捕获
            // 移动捕获的话会有内存安全问题，因为packaged_task是栈上创建的变量，函数返回时会被销毁
            // 此时会造成“悬空引用”的问题
            // mutable是因为lambda的operator()是const的，即不允许在lambda函数体内修改按值捕获的变量副本

            // C++14支持移动捕获的语法
            // tasks.emplace([p = move(pack_task)] mutable {
            //     p();
            //     });

            // 但其实根本没必要这么麻烦，packaged_task也是可调用对象，并且其operator()也是void类型
            // tasks.emplace(move(pack_task));

            // 我错了，还真不行。疑似避开了function的SBO小对象优化，在堆上直接分配内存时触发了可拷贝性检查
            // 用shared_ptr包装的packaged_task是可以拷贝的。捕获的lambda也因此可拷贝。
            tasks.emplace([pack_task_ptr] {
                (*pack_task_ptr)();
                });
        }
        
        condition.notify_one();
        return result_future;
    }

private:
    vector<thread> workers;
    queue<function<void()>> tasks;

    mutex queue_mutex; // 保护任务队列
    condition_variable condition; // 用于线程间同步
    bool stop; // 用于停止线程池

};


#endif