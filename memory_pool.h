#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <vector>
#include <stdexcept>
#include <iostream>

using std::vector;
using std::bad_alloc;
using std::invalid_argument; // 继承自logic_error，logic_error继承自exception
using std::cerr;
using std::endl;

class MemoryPool {
public:
    // chunkSize: 每个内存块的大小（字节）
    // numChunks: 内存池中块的总数
    explicit MemoryPool(size_t chunkSize, size_t numChunks) : chunkSize_(chunkSize), numChunks_(numChunks),
        pool_(nullptr), freeListHead_(nullptr) {
        if (chunkSize == 0 || numChunks == 0) {
            throw invalid_argument("Chunk size and number of chunks must be positive.");
        }
        // 确保每个块至少能容纳一个指针的大小，因为将用它来构建自由列表
        if (chunkSize_ < sizeof(unsigned char*)) {
            throw invalid_argument("Chunk size must be at least the size of a pointer.");
        }

        size_t totalPoolSize = chunkSize_ * numChunks_;
        try {
            // 申请一个totalPoolSize大小内存块，每个元素都解释为1字节的纯内存块，返回首地址
            pool_ = new unsigned char[totalPoolSize];
        }
        catch (const bad_alloc& e) {
            cerr << "MemoryPool: Failed to allocate memory: " << e.what() << endl;
            throw; // 重新抛出当前正在处理的异常
        }

        initializePool();
    }

    ~MemoryPool() {
        delete[] pool_;
        pool_ = nullptr;
        freeListHead_ = nullptr;
    }

    // 禁止拷贝构造和赋值操作
    MemoryPool(const MemoryPool&) = delete;
    // 返回值为引用类型是为了实现链式赋值
    MemoryPool& operator=(const MemoryPool&) = delete;

    void* allocate() {
        if (freeListHead_ == nullptr) {
            // 没有可用的空闲块了
            // 选择抛出 std::bad_alloc 异常
            // 调用默认构造函数，生成匿名对象
            throw bad_alloc();
        }
        // 从自由列表中取出一个块
        unsigned char* blockToAllocate = freeListHead_;

        // 更新自由列表的头部，使其指向下一个空闲块
        // 我们之前在 blockToAllocate 的起始位置存储了下一个空闲块的地址
        freeListHead_ = *reinterpret_cast<unsigned char**> (freeListHead_);

        return static_cast<void*> (blockToAllocate);
    }


    void deallocate(void* ptr) {
        if (ptr == nullptr) {
            return; // 释放空指针是无害的，直接返回
        }
        // 进行一些基本的指针校验，确保它属于我们的内存池范围
        // 这可以帮助捕获一些错误，但会增加开销
        // 注意：这个检查并不完美，因为 ptr 可能在范围内但不是一个块的起始地址
        // 或者它可能是一个已经被释放的块。更复杂的校验需要更多信息。
        /*
        if (static_cast<unsigned char*>(ptr) < pool_ ||
            static_cast<unsigned char*>(ptr) >= pool_ + (numChunks_ * chunkSize_)) {
            // 指针不在内存池管理的范围内，这可能是一个错误
            // 可以选择抛出异常、记录日志或忽略
            std::cerr << "MemoryPool: Attempting to deallocate memory not managed by this pool." << std::endl;
            // throw std::invalid_argument("Pointer out of pool bounds");
            return; // 或者直接返回，避免崩溃
        }
        // 还可以检查 ptr 是否按 chunkSize_ 对齐
        if ((static_cast<unsigned char*>(ptr) - pool_) % chunkSize_ != 0) {
            std::cerr << "MemoryPool: Attempting to deallocate misaligned pointer." << std::endl;
            // throw std::invalid_argument("Misaligned pointer deallocation");
            return;
        }
        */

        unsigned char* blockToDeallocate = static_cast<unsigned char*> (ptr);
        // 将释放的块重新添加到自由列表的头部
        // 1. 让这个被释放的块指向原自由列表的头部
        // 把 blockToDeallocate 指向的这块内存的前N个字节（N = 指针大小）当作一个指针变量来对待，
        // 然后把 freeListHead_ 的值赋给这个‘指针变量’。
        // blockToDeallocate 仅代表首字节地址
        *reinterpret_cast<unsigned char**> (blockToDeallocate) = freeListHead_;
        // 2. 更新自由列表的头部为这个刚被释放的块
        freeListHead_ = blockToDeallocate;

    }

private:
    size_t chunkSize_;
    size_t numChunks_;
    // 使用unsigned是因为不同平台往往char的默认类型不一样，可能signed可能unsigned
    // 此时明确指定，以免歧义。使用unsigned时可以看作一个字节大小的内存块，很方便
    unsigned char* pool_; // 指向内存池起始位置的指针，一个指向我们预分配的大块内存区域的指针
    unsigned char* freeListHead_; // 指向自由列表头部的指针，一个指向当前第一个可用内存块的指针

    void initializePool() {
        // 切割大内存块为chunk，并链接到自由列表中
        freeListHead_ = pool_;

        for (size_t i = 0;i < numChunks_;i++) {
            // 计算当前chunk的起始地址
            unsigned char* currentChunk = pool_ + (i * chunkSize_);
            unsigned char* nextChunk = nullptr;

            if (i < numChunks_ - 1) {
                nextChunk = pool_ + ((i + 1) * chunkSize_);
            }

            // 难点来了
            // 将当前块的起始位置解释为一个指向下一个空闲块的指针，并存储 nextChunk 的地址
            // 这就是所谓的“侵入式链表” (intrusive list)
            // 我们将 nextChunk 的地址写入到 currentChunk 指向的内存位置
            *reinterpret_cast<unsigned char**>(currentChunk) = nextChunk;
        }
    }

};

#endif