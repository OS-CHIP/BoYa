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

#include "vcd_reader.h"
#include <iostream>
#include <algorithm>
#include <set>
VcdReader::VcdReader() : is_open(false) {
    vcd_hierarchy = QSharedPointer<VcdHierarchy>::create();
}
VcdReader::~VcdReader() {
    close();
}
bool VcdReader::open(const std::string& filename) {
    close();
    this->filename = filename;
    is_open = true;
    return true;
}
void VcdReader::close() {
    if (vcd_hierarchy) {
        vcd_hierarchy->clear();
    }
    filename.clear();
    is_open = false;
}
QSharedPointer<Waveform> VcdReader::read() {
    if (!is_open || filename.empty()) {
        return nullptr;
    }
    
    if (!vcd_hierarchy->build_from_vcd(filename)) {
        std::cerr << "Failed to parse VCD file: " << filename << std::endl;
        return nullptr;
    }
    QSharedPointer<Waveform> waveform = QSharedPointer<Waveform>::create();
    
    if (!build_hierarchy(*waveform)) {
        return nullptr;
    }
    
    if (!build_time_table(*waveform)) {
        return nullptr;
    }
    
    build_signal_data(*waveform);
    return waveform;
}
bool VcdReader::build_hierarchy(Waveform& waveform) {
    
    waveform.get_hierarchy() = *vcd_hierarchy;
    return true;
}
bool VcdReader::build_time_table(Waveform& waveform) {
    
    const auto& signal_changes = vcd_hierarchy->get_signal_changes();
    std::set<uint64_t> time_set;
    
    for (const auto& [signal_ref, changes] : signal_changes) {
        for (const auto& change : changes) {
            time_set.insert(change.first);
        }
    }
    
    std::vector<Time> time_table(time_set.begin(), time_set.end());
    waveform.set_time_table(time_table);
    return true;
}
void VcdReader::build_signal_data(Waveform& waveform) {
    const auto& signal_changes = vcd_hierarchy->get_signal_changes();
    const std::vector<Time>& time_table = waveform.get_time_table();
    
    for (const auto& [signal_ref, changes] : signal_changes) {
        QSharedPointer<Signal> signal = QSharedPointer<Signal>::create(signal_ref);
        
        for (const auto& change : changes) {
            uint64_t time = change.first;
            const std::string& value = change.second;
            
            auto it = std::lower_bound(time_table.begin(), time_table.end(), time);
            if (it != time_table.end() && *it == time) {
                TimeTableIdx time_idx = std::distance(time_table.begin(), it);
                signal->add_value_change(time_idx, value);
            }
        }
        waveform.add_signal(signal_ref, signal);
    }
}
bool VcdReader::load_signal_data(QSharedPointer<Waveform>& waveform, SignalRef signal_ref) {

    return waveform->has_signal(signal_ref);
}
void VcdReader::load_signals_data(QSharedPointer<Waveform>& waveform, const std::vector<SignalRef>& signal_refs) {

    for (SignalRef signal_ref : signal_refs) {
        if (!waveform->has_signal(signal_ref)) {
            std::cerr << "Warning: Signal " << signal_ref << " not found in waveform" << std::endl;
        }
    }
}