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

#include <QDebug>
#include <regex>
#include "waveform.h"
#include "waveform_types.h"
Waveform::Waveform() {}
Waveform::~Waveform() {
    clear();
}
void Waveform::set_time_table(const std::vector<Time>& table) {
    time_table = table;
}
Time Waveform::get_time_at_idx(TimeTableIdx idx) const {
    if (idx < time_table.size()) {
        return time_table[idx];
    }
    return Time{0};
}
void Waveform::add_signal(SignalRef ref, QSharedPointer<Signal> signal) {
    std::lock_guard<std::mutex> lock(signal_map_mutex);
    signal_map[ref] = signal;
}
Signal* Waveform::get_signal(SignalRef ref) {
    std::lock_guard<std::mutex> lock(signal_map_mutex);
    auto it = signal_map.find(ref);
    if (it != signal_map.end()) {
        return it->second.get();
    }
    return nullptr;
}
const Signal* Waveform::get_signal(SignalRef ref) const {
    std::lock_guard<std::mutex> lock(signal_map_mutex);
    auto it = signal_map.find(ref);
    if (it != signal_map.end()) {
        return it->second.get();
    }
    return nullptr;
}
QSharedPointer<Signal> Waveform::getSignalShared(SignalRef ref) const
{
    std::lock_guard<std::mutex> lock(signal_map_mutex);
    auto it = signal_map.find(ref);
    if (it != signal_map.end()) {
        return it->second;
    }
    return QSharedPointer<Signal>();
}
bool Waveform::has_signal(SignalRef ref) const {
    std::lock_guard<std::mutex> lock(signal_map_mutex);
    return signal_map.find(ref) != signal_map.end();
}
void Waveform::remove_signal(SignalRef ref) {
    std::lock_guard<std::mutex> lock(signal_map_mutex);
    signal_map.erase(ref);
}
void Waveform::clear() {
    std::lock_guard<std::mutex> lock(signal_map_mutex);
    hierarchy.clear();
    signal_map.clear();
    time_table.clear();
}
size_t Waveform::binary_search_timeindex(const std::vector<TimeTableIdx>& indices, TimeTableIdx needle) {
    if (indices.empty()) {
        return static_cast<size_t>(-1); 
    }
    size_t lower_idx = 0;
    size_t upper_idx = indices.size() - 1;
    while (lower_idx <= upper_idx) {
        size_t mid_idx = lower_idx + (upper_idx - lower_idx) / 2;
        if (indices[mid_idx] < needle) {
            lower_idx = mid_idx + 1;
        } else if (indices[mid_idx] > needle) {
            if (mid_idx == 0) break;
            upper_idx = mid_idx - 1;
        } else {
            return mid_idx;
        }
    }
    
    if (lower_idx == 0) {
        return static_cast<size_t>(-1); 
    }
    return lower_idx - 1;
}
size_t Waveform::binary_search(const std::vector<Time>& times, Time needle) {
    
    if (times.empty()) {
        return static_cast<size_t>(-1); 
    }
    size_t lower_idx = 0;
    size_t upper_idx = times.size() - 1;
    while (lower_idx <= upper_idx) {
        
        size_t mid_idx = lower_idx + (upper_idx - lower_idx) / 2;
        if (times[mid_idx] < needle) {
            lower_idx = mid_idx + 1;
        } else if (times[mid_idx] > needle) {
            
            if (mid_idx == 0) break;
            upper_idx = mid_idx - 1;
        } else { 
            return mid_idx;
        }
    }

    if (lower_idx == 0) {
        return static_cast<size_t>(-1); 
    }
    return lower_idx - 1;
}
std::optional<TimeTableIdx> Waveform::time_to_time_table_idx( const Time time ) {
    
    if (time_table.empty() || time_table[0] > time) {
        return std::nullopt;
    }
    
    size_t idx = binary_search(time_table, time);
    
    if (idx == static_cast<size_t>(-1)) {
        return std::nullopt;
    }
    
    assert(time_table[idx] <= time && "Found time should be <= target time");
    return static_cast<TimeTableIdx>(idx);
}
size_t Waveform::get_index_by_signal_and_time(QSharedPointer<Signal> signal, Time time) {
    auto time_index =  time_to_time_table_idx(time);
    auto time_index_table = signal->get_time_indices();
    if (time_index.has_value()) {
        auto idx = binary_search_timeindex(time_index_table, time_index.value());
        return idx;
    } else {
        return -1; 
    }
}
Time Waveform::get_next_time(QSharedPointer<Signal> signal, Time time,bool next) {
    auto idx =  get_index_by_signal_and_time(signal,time);
    auto time_index_table = signal->get_time_indices();
    std::vector<Time> time_table = get_time_table();
    if(next) {
        if (idx < time_index_table.size() -1) {
            return time_table[time_index_table[idx+1]];
        } else {
            return time_table[time_table.size() -1];;
        }
    } else {
        if(time_table[time_index_table[idx]] != time) {
            return time_table[time_index_table[idx]];
        } else{
            if (idx > 0) {
                return time_table[time_index_table[idx-1]];
            }
            return time_table[0];
        }
    }
}
size_t Waveform::memory_usage() const {
    size_t total = hierarchy.memory_usage();
    
    total += time_table.size() * sizeof(Time);
    
    for (const auto& pair : signal_map) {
        total += sizeof(SignalRef) + sizeof(Signal);
        total += pair.second->get_change_count() * (sizeof(TimeTableIdx) + sizeof(std::string));
    }
    return total;
}
std::pair<std::vector<double>, std::vector<double>> Waveform::to_plot_data(QSharedPointer<Signal> signal) const {
    std::vector<double> x, y;
    std::vector<Time> time_table = get_time_table();
    Time end_time = time_table.back();
    
    std::vector<TimeTableIdx> time_indices = signal->get_time_indices();
    std::vector<std::string> values = signal->get_signal_values();
    if (time_indices.empty()) {
        return {x, y};
    }
    
    for (size_t i = 0; i < time_indices.size(); i++) {
        double time = static_cast<double>(time_table[time_indices[i]]);
        double value = 0.0;
        
        if (values[i] == "1" || values[i] == "H" || values[i] == "h") {
            value = 1.0;
        } else if (values[i] == "X" || values[i] == "x") {
            value = 0.5; 
        } else if (values[i] == "Z" || values[i] == "z") {
            value = 0.625; 
        } else {
            try {
                value = std::stod(values[i]);
            } catch (...) {
                value = 0.5; 
            }
        }
        
        x.push_back(time);
        y.push_back(value);
    }

    if (!time_indices.empty() && time_table[time_indices.back()] != end_time) {
        x.push_back(static_cast<double>(end_time)); 
        double last_value = 0.0;
        if (values.back() == "1" || values.back() == "H" || values.back() == "h") {
            last_value = 1.0;
        } else if (values.back() == "X" || values.back() == "x") {
            last_value = 0.5;
        } else if (values.back() == "Z" || values.back() == "z") {
            last_value = 0.625;
        }else {
            
            try {
                last_value = std::stod(values.back());
            } catch (...) {
                last_value = 0.5; 
            }
        }
        y.push_back(last_value);
    }
    return {x, y};
}
VariableMeta Waveform::var_to_meta(const VarRef& r) {
    VariableMeta meta;
    Var var= hierarchy.get_var(r);
    meta.var_ref = r;
    meta.numBits = var.signal_type.width;
    
    meta.type = var.type;

    return meta;
}
bool Waveform::is_high_value(const std::string& value) const {
    return value == "1" || value == "H" || value == "h" || value == "HIGH";
}
bool Waveform::is_low_value(const std::string& value) const {
    return value == "0" || value == "L" || value == "l" || value == "LOW";
}
bool Waveform::is_rising_edge(const std::string& prev_value, const std::string& curr_value) const {
    return is_low_value(prev_value) && is_high_value(curr_value);
}
bool Waveform::is_falling_edge(const std::string& prev_value, const std::string& curr_value) const {
    return is_high_value(prev_value) && is_low_value(curr_value);
}
std::optional<Time> Waveform::get_previous_rising_edge(QSharedPointer<Signal> signal, Time current_time) {
    if (!signal) {
        return std::nullopt;
    }
    
    std::vector<TimeTableIdx> time_indices = signal->get_time_indices();
    std::vector<std::string> values = signal->get_signal_values();
    std::vector<Time> time_table = get_time_table();
    if (time_indices.empty() || values.empty() || time_table.empty()) {
        return std::nullopt;
    }
    
    size_t current_idx = get_index_by_signal_and_time(signal, current_time);
    if (current_idx == static_cast<size_t>(-1) || current_idx >= time_indices.size()) {
        return std::nullopt;
    }
    
    for (size_t i = current_idx - 1; i > 0; i--) {
        
        if (is_rising_edge(values[i-1], values[i])) {
            return time_table[time_indices[i]]; 
        }
    }
    
    return std::nullopt;
}
std::optional<Time> Waveform::get_previous_falling_edge(QSharedPointer<Signal> signal, Time current_time) {
    if (!signal) {
        return std::nullopt;
    }
    std::vector<TimeTableIdx> time_indices = signal->get_time_indices();
    std::vector<std::string> values = signal->get_signal_values();
    std::vector<Time> time_table = get_time_table();
    if (time_indices.empty() || values.empty() || time_table.empty()) {
        return std::nullopt;
    }
    size_t current_idx = get_index_by_signal_and_time(signal, current_time);
    if (current_idx == static_cast<size_t>(-1) || current_idx >= time_indices.size()) {
        return std::nullopt;
    }
    for (size_t i = current_idx - 1; i > 0; i--) {
        if (is_falling_edge(values[i-1], values[i])) {
            return time_table[time_indices[i]];
        }
    }
    return std::nullopt;
}
std::optional<Time> Waveform::get_next_rising_edge(QSharedPointer<Signal> signal, Time current_time) {
    if (!signal) {
        return std::nullopt;
    }
    std::vector<TimeTableIdx> time_indices = signal->get_time_indices();
    std::vector<std::string> values = signal->get_signal_values();
    std::vector<Time> time_table = get_time_table();
    if (time_indices.empty() || values.empty() || time_table.empty()) {
        return std::nullopt;
    }
    size_t current_idx = get_index_by_signal_and_time(signal, current_time);
    if (current_idx == static_cast<size_t>(-1) || current_idx >= time_indices.size()) {
        return std::nullopt;
    }
    
    for (size_t i = current_idx + 1; i < time_indices.size(); i++) {
        if (is_rising_edge(values[i-1], values[i])) {
            return time_table[time_indices[i]];
        }
    }
    return std::nullopt;
}
std::optional<Time> Waveform::get_next_falling_edge(QSharedPointer<Signal> signal, Time current_time) {
    if (!signal) {
        return std::nullopt;
    }
    std::vector<TimeTableIdx> time_indices = signal->get_time_indices();
    std::vector<std::string> values = signal->get_signal_values();
    std::vector<Time> time_table = get_time_table();
    if (time_indices.empty() || values.empty() || time_table.empty()) {
        return std::nullopt;
    }
    size_t current_idx = get_index_by_signal_and_time(signal, current_time);
    if (current_idx == static_cast<size_t>(-1) || current_idx >= time_indices.size()) {
        return std::nullopt;
    }
    
    for (size_t i = current_idx + 1; i < time_indices.size(); i++) {
        if (is_falling_edge(values[i-1], values[i])) {
            return time_table[time_indices[i]];
        }
    }
    return std::nullopt;
}
std::vector<DimInfo> Waveform::parse_dim_string(const std::string& dim_str) {
    std::vector<DimInfo> dims;
    
    std::regex dim_regex(R"(\[(\d+)(?::(\d+))?\])");
    std::sregex_iterator it(dim_str.begin(), dim_str.end(), dim_regex);
    std::sregex_iterator end;
    while (it != end) {
        std::smatch match = *it;
        int left = std::stoi(match[1]);
        int right = left;  
        bool is_range = false;
        if (match[2].matched) {
            right = std::stoi(match[2]);
            is_range = true;
        }
        dims.push_back(DimInfo(left, right, is_range));
        ++it;
    }
    return dims;
}
QSharedPointer<Signal> Waveform::extract_bit(QSharedPointer<Waveform>& waveform,
                                             VarRef array_var_ref, int flat_index,
                                             DimInfo dimInfo, int width) {
    const Var& var = waveform->get_hierarchy().get_var(array_var_ref);
    auto array_signal = waveform->get_signal(var.handle);
    if (!array_signal) {
        return nullptr;
    }
    auto bit_signal = QSharedPointer<Signal>::create();
    std::string prev_value;
    Endianness endian = dimInfo.endian;  
    int total_width = dimInfo.width();             
    int slice_start = dimInfo.logical_to_physical(flat_index) * width;                
    if (endian == ENDIAN_BIG) {
        slice_start = (total_width-1-dimInfo.logical_to_physical(flat_index)) * width;
    }
    std::vector<TimeTableIdx> time_indices = array_signal->get_time_indices();
    std::vector<std::string> values = array_signal->get_signal_values();
    for (int i = 0; i < time_indices.size(); i++) {
        std::string value = values[i];
        std::string extracted_bits(width, '0');  
        if (!value.empty() && static_cast<int>(value.length()) >= total_width) {
            for (int j = 0; j < width; j++) {
                int src_pos = slice_start + j;
                if (src_pos < static_cast<int>(value.length())) {
                    extracted_bits[j] = value[src_pos];
                }
            }
            
            int decimal_value = 0;
            for (int j = 0; j < extracted_bits.length(); j++) {
                if (extracted_bits[j] == '1') {
                    decimal_value += (1 << (extracted_bits.length() - 1 - j));
                }
            }
        }
        if (i == 0 || extracted_bits != prev_value) {
            bit_signal->add_value_change(time_indices[i], extracted_bits);
            prev_value = extracted_bits;
        }
    }
    return bit_signal;
}