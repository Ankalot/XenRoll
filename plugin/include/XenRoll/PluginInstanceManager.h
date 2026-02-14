#pragma once

#include "Note.h"
#include "PlatformUtils.h"
#include <juce_audio_processors/juce_audio_processors.h>
#pragma warning(push, 0) // Disable all warnings for boost
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#pragma warning(pop) // Restore warnings
#include "libMTSMaster.h"
#include <atomic>
#include <chrono>
#include <thread>

namespace audio_plugin {
namespace bip = boost::interprocess;

using ShmemAllocator = bip::allocator<Note, bip::managed_shared_memory::segment_manager>;
using ShmemVector = bip::vector<Note, ShmemAllocator>;

struct ChannelsSheet {
    ///< Index in instanceSlots array. -1 means there is no server.
    int serverIndex = -1;
    ///< Array that indicates what midi channels are taken by instances
    bool instanceSlots[16]{false};
    /**
     * We need pids because instance can crash and don't set instanceSlots[i] = false.
     * It's that most likely everyone will be in the same process, then it turns out
     * that the crash will be noticeable only when the DAW crashes, but it's okay.
     */
    os_things::process_id pids[16]{0};
    /**
     * Heartbeat timestamp (milliseconds since epoch) - updated by server.
     * Is needed because os can assign same pid to new process after crash and
     * then there won't be any server
     */
    std::atomic<uint64_t> serverHeartbeat{0};
};

struct ChannelFreqs {
    /**
     * serverAction = 1   -  MTS_SetMultiChannel(true, midichannel)
     * serverAction = -1  -  MTS_SetMultiChannel(false, midichannel)
     * serverAction = 0   -  nothing
     */
    int serverAction = 0;
    /**
     * needToUpdate = true   -  MTS_SetMultiChannelNoteTunings(freqs, midichannel)
     * needToUpdate = false  -  nothing
     */
    std::atomic<bool> needToUpdate{false};
    double freqs[128]{0.0};
};

/**
 * @brief The class that is responsible for synchronization between different instances of this
 * plugin
 */
class PluginInstanceManager {
  public:
    PluginInstanceManager();
    ~PluginInstanceManager();

    /**
     * @brief Perform emergency cleanup of shared memory resources
     * @note Call this if the plugin crashes or exits unexpectedly
     */
    void performEmergencyCleanup();

    void waitChannelUpdate();
    void updateFreqs(const double freqs[128]);
    void updateNotes(const std::vector<Note> &notes);
    /**
     * It may take up channel already occupied, but in theory, this won't happen after all instance
     * initializations are complete.
     * Also it doesn't care about notes info (doesn't move/swap it).
     */
    void changeChannelIndex(int desChInd);

    /**
     * @brief Get notes from specific channels
     * @param chIndxs Set of channel indices (0-15)
     * @return Vector of notes from the specified channels
     * @note Is needed for obtaining ghost notes (notes from other instances)
     */
    std::vector<Note> getChannelsNotes(const std::set<int> chIndxs);
    bool getIsActive() const { return isActive; }
    int getChannelIndex() { return channelIndex; }
    std::string getErrorMessage() { return errorMessage; }

  private:
    void initAll();
    void initSharedMemory();
    void initInstance();

    void becomeClient();
    void becomeServer();

    bool isServerHeartbeatOk();
    void checkServer();
    void runServer();

    const int initTimeoutTime = 5000; ///< in ms
    const int lockTimeoutTime = 500;  ///< in ms

    std::unique_ptr<bip::managed_shared_memory> sharedMemory;
    std::unique_ptr<bip::named_mutex> chShMutex;
    std::unique_ptr<bip::named_mutex> updServCondMutex;
    std::unique_ptr<bip::named_condition> updateServerCondition;
    ChannelsSheet *channelsSheet = nullptr;

    ChannelFreqs *channelsFreqs[16]{nullptr};
    std::unique_ptr<bip::named_mutex> chFqMutex[16];

    ShmemVector *channelsNotes[16]{nullptr};
    std::unique_ptr<bip::named_mutex> chNtMutex[16];

    std::thread checkServerThread, runServerThread;
    std::atomic<bool> checkServerFlag{false}, runServerFlag{false};
    const int checkServerDeltaTime = 500; ///< in ms

    //            needed if server
    const int heartbeatDeltaTime = 300;            ///< in ms
    const int heartbeatCheckFailedExtraTime = 400; ///< in ms
    uint64_t latestHeartbeat{0};                   ///< in ms, since epoch

    int channelIndex = -1; ///< 0-15 range
    std::atomic<bool> isActive{false};
    std::atomic<bool> isServer{false};
    std::string errorMessage = "";
};
} // namespace audio_plugin