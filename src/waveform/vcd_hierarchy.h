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
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
class VcdHierarchy : public Hierarchy {
private:
    
    std::unordered_map<std::string, SignalRef> vcd_id_to_signal_ref;
    
    std::vector<ScopeRef> scope_stack;
    uint64_t current_time;
    
    std::unordered_map<SignalRef, std::vector<std::pair<uint64_t, std::string>>> signal_changes;
public:
    VcdHierarchy();
    ~VcdHierarchy();
    
    bool build_from_vcd(const std::string& filename);
    
    const std::unordered_map<SignalRef, std::vector<std::pair<uint64_t, std::string>>>&
    get_signal_changes() const { return signal_changes; }
    
    void clear();
private:
    
    void parse_header_line(std::ifstream& file, const std::string& line);
    void parse_value_line(const std::string& line);
    void skip_multiline_command(std::ifstream& file, const std::string& command);
    
    void process_scope(const std::string& scope_type, const std::string& scope_name);
    void process_var(const std::string& var_type, int width, const std::string& vcd_id,
                     const std::string& var_name);
    void process_upscope();
    void process_timescale(const std::string& timescale_str);
    std::string parse_timescale_command(std::ifstream& file, const std::string& line);
    void process_time(uint64_t time);
    void process_value_change(const std::string& value, const std::string& vcd_id);
    
    VarType vcd_var_type_to_var_type(const std::string& vcd_type) const;
    ScopeType vcd_scope_type_to_scope_type(const std::string& vcd_type) const;
    
    std::string trim(const std::string& str) const;
};