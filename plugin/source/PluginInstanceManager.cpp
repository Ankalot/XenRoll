#include "XenRoll/PluginInstanceManager.h"

namespace audio_plugin {
PluginInstanceManager::PluginInstanceManager() {
    std::promise<bool> initPromise;
    auto initFuture = initPromise.get_future();

    std::thread initThread([this, promise = std::move(initPromise)]() mutable {
        try {
            initAll();
            promise.set_value(true);
        } catch (...) {
            promise.set_exception(std::current_exception());
        }
    });

    if (initFuture.wait_for(std::chrono::milliseconds(initTimeoutTime)) !=
        std::future_status::ready) {
        performEmergencyCleanup();
        initThread.detach();
    } else {
        initThread.join();
        try {
            initFuture.get();
        } catch (...) {
            performEmergencyCleanup();
        }
    }
}

void PluginInstanceManager::performEmergencyCleanup() {
    errorMessage = "Failed to init, deadlock/error occured";
    bip::shared_memory_object::remove("XenRollSharedMemory");
    bip::named_mutex::remove("XenRollMutexChannelsSheet");
    bip::named_mutex::remove("XenRollMutexUpdateServerCondition");
    bip::named_condition::remove("XenRollUpdateServerCondition");
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

void PluginInstanceManager::initAll() {
    initSharedMemory();
    initInstance();
}

bool PluginInstanceManager::isServerHeartbeatOk() {
    auto now = std::chrono::system_clock::now();
    auto now_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return (now_ms - channelsSheet->serverHeartbeat) <
           (heartbeatDeltaTime + heartbeatCheckFailedExtraTime);
}

void PluginInstanceManager::initSharedMemory() {
    bip::permissions perm;
    perm.set_unrestricted();

    sharedMemory = std::make_unique<bip::managed_shared_memory>(
        bip::open_or_create, "XenRollSharedMemory", (18 + 512) * 1024, nullptr, perm);

    chShMutex =
        std::make_unique<bip::named_mutex>(bip::open_or_create, "XenRollMutexChannelsSheet");

    bip::scoped_lock<bip::named_mutex> lock(*chShMutex, bip::defer_lock);
    if (!lock.try_lock_for(std::chrono::milliseconds(lockTimeoutTime))) {
        throw std::runtime_error("Unable to acquire mutex lock within timeout - possible deadlock");
    }

    channelsSheet = sharedMemory->find_or_construct<ChannelsSheet>("ChannelsSheet")();

    updServCondMutex = std::make_unique<bip::named_mutex>(bip::open_or_create,
                                                          "XenRollMutexUpdateServerCondition");

    updateServerCondition =
        std::make_unique<bip::named_condition>(bip::open_or_create, "XenRollUpdateServerCondition");

    // CREATE/OPEN ALL MUTEXES FOR EVERYONE
    // CHECK FOR STUCK MUTEXES AFTER CRASH
    // (need to detect them so runServer and other functions don't wait forever)
    {
        bip::scoped_lock<bip::named_mutex> lock2(*updServCondMutex, bip::defer_lock);
        if (!lock2.try_lock_for(std::chrono::milliseconds(lockTimeoutTime))) {
            throw std::runtime_error("Unable to acquire mutex lock within timeout - possible deadlock");
        }
    }
    for (int i = 0; i < 16; ++i) {
        {
            chNtMutex[i] = std::make_unique<bip::named_mutex>(
                bip::open_or_create, ("XenRollMutexChannel" + std::to_string(i) + "Note").c_str());
            bip::scoped_lock<bip::named_mutex> lockNt(*chNtMutex[i], bip::defer_lock);
            if (!lockNt.try_lock_for(std::chrono::milliseconds(lockTimeoutTime))) {
                throw std::runtime_error(
                    "Unable to acquire mutex lock within timeout - possible deadlock");
            }
        }
        {
            chFqMutex[i] = std::make_unique<bip::named_mutex>(
                bip::open_or_create, ("XenRollMutexChannel" + std::to_string(i) + "Freq").c_str());

            bip::scoped_lock<bip::named_mutex> lockFq(*chFqMutex[i], bip::defer_lock);
            if (!lockFq.try_lock_for(std::chrono::milliseconds(lockTimeoutTime))) {
                throw std::runtime_error(
                    "Unable to acquire mutex lock within timeout - possible deadlock");
            }
        }
    }
}

void PluginInstanceManager::initInstance() {
    bip::scoped_lock<bip::named_mutex> lock(*chShMutex, bip::defer_lock);
    if (!lock.try_lock_for(std::chrono::milliseconds(lockTimeoutTime))) {
        throw std::runtime_error("Unable to acquire mutex lock within timeout - possible deadlock");
    }

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
        std::string notesName = "Channel" + std::to_string(i) + "Notes";
        channelsNotes[i] = sharedMemory->find_or_construct<ShmemVector>(notesName.c_str())(
            ShmemAllocator(sharedMemory->get_segment_manager()));
        if ((i == channelIndex) && channelsNotes[i]) {
            bip::scoped_lock<bip::named_mutex> lockNt(*chNtMutex[i]);
            channelsNotes[i]->clear();
        }
    }

    int serverIndex = channelsSheet->serverIndex;
    if ((serverIndex != -1) && channelsSheet->instanceSlots[serverIndex] &&
        os_things::is_process_active(channelsSheet->pids[serverIndex]) && isServerHeartbeatOk()) {
        becomeClient();
    } else {
        becomeServer();
    }
}

void PluginInstanceManager::becomeClient() {
    channelsFreqs[channelIndex] = sharedMemory->find_or_construct<ChannelFreqs>(
        ("Channel" + std::to_string(channelIndex) + "Freqs").c_str())();

    bip::scoped_lock<bip::named_mutex> lock(*chFqMutex[channelIndex]);
    channelsFreqs[channelIndex]->serverAction = 1;

    isActive = true;
    checkServerFlag = true;
    checkServerThread = std::thread(&PluginInstanceManager::checkServer, this);

    // Notify server to process the new client
    bip::scoped_lock<bip::named_mutex> lock2(*updServCondMutex);
    updateServerCondition->notify_one();
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
        }
    } else {
        for (int i = 0; i < 16; ++i) {
            channelsFreqs[i] = sharedMemory->find_or_construct<ChannelFreqs>(
                ("Channel" + std::to_string(i) + "Freqs").c_str())();
        }
    }

    for (int i = 0; i < 16; ++i) {
        if (channelsSheet->instanceSlots[i]) {
            bip::scoped_lock<bip::named_mutex> lock(*chFqMutex[i]);
            channelsFreqs[i]->serverAction = 1;
            channelsFreqs[i]->needToUpdate.store(true, std::memory_order_release);
        }
    }

    isActive = true;
    isServer = true;
    runServerFlag = true;
    channelsSheet->serverIndex = channelIndex;
    MTS_RegisterMaster();

    auto now = std::chrono::system_clock::now();
    auto now_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    channelsSheet->serverHeartbeat = now_ms;

    runServerThread = std::thread(&PluginInstanceManager::runServer, this);
}

void PluginInstanceManager::checkServer() {
    while (checkServerFlag) {
        {
            bip::scoped_lock<bip::named_mutex> lock(*chShMutex);
            int serverIndex = channelsSheet->serverIndex;
            if ((serverIndex == -1) || !channelsSheet->instanceSlots[serverIndex] ||
                !os_things::is_process_active(channelsSheet->pids[serverIndex])
                /*|| !isServerHeartbeatOk()*/) {
                // Idk, !isServerHeartbeatOk() can either improve or worsen the situation.
                // So far, it's working fine without it, so don't need it.
                becomeServer();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(checkServerDeltaTime));
    }
}

// This eats LOTS of CPU, BUT it is necessary for smooth notes' bends
// (and for no artifacts when playing note with same pitch after bended note)
void PluginInstanceManager::runServer() {
    while (runServerFlag) {
        auto now = std::chrono::system_clock::now();
        auto timestamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        if (timestamp - latestHeartbeat > heartbeatDeltaTime) {
            latestHeartbeat = timestamp;
            channelsSheet->serverHeartbeat = timestamp;
        }

        bool anyUpdates = false;
        for (int i = 0; i < 16; ++i) {
            bip::scoped_lock<bip::named_mutex> lock(*chFqMutex[i]);
            if (channelsFreqs[i]->serverAction == -1) {
                MTS_SetMultiChannel(false, char(i));
                channelsFreqs[i]->needToUpdate.store(false, std::memory_order_release);
                channelsFreqs[i]->serverAction = 0;
                anyUpdates = true;
                continue;
            }
            if (channelsFreqs[i]->serverAction == 1) {
                MTS_SetMultiChannel(true, char(i));
                channelsFreqs[i]->serverAction = 0;
                anyUpdates = true;
            }
            if (channelsFreqs[i]->needToUpdate.load(std::memory_order_acquire)) {
                MTS_SetMultiChannelNoteTunings(channelsFreqs[i]->freqs, char(i));
                channelsFreqs[i]->needToUpdate.store(false, std::memory_order_release);
                anyUpdates = true;
            }
        }

        // Wait for notification or timeout (for heartbeat)
        if (!anyUpdates) {
            bip::scoped_lock<bip::named_mutex> lock(*updServCondMutex);
            updateServerCondition->wait_for(lock, std::chrono::milliseconds(heartbeatDeltaTime));
        } else {
            std::this_thread::yield();
        }
    }
}

void PluginInstanceManager::waitChannelUpdate() {
    if (isActive) {
        ChannelFreqs *channel = channelsFreqs[channelIndex];
        while (channel->needToUpdate.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }
}

void PluginInstanceManager::updateFreqs(const double freqs[128]) {
    if (isActive) {
        bip::scoped_lock<bip::named_mutex> lock(*chFqMutex[channelIndex]);
        ChannelFreqs *channel = channelsFreqs[channelIndex];
        std::memcpy(channel->freqs, freqs, sizeof(double) * 128);
        channel->needToUpdate.store(true, std::memory_order_release);
    }
    // Notify server to wake up and process the update
    bip::scoped_lock<bip::named_mutex> lock(*updServCondMutex);
    updateServerCondition->notify_one();
}

void PluginInstanceManager::updateNotes(const std::vector<Note> &notes) {
    if (isActive) {
        bip::scoped_lock<bip::named_mutex> lock(*chNtMutex[channelIndex]);
        ShmemVector *channelNotes = channelsNotes[channelIndex];
        channelNotes->assign(notes.begin(), notes.end());
    }
}

void PluginInstanceManager::changeChannelIndex(int desChInd) {
    if (!isActive) {
        return;
    }

    bip::scoped_lock<bip::named_mutex> lock1(*chShMutex, bip::defer_lock);
    if (!lock1.try_lock_for(std::chrono::milliseconds(lockTimeoutTime))) {
        errorMessage = "Deadlock";
        checkServerFlag = false;
        runServerFlag = false;
        isActive = false;
        return;
    }

    // if desired channel is free our current channel will become abandoned
    if (!os_things::is_process_active(channelsSheet->pids[desChInd]) ||
        !channelsSheet->instanceSlots[desChInd]) {
        channelsSheet->instanceSlots[channelIndex] = false;
        channelsSheet->pids[channelIndex] = 0;

        bip::scoped_lock<bip::named_mutex> lock2(*chFqMutex[channelIndex]);
        channelsFreqs[channelIndex]->serverAction = -1;

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

    // Notify server to process the channel change
    bip::scoped_lock<bip::named_mutex> lock2(*updServCondMutex);
    updateServerCondition->notify_one();
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
        channelsSheet->serverHeartbeat = 0;
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
    bip::named_mutex::remove("XenRollMutexUpdateServerCondition");
    bip::named_condition::remove("XenRollUpdateServerCondition");
    for (int i = 0; i < 16; ++i) {
        bip::named_mutex::remove(("XenRollMutexChannel" + std::to_string(i) + "Freq").c_str());
    }
    for (int i = 0; i < 16; ++i) {
        bip::named_mutex::remove(("XenRollMutexChannel" + std::to_string(i) + "Note").c_str());
    }
}
} // namespace audio_plugin