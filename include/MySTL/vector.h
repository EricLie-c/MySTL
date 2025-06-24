#pragma once 

#include <cstddef> // 引入size_t

namespace MySTL {

    template<typename T>
    class vector {
    public:
        vector() noexcept :data_(nullptr), size_(0), capacity_(0) {}

        ~vector() {
            // 销毁所有在内存中构造的对象
            for (size_t i = 0;i < size_;i++) {
                data[i].~T();
            }
            // 释放原始内存 使用全局命名空间的delete运算符函数
            ::operator delete(data_);
        }

        size_t size() const noexcept { return size_; }
        // 返回已分配存储的容量
        size_t capacity() const noexcept { return capacity_; }
        size_t empty() const noexcept { return size_ == 0; }

        // noexcept: 1. 性能优化。编译器无需添加栈回退代码、记录当前状态、正确销毁对象等。因此比较简单，绝不会异常的代码建议使用
        // 2. 可以不用try catch包裹 3. 可以开启移动语义优化 
        // 4. 析构函数最好是，否则可能在栈回退销毁对象析构时又抛出异常，导致调用std::terminate直接崩溃

        void reserve(size_t new_capacity) {
            if (new_capacity <= capacity_) {
                return;
            }
            T* new_data = static_cast<T*> (::operator new(new_capacity * sizeof(T)));

            for (size_t i = 0;i < size_;i++) {
                // 使用 placement new 和 move 来移动构造对象，避免拷贝
                new(&new_data[i]) T(std::move(data_[i]));
            }

            // 销毁旧内存中的对象
            for (size_t i = 0;i < size_;i++) {
                data_[i].~T();
            }

            // 释放内存
            ::operator delete(data_);

            data_ = new_data;
            capacity_ = new_capacity;
        }

        void push_back(const T& value) {
            if (size_ == capacity_) {
                // 重新分配
                reserve(capacity_ == 0 ? 8 : capacity_ * 2);
            }
            new(&data_[size_]) T(value);
            size_++;
        }

        // 重载[]运算符以访问元素
        T& operator[] (size_t index) {
            // 根据std::STL的实现，不进行边界检查
            return data_[index];
        }

        const T& operator[](size_t index) const {
            // 用于const vector对象
            return data_[index];
        }


    private:
        T* data_;
        size_t capacity_;
        size_t size_;
    };
}