#include "CircularBuffer/CircularBuffer.h"
#include <algorithm>

CircularBuffer::CircularBuffer(size_t size, size_t imageSize)
    : buffer_(size * imageSize), size_(size), imageSize_(imageSize), head_(0), count_(0) {}

void CircularBuffer::push(const uint8_t *data)
{
    std::copy(data, data + imageSize_, buffer_.begin() + (head_ * imageSize_));
    head_ = (head_ + 1) % size_;
    if (count_ < size_)
        count_++;
}

std::vector<uint8_t> CircularBuffer::get(size_t index) const
{
    if (index >= count_)
        throw std::out_of_range("Index out of range");
    size_t actualIndex = (head_ - 1 - index + size_) % size_;
    return std::vector<uint8_t>(buffer_.begin() + (actualIndex * imageSize_),
                                buffer_.begin() + ((actualIndex + 1) * imageSize_));
}

const uint8_t *CircularBuffer::getPointer(size_t index) const
{
    if (index >= count_)
        throw std::out_of_range("Index out of range");
    size_t actualIndex = (head_ - 1 - index + size_) % size_;
    return buffer_.data() + (actualIndex * imageSize_);
}

size_t CircularBuffer::size() const { return count_; }

bool CircularBuffer::isFull() const { return count_ == size_; }

CircularBuffer::Iterator CircularBuffer::begin() const { return Iterator(*this, 0); }

CircularBuffer::Iterator CircularBuffer::end() const { return Iterator(*this, count_); }

CircularBuffer::Iterator::Iterator(const CircularBuffer &buffer, size_t index)
    : buffer_(buffer), index_(index) {}

std::vector<uint8_t> CircularBuffer::Iterator::operator*() const { return buffer_.get(index_); }

CircularBuffer::Iterator &CircularBuffer::Iterator::operator++()
{
    ++index_;
    return *this;
}

bool CircularBuffer::Iterator::operator!=(const Iterator &other) const { return index_ != other.index_; }