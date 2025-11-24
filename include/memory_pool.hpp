#pragma once

#include <cstddef>
#include <cstdlib>
#include <vector>
#include <new>

template<typename T, size_t ChunkSize = 4096>
class MemoryPool {
    union Slot {
        T element;
        Slot* next;
    };
    
    struct alignas(64) Chunk {
        Slot slots[ChunkSize];
        Chunk* next;
    };
    
    Chunk* chunks_;
    Slot* free_list_;
    size_t allocated_count_;
    
    void allocate_chunk() {
        Chunk* chunk = static_cast<Chunk*>(
            aligned_alloc(64, sizeof(Chunk))
        );
        
        chunk->next = chunks_;
        chunks_ = chunk;
        
        for (size_t i = 0; i < ChunkSize - 1; ++i) {
            chunk->slots[i].next = &chunk->slots[i + 1];
        }
        chunk->slots[ChunkSize - 1].next = free_list_;
        free_list_ = &chunk->slots[0];
    }
    
public:
    MemoryPool() : chunks_(nullptr), free_list_(nullptr), allocated_count_(0) {
        allocate_chunk();
    }
    
    ~MemoryPool() {
        while (chunks_) {
            Chunk* next = chunks_->next;
            free(chunks_);
            chunks_ = next;
        }
    }
    
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    
    template<typename... Args>
    T* allocate(Args&&... args) {
        if (!free_list_) {
            allocate_chunk();
        }
        
        Slot* slot = free_list_;
        free_list_ = slot->next;
        ++allocated_count_;
        
        return new (&slot->element) T(std::forward<Args>(args)...);
    }
    
    void deallocate(T* ptr) {
        if (!ptr) return;
        
        ptr->~T();
        
        Slot* slot = reinterpret_cast<Slot*>(ptr);
        slot->next = free_list_;
        free_list_ = slot;
        --allocated_count_;
    }
    
    size_t allocated() const { return allocated_count_; }
};
