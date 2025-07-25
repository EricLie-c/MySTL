#pragma once

#include "vector.h"
#include "list.h"

#include <bits/c++config.h>
#include <cstddef>
#include <functional>
#include <utility>

namespace MySTL
{
    template<typename Key, typename T>
    class unordered_map
    {
        struct Node
        {
            Key key;
            T value;
            Node(const Key& k, const T& v): key(k), value(v) {}
        };

        MySTL::vector<MySTL::list<Node>> buckets; // 哈希桶
        size_t bucket_count;
        size_t elem_count;
        // 简单理解为重载了括号运算符的模板结构体
        std::hash<Key> hash_func; // 默认哈希函数

        // 元素数量找过负载因子的阈值 bucket_count * load_factor 后自动扩容
        float load_factor = 0.75f;

    public:
        class iterator
        {
            // 内部类和外部类互相的访问也受到访问权限的控制，private和protect都不行，必须声明友元
            friend class unordered_map;
            size_t bucket_idx;
            typename MySTL::list<Node>::iterator
                    list_it; // 当前桶中链表的某个节点
            unordered_map* map_ptr;

        public:
            iterator(): bucket_idx(0), map_ptr(nullptr) {}
            iterator(
                    size_t idx,
                    typename MySTL::list<Node>::iterator it,
                    unordered_map* ptr)
                : bucket_idx(idx),
                  map_ptr(ptr),
                  list_it(it)
            {
            }
            Node& operator*() const
            {
                return *list_it;
            }
            // it-> 等价于 it.operator ->() -> ，这是为了方便递归指向
            // 因此需要返回指针类型
            Node* operator->() const
            {
                return &(*list_it);
            }

            iterator& operator++()
            {
                ++list_it;
                // 如果当前桶遍历完，跳到下一个非空的桶
                while (bucket_idx < map_ptr->bucket_count &&
                       list_it == map_ptr->buckets[bucket_idx].end()) {
                    ++bucket_idx;
                    if (bucket_idx < map_ptr->bucket_count)
                        list_it = map_ptr->buckets[bucket_idx].begin();
                }
                return *this;
            }
            // 后置 ++
            iterator& operator++(int)
            {
                auto it = *this;
                ++(*this);
                return it;
            }
            // 理论上也可以反向遍历的，但是标准库没有实现，那算了
        };
        iterator begin()
        {
            for (size_t i = 0; i < bucket_count; i++) {
                if (!buckets[i].empty()) {
                    return iterator(i, buckets[i].begin(), this);
                }
                return end(); // 类中的函数互相可见
            }
        }
        iterator end()
        {
            return iterator(bucket_count, {}, this);
        }

        iterator find(const Key& key)
        {
            size_t idx = hash_func(key) % bucket_count;
            for (auto it = buckets[idx].begin(); it != buckets[idx].end();
                 it++) {
                if (it->key == key) {
                    return iterator(idx, it, this);
                }
            }
            return end();
        }

        // 扩容
        void rehash(size_t new_bucket_count)
        {
            MySTL::vector<MySTL::list<Node>> new_buckets;
            new_buckets.resize(new_bucket_count);
            for (size_t i = 0; i < bucket_count; i++) {
                for (auto& node : buckets[i]) {
                    // 重新计算扩容后的哈希值并获得下标
                    size_t idx = hash_func(node.key) % new_bucket_count;
                    new_buckets[idx].push_back(node);
                }
            }
            buckets.swap(new_buckets);
            bucket_count = new_bucket_count;
        }

        void check_rehash()
        {
            if (elem_count > bucket_count * load_factor) {
                rehash(2 * bucket_count); // 两倍扩容
            }
        }

        // 默认构造
        unordered_map(size_t n = 16): bucket_count(n), elem_count(0)
        {
            buckets.resize(bucket_count);
        }

        // 拷贝构造
        unordered_map(const unordered_map& other)
            : bucket_count(other.bucket_count),
              elem_count(other.elem_count),
              hash_func(other.hash_func),
              load_factor(other.load_factor)
        {
            buckets.resize(bucket_count);
            for (size_t i=0;i<bucket_count;i++) {
                for (auto& node : other.buckets[i]) {
                    buckets[i].push_back(node);
                }
            }
        }

        // 拷贝赋值
        unordered_map& operator=(const unordered_map& other)
        {
            if (this != &other) {
                bucket_count = other.bucket_count;
                hash_func = other.hash_func;
                load_factor = other.load_factor;
                buckets.resize(bucket_count);
                clear();
                for (size_t i = 0; i < bucket_count; i++) {
                    for (auto& node : other.buckets[i]) {
                        buckets[i].push_back(node);
                        ++elem_count;
                    }
                }
            }
            return *this; // 支持连续赋值
        }

        // 移动构造
        unordered_map(unordered_map&& other) noexcept
            : buckets(std::move(other.buckets)),
              bucket_count(other.bucket_count),
              elem_count(other.elem_count),
              hash_func(std::move(other.hash_func)),
              load_factor(other.load_factor)
        {
            other.bucket_count = 0;
            other.elem_count = 0;
        }

        // 移动赋值
        unordered_map& operator=(unordered_map&& other) noexcept
        {
            if (this != &other) {
                buckets = std::move(other.buckets);
                bucket_count = other.bucket_count;
                elem_count = other.elem_count;
                hash_func = std::move(other.hash_func);
                load_factor = other.load_factor;
                other.bucket_count = 0;
                other.elem_count = 0;
            }
            return *this;
        }

        bool insert(const Key& key, const T& value)
        {
            check_rehash();
            size_t idx = hash_func(key) % bucket_count;
            for (auto& node : buckets[idx]) {
                if (node.key == key) {
                    return false; // 已存在
                }
            }
            buckets[idx].push_back(Node(key, value));
            ++elem_count;
            return true;
        }

        // 下标访问，如果不存在则默认初始化
        T& operator[](const Key& key)
        {
            check_rehash();
            size_t idx = hash_func(key) % bucket_count;
            for (auto& node : buckets[idx]) {
                if (node.key == key) {
                    return node.value;
                }
            }

            buckets[idx].push_back(Node(key, T{}));
            elem_count++;
            return buckets[idx].back().value; // 其实就是默认值
        }

        T* find(const Key* key)
        {
            size_t idx = hash_func(key) % bucket_count;
            for (auto& node : buckets[idx]) {
                if (node.key == key) {
                    return &(node.value);
                }
            }
            return nullptr;
        }

        bool erase(const Key& key)
        {
            size_t idx = hash_func(key) % bucket_count;
            for (auto it = buckets[idx].begin(); it != buckets[idx].end();
                 it++) {
                if (it->key == key) {
                    buckets[idx].erase(it);
                    --elem_count;
                    return true;
                }
            }
            return false;
        }

        template<typename... Args>
        bool emplace(Args&&... args)
        {
            Node node(std::forward<Args>(args)...);
            size_t idx = hash_func(node.key) % bucket_count;
            for (auto& n : buckets[idx]) {
                if (n.key == node.key)
                    return false;
            }
            buckets[idx].push_back(std::move(node));
            ++elem_count;
            check_rehash();
            return true;
        }

        size_t size() const noexcept
        {
            return elem_count;
        }

        bool empty() const noexcept
        {
            return elem_count == 0;
        }

        void clear()
        {
            for (size_t i = 0; i < bucket_count; i++) {
                buckets[i].clear();
            }
            elem_count = 0;
        }

        size_t count(const Key& key) const
        {
            size_t idx = hash_func(key) % bucket_count;
            for (auto& node : buckets[idx]) {
                if (node.key == key) {
                    return 1;
                }
            }
            return 0;
        }

        void reserve(size_t new_bucket_count)
        {
            if (new_bucket_count > bucket_count) {
                rehash(new_bucket_count);
            }
        }

        
    };

} // namespace MySTL
