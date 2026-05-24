#include <algorithm>
#include <unordered_set>
#include <vector>

namespace audio_plugin {
/**
 * @brief Queue that maintains unique values in order of recent access
 * @tparam T Type of values to store
 * @note When an existing value is added again, it moves to the end (most recent).
 *       Oldest values are removed when max_size is exceeded.
 */
template <typename T> class RelevanceQueue {
  private:
    std::vector<T> values;
    std::unordered_set<T> value_set;
    size_t max_size;

  public:
    /**
     * @brief Construct a RelevanceQueue
     * @param max_size Maximum number of values to store (0 = unlimited)
     */
    explicit RelevanceQueue(size_t max_size = 0) : max_size(max_size) {}

    /**
     * @brief Add a value to the structure
     * @param value Value to add
     * @note If value already exists, it moves to the end (most recent).
     *       If max_size is exceeded, the oldest value is removed.
     */
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

    /**
     * @brief Get the last k values (most recent)
     * @param k Number of values to return
     * @return Vector of last k values (or all if k > size)
     */
    std::vector<T> getLast(size_t k) const {
        if (k >= values.size()) {
            return values;
        }
        return std::vector<T>(values.end() - k, values.end());
    }

    /**
     * @brief Get current size
     * @return Number of values stored
     */
    size_t size() const { return values.size(); }

    /**
     * @brief Check if empty
     * @return true if no values stored
     */
    bool empty() const { return values.empty(); }

    /**
     * @brief Clear the structure
     */
    void clear() {
        values.clear();
        value_set.clear();
    }
};
} // namespace audio_plugin