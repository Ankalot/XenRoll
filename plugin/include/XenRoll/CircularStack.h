#pragma once

#include <deque>

namespace audio_plugin {
template <typename T> class CircularStack {
  private:
    std::deque<T> data;
    const size_t max_size;

  public:
    /**
     * @brief Construct a CircularStack
     * @param max Maximum number of elements
     */
    CircularStack(size_t max) : max_size(max) {}

    /**
     * @brief Push a value onto the stack
     * @param value Value to push
     * @note If stack is full, oldest element is removed
     */
    void push(T value) {
        if (data.size() >= max_size) {
            data.pop_front();
        }
        data.push_back(value);
    }

    /**
     * @brief Pop the top element from the stack
     * @warning Not safe if stack is empty
     */
    void pop() { data.pop_back(); }

    /**
     * @brief Get the top (newest) element
     * @warning Not safe if stack is empty
     * @return Reference to the top element
     */
    T &top() { return data.back(); }

    /**
     * @brief Check if the stack is empty
     * @return true if stack is empty
     */
    bool empty() const { return data.empty(); }

    /**
     * @brief Get the current size of the stack
     * @return Number of elements in the stack
     */
    size_t size() const { return data.size(); }

    /**
     * @brief Get the bottom (oldest) element
     * @warning Not safe if stack is empty
     * @return Reference to the bottom element
     */
    T &bottom() { return data.front(); }
};
} // namespace audio_plugin