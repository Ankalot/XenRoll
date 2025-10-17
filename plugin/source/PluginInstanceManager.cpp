#include "XenRoll/PluginInstanceManager.h"

namespace audio_plugin {
PluginInstanceManager::PluginInstanceManager() {
    auto future = std::async(std::launch::async, [this]() { initAll(); });

    if (future.wait_for(std::chrono::milliseconds(initTimeoutTime)) != std::future_status::ready) {
        errorMessage = "Failed to init, deadlock occured";
        bip::shared_memory_object::remove("XenRollSharedMemory");
        bip::named_mutex::remove("XenRollMutexChannelsSheet");
        for (int i = 0; i < 16; ++i) {
            bip::named_mutex::remove(("XenRollMutexChannel" + std::to_string(i) + "Freq").c_str());
        }
        for (int i = 0; i < 16; ++i) {
            bip::named_mutex::remove(("XenRollMutexChannel" + std::to_string(i) + "Note").c_str());
        }
        checkServerFlag = false;
        runServerFlag = false;
        isActive = false;
    }
}

void PluginInstanceManager::initAll() {
    if (initSharedMemory()) {
        initInstance();
    }
}

bool PluginInstanceManager::initSharedMemory() {
    try {
        bip::permissions perm;
        perm.set_unrestricted();

        sharedMemory = std::make_unique<bip::managed_shared_memory>(
            bip::open_or_create, "XenRollSharedMemory", (18 + 512) * 1024, nullptr, perm);

        channelsSheet = sharedMemory->find_or_construct<ChannelsSheet>("ChannelsSheet")();

        chShMutex =
            std::make_unique<bip::named_mutex>(bip::open_or_create, "XenRollMutexChannelsSheet");

        return true;
    } catch (const bip::interprocess_exception &e) {
        DBG("Shared memory initialization failed (Error code: " << e.get_error_code() << ")");
        errorMessage = "Failed to init shared memory";
        bip::shared_memory_object::remove("XenRollSharedMemory");
        isActive = false;
        return false;
    }
}

void PluginInstanceManager::initInstance() {
    bip::scoped_lock<bip::named_mutex> lock(*chShMutex);

    for (int i = 0; i < 16; ++i) {
        if (!os_things::is_process_active(channelsSheet->pids[i]) ||
            !channelsSheet->instanceSlots[i]) {
            channelsSheet->instanceSlots[i] = true;
            channelsSheet->pids[i] = os_things::get_current_pid();
            channelIndex = i;
            break;
        }
    }

    if (channelIndex == -1) {
        errorMessage = "No avaible midi channels";
        return;
    }

    for (int i = 0; i < 16; ++i) {
        channelsNotes[i] = sharedMemory->find_or_construct<std::vector<Note>>(
            ("Channel" + std::to_string(i) + "Notes").c_str())();
        chNtMutex[i] = std::make_unique<bip::named_mutex>(
            bip::open_or_create, ("XenRollMutexChannel" + std::to_string(i) + "Note").c_str());
        if (i == channelIndex)
            if (channelsNotes[i])
                channelsNotes[i]->clear();
    }

    int serverIndex = channelsSheet->serverIndex;
    if ((serverIndex != -1) && channelsSheet->instanceSlots[serverIndex] &&
        os_things::is_process_active(channelsSheet->pids[serverIndex])) {
        becomeClient();
    } else {
        becomeServer();
    }
}

void PluginInstanceManager::becomeClient() {
    channelsFreqs[channelIndex] = sharedMemory->find_or_construct<ChannelFreqs>(
        ("Channel" + std::to_string(channelIndex) + "Freqs").c_str())();
    chFqMutex[channelIndex] = std::make_unique<bip::named_mutex>(
        bip::open_or_create,
        ("XenRollMutexChannel" + std::to_string(channelIndex) + "Freq").c_str());

    bip::scoped_lock<bip::named_mutex> lock(*chFqMutex[channelIndex]);
    channelsFreqs[channelIndex]->serverAction = 1;

    isActive = true;
    checkServerFlag = true;
    checkServerThread = std::thread(&PluginInstanceManager::checkServer, this);
}

void PluginInstanceManager::becomeServer() {
    if (isActive) {
        checkServerFlag = false;
        checkServerThread.detach();
        for (int i = 0; i < 16; ++i) {
            if (i == channelIndex)
                continue;
            channelsFreqs[i] = sharedMemory->find_or_construct<ChannelFreqs>(
                ("Channel" + std::to_string(i) + "Freqs").c_str())();
            chFqMutex[i] = std::make_unique<bip::named_mutex>(
                bip::open_or_create, ("XenRollMutexChannel" + std::to_string(i) + "Freq").c_str());
        }
    } else {
        for (int i = 0; i < 16; ++i) {
            channelsFreqs[i] = sharedMemory->find_or_construct<ChannelFreqs>(
                ("Channel" + std::to_string(i) + "Freqs").c_str())();
            chFqMutex[i] = std::make_unique<bip::named_mutex>(
                bip::open_or_create, ("XenRollMutexChannel" + std::to_string(i) + "Freq").c_str());
        }
    }

    for (int i = 0; i < 16; ++i) {
        if (channelsSheet->instanceSlots[i]) {
            bip::scoped_lock<bip::named_mutex> lock(*chFqMutex[i]);
            channelsFreqs[i]->serverAction = 1;
            channelsFreqs[i]->needToUpdate = true;
        }
    }

    isActive = true;
    isServer = true;
    runServerFlag = true;
    channelsSheet->serverIndex = channelIndex;
    MTS_RegisterMaster();
    runServerThread = std::thread(&PluginInstanceManager::runServer, this);
}

void PluginInstanceManager::checkServer() {
    while (checkServerFlag) {
        {
            bip::scoped_lock<bip::named_mutex> lock(*chShMutex);
            int serverIndex = channelsSheet->serverIndex;
            if ((serverIndex == -1) || !channelsSheet->instanceSlots[serverIndex] ||
                !os_things::is_process_active(channelsSheet->pids[serverIndex])) {
                becomeServer();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(checkServerDeltaTime));
    }
}

void PluginInstanceManager::runServer() {
    while (runServerFlag) {
        for (int i = 0; i < 16; ++i) {
            {
                bip::scoped_lock<bip::named_mutex> lock(*chFqMutex[i]);
                if (channelsFreqs[i]->serverAction == -1) {
                    MTS_SetMultiChannel(false, char(i));
                    channelsFreqs[i]->needToUpdate = false;
                    channelsFreqs[i]->serverAction = 0;
                    continue;
                }
                if (channelsFreqs[i]->serverAction == 1) {
                    MTS_SetMultiChannel(true, char(i));
                    channelsFreqs[i]->serverAction = 0;
                }
                if (channelsFreqs[i]->needToUpdate) {
                    MTS_SetMultiChannelNoteTunings(channelsFreqs[i]->freqs, char(i));
                    channelsFreqs[i]->needToUpdate = false;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(runServerDeltaTime));
    }
}

void PluginInstanceManager::updateFreqs(const double freqs[128]) {
    if (isActive) {
        bip::scoped_lock<bip::named_mutex> lock(*chFqMutex[channelIndex]);
        ChannelFreqs *channel = channelsFreqs[channelIndex];
        std::memcpy(channel->freqs, freqs, sizeof(double) * 128);
        channel->needToUpdate = true;
    }
}

void PluginInstanceManager::updateNotes(const std::vector<Note> &notes) {
    if (isActive) {
        bip::scoped_lock<bip::named_mutex> lock(*chNtMutex[channelIndex]);
        std::vector<Note> *channelNotes = channelsNotes[channelIndex];
        *channelNotes = notes;
    }
}

void PluginInstanceManager::changeChannelIndex(int desChInd) {
    bip::scoped_lock<bip::named_mutex> lock1(*chShMutex);

    // if desired channel is free our current channel will become abandoned
    if (!os_things::is_process_active(channelsSheet->pids[desChInd]) ||
            !channelsSheet->instanceSlots[desChInd]) {
        channelsSheet->instanceSlots[channelIndex] = false;
        channelsSheet->pids[channelIndex] = 0;

        bip::scoped_lock<bip::named_mutex> lock2(*chFqMutex[channelIndex]);
        channelsFreqs[channelIndex]->serverAction = -1;

        // because it can be not created by server yet
        chFqMutex[desChInd] = std::make_unique<bip::named_mutex>(
            bip::open_or_create,
            ("XenRollMutexChannel" + std::to_string(desChInd) + "Freq").c_str());
        bip::scoped_lock<bip::named_mutex> lock3(*chFqMutex[desChInd]);
        // because it can be not created by server yet
        channelsFreqs[desChInd] = sharedMemory->find_or_construct<ChannelFreqs>(
            ("Channel" + std::to_string(desChInd) + "Freqs").c_str())();
        channelsFreqs[desChInd]->serverAction = 1;
    }

    if (isServer) {
        channelsSheet->serverIndex = desChInd;
    }

    channelsSheet->instanceSlots[desChInd] = true;
    channelsSheet->pids[desChInd] = os_things::get_current_pid();

    channelIndex = desChInd;
}

std::vector<Note> PluginInstanceManager::getChannelsNotes(const std::set<int> chIndxs) {
    std::vector<Note> chNotes = {};
    for (const auto i : chIndxs) {
        if (channelsSheet->instanceSlots[i]) {
            bip::scoped_lock<bip::named_mutex> lock(*chNtMutex[i]);
            if (channelsNotes[i] != nullptr) {
                chNotes.insert(chNotes.end(), channelsNotes[i]->begin(), channelsNotes[i]->end());
            }
        }
    }
    return chNotes;
}

PluginInstanceManager::~PluginInstanceManager() {
    if (!isActive)
        return;

    {
        bip::scoped_lock<bip::named_mutex> lock(*chFqMutex[channelIndex]);
        channelsFreqs[channelIndex]->serverAction = -1;
    }

    if (!isServer) {
        checkServerFlag = false;
        if (checkServerThread.joinable())
            checkServerThread.join();
    }

    bip::scoped_lock<bip::named_mutex> lock(*chShMutex);

    if (isServer) {
        channelsSheet->serverIndex = -1;
        runServerFlag = false;
        if (runServerThread.joinable())
            runServerThread.join();
        MTS_DeregisterMaster();
    }

    channelsSheet->instanceSlots[channelIndex] = false;
    channelsSheet->pids[channelIndex] = 0;

    for (int i = 0; i < 16; ++i) {
        if (!os_things::is_process_active(channelsSheet->pids[i]) &&
            channelsSheet->instanceSlots[i]) {
            channelsSheet->pids[i] = 0;
            channelsSheet->instanceSlots[i] = false;
        }
        if (channelsSheet->instanceSlots[i]) {
            return;
        }
    }

    // We are 100% the last instance, so we need to clean up
    bip::shared_memory_object::remove("XenRollSharedMemory");
    bip::named_mutex::remove("XenRollMutexChannelsSheet");
    for (int i = 0; i < 16; ++i) {
        bip::named_mutex::remove(("XenRollMutexChannel" + std::to_string(i) + "Freq").c_str());
    }
    for (int i = 0; i < 16; ++i) {
        bip::named_mutex::remove(("XenRollMutexChannel" + std::to_string(i) + "Note").c_str());
    }
}
} // namespace audio_plugin