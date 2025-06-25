#pragma once 

#include <cstddef> // 引入size_t
#include <utility> // 引入forward、move
#include <cmath>  // 引入 max


namespace MySTL {

    template<typename T>
    // 按17标准需要实现5法则，按20标准要实现6法则
    class vector {
    public:
        // 为迭代器提供类型定义，以便与STL算法一起使用
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using difference_type = std::ptrdiff_t;  // 本质是long类型，保证其值能囊括本平台上的最远指针距离

        // ================== 迭代器实现 ==================
        class iterator {
        public:
            friend class const_iterator;  // 不需要提前声明，因为内部类是互相可见的。
            
            iterator(pointer ptr) : ptr_(ptr) {}

            reference operator*() const { return *ptr_; }
            pointer operator->() const { return ptr_; }

            iterator& operator++() { ptr_++; return *this; }
            iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

            iterator& operator--() { ptr_--; return *this; }
            iterator operator--(int) { iterator tmp = *this; --(*this); return tmp; }

            // 随机访问迭代器所需的操作
            iterator operator+(difference_type n) const { return iterator(ptr_ + n); }
            iterator operator-(difference_type n) const { return iterator(ptr_ - n); }
            difference_type operator-(const iterator& other) const { return ptr_ - other.ptr_; }

            bool operator<(const iterator& other) const { return ptr_ < other.ptr_; }
            bool operator>(const iterator& other) const { return ptr_ > other.ptr_; }
            bool operator<=(const iterator& other) const { return ptr_ <= other.ptr_; }
            bool operator>=(const iterator& other) const { return ptr_ >= other.ptr_; }

            bool operator==(const iterator& other) const { return ptr_ == other.ptr_; };
            bool operator!=(const iterator& other) const { return ptr_ != other.ptr_; };

        private:
            pointer ptr_;
        };

        class const_iterator {
        public:
            const_iterator(pointer ptr) : ptr_(ptr) {}

            const_iterator(const iterator& other) : ptr_(other.ptr_) {}

            const_reference operator*() const { return *ptr_; }
            const_pointer operator->() const { return ptr_; }

            const_iterator& operator++() { ptr_++; return *this; }
            const_iterator operator++(int) { const_iterator tmp = *this; ++(*this); return tmp; }

            const_iterator& operator--() { ptr_--; return *this; }
            const_iterator operator--(int) { const_iterator tmp = *this; --(*this); return tmp; }

            // 随机访问迭代器所需的操作
            const_iterator operator+(difference_type n) const { return const_iterator(ptr_ + n); }
            const_iterator operator-(difference_type n) const { return const_iterator(ptr_ - n); }
            difference_type operator-(const const_iterator& other) const { return ptr_ - other.ptr_; }

            bool operator<(const const_iterator& other) const { return ptr_ < other.ptr_; }
            bool operator>(const const_iterator& other) const { return ptr_ > other.ptr_; }
            bool operator<=(const const_iterator& other) const { return ptr_ <= other.ptr_; }
            bool operator>=(const const_iterator& other) const { return ptr_ >= other.ptr_; }

            bool operator==(const const_iterator& other) const { return ptr_ == other.ptr_; };
            bool operator!=(const const_iterator& other) const { return ptr_ != other.ptr_; };

        private:
            pointer ptr_; // 使用 T* 即可，因为我们只在 const 方法中返回 const T& 或 const T*
        };


        vector() noexcept :data_(nullptr), size_(0), capacity_(0) {}

        ~vector() {
            // 销毁所有在内存中构造的对象
            clear();
            // 释放原始内存 使用全局命名空间的delete运算符函数
            ::operator delete(data_);
        }

        // ================== 五法则实现 ==================

        // 拷贝构造
        vector(const vector& other) {
            // 分配足够的内存以拷贝构造元素
            data_ = static_cast<pointer> (::operator new(other.capacity_ * sizeof(value_type)));
            for (size_t i = 0;i < other.size_;i++) {
                new(&data_[i]) value_type(other.data_[i]);
            }
            size_ = other.size_;
            capacity_ = other.capacity_;
        }

        // 拷贝赋值
        vector& operator=(const vector& other) {
            if (this == &other) {
                return *this;
            }

            clear(); // 清理现有资源
            ::operator delete(data_);

            // 拷贝
            data_ = static_cast<pointer> (::operator new(other.capacity_ * sizeof(value_type)));
            for (size_t i = 0;i < other.size_;i++) {
                new(&data_[i]) value_type(other.data_[i]);
            }
            size_ = other.size_;
            capacity_ = other.capacity_;
        }

        // 移动构造
        vector(vector&& other) noexcept {
            // 窃取资源
            size_ = other.size_;
            capacity_ = other.capacity_;
            data_ = other.data_;

            // 原对象置空
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }

        // 移动赋值
        vector& operator=(vector&& other) noexcept {
            if (this == &other) {
                return *this;
            }
            clear(); // 析构
            ::operator delete(data_);

            // 窃取资源
            size_ = other.size_;
            capacity_ = other.capacity_;
            data_ = other.data_;

            // 原对象置空
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;

            return *this;
            
        }

        // ================== 迭代器访问 ==================
        iterator begin() noexcept { return iterator(data_); }
        iterator end() noexcept { return iterator(data_ + size_); }
        const_iterator begin() const noexcept { return const_iterator(data_); }
        const_iterator end() const noexcept { return const_iterator(data_ + size_); }
        const_iterator cbegin() const noexcept { return const_iterator(data_); }
        const_iterator cend() const noexcept { return const_iterator(data_ + size_); }


        size_t size() const noexcept { return size_; }
        // 返回已分配存储的容量
        size_t capacity() const noexcept { return capacity_; }
        bool empty() const noexcept { return size_ == 0; }

        // noexcept: 1. 性能优化。编译器无需添加栈回退代码、记录当前状态、正确销毁对象等。因此比较简单，绝不会异常的代码建议使用
        // 2. 可以不用try catch包裹 3. 可以开启移动语义优化 
        // 4. 析构函数最好是，否则可能在栈回退销毁对象析构时又抛出异常，导致调用std::terminate直接崩溃

        void reserve(size_t new_capacity) {
            if (new_capacity <= capacity_) {
                return;
            }
            pointer new_data = static_cast<pointer> (::operator new(new_capacity * sizeof(value_type)));

            for (size_t i = 0;i < size_;i++) {
                // 使用 placement new 和 move 来移动构造对象，避免拷贝
                new(&new_data[i]) value_type(std::move(data_[i]));
            }

            // 销毁旧内存中的对象
            for (size_t i = 0;i < size_;i++) {
                data_[i].~value_type();
            }

            // 释放内存
            ::operator delete(data_);

            data_ = new_data;
            capacity_ = new_capacity;
        }

        void push_back(const_reference value) {
            if (size_ == capacity_) {
                // 重新分配
                reserve(capacity_ == 0 ? 8 : capacity_ * 2);
            }
            new(&data_[size_]) value_type(value);
            size_++;
        }

        // push_back 的移动语义版本
        void push_back(value_type&& value) {
            if (size_ == capacity_) {
                // 相等时就必须扩容，否则会逻辑上越界访问（尽管由于placement new不会出现报错，但事实上超出了capacity）
                reserve(capacity_ == 0 ? 8 : capacity_ * 2);
            }
            // 使用 placement new 和 move 来构造对象
            new(&data_[size_]) value_type(std::move(value));
            size_++;
        }

        // 就地构造  使用可变参数模板  接收构造所需的所有参数，而非对象本身
        template<typename... Args>
        // ... 对左侧意味着包展开并应用&&，对右侧意味着包声明
        void emplace_back(Args&&... args) {  // 触发转发引用规则、引用折叠规则，从而使得左右值语义能够被完整转发
            if (size_ == capacity_) {
                reserve(capacity_ == 0 ? 8 : capacity_ * 2);
            }
            // 使用完美转发将参数传递给T的构造参数，以使编译器能选择最适合的构造函数
            // forward的工作方式：检查模板参数。若是引用，则转回左值，若是非引用，转回右值。
            // 这里要注意区分模板参数被推导的值和实际的参数值
            new(&data_[size_]) value_type(std::forward<Args>(args)...);  // 参数包展开
            size_++;
        }

        // 在pos位置擦除元素
        iterator erase(const_iterator pos) {
            difference_type index = pos - cbegin();
            // 将pos后的元素向前移动一个位置
            for (size_t i = index;i < size_ - 1;i++) {
                data_[i] = std::move(data_[i + 1]);
            }
            // 销毁多余的
            pop_back();
            return iterator(data_ + index);
        }

        // 擦除范围内的元素
        iterator erase(const_iterator first, const_iterator last) {
            difference_type index_first = first - cbegin();
            difference_type index_last = last - cbegin();
            difference_type count = index_last - index_first;
            if (count > 0) {
                for (size_t i = index_last; i < size_;i++) {
                    data_[i - count] = std::move(data_[i]);
                }
                // 销毁末尾count个剩余元素
                for (size_t i = 0;i < count;i++) {
                    pop_back();  // 此时会减小size_，无需额外动作
                }
            }
            return iterator(data_ + index_first);
        }

        // 在pos位置插入元素 拷贝
        iterator insert(const_iterator pos, const_reference value) {
            difference_type index = pos - cbegin();
            if (size_ == capacity_) {
                reserve(capacity_ == 0 ? 8 : capacity_ * 2);
            }
            if (index < size_) {
                new(&data_[size_]) value_type(std::move(data_[size_ - 1]));
                for (size_t i = size_ - 1;i > index;i--) {
                    data_[i] = std::move(data_[i - 1]);
                }
                data_[index] = value;
            }
            else {
                new(&data_[size_]) value_type(value);
            }

            size_++;
            return iterator(data_ + index);
        }

        // 在pos位置插入元素  移动版
        iterator insert(const_iterator pos, T&& value) {
            difference_type index = pos - cbegin();
            if (size_ == capacity_) {
                reserve(capacity_ == 0 ? 8 : capacity_ * 2);
            }

            if (index < size_) {
                new(&data_[size_]) value_type(std::move(data_[size - 1]));
                for (size_t i = size_ - 1;i > index;i--) {   // for循环放外面可能是比较好的实践
                    data_[i] = std::move(data_[i - 1]);
                }
                data_[index] = std::move(value); // 唯一区别。这里使用移动赋值
            }
            else {
                new(&data_[size_]) value_type(std::move(value));
            }

            size_++;
            return iterator(data_ + index);
        }

        void pop_back() {
            if (size_ > 0) {
                size_--;
                data_[size_].~value_type();
            }
        }

        void clear() noexcept {
            for (size_t i = 0; i < size_; ++i) {
                data_[i].~value_type();
            }
            size_ = 0;
        }

        // 带边界检查的访问，会抛出out_of_range异常
        reference at(size_t index) {
            if (index >= size_) {
                throw std::out_of_range("MySTL::vector::at: index out of range!");
            }
            return data_(index);
        }
        
        // 当且仅当vector本身为const时会调用该函数。const的对象只能调用const函数。同时还必须防止用户修改。
        const_reference at(size_t index) const {
            if (index >= size_) {
                throw std::out_of_range("vector::at: index out of range");
            }
            return data_[index];
        }

        void resize(size_t new_size) {
            if (new_size < size_) {
                // 缩小尺寸，删除多余元素
                while (size_ > new_size) {
                    pop_back();
                }
            }
            else if (new_size > size_) {
                // 扩大size_，同时需保证容量
                if (new_size > capacity_) {
                    reserve(std::max(new_size, capacity_*2));  // 增长因子为2,推荐容量和必须容量选最大
                }
                // 默认构造新添加的元素
                for (size_type i = size_; i < new_size; ++i) {
                    new(&data_[i]) value_type();
                }
            }
            size_ = new_size;
        }

        // 重载[]运算符以访问元素
        value_type& operator[] (size_t index) {
            return data_[index];
        }

        const value_type& operator[](size_t index) const {
            // 用于const vector对象
            return data_[index];
        }


    private:
        value_type* data_;
        size_t capacity_;
        size_t size_;
    };
}