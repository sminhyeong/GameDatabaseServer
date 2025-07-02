#pragma once
#include <atomic>
#include <memory>

template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::atomic<T*> data;
        std::atomic<Node*> next;

        Node() : data(nullptr), next(nullptr) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

public:
    LockFreeQueue() {
        Node* dummy = new Node;
        head_.store(dummy);
        tail_.store(dummy);
    }

    ~LockFreeQueue() {
        // 남은 데이터들 정리
        while (Node* const old_head = head_.load()) {
            head_.store(old_head->next);
            T* data = old_head->data.load();
            if (data) {
                delete data;
            }
            delete old_head;
        }
    }

    void enqueue(const T& item) {
        Node* new_node = new Node;
        T* data = new T(std::move(item));
        new_node->data.store(data);

        while (true) {
            Node* last = tail_.load();
            Node* next = last->next.load();

            if (last == tail_.load()) {  // tail이 변경되지 않았는지 확인
                if (next == nullptr) {
                    // tail의 next를 새 노드로 설정 시도
                    if (last->next.compare_exchange_weak(next, new_node)) {
                        // 성공하면 tail을 새 노드로 이동
                        tail_.compare_exchange_weak(last, new_node);
                        break;
                    }
                }
                else {
                    // tail이 실제 마지막이 아니면 tail을 앞으로 이동 시도
                    tail_.compare_exchange_weak(last, next);
                }
            }
        }
    }

    bool dequeue(T& result) {
        while (true) {
            Node* first = head_.load();
            Node* last = tail_.load();
            Node* next = first->next.load();

            if (first == head_.load()) {  // head가 변경되지 않았는지 확인
                if (first == last) {
                    if (next == nullptr) {
                        return false;  // 큐가 비어있음
                    }
                    // tail이 뒤처져 있으면 앞으로 이동 시도
                    tail_.compare_exchange_weak(last, next);
                }
                else {
                    if (next == nullptr) {
                        continue;  // 다시 시도
                    }

                    // 데이터 읽기
                    T* data = next->data.load();
                    if (data == nullptr) {
                        continue;  // 다시 시도
                    }

                    // head를 다음 노드로 이동 시도
                    if (head_.compare_exchange_weak(first, next)) {
                        result = *data;
                        delete data;
                        delete first;
                        return true;
                    }
                }
            }
        }
    }

    bool empty() const {
        Node* first = head_.load();
        Node* last = tail_.load();
        return (first == last) && (first->next.load() == nullptr);
    }

    // 대략적인 크기 (정확하지 않을 수 있음 - lock-free 특성상)
    size_t size() const {
        size_t count = 0;
        Node* current = head_.load()->next.load();
        while (current != nullptr) {
            ++count;
            current = current->next.load();
        }
        return count;
    }
};

// Task와 Response용 특화 큐들
//using TaskQueue = LockFreeQueue<Task>;
//using ResponseQueue = LockFreeQueue<Response>;
