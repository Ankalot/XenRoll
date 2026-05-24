#include "XenRoll/processor/managers/NotesSharingMPE.h"
#include <chrono>
#include <future>
#include <thread>

namespace audio_plugin {

NotesSharingMPE::NotesSharingMPE(int id) {
    std::promise<bool> initPromise;
    auto initFuture = initPromise.get_future();

    std::thread initThread([this, promise = std::move(initPromise), id]() mutable {
        try {
            initSharedMemory();
            initInstance(id);
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

void NotesSharingMPE::performEmergencyCleanup() {
    bip::shared_memory_object::remove("NotesSharingMPESharedMemory");
    bip::named_mutex::remove("NotesSharingMPEMutex");
    instanceId = -1;
    isActive = false;
}

void NotesSharingMPE::initSharedMemory() {
    bip::permissions perm;
    perm.set_unrestricted();

    const size_t totalSize = 2 * 1024 * 1024; // 2MB

    sharedMemory = std::make_unique<bip::managed_shared_memory>(
        bip::open_or_create, "NotesSharingMPESharedMemory", totalSize, nullptr, perm);

    mutex = std::make_unique<bip::named_mutex>(bip::open_or_create, "NotesSharingMPEMutex");

    bip::scoped_lock<bip::named_mutex> lock(*mutex, bip::defer_lock);
    if (!lock.try_lock_for(std::chrono::milliseconds(lockTimeoutTime))) {
        throw std::runtime_error("Unable to acquire mutex lock within timeout - possible deadlock");
    }

    instancesData = sharedMemory->find_or_construct<ShmemMap>("InstancesData")(
        IntComparator(), ShmemAllocator(sharedMemory->get_segment_manager()));
}

void NotesSharingMPE::initInstance(int id) {
    bip::scoped_lock<bip::named_mutex> lock(*mutex);

    if (id >= 0) {
        instanceId = id;
    } else {
        instanceId = findSmallestFreeId();
    }

    uniqueId = boost::uuids::random_generator()();
    os_things::process_id myProcessId = os_things::get_current_pid();

    instancesData->emplace(
        std::piecewise_construct, std::forward_as_tuple(instanceId),
        std::forward_as_tuple(myProcessId, uniqueId, sharedMemory->get_segment_manager()));

    isActive = true;
}

int NotesSharingMPE::findSmallestFreeId() {
    int id = 0;
    while (true) {
        auto it = instancesData->find(id);
        if (it != instancesData->end()) {
            if (!os_things::is_process_active(it->second.process_id)) {
                return id;
            }
        } else {
            return id;
        }
        ++id;
    }
}

NotesSharingMPE::~NotesSharingMPE() {
    if (!isActive || instanceId < 0) {
        return;
    }

    bool shouldCleanup = true;
    {
        bip::scoped_lock<bip::named_mutex> lock(*mutex);

        instancesData->erase(instanceId);

        // Check if we're the last instance
        for (const auto &[id, val] : *instancesData) {
            if (os_things::is_process_active(val.process_id)) {
                shouldCleanup = false;
                break;
            }
        }
    }

    // Clean up outside the lock to avoid holding it during removal
    if (shouldCleanup) {
        bip::shared_memory_object::remove("NotesSharingMPESharedMemory");
        bip::named_mutex::remove("NotesSharingMPEMutex");
    }

    isActive = false;
    instanceId = -1;
}

std::set<int> NotesSharingMPE::getAllInstanceIds() const {
    std::set<int> ids;

    if (!isActive) {
        return ids;
    }

    bip::scoped_lock<bip::named_mutex> lock(*mutex);

    for (auto it = instancesData->begin(); it != instancesData->end(); ++it) {
        ids.insert(it->first);
    }

    return ids;
}

void NotesSharingMPE::updateNotes(const std::vector<Note> &notes) {
    if (!isActive) {
        return;
    }

    bip::scoped_lock<bip::named_mutex> lock(*mutex);

    auto it = instancesData->find(instanceId);
    if (it != instancesData->end()) {
        it->second.notes.assign(notes.begin(), notes.end());
    }
}

void NotesSharingMPE::changeInstanceId(int desInstId) {
    if (!isActive) {
        return;
    }

    bip::scoped_lock<bip::named_mutex> lock(*mutex, bip::defer_lock);
    if (!lock.try_lock_for(std::chrono::milliseconds(lockTimeoutTime))) {
        isActive = false;
        return;
    }

    // Delete our previous id
    if (instancesData->contains(instanceId) && (*instancesData)[instanceId].uniqueId == uniqueId) {
        instancesData->erase(instanceId);
    }

    os_things::process_id myProcessId = os_things::get_current_pid();
    instancesData->emplace(
        std::piecewise_construct, std::forward_as_tuple(desInstId),
        std::forward_as_tuple(myProcessId, uniqueId, sharedMemory->get_segment_manager()));

    instanceId = desInstId;
}

std::vector<Note> NotesSharingMPE::getInstancesNotes(const std::set<int> &instancesIds) const {
    std::vector<Note> result;

    if (!isActive) {
        return result;
    }

    bip::scoped_lock<bip::named_mutex> lock(*mutex);

    for (int id : instancesIds) {
        auto it = instancesData->find(id);
        if (it != instancesData->end()) {
            result.insert(result.end(), it->second.notes.begin(), it->second.notes.end());
        }
    }

    return result;
}

} // namespace audio_plugin