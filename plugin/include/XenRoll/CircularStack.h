#pragma once

#include <deque>

namespace audio_plugin {
template <typename T> class CircularStack {
  private:
    std::deque<T> data;
    const size_t max_size;

  public:
    CircularStack(size_t max) : max_size(max) {}

    void push(T value) {
        if (data.size() >= max_size) {
            data.pop_front();
        }
        data.push_back(value);
    }

    void pop() { data.pop_back(); }
    T &top() { return data.back(); }
    bool empty() const { return data.empty(); }
    size_t size() const { return data.size(); }

    T &bottom() { return data.front(); }
};
} // namespace audio_plugin