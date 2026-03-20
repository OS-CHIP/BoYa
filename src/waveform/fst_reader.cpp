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

#include "fst_reader.h"
#include "fst_hierarchy.h"
#include <iostream>
#include <vector>
#include <thread>
#include <algorithm>
FstReader::FstReader() : fst_reader(nullptr), is_open(false) {}
FstReader::~FstReader() {
    close();
}
bool FstReader::open(const std::string& filename) {
    close();
    fst_reader = fstReaderOpen(filename.c_str());
    if (!fst_reader) {
        std::cerr << "Failed to open FST file: " << filename << std::endl;
        return false;
    }
    is_open = true;
    return true;
}
void FstReader::close() {
    if (fst_reader) {
        fstReaderClose(fst_reader);
        fst_reader = nullptr;
    }
    is_open = false;
}
QSharedPointer<Waveform> FstReader::read() {
    if (!is_open || !fst_reader) {
        return nullptr;
    }
    
    QSharedPointer<Waveform> waveform = QSharedPointer<Waveform>::create();
    
    if (!build_hierarchy(*waveform)) {
        return nullptr;
    }
    
    if (!build_time_table(*waveform)) {
        return nullptr;
    }
    return waveform;
}
bool FstReader::build_hierarchy(Waveform& waveform) {
    FstHierarchy fst_hierarchy;
    if (!fst_hierarchy.build_from_fst(fst_reader)) {
        std::cerr << "Failed to build hierarchy from FST file" << std::endl;
        return false;
    }
    
    waveform.get_hierarchy() = std::move(fst_hierarchy);
    return true;
}
bool FstReader::build_time_table(Waveform& waveform) {
    FstHierarchy fst_hierarchy;
    std::vector<Time> time_table = fst_hierarchy.get_time_table(fst_reader);
    waveform.set_time_table(time_table);
    return true;
}
bool FstReader::load_signal_data(QSharedPointer<Waveform>& waveform, SignalRef signal_ref) {
    if (!is_open || !fst_reader) {
        return false;
    }

    if (waveform->has_signal(signal_ref)) {
        return true;
    }
    
    QSharedPointer<Signal> signal = QSharedPointer<Signal>::create();
    
    const std::vector<Time>& time_table = waveform->get_time_table();
    std::string prev_value;
    
    constexpr size_t DEFAULT_BUF_SIZE = 1024;
    std::vector<char> buffer(DEFAULT_BUF_SIZE);
    for (size_t time_idx = 0; time_idx < time_table.size(); time_idx++) {
        Time time = time_table[time_idx];
        char* value = fstReaderGetValueFromHandleAtTime(
            fst_reader, time, signal_ref, buffer.data());
        std::string current_value = value ? std::string(value) : "";
        
        if (time_idx == 0 || current_value != prev_value) {
            signal->add_value_change(time_idx, current_value);
            prev_value = current_value;
        }
    }
    
    waveform->add_signal(signal_ref, signal);
    return true;
}
void FstReader::load_signals_data(QSharedPointer<Waveform>& waveform, const std::vector<SignalRef>& signal_refs) {
    size_t num_signals = signal_refs.size();
    if (num_signals == 0) {
        return;
    }

    for (size_t i = 0; i < signal_refs.size(); i++) {
        this->load_signal_data(waveform, signal_refs[i]);
    }
}