#pragma once

#include <array>
#include <atomic>
#include <vector>

namespace audio_plugin {
class ChannelsManagerMPE {
  public:
    ChannelsManagerMPE(bool economyMode = false) : economyMode(economyMode) {
        channelsNumMPE.fill(0);
        channelsBendMPE.fill(-1);
    }

    void setEconomyMode(bool newEconomyMode) { economyMode = newEconomyMode; }

    /**
     * @brief Get midi channel
     * @param bendMPE Midi pitch bend of candidate note
     * @param noteWithBend Candidate note has bend (Note.bend != 0 cents)
     * @return Midi channel 2-16, or -1 if there are no free channels.
     */
    int allocateChannelMPE(int bendMPE, bool noteWithBend) {
        // First priority - find midi channel with same midi pitch bend
        // We'll start searching starting with most recently used midi channels.
        for (int i = 14; i >= 0; --i) {
            int ind = channelsIndexesMPE[i];
            if ((channelsNumMPE[ind] == 0 || !noteWithBend) && (channelsBendMPE[ind] == bendMPE) &&
                (economyMode || (channelsNumMPE[ind] == 0))) {
                channelsNumMPE[ind]++;
                if (noteWithBend) {
                    channelsBendMPE[ind] = -1;
                }
                return ind + 2;
            }
        }
        // Second priority - find free midi channel
        // We'll start searching starting with least recently used midi channels.
        for (int i = 0; i < 15; ++i) {
            int ind = channelsIndexesMPE[i];
            if (channelsNumMPE[ind] == 0) {
                if (noteWithBend) {
                    channelsBendMPE[ind] = -1;
                } else {
                    channelsBendMPE[ind] = bendMPE;
                }
                channelsNumMPE[ind] = 1;
                return ind + 2;
            }
        }
        return -1;
    }

    /**
     * @brief Call this method when releasing note (note off) (for MPE)
     * @param ch Midi channel 2-16
     */
    void noteReleasedMPE(int ch) {
        int ind = ch - 2;
        if (channelsNumMPE[ind] > 1) {
            channelsNumMPE[ind]--;
        } else if (channelsNumMPE[ind] == 1) {
            channelsNumMPE[ind] = 0;
            // Send the midi channel to the end of the "relevance queue"
            auto it = std::find(channelsIndexesMPE.begin(), channelsIndexesMPE.end(), ch - 2);
            if (it != channelsIndexesMPE.end()) {
                std::rotate(it, it + 1, channelsIndexesMPE.end());
            }
        }
    }

    int getNumNotesInChannel(int ch) {
        int ind = ch - 2;
        return channelsNumMPE[ind];
    }

  private:
    ///< Value represents number of currently playing notes in midi channel (2-16)
    std::array<int, 15> channelsNumMPE;
    /**
     * If false: Each note must have is't own midi channel, so max number in channelsNumMPE
     *           is 1.
     */
    std::atomic<bool> economyMode = false;
    /**
     * Value represents midi pitch bend (0-16383) for midi channel (2-16) that is currently
     * used. Value -1 means that this midi channel has note with bend, so other notes should
     * not occupy this channel (because in this midi channel pitch bend is not constant).
     */
    std::array<int, 15> channelsBendMPE;
    /**
     * When the same channel is reused almost immediately for another note with a different
     * pitch bend, artifacts may appear in some synthesizers. Therefore, this vector serves as
     * a queue. When the channel is released, it is moved to the end of the queue to be used
     * last.
     */
    std::vector<int> channelsIndexesMPE = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
};
} // namespace audio_plugin