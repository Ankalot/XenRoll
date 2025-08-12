#include <algorithm>
#include <unordered_set>
#include <vector>

namespace audio_plugin {
template <typename T> class RelevanceQueue {
  private:
    std::vector<T> values;
    std::unordered_set<T> value_set;
    size_t max_size;

  public:
    explicit RelevanceQueue(size_t max_size = 0) : max_size(max_size) {}

    // Add a value to the structure
    void add(const T &value) {
        if (value_set.find(value) != value_set.end()) {
            auto it = std::find(values.begin(), values.end(), value);
            values.erase(it);
            values.push_back(value);
        } else {
            if (max_size > 0 && values.size() >= max_size) {
                value_set.erase(values.front());
                values.erase(values.begin());
            }
            values.push_back(value);
            value_set.insert(value);
        }
    }

    // Get the last k values (or all if k > size)
    std::vector<T> getLast(size_t k) const {
        if (k >= values.size()) {
            return values;
        }
        return std::vector<T>(values.end() - k, values.end());
    }

    // Get current size
    size_t size() const { return values.size(); }

    // Check if empty
    bool empty() const { return values.empty(); }

    // Clear the structure
    void clear() {
        values.clear();
        value_set.clear();
    }
};
} // namespace audio_plugin