#include<iostream>
#include<string>
#include<vector>
#include<numeric>
#include<exception>
#include <cstdio> 
#include"thread_pool.h"

using std::string;
using std::cout;
using std::endl;
using std::to_string;
using std::exception;
using std::cerr;
// using namespace std::chrono;

string string_task(int id, const string& input, int duration_ms) {
    cout << "Task " << id << " (string) starting on thread " << std::this_thread::get_id()
        << " with input '" << input << "'. Will sleep for " << duration_ms << "ms." << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    string res = "Result from task " + to_string(id) + ": Processed '" + input + "'";
    cout << "Task " << id << " (string) finished on thread " << std::this_thread::get_id() << endl;
    return res;
}

int main() {
    printf("Forcing a PLT call!\n");

    cout << "Starting ThreadPool test..." << endl;
    size_t num_threads = 4; // 线程池中的线程数
    ThreadPool pool(num_threads);
    cout << "ThreadPool created with " << num_threads << " worker threads." << endl;
    cout << "----------------------------------------" << endl;

    cout << "\nEnqueuing value-returning tasks..." << endl;

    vector<future<string>> string_futures; // 承接结果

    for (int i = 0;i < 3;i++) {
        function<string()> str_task_func = [i] {
            return string_task(i, "data_" + to_string(i), (i + 1) * 150);
            };
        // 只有packaged_task/thread是不允许复制的，function可以。但是出于效率和语义需要一般用移动比较好
        string_futures.emplace_back(pool.enqueue<string>(move(str_task_func)));
        cout << "Enqueued string_task " << i << endl;
    }

    cout << "\nRetrieving results from string_futures..." << endl;
    for (size_t i = 0;i < string_futures.size();i++) {
        try {
            cout << "Waiting for string_future " << i << "..." << endl;
            string result = string_futures[i].get();
            cout << "Result from string_future " << i << ": " << result << endl;
        }
        // 为什么用引用？也有讲究。可以避免“对象切割”问题。
        // 异常通常形成一个继承体系（例如，std::invalid_argument 继承自 std::logic_error，后者又继承自 std::exception）。
        // 如果按值捕获基类异常(catch (std::exception e)), 而实际抛出的是一个派生类异常(如 std::invalid_argument), 
        // 那么在 catch 块中的 e 对象将只是原始派生类异常对象中 std::exception 部分的一个副本。
        // 所有派生类特有的信息和行为（包括通过虚函数如 what() 实现的多态行为）都会丢失。

        // 通过引用捕获可以保留原始异常对象的实际类型，从而允许正确的虚函数调用和多态行为。
        // 同时可以减少拷贝开销。
        catch (const exception& e) {
            cerr << "Exception caught while getting result from string_future " << i << ": " << e.what() << endl;
        }
    }
    return 0;
}