#pragma once

#include "Note.h"
#include "PlatformUtils.h"
#include <juce_audio_processors/juce_audio_processors.h>
#pragma warning(push, 0) // Disable all warnings for boost
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#pragma warning(pop) // Restore warnings
#include "libMTSMaster.h"
#include <atomic>
#include <chrono>
#include <thread>

namespace audio_plugin {
namespace bip = boost::interprocess;

struct ChannelsSheet {
    // index in instanceSlots array. -1 means there is no server
    int serverIndex = -1;
    // array that indicates what midi channels are taken by instances
    bool instanceSlots[16]{false};
    // we need pids because instance can crash and don't set instanceSlots[i] = false
    // It's that most likely everyone will be in the same process, then it turns out
    //    that the crash will be noticeable only when the DAW crashes, but it's okay.
    os_things::process_id pids[16]{0};
};

struct ChannelFreqs {
    // serverAction = 1   -  MTS_SetMultiChannel(true, midichannel)
    // serverAction = -1  -  MTS_SetMultiChannel(false, midichannel)
    // serverAction = 0   -  nothing
    int serverAction = 0;
    // needToUpdate = true   -  MTS_SetMultiChannelNoteTunings(freqs, midichannel)
    // needToUpdate = false  -  nothing
    bool needToUpdate = false;
    double freqs[128]{0.0};
};

class PluginInstanceManager {
  public:
    PluginInstanceManager();
    ~PluginInstanceManager();

    void updateFreqs(const double freqs[128]);
    void updateNotes(const std::vector<Note> &notes);
    std::vector<Note> getChannelsNotes(const std::set<int> chIndxs);
    bool getIsActive() const { return isActive; }
    int getChannelIndex() { return channelIndex; }
    std::string getErrorMessage() { return errorMessage; }

  private:
    void initAll();
    bool initSharedMemory();
    void initInstance();

    void becomeClient();
    void becomeServer();

    void checkServer();
    void runServer();

    const int initTimeoutTime = 2000; // in ms

    std::unique_ptr<bip::managed_shared_memory> sharedMemory;
    std::unique_ptr<bip::named_mutex> chShMutex;
    ChannelsSheet *channelsSheet = nullptr;

    ChannelFreqs *channelsFreqs[16]{nullptr};
    std::unique_ptr<bip::named_mutex> chFqMutex[16];

    std::vector<Note> *channelsNotes[16]{nullptr};
    std::unique_ptr<bip::named_mutex> chNtMutex[16];

    std::thread checkServerThread, runServerThread;
    std::atomic<bool> checkServerFlag, runServerFlag;
    const int checkServerDeltaTime = 500; // in ms
    const int runServerDeltaTime = 10;    // in ms

    int channelIndex = -1; // 0-15 range
    bool isActive = false;
    bool isServer = false;
    std::string errorMessage = "";
};
} // namespace audio_plugin