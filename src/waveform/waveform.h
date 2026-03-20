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
#include "hierarchy.h"
#include "signal.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <QSharedPointer>
struct VariableMeta {
    VarRef var_ref;
    int numBits = -1; 
    VarType type;
    QString index;
};
class Waveform {
private:
    Hierarchy hierarchy;
    std::vector<Time> time_table; 
    std::unordered_map<SignalRef, QSharedPointer<Signal>> signal_map; 
    mutable std::mutex signal_map_mutex; 
    size_t binary_search(const std::vector<Time>& times, Time needle);
    
    bool is_rising_edge(const std::string& prev_value, const std::string& curr_value) const;
    
    bool is_falling_edge(const std::string& prev_value, const std::string& curr_value) const;
    
    bool is_high_value(const std::string& value) const;
    
    bool is_low_value(const std::string& value) const;
public:
    Waveform();
    ~Waveform();
    
    Hierarchy& get_hierarchy() { return hierarchy; }
    const Hierarchy& get_hierarchy() const { return hierarchy; }
    
    void set_time_table(const std::vector<Time>& table);
    const std::vector<Time>& get_time_table() const { return time_table; }
    Time get_time_at_idx(TimeTableIdx idx) const;
    
    void add_signal(SignalRef ref, QSharedPointer<Signal> signal);
    Signal* get_signal(SignalRef ref);
    const Signal* get_signal(SignalRef ref) const;
    bool has_signal(SignalRef ref) const;
    void remove_signal(SignalRef ref);
    QSharedPointer<Signal> getSignalShared(SignalRef ref) const;
    std::optional<TimeTableIdx> time_to_time_table_idx( const Time time );
    size_t get_index_by_signal_and_time(QSharedPointer<Signal> singal, Time time);
    Time get_next_time(QSharedPointer<Signal> signal, Time time,bool next);
    VariableMeta var_to_meta(const VarRef& r);
    Time getBeginTime() const {
        const auto& table = get_time_table();
        if (table.empty()) {
            throw std::out_of_range("Time table is empty");
        }
        return table.front();
    }
    Time getEndTime() const {
        const auto& table = get_time_table();
        if (table.empty()) {
            throw std::out_of_range("Time table is empty");
        }
        return table.back();
    }
    
    void clear();
    
    size_t memory_usage() const;
    
    std::pair<std::vector<double>, std::vector<double>> to_plot_data(QSharedPointer<Signal> signal) const;
    
    std::optional<Time> get_previous_rising_edge(QSharedPointer<Signal> signal, Time current_time);
    
    std::optional<Time> get_previous_falling_edge(QSharedPointer<Signal> signal, Time current_time);
    
    std::optional<Time> get_next_rising_edge(QSharedPointer<Signal> signal, Time current_time);
    
    std::optional<Time> get_next_falling_edge(QSharedPointer<Signal> signal, Time current_time);
    size_t binary_search_timeindex(const std::vector<TimeTableIdx>& times, TimeTableIdx needle);
    std::vector<DimInfo> parse_dim_string(const std::string& dim_str);
    QSharedPointer<Signal> extract_bit(QSharedPointer<Waveform>& waveform, VarRef array_var_ref, int flat_index,
                                       DimInfo dimInfo,int width);
};