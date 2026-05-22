#pragma once

#include "XenRoll/Note.h"
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <unordered_map>

namespace audio_plugin {

class NotePathManager {
  public:
    NotePathManager(int numOctaves) : numOctaves(numOctaves) {};

    juce::Path getPath(const Note &note, float barWidthPx, float octaveHeightPx, float noteHeight,
                       float noteRoundCoef) const;

    ///< Call this when scale in main panel is changed or note height/rounding in settings
    void invalidateCache() { cache.clear(); }

  private:
    const int numOctaves;

    ///< Is used to scale duration in CacheKey & to round corner radius in path (tol)
    static constexpr float MAX_PX_SCALE = 4.0f;
    static constexpr size_t MAX_CACHE_SIZE = 1000; // 1000 elements = ~0.5 Mb total

    ///< Cache key, contains only params that affect the shape of note
    struct CacheKey {
        int durationScaledPx; ///< round(note.duration * barWidthPx * MAX_PX_SCALE)
        int bendCents;        ///< note.bend

        bool operator==(const CacheKey &other) const noexcept {
            return durationScaledPx == other.durationScaledPx && bendCents == other.bendCents;
        }
    };

    struct CacheKeyHash {
        std::size_t operator()(const CacheKey &k) const noexcept {
            std::size_t h1 = static_cast<std::size_t>(k.durationScaledPx);
            std::size_t h2 = static_cast<std::size_t>(k.bendCents);

            // Standard gold-ratio hash combination
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    ///< CacheKey -> Canonical path (x1 = y1 = 0)
    mutable std::unordered_map<CacheKey, juce::Path, CacheKeyHash> cache;

    /**
     * @brief Build canonical path (x1 = y1 = 0)
     * @param w Note width in px
     * @param dy Note bend in px
     * @param h Note height in px
     * @param noteRoundCoef Corner rounding coefficient in range [0.0, 1.0]
     * @return juce::Path Canonical path
     */
    juce::Path buildCanonicalPath(float w, float dy, float h, float noteRoundCoef) const;
};

} // namespace audio_plugin