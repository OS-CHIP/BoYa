/*
 * Copyright 2025 OSCHIP
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "signal.h"
Signal::Signal() : id(0) {}
Signal::Signal(SignalRef id) : id(id) {}
Signal::~Signal() {}
void Signal::add_value_change(TimeTableIdx time_idx, const std::string& value) {
    time_indices.push_back(time_idx);
    values.push_back(value);
}
std::string Signal::get_value_at_time_idx(TimeTableIdx time_idx) const {
    
    for (int i = time_indices.size() - 1; i >= 0; i--) {
        if (time_indices[i] <= time_idx) {
            return values[i];
        }
    }
    
    return "";
}
void Signal::clear() {
    time_indices.clear();
    values.clear();
}