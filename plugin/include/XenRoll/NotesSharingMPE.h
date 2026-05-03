#pragma once

#include "Note.h"
#include "PlatformUtils.h"
#include <juce_audio_processors/juce_audio_processors.h>
#pragma warning(push, 0) // Disable all warnings for boost
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#pragma warning(pop) // Restore warnings
#include <atomic>
#include <chrono>
#include <thread>

namespace audio_plugin {
namespace bip = boost::interprocess;
namespace bid = boost::uuids;

/**
 * @brief The class that is responsible for notes sharing between different instances of this
 * plugin. ONLY WITH MPE TUNING.
 */
class NotesSharingMPE {
  public:
    ///< id = -1 means "Give free ID"
    NotesSharingMPE(int id = -1);
    ~NotesSharingMPE();

    /**
     * @brief Perform emergency cleanup of shared memory resources
     * @note Call this if the plugin crashes or exits unexpectedly
     */
    void performEmergencyCleanup();

    int getInstanceId() const { return instanceId; }
    bool getIsActive() const { return isActive; }

    std::set<int> getAllInstanceIds() const;

    void updateNotes(const std::vector<Note> &notes);

    void changeInstanceId(int desInstId);

    std::vector<Note> getInstancesNotes(const std::set<int> &instancesIds) const;

  private:
    // Comparator for map key in shared memory
    struct IntComparator {
        bool operator()(const int &a, const int &b) const { return a < b; }
    };

    using ShmemVector =
        bip::vector<Note, bip::allocator<Note, bip::managed_shared_memory::segment_manager>>;

    struct ShmemData {
        os_things::process_id process_id{};
        bid::uuid uniqueId;
        ShmemVector notes;

        // Default ctor needed for boost::container::map internals (try_emplace).
        // Do NOT use — notes has nullptr allocator until properly constructed.
        ShmemData()
            : notes(ShmemVector::allocator_type(nullptr)) {}

        ShmemData(os_things::process_id pid, bid::uuid uid,
                  bip::managed_shared_memory::segment_manager *seg_mgr)
            : process_id(pid), uniqueId(uid), notes(ShmemVector::allocator_type(seg_mgr)) {}
    };

    using ShmemAllocator = bip::allocator<std::pair<const int, ShmemData>,
                                          bip::managed_shared_memory::segment_manager>;

    using ShmemMap = bip::map<int, ShmemData, IntComparator, ShmemAllocator>;

    void initSharedMemory();
    void initInstance(int id);
    int findSmallestFreeId();

    const int initTimeoutTime = 5000; ///< in ms
    const int lockTimeoutTime = 500;  ///< in ms

    std::unique_ptr<bip::managed_shared_memory> sharedMemory;
    ///< Map: instanceId -> {process_id, uniqueId, notes}
    ShmemMap *instancesData = nullptr;
    std::unique_ptr<bip::named_mutex> mutex;

    int instanceId = -1;
    bid::uuid uniqueId;
    std::atomic<bool> isActive{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotesSharingMPE)
};

} // namespace audio_plugin