#include "XenRoll/NotesSharingMPE.h"

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

    instancesNotes = sharedMemory->find_or_construct<ShmemMap>("InstanceNotes")(
        IntComparator(), ShmemAllocator(sharedMemory->get_segment_manager()));
}

void NotesSharingMPE::initInstance(int id) {
    bip::scoped_lock<bip::named_mutex> lock(*mutex);

    if (id >= 0) {
        instanceId = id;
    } else {
        instanceId = findSmallestFreeId();
    }

    // Create empty vector for this instance directly in the map
    // The map's allocator will handle the vector's memory in shared space
    ShmemVector emptyVector(ShmemAllocator(sharedMemory->get_segment_manager()));
    instancesNotes->emplace(instanceId, emptyVector);

    isActive = true;
}

int NotesSharingMPE::findSmallestFreeId() {
    int id = 0;
    while (instancesNotes->find(id) != instancesNotes->end()) {
        ++id;
    }
    return id;
}

NotesSharingMPE::~NotesSharingMPE() {
    if (!isActive || instanceId < 0) {
        return;
    }

    bool shouldCleanup = false;
    {
        bip::scoped_lock<bip::named_mutex> lock(*mutex);

        instancesNotes->erase(instanceId);

        // Check if we're the last instance
        if (instancesNotes->empty()) {
            shouldCleanup = true;
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

    for (auto it = instancesNotes->begin(); it != instancesNotes->end(); ++it) {
        ids.insert(it->first);
    }

    return ids;
}

void NotesSharingMPE::updateNotes(const std::vector<Note> &notes) {
    if (!isActive) {
        return;
    }

    bip::scoped_lock<bip::named_mutex> lock(*mutex);

    auto it = instancesNotes->find(instanceId);
    if (it != instancesNotes->end()) {
        it->second.assign(notes.begin(), notes.end());
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

    ShmemVector emptyVector(ShmemAllocator(sharedMemory->get_segment_manager()));
    instancesNotes->emplace(instanceId, emptyVector);

    instanceId = desInstId;
}

std::vector<Note> NotesSharingMPE::getInstancesNotes(const std::set<int> &instancesIds) const {
    std::vector<Note> result;

    if (!isActive) {
        return result;
    }

    bip::scoped_lock<bip::named_mutex> lock(*mutex);

    for (int id : instancesIds) {
        auto it = instancesNotes->find(id);
        if (it != instancesNotes->end()) {
            result.insert(result.end(), it->second.begin(), it->second.end());
        }
    }

    return result;
}

} // namespace audio_plugin