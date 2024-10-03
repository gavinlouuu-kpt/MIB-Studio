#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>

class CircularBuffer {
public:
    CircularBuffer(size_t size, size_t imageSize);
    void push(const uint8_t* data);
    std::vector<uint8_t> get(size_t index) const;
    const uint8_t* getPointer(size_t index) const;
    size_t size() const;
    bool isFull() const;

    class Iterator {
    public:
        Iterator(const CircularBuffer& buffer, size_t index);
        std::vector<uint8_t> operator*() const;
        Iterator& operator++();
        bool operator!=(const Iterator& other) const;
    private:
        const CircularBuffer& buffer_;
        size_t index_;
    };

    Iterator begin() const;
    Iterator end() const;

private:
    std::vector<uint8_t> buffer_;
    size_t size_;
    size_t imageSize_;
    size_t head_;
    size_t count_;
};