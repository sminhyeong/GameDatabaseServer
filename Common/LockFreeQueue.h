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
        // ���� �����͵� ����
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

            if (last == tail_.load()) {  // tail�� ������� �ʾҴ��� Ȯ��
                if (next == nullptr) {
                    // tail�� next�� �� ���� ���� �õ�
                    if (last->next.compare_exchange_weak(next, new_node)) {
                        // �����ϸ� tail�� �� ���� �̵�
                        tail_.compare_exchange_weak(last, new_node);
                        break;
                    }
                }
                else {
                    // tail�� ���� �������� �ƴϸ� tail�� ������ �̵� �õ�
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

            if (first == head_.load()) {  // head�� ������� �ʾҴ��� Ȯ��
                if (first == last) {
                    if (next == nullptr) {
                        return false;  // ť�� �������
                    }
                    // tail�� ��ó�� ������ ������ �̵� �õ�
                    tail_.compare_exchange_weak(last, next);
                }
                else {
                    if (next == nullptr) {
                        continue;  // �ٽ� �õ�
                    }

                    // ������ �б�
                    T* data = next->data.load();
                    if (data == nullptr) {
                        continue;  // �ٽ� �õ�
                    }

                    // head�� ���� ���� �̵� �õ�
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

    // �뷫���� ũ�� (��Ȯ���� ���� �� ���� - lock-free Ư����)
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

// Task�� Response�� Ưȭ ť��
//using TaskQueue = LockFreeQueue<Task>;
//using ResponseQueue = LockFreeQueue<Response>;
