#include "memory_pool.h"
#include <iostream>
#include <vector>

using std::cout;

class MyObject {
public:
    MyObject(int i) :id(i) {

    }
    ~MyObject() {

    }

    // 重载 operator new
    static void* operator new(size_t size) {
        cout << "MyObject::operator new called, size = " << size << endl;
        if (size != sizeof(MyObject)) {
            cout << "  Requested size " << size << " != sizeof(MyObject) " << sizeof(MyObject) << ". Falling back to global new." << endl;
            return ::operator new(size);
        }
        return pool_.allocate();
    }
    static void operator delete(void* ptr) {
        cout << "MyObject::operator delete called for " << ptr << endl;
        if (ptr) {
            pool_.deallocate(ptr);
        }
    }

private:
    double data[4];
    int id;
    
    static MemoryPool pool_;
};

const size_t chunkSize = sizeof(MyObject);
const size_t numChunks = 100;
MemoryPool MyObject::pool_(sizeof(MyObject), 100);


int main() {

    cout << "MemoryPool created with chunkSize=" << chunkSize << ", numChunks=" << numChunks << endl;

    vector<MyObject*> objects;

    try {
        cout << "\nAllocating objects from pool:" << endl;
        for (int i = 0;i < 5;i++) {
            objects.push_back(new MyObject(i));
        }
    }
    catch (const bad_alloc& e) {
        cerr << "Allocation failed: " << e.what() << endl;
    }

    cout << "\nDeallocating objects back to pool:" << endl;

    for (MyObject* obj : objects) {
        delete obj;
    }
    objects.clear();

    cout << "\nSingle object test:" << endl;
    MyObject* single = new MyObject(99);
    delete single;

}


// 也可以不重载new delete，直接使用placement new在内存池中获取的内存块上直接构造对象
// 但这样一来，无法使用delete，必须手动调用析构函数再deallocate归还内存给pool