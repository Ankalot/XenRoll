#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #define _NO_RPC_HANDLE_T
    #include <windows.h>
    #undef min
    #undef max
#else
    #include <signal.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

#include "XenRoll/PlatformUtils.h"

namespace os_things {
process_id get_current_pid() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

bool is_process_active(process_id pid) {
    if (pid == 0)
        return false;

#ifdef _WIN32
    if (pid == 0)
        return false;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        return false;
    }

    DWORD exitCode;
    bool result = GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);  // Always close handle before returning
    if (result) {
        return (exitCode == STILL_ACTIVE);
    }
    return false;
#else
    return (kill(pid, 0) == 0);
#endif
}
} // namespace os_things