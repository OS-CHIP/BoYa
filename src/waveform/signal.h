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

#pragma once
#include "waveform_types.h"
#include <vector>
#include <string>
class Signal {
private:
    SignalRef id;
    SignalType type;
    std::vector<TimeTableIdx> time_indices; 
    std::vector<std::string> values; 
public:
    Signal();
    Signal(SignalRef id);
    ~Signal();
    
    void add_value_change(TimeTableIdx time_idx, const std::string& value);
    
    std::string get_value_at_time_idx(TimeTableIdx time_idx) const;
    
    SignalRef get_id() const { return id; }
    
    SignalType get_type() const { return type; }
    
    size_t get_change_count() const { return values.size(); }
    std::vector<TimeTableIdx> get_time_indices() const { return time_indices; }
    std::vector<std::string> get_signal_values() const { return values;}
    
    void clear();
};