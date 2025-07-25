#pragma once

#include <cstddef>
#include "algorithm.h"
#include <utility>

namespace MySTL
{
    template<typename T>
    class list
    {
        // 双向链表节点结构体
        // 类和结构体只有在没有定义 直接 构造函数时提供默认的无参构造
        // 即使定义了复制和移动构造，也还会提供无参构造
        struct Node
        {
            T data;
            Node *prev;
            Node *next;
            Node(): prev(nullptr), next(nullptr) {}
            Node(const T &value): data(value), prev(nullptr), next(nullptr) {}
        };

        Node *head; // 首哨兵
        Node *tail; // 尾哨兵

        size_t size_;

    public:
        // 拷贝构造函数
        list(const list &other): size_(0)
        {
            head = new Node();
            tail = new Node();
            head->next = tail;
            tail->prev = head;
            for (Node *cur = other.head->next; cur != other.tail;
                 cur = cur->next) {
                push_back(cur->data);
            }
        }

        // 拷贝赋值运算符
        list &operator=(const list &other)
        {
            if (this == &other)
                return this;
            clear();
            for (Node *cur = other.head->next; cur != other.tail;
                 cur = cur->next) {
                push_back(cur->data);
            }
            return *this;
        }

        // 移动构造函数
        /*
           移动构造函数推荐声明为 noexcept，因为 STL
           容器在需要移动元素时（如扩容、重排），会优先用 noexcept
           的移动构造。如果没有 noexcept，有些操作会退化为拷贝，影响效率。

           拷贝构造函数一般不会声明为noexcept，因为拷贝通常涉及资源分配（如new），可能抛异常。

           移动构造如果能保证不会抛异常（比如只是指针交换），就应该加
           noexcept，这样容器才能安全高效地使用。
        */
        list(list &&other) noexcept: head(nullptr), tail(nullptr), size_(0)
        {
            head = other.head;
            tail = other.tail;
            size_ = other.size_;
            other.head = new Node();
            other.tail = new Node();
            other.head->next = other.tail;
            other.tail->prev = other.head;
            other.size_ = 0;
        }

        // 移动赋值操作符
        list &operator=(list &&other) noexcept
        {
            if (this != &other) {
                clear();
                delete head; // 彻底变为nullptr，释放内存
                delete tail;
                head = other.head;
                tail = other.tail;
                size_ = other.size_;
                other.head = new Node();
                other.tail = new Node();
                other.head->next = other.tail;
                other.tail->prev = other.head;
                other.size_ = 0;
            }
            return *this;
        }

        class iterator
        {
            friend class list;
            Node *node_;

        public:
            iterator(Node *n = nullptr): node_(n) {}

            // 解引用返回的必须是引用，对其修改是有效的
            T &operator*() const
            {
                return node_->data;
            }

            // 当使用 it-> 时，实际上在使用 it.operator ->() ->
            // 仅仅使用于 T 为结构体类型时
            // 把 iterator 本身理解为 T* 就好理解了
            // 因为 Node 对用户是透明的，属于内部结构，真正的 list 是 T 的链表
            // 自然，list 返回的指针必须能够直接访问 T 的内部结构
            T *operator->() const
            {
                return &(node_->data);
            }

            // 前置++
            iterator &operator++()
            {
                node_ = node_->next;
                return *this;
            }

            // 后置++ 依靠哑元区分
            iterator operator++(int)
            {
                // 默认复制构造tmp
                iterator tmp = *this;
                node_ = node_->next;
                return tmp;
            }

            iterator &operator--()
            {
                node_ = node_->prev;
                return *this;
            }

            iterator operator--(int)
            {
                iterator tmp = *this;
                node_ = node_->prev;
                return tmp;
            }

            // right hand side
            bool operator==(const iterator &rhs) const
            {
                return node_ == rhs.node_;
            }

            bool operator!=(const iterator &rhs) const
            {
                return node_ != rhs.node_;
            }
        };

        iterator begin()
        {
            return iterator(head->next);
        }

        iterator end()
        {
            return iterator(tail);
        }

        // 反向迭代器
        class reverse_iterator
        {
            friend class list;
            Node *node_;

        public:
            reverse_iterator(Node *n = nullptr): node_(n) {}

            T &operator*() const
            {
                return node_->data;
            }

            T *operator->() const
            {
                return &(node_->data);
            }

            reverse_iterator &operator++()
            {
                node_ = node_->prev;
                return *this;
            }

            reverse_iterator operator++(int)
            {
                reverse_iterator tmp = *this;
                node_ = node_->prev;
                return tmp;
            }

            reverse_iterator &operator--()
            {
                node_ = node_->next;
                return *this;
            }

            reverse_iterator operator--(int)
            {
                reverse_iterator tmp = *this;
                node_ = node_->next;
                return tmp;
            }

            bool operator==(const reverse_iterator &rhs) const
            {
                return node_ == rhs.node_;
            }

            bool operator!=(const reverse_iterator &rhs) const
            {
                return node_ != rhs.node_;
            }
        };

        reverse_iterator rbegin()
        {
            return reverse_iterator(tail->prev);
        }

        reverse_iterator rend()
        {
            return reverse_iterator(head);
        }

        list(): size_(0)
        {
            head = new Node();
            tail = new Node();
            head->next = tail;
            tail->prev = head;
        }

        ~list()
        {
            clear();
            delete head;
            delete tail;
        }

        void clear()
        {
            Node *cur = head->next;
            while (cur != tail) {
                Node *tmp = cur;
                cur = cur->next;
                delete tmp;
            }
            head->next = tail;
            tail->prev = head;
            size_ = 0;
        }

        void swap(list &other) noexcept
        {
            MySTL::swap(head, other.head);
            MySTL::swap(tail, other.tail);
            MySTL::swap(size_, other.size_);
        }

        size_t size() const
        {
            return size_;
        }

        bool empty() const
        {
            return size_ == 0;
        }

        void push_back(const T &value)
        {
            Node *node = new Node(value);
            node->prev = tail->prev;
            node->next = tail;
            tail->prev = node;
            tail->prev->next = node;
            ++size_;
        }

        void push_front(const T &value)
        {
            Node *node = new Node(value);
            node->next = head->next;
            node->prev = head;
            head->next->prev = node;
            head->next = node;
            ++size_;
        }

        void pop_back()
        {
            if (empty())
                return;
            Node *node = tail->prev;
            node->prev->next = tail;
            tail->prev = node->prev;
            delete node;
            --size_;
        }

        void pop_front()
        {
            if (empty())
                return;
            Node *node = head->next;
            head->next = node->next;
            node->next->prev = head;
            delete node;
            --size_;
        }

        // 在 pos 位置前插入 value，返回新元素的迭代器
        iterator insert(iterator pos, const T &value)
        {
            Node *cur = pos.node_;
            Node *prev = cur->prev;
            Node *node = new Node(value);
            node->prev = prev;
            node->next = cur;
            prev->next = node;
            cur->prev = node;
            ++size_;
            return iterator(node);
        }

        // 删除 pos 位置的元素，返回下一个元素的迭代器
        iterator erase(iterator pos)
        {
            Node *node = pos.node_;
            if (node == head || node == tail)
                return end(); // 不允许删哨兵
            Node *prev = node->prev;
            Node *next = node->next;
            prev->next = next;
            next->prev = prev;
            delete node;
            --size_;
            return iterator(next);
        }

        template<typename... Args>
        void emplace_back(Args &&... args)
        {
            Node *node = new Node(T(std::forward<Args>(args)...));
            node->prev = tail->prev;
            node->next = tail;
            tail->prev->next = node;
            tail->prev = node;
            ++size_;
        }

    };
} // namespace MySTL