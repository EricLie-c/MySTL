#include <iostream>
#include <vector>
#include <iterator> // 为了 std::distance
#include <algorithm> // 为了 std::swap

// heapify 函数：维护最大堆
// It：迭代器类型
// begin: 堆数据范围的起始迭代器
// heap_size: 堆中元素的数量
// i: 需要进行 heapify 操作的节点的索引
// 其实是sift_down
template<typename RandomIt>
void heapify(RandomIt begin, typename std::iterator_traits<RandomIt>::difference_type heap_size,
    typename std::iterator_traits<RandomIt>::difference_type i) {
    auto largest = i; // 初始化最大值为当前节点
    auto left = 2 * i + 1; // 左子结点的索引
    auto right = 2 * i + 2; // 右子节点

    // 如果左子节点在堆范围内且大于当前最大值
    if (left<heap_size && *(begin + left)>*(begin + largest)) {
        largest = left;
    }

    //如果右子节点在堆范围内且大于当前最大值
    if (right<heap_size && *(begin + right)>*(begin + largest)) {
        largest = right;
    }

    if (largest != i) {
        std::swap(*(begin + i), *(begin + largest));
        heapify(begin, heap_size, largest);
    }

}

// 将一个范围内的元素构造成一个最大堆
// O(n)
template<typename RandomIt>
void build_heap(RandomIt begin, RandomIt end) {
    auto n = std::distance(begin, end);
    // 从最后一个非叶子节点开始向前heapify n/2-1
    for (auto i = n / 2 - 1;i >= 0;i--) {
        heapify(begin, n, i);
    }
}

// 堆排序
template<typename RandomIt>
void heap_sort(RandomIt begin, RandomIt end) {
    auto n = std::distance(begin, end);

    // 构建最大堆
    build_heap(begin, end);

    // 依次取出元素
    for (auto i = n - 1;i > 0;i--) {
        std::swap(*(begin), *(begin + i));
        // 缩小堆的大小，并对新的堆顶heapify
        heapify(begin, i, 0);
    }
}

// 另一种建堆方法：模拟逐个插入后sift_up.虽然总体的堆排序复杂度仍是O(n)，但仅建堆这一步复杂度较高
// i是数组中最后一个数的下标
template<typename RandomIt>
void sift_up(RandomIt begin, typename std::iterator_traits<RandomIt>::difference_type i) {
    if (i == 0) return;
    auto parent = (i - 1) / 2;
    if (*(begin + i) > *(begin + parent)) {
        std::swap(*(begin + i), *(begin + parent));
        sift_up(begin, parent);
    }
}

// 建堆
template<typename RandomIt>
void build_heap_slow(RandomIt begin, RandomIt end) {
    auto n = std::distance(begin, end);
    // 从第二个元素开始依次插入堆中
    for (auto i = 1;i < n;i++) {
        sift_up(begin, i);
    }
}

// 尽管方法有差，但是堆排序的主过程是一致的。即把最大元素放到正确位置上，然后缩小范围继续
