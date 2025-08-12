#pragma once

namespace os_things {
using process_id = unsigned long;

process_id get_current_pid();
bool is_process_active(process_id pid);
}; // namespace os_things