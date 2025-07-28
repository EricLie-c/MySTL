#include <iostream>
#include <atomic>

template<typename T>
class unique_ptr {
    T* ptr;
public:
    // 避免发生隐式构造导致指针所有权转移
    explicit unique_ptr(T* p = nullptr) : ptr(p) {}

    // 禁用拷贝
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator = (const unique_ptr&) = delete;

    // 移动构造
    // 声明noexcept可以保证移动操作不会进行到一半被终止，或者退化为拷贝操作
    unique_ptr(unique_ptr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    // 移动赋值
    unique_ptr& operator = (unique_ptr&& other) noexcept {
        if (this != &other) {
            delete ptr; // 析构原来的 且 ptr = nullptr
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    // 析构函数
    ~unique_ptr() {
        delete ptr;
    }

    // 重载指针操作符
    T& operator*() const { return *ptr; }
    // 特殊的设计。本质上是递归解引用，然后访问成员
    T* operator->() const { return ptr; }
    T* get() const { return ptr; }
    void reset(T* p = nullptr) {
        delete ptr;
        ptr = p;
    }
    // 放弃所有权，返回裸指针
    T* release() {
        T* temp = ptr;
        ptr = nullptr;
        return temp;
    }
};

template<typename T>
class shared_ptr;

// 控制块
class ControlBlock {
public:
    // 为了保证线程安全，且避免加锁导致频繁上下文切换，使用原子操作
    std::atomic<int> shared_count; // 强引用计数
    std::atomic<int> weak_count; // 弱引用计数

    ControlBlock() :shared_count(1), weak_count(1) {}
};

template <typename T>
class weak_ptr {
    T* ptr;
    // int* count; 不安全。如果绑定的shared_ptr被销毁，那会变成悬空指针
    // 改用控制块
    ControlBlock* control_block;
public:
    weak_ptr() : ptr(nullptr), control_block(nullptr) {}
    weak_ptr(const shared_ptr<T>& sp) noexcept
        : ptr(sp.ptr), control_block(sp.control_block) {
        if (control_block) {
            control_block->weak_count++;
        }
    }
    // 拷贝构造
    weak_ptr(const weak_ptr& other) noexcept
        : ptr(other.ptr), control_block(other.control_block)
    {
        if (control_block) {
            control_block->weak_count++;
        }
    }
    // 析构函数
    ~weak_ptr() {
        release();
    }

    int use_count() const { return control_block ? *control_block->shared_count : 0; }
    bool expired() const {
        return !control_block || control_block->shared_count == 0;
    }

    // 获取一个shared_ptr
    shared_ptr<T> lock() const {
        if (expired()) {
            return shared_ptr<T>(); // 返回空的 shared_ptr
        }
        // 对象还存在，通过weak_ptr构造创建一个新的 shared_ptr
        return shared_ptr<T>(*this);
    }

private:
    void release() {
        if (control_block) {
            control_block->weak_count--;
            if (control_block->shared_count == 0 && control_block->weak_count == 0) {
                delete control_block;
            }
        }
    }
    // 允许 shared_ptr 访问私有成员来构造自己
    friend class shared_ptr<T>;

};

template<typename T>
class shared_ptr {
    friend class weak_ptr<T>;
    T* ptr;
    // int* count; // 确保所有共享指针能有相同的引用计数
    ControlBlock* control_block;
    void release() {
        if (control_block) {
            control_block->shared_count--;
            if (control_block == 0) {
                delete ptr;
                control_block->weak_count--;
                if (control_block->weak_count == 0) {
                    delete control_block;
                }
            }
        }
    }

public:
    explicit shared_ptr(T* p = nullptr)
        : ptr(p), control_block(p ? new ControlBlock() : nullptr)) {}

    // 拷贝构造
    // 为什么可以直接访问私有成员初始化？因为访问控制权限是基于“类”而非对象的
    // 即，只要同类内，就可以随意访问
    shared_ptr(const shared_ptr& other) noexcept
        : ptr(other.ptr), control_block(other.control_block) {
        // ++(*count);
        if (control_block) {
            control_block->shared_count++;
        }
    }

    // 从 weak_ptr 构造 (用于 lock)
    explicit shared_ptr(const weak_ptr<T>& wp)
        : ptr(wp.ptr), control_block(wp.control_block)
    {
        if (control_block) {
            control_block->shared_count++;
        }
    }

    // 拷贝赋值
    shared_ptr& operator=(const shared_ptr& other) {
        if (this != &other) {
            release();
            ptr = other.ptr;
            control_block = other.control_block;
            if (control_block) // 考虑空指针拷贝赋值
                ++control_block->shared_count;
        }
        return *this;
    }

    // 移动构造
    shared_ptr(shared_ptr&& other) noexcept
        :ptr(other.ptr), control_block(other, control_block) {
        other.ptr = nullptr;
        other.control_block = nullptr;
    }

    // 移动赋值
    shared_ptr& operator=(const shared_ptr&& other) noexcept {
        if (this != &other) {
            release();
            ptr = other.ptr;
            control_block = other.control_block;
            other.ptr = nullptr;
            other.control_block = nullptr;
        }
        return *this;
    }

    // 析构函数
    ~shared_ptr() {
        release();
    }

    T* get() {
        return ptr;
    }

    T& operator*() const {
        return *ptr;
    }
    T* operator->() const { return ptr; }
    int use_count() const { return control_block->shared_count; }
};
