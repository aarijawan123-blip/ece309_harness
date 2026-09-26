#include "core/conversation.h"

#include <limits>
#include <stdexcept>
#include <utility>

Conversation::Conversation() = default;

Conversation::~Conversation() {
    delete[] data_;
}

Conversation::Conversation(const Conversation& other) {
    if (other.size_ == 0) return;

    data_ = new Message[other.capacity_];
    try {
        for (std::size_t i = 0; i < other.size_; ++i) {
            data_[i] = other.data_[i];
        }
    } catch (...) {
        // A failed constructor does not run this object's destructor.
        delete[] data_;
        throw;
    }
    size_ = other.size_;
    capacity_ = other.capacity_;
}

Conversation& Conversation::operator=(const Conversation& other) {
    if (this != &other) {
        Conversation copy(other);
        *this = std::move(copy);
    }
    return *this;
}

Conversation::Conversation(Conversation&& other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

Conversation& Conversation::operator=(Conversation&& other) noexcept {
    if (this != &other) {
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

void Conversation::append(Message m) {
    if (m.role() == Role::System && size_ != 0) {
        throw std::invalid_argument("System message must be first");
    }

    if (size_ == capacity_) {
        const std::size_t max_capacity =
            std::numeric_limits<std::size_t>::max() / sizeof(Message);
        if (capacity_ > max_capacity / 2) {
            throw std::length_error("Conversation capacity overflow");
        }
        const std::size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
        Message* grown = new Message[new_capacity];
        // Message's string move assignment cannot throw; allocation happens first.
        for (std::size_t i = 0; i < size_; ++i) {
            grown[i] = std::move(data_[i]);
        }
        delete[] data_;
        data_ = grown;
        capacity_ = new_capacity;
    }
    data_[size_] = std::move(m);
    ++size_;
}

std::size_t Conversation::size() const noexcept {
    return size_;
}

const Message& Conversation::at(std::size_t i) const {
    if (i >= size_) {
        throw std::out_of_range("Conversation index out of range");
    }
    return data_[i];
}

const Message* Conversation::begin() const noexcept {
    return data_;
}

const Message* Conversation::end() const noexcept {
    return size_ == 0 ? data_ : data_ + size_;
}
