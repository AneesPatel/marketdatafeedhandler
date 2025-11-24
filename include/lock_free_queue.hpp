#pragma once

#include <atomic>
#include <optional>
#include <memory>
#include <new>

template<typename T>
class SPSCQueue {
    struct alignas(64) Node {
        std::atomic<uint64_t> sequence;
        T data;
    };

    static constexpr size_t CACHE_LINE = 64;
    
    alignas(CACHE_LINE) std::atomic<uint64_t> head_{0};
    alignas(CACHE_LINE) std::atomic<uint64_t> tail_{0};
    alignas(CACHE_LINE) size_t capacity_;
    alignas(CACHE_LINE) Node* buffer_;
    
    size_t mask_;

public:
    explicit SPSCQueue(size_t capacity) 
        : capacity_(capacity)
        , mask_(capacity - 1) {
        
        if ((capacity & (capacity - 1)) != 0) {
            throw std::invalid_argument("capacity must be power of 2");
        }
        
        buffer_ = static_cast<Node*>(
            aligned_alloc(CACHE_LINE, sizeof(Node) * capacity)
        );
        
        for (size_t i = 0; i < capacity; ++i) {
            new (&buffer_[i]) Node();
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }
    
    ~SPSCQueue() {
        for (size_t i = 0; i < capacity_; ++i) {
            buffer_[i].~Node();
        }
        free(buffer_);
    }
    
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    
    bool try_push(const T& item) {
        uint64_t head = head_.load(std::memory_order_relaxed);
        Node* node = &buffer_[head & mask_];
        uint64_t seq = node->sequence.load(std::memory_order_acquire);
        
        if (seq != head) {
            return false;
        }
        
        node->data = item;
        node->sequence.store(head + 1, std::memory_order_release);
        head_.store(head + 1, std::memory_order_relaxed);
        
        return true;
    }
    
    bool try_push(T&& item) {
        uint64_t head = head_.load(std::memory_order_relaxed);
        Node* node = &buffer_[head & mask_];
        uint64_t seq = node->sequence.load(std::memory_order_acquire);
        
        if (seq != head) {
            return false;
        }
        
        node->data = std::move(item);
        node->sequence.store(head + 1, std::memory_order_release);
        head_.store(head + 1, std::memory_order_relaxed);
        
        return true;
    }
    
    std::optional<T> try_pop() {
        uint64_t tail = tail_.load(std::memory_order_relaxed);
        Node* node = &buffer_[tail & mask_];
        uint64_t seq = node->sequence.load(std::memory_order_acquire);
        
        if (seq != tail + 1) {
            return std::nullopt;
        }
        
        T result = std::move(node->data);
        node->sequence.store(tail + capacity_, std::memory_order_release);
        tail_.store(tail + 1, std::memory_order_relaxed);
        
        return result;
    }
    
    size_t size() const {
        uint64_t head = head_.load(std::memory_order_acquire);
        uint64_t tail = tail_.load(std::memory_order_acquire);
        return head - tail;
    }
    
    bool empty() const {
        return size() == 0;
    }
};

template<typename T>
class MPSCQueue {
    struct alignas(64) Node {
        std::atomic<Node*> next;
        T data;
        
        Node() : next(nullptr) {}
        explicit Node(const T& val) : next(nullptr), data(val) {}
        explicit Node(T&& val) : next(nullptr), data(std::move(val)) {}
    };
    
    alignas(64) std::atomic<Node*> head_;
    alignas(64) std::atomic<Node*> tail_;
    
public:
    MPSCQueue() {
        Node* sentinel = new Node();
        head_.store(sentinel, std::memory_order_relaxed);
        tail_.store(sentinel, std::memory_order_relaxed);
    }
    
    ~MPSCQueue() {
        while (auto item = try_pop()) {}
        Node* front = head_.load(std::memory_order_relaxed);
        delete front;
    }
    
    MPSCQueue(const MPSCQueue&) = delete;
    MPSCQueue& operator=(const MPSCQueue&) = delete;
    
    void push(const T& item) {
        Node* node = new Node(item);
        Node* prev = head_.exchange(node, std::memory_order_acq_rel);
        prev->next.store(node, std::memory_order_release);
    }
    
    void push(T&& item) {
        Node* node = new Node(std::move(item));
        Node* prev = head_.exchange(node, std::memory_order_acq_rel);
        prev->next.store(node, std::memory_order_release);
    }
    
    std::optional<T> try_pop() {
        Node* tail = tail_.load(std::memory_order_relaxed);
        Node* next = tail->next.load(std::memory_order_acquire);
        
        if (next == nullptr) {
            return std::nullopt;
        }
        
        T result = std::move(next->data);
        tail_.store(next, std::memory_order_release);
        delete tail;
        
        return result;
    }
};
