#pragma once
#include <type_traits>
#include <utility> // move pair
#include <cstddef> // size_t

// 颜色 enum
enum Color
{
    RED,
    BLACK
};

// 红黑树节点
template<typename Key, typename Value>
struct rb_tree_node
{
    std::pair<const Key, Value> data;
    Color color;
    rb_tree_node* parent;
    rb_tree_node* left;
    rb_tree_node* right;

    rb_tree_node(const Key& k, const Value& v)
        : data(k, v),
          color(RED),
          parent(nullptr),
          left(nullptr),
          right(nullptr)
    {
    }
};

// 本质上是二叉搜索树BST 加上6条性质：
//
// 1. 每个节点要么是红色，要么是黑色。
// 2. 根节点是黑色。
// 3. 所有叶子节点（空节点 NIL）都是黑色。
// 4. 每个红色节点的两个子节点都是黑色（不能有两个连续的红色节点）。
// 5. 从任一节点到其所有后代叶子节点的路径上，经过的黑色节点数目相同。
// 6. 新插入的节点总是红色。
//
template<typename Key, typename Value>
class rb_tree
{
public:
    using Node = rb_tree_node<Key, Value>;

    class iterator
    {
    public:
        iterator(Node* node, const rb_tree* tree): node_(node), tree_(tree) {}
        std::pair<const Key, Value>& operator*() const
        {
            return node_->data;
        }
        std::pair<const Key, Value>* operator->() const
        {
            return &(node_->data);
        }
        // 前置自增
        iterator& operator++()
        {
            node_ = tree_->successor(node_);
            return *this;
        }
        // 后置自增
        iterator& operator++(int)
        {
            auto tmp = this;
            node_ = tree_->successor(node_);
            return *tmp;
        }
        bool operator!=(const iterator& other) const
        {
            return node_ != other.node_;
        }

    private:
        const rb_tree* tree_;
        Node* node_;
        friend class rb_tree;
    };

    rb_tree(): root_(nullptr), size_(0) {}
    ~rb_tree()
    {
        clear(root_);
    }

    iterator begin() const
    {
        return iterator(minimum(root_), this);
    }
    iterator end() const
    {
        return iterator(nullptr, this); // ？？？
    }
    size_t size() const
    {
        return size_;
    }
    bool empty() const
    {
        return size_ == 0;
    }

    // 当二叉搜索树找就行
    iterator find(const Key& key) const
    {
        Node* cur = root_;
        while (cur) {
            if (key < cur->data.first)
                cur = cur->left;
            else if (key > cur->data.first)
                cur = cur->right;
            else
                return iterator(cur, this);
        }
    }

    std::pair<iterator, bool> insert(const std::pair<Key, Value>& kv)
    {
        Node* parent = nullptr;
        Node* cur = root_;
        while (cur) {
            parent = cur;
            if (kv.first < cur->data.first)
                cur = cur->left;
            else if (kv.first > cur->data.first)
                cur = cur->right;
            else
                return {iterator(cur, this),
                        false}; // 插入失败，已存在，返回迭代器
        }
        Node* node = new Node(kv.first, kv.second);
        node->parent = parent;

        if (!parent) // 只有可能root一开始就是空的
            root_ = node;
        // 放到正确的位置上
        else if (kv.first < parent->data.first) {
            parent->left = node;
        } else {
            parent->right = node;
        }
        fix_insert(node);
        ++size_;
        return {iterator(node), true};
    }

    bool erase(const Key& key)
    {
        Node* node = root_;
        // 查找要删除的节点
        while (node) {
            if (key < node->data.first)
                node = node->left;
            else if (key > node->data.first)
                node = node->right;
            else
                break; // 找到了
        }
        if (!node)
            return false; // 不存在

        Node* y = node;    // y是实际要删除或被替换的节点
        Node* x = nullptr; // x是y的子节点（可能为空）
        Color y_original_color =
                y->color; // 记录y(确切地说是被移除的节点)原来的颜色

        // 如果有两个孩子，就用后继节点替换
        if (node->left && node->right) {
            y = minimum(node->right); // 找到右子树的最小后继节点
            y_original_color = y->color;
            x = y->right;
            if (y->parent == node) {
                if (x)
                    x->parent = y;
            } else {
                transplant(y, y->right); // 用y的右子节点替换y，因为y要往上面走

                y->right = node->right; // y接管node的右子节点
                if (y->right)
                    y->right->parent = y;
            }
            transplant(node, y);  // 用y替换node
                                  //
            y->left = node->left; // y接管node的左子节点
            if (y->left)
                y->left->parent = y;

            y->color = node->color; // 保持原有颜色
        } else {
            // 只有一个孩子或没有孩子
            x = node->left ? node->left : node->right;
            transplant(node, x); // 用x替换node
        }

        // 如果被删除的节点是黑色，需要恢复红黑树性质
        if (y_original_color == BLACK)
            fix_delete(x);

        delete node; // 释放内存
        --size_;
        return true;
    }

private:
    friend class iterator;
    Node* root_;
    size_t size_;

    // 递归删除
    void clear(Node* node)
    {
        if (!node)
            return;
        clear(node->left);
        clear(node->right);
        delete node;
    }

    // 返回以当前节点为根的最左侧节点（其实就是最小的节点）
    Node* minimum(Node* node) const
    {
        if (!node)
            return nullptr;
        while (node->left)
            node = node->left;
        return node;
    }

    // 返回Node的中序后继节点
    Node* successor(Node* node) const
    {
        if (!node)
            return nullptr;

        // 如果有右子节点，说明自己也是一个小根节点，那么下一步是从右子树的最左侧开始遍历
        if (node->right)
            return minimum(node->right);

        Node* p = node->parent;

        // 如果当前节点是父节点的右子节点，说明右子树已经遍历完成
        // 说明当前子树已经遍历完了，需要继续往上走
        while (p && node == p->right) {
            node = p;
            p = p->parent;
        }

        return p;
    }

    // 将某个节点的右子节点提升为父节点，原节点变为左子节点
    //     x                 y
    //    / \               / \
    //   a   y    左旋     x   c
    //      / \   --->    / \
    //     b   c         a   b
    void rotate_left(Node* x)
    {
        Node* y = x->right;

        // 改x的右子节点 以及x右子节点的父节点
        x->right = y->left;
        if (y->left)
            y->left->parent = x;

        y->parent = x->parent;

        // 如果x没有父节点了（x是根节点），那么直接赋值
        if (!x->parent)
            root_ = y;
        else if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;

        y->left = x;
        x->parent = y;
    }

    // 将某个节点的左孩子提升为父节点，原节点变为右孩子。
    //       y             x
    //      / \           / \
    //     x   c 右旋    a   y
    //    / \    --->      / \
    //   a   b            b   c
    void rotate_right(Node* y)
    {
        Node* x = y->left;
        // 因为x自己的左节点要保留，只能给出右节点
        y->left = x->right;
        if (x->right)
            x->right->parent = y;
        x->parent = y->parent;
        if (!y->parent)
            root_ = x;
        else if (y == y->parent->left)
            y->parent->left = x;
        else
            y->parent->right = x;
        x->right = y;
        y->parent = x;
    }
    // 上面这两个rotate并不破坏大小关系，只是树高的调整

    void fix_insert(Node* node)
    {
        // 新插入的节点是红色，父节点又是红色，需要修复
        while (node != root_ && node->parent->color == RED) {
            Node* parent = node->parent;
            Node* grandparent = parent->parent;
            if (parent == grandparent->left) {
                Node* uncle = grandparent->right;
                // 若叔叔节点也为红色，则全部转为黑色，祖父节点转为红色
                // 这样路径上的黑色节点数目可以保持不变
                if (uncle && uncle->color == RED) {
                    parent->color = BLACK;
                    uncle->color = BLACK;
                    grandparent->color = RED;
                    node = grandparent; // 继续往上检查有无连续红色节点
                } else {
                    // 此时叔叔节点为黑色，需要左旋，以使得新节点都统一在外侧
                    if (node == parent->right) {
                        node = parent;
                        rotate_left(node);
                    }
                    parent->color = BLACK;
                    grandparent->color = RED;
                    rotate_right(grandparent);
                }
            } else {
                // 和上面反过来
                Node* uncle = grandparent->left;
                if (uncle && uncle->color == RED) {
                    parent->color = BLACK;
                    uncle->color = BLACK;
                    grandparent->color = RED;
                    node = grandparent;
                } else {
                    if (node == parent->left) {
                        node = parent;
                        rotate_right(node);
                    }
                    parent->color = BLACK;
                    grandparent->color = RED;
                    rotate_left(grandparent);
                }
            }
        }
        root_->color = BLACK; // 根节点保持黑色
    }

    // x是用来替换的节点
    void fix_delete(Node* x)
    {
        Node* parent = x->parent;
        // x不是根且x是黑色（或为空），需要修复
        // 因为少了一个黑色（被替换了），这也是执行这个函数的原因
        while (x != root_ && (!x || x->color == BLACK)) {
            if (x == parent->left) {
                Node* w = parent->right; // 兄弟节点
                // case1: 兄弟节点为红色
                if (w && w->color == RED) {
                    w->color = BLACK;
                    parent->color = RED;
                    rotate_left(parent);
                    w= parent->right;
                }
                // case2: 兄弟为黑色，且兄弟的两个孩子都是黑色
                if ((!w->left || w->left->color == BLACK) &&
                    (!w->right || w->right->color == BLACK)) {
                    if (w)
                        w->color = RED;
                    x = parent;
                    parent = x->parent;
                } else {
                    // 情况3：兄弟是黑色，兄弟的左孩子是红色，右孩子是黑色
                    if (!w->right || w->right->color == BLACK) {
                        if (w->left)
                            w->left->color = BLACK;
                        w->color = RED;
                        rotate_right(w);
                        w = parent->right;
                    }
                    // 情况4：兄弟是黑色，兄弟的右孩子是红色
                    if (w)
                        w->color = parent->color;
                    parent->color = BLACK;
                    if (w && w->right)
                        w->right->color = BLACK;
                    rotate_left(parent);
                    x = root_;
                }
            } else {
                // 对称处理，x 是右孩子
                Node* w = parent->left;
                if (w && w->color == RED) {
                    w->color = BLACK;
                    parent->color = RED;
                    rotate_right(parent);
                    w = parent->left;
                }
                if ((!w->left || w->left->color == BLACK) &&
                    (!w->right || w->right->color == BLACK)) {
                    if (w)
                        w->color = RED;
                    x = parent;
                } else {
                    if (!w->left || w->left->color == BLACK) {
                        if (w->right)
                            w->right->color = BLACK;
                        w->color = RED;
                        rotate_left(w);
                        w = parent->left;
                    }
                    if (w)
                        w->color = parent->color;
                    parent->color = BLACK;
                    if (w && w->left)
                        w->left->color = BLACK;
                    rotate_right(parent);
                    x = root_;
                }
            }
        }
        if (x)
            x->color = BLACK;
    }

    // 用v替换u
    void transplant(Node* u, Node* v)
    {
        if (!u->parent)
            root_ = v;
        else if (u == u->parent->left)
            u->parent->left = v;
        else
            u->parent->right = v;
        if (v)
            v->parent = u->parent;
    }
};