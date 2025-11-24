#include <gtest/gtest.h>
#include "memory_pool.hpp"

TEST(MemoryPoolTest, BasicAllocation) {
    MemoryPool<int> pool;
    
    int* ptr = pool.allocate(42);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 42);
    EXPECT_EQ(pool.allocated(), 1);
    
    pool.deallocate(ptr);
    EXPECT_EQ(pool.allocated(), 0);
}

TEST(MemoryPoolTest, MultipleAllocations) {
    MemoryPool<int> pool;
    std::vector<int*> ptrs;
    
    for (int i = 0; i < 100; ++i) {
        int* ptr = pool.allocate(i);
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(*ptr, i);
        ptrs.push_back(ptr);
    }
    
    EXPECT_EQ(pool.allocated(), 100);
    
    for (auto* ptr : ptrs) {
        pool.deallocate(ptr);
    }
    
    EXPECT_EQ(pool.allocated(), 0);
}

TEST(MemoryPoolTest, ReuseMemory) {
    MemoryPool<int> pool;
    
    int* ptr1 = pool.allocate(42);
    pool.deallocate(ptr1);
    
    int* ptr2 = pool.allocate(100);
    
    EXPECT_EQ(ptr1, ptr2);
    EXPECT_EQ(*ptr2, 100);
}

TEST(MemoryPoolTest, LargeAllocations) {
    MemoryPool<int, 4096> pool;
    std::vector<int*> ptrs;
    
    for (int i = 0; i < 10000; ++i) {
        int* ptr = pool.allocate(i);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    EXPECT_EQ(pool.allocated(), 10000);
    
    for (auto* ptr : ptrs) {
        pool.deallocate(ptr);
    }
    
    EXPECT_EQ(pool.allocated(), 0);
}

TEST(MemoryPoolTest, ComplexType) {
    struct TestStruct {
        int a;
        double b;
        std::string c;
        
        TestStruct(int x, double y, const std::string& z) : a(x), b(y), c(z) {}
    };
    
    MemoryPool<TestStruct> pool;
    
    auto* obj = pool.allocate(42, 3.14, "test");
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->a, 42);
    EXPECT_DOUBLE_EQ(obj->b, 3.14);
    EXPECT_EQ(obj->c, "test");
    
    pool.deallocate(obj);
}

TEST(MemoryPoolTest, NullptrDeallocate) {
    MemoryPool<int> pool;
    
    pool.deallocate(nullptr);
    
    EXPECT_EQ(pool.allocated(), 0);
}
