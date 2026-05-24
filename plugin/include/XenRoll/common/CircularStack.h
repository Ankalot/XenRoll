#pragma once

#include <deque>

namespace audio_plugin {
template <typename T> class CircularStack {
  private:
    std::deque<T> data;
    const size_t max_size;
    int currentIndex = -1; // Current position in the stack

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
        // Remove any elements after current index (clear redo history)
        while (data.size() > (size_t)(currentIndex + 1)) {
            data.pop_back();
        }

        if (data.size() >= max_size) {
            data.pop_front();
        }
        data.push_back(value);
        currentIndex = (int)data.size() - 1;
    }

    void undo() {
        if (canUndo()) {
            currentIndex--;
        }
    }

    void redo() {
        if (canRedo()) {
            currentIndex++;
        }
    }

    bool canUndo() const { return currentIndex > 0; }
    bool canRedo() const { return currentIndex < (int)data.size() - 1; }

    T &getCurrent() { return data[currentIndex]; }
    const T &getCurrent() const { return data[currentIndex]; }

    void clearRedo() {
        while (data.size() > (size_t)(currentIndex + 1)) {
            data.pop_back();
        }
    }

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