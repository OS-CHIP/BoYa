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

#include "vcd_hierarchy.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <set>
#include <string>
VcdHierarchy::VcdHierarchy() : Hierarchy(), current_time(0) {
    
}
VcdHierarchy::~VcdHierarchy() {
    clear();
}
bool VcdHierarchy::build_from_vcd(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open VCD file: " << filename << std::endl;
        return false;
    }
    
    clear();
    
    file.seekg(0, std::ios::end);
    if (file.tellg() == 0) {
        std::cerr << "VCD file is empty: " << filename << std::endl;
        file.close();
        return false;
    }
    file.seekg(0, std::ios::beg);
    
    bool has_valid_content = false;
    bool has_signals = false;
    bool has_timestamps = false;
    bool has_header_sections = false;
    
    std::set<std::string> required_markers = {"$date", "$version", "$timescale", "$enddefinitions"};
    
    root_scope = add_scope("", "", ScopeType::Module, INVALID_SCOPE_REF);
    scope_stack.push_back(root_scope);
    
    std::string line;
    bool in_header = true;
    int line_count = 0;
    while (std::getline(file, line)) {
        line_count++;
        line = trim(line);
        if (line.empty()) continue;
        has_valid_content = true;
        
        auto it = required_markers.begin();
        while (it != required_markers.end()) {
            if (line.find(*it) != std::string::npos) {
                has_header_sections = true;
                it = required_markers.erase(it);
            } else {
                ++it;
            }
        }
        
        if (in_header && (line[0] == '#' || line.find("$dump") != std::string::npos)) {
            in_header = false;
        }
        if (in_header) {
            parse_header_line(file, line);
        } else {
            parse_value_line(line);
            
            if (line[0] == '#') {
                has_timestamps = true;
            }
        }
    }
    file.close();
    
    if (!has_valid_content) {
        std::cerr << "VCD file contains no valid content" << std::endl;
        clear();
        return false;
    }
    if (!has_header_sections) {
        std::cerr << "VCD file missing required header sections" << std::endl;
        clear();
        return false;
    }
    if (!has_signals) {
        
        if (get_var_count() == 0) {
            std::cerr << "VCD file contains no signal definitions" << std::endl;
            clear();
            return false;
        }
    }
    if (!has_timestamps) {
        std::cerr << "VCD file contains no timestamps" << std::endl;
        clear();
        return false;
    }
    
    if (signal_changes.empty()) {
        std::cerr << "No signal changes parsed from VCD file" << std::endl;
        clear();
        return false;
    }
    return true;
}
void VcdHierarchy::parse_header_line(std::ifstream& file, const std::string& line) {
    if (line[0] != '$') return;
    std::istringstream iss(line);
    std::string command;
    iss >> command;
    if (command == "$scope") {
        std::string scope_type, scope_name;
        iss >> scope_type;
        
        std::string rest;
        std::getline(iss, rest);
        
        size_t end_pos = rest.find("$end");
        if (end_pos != std::string::npos) {
            rest = rest.substr(0, end_pos);
        }
        scope_name = trim(rest);
        process_scope(scope_type, scope_name);
    }
    else if (command == "$upscope") {
        process_upscope();
    }
    else if (command == "$var") {
        std::string var_type, width_str, vcd_id;
        iss >> var_type >> width_str >> vcd_id;
        
        std::string rest;
        std::getline(iss, rest);
        
        size_t end_pos = rest.find("$end");
        if (end_pos != std::string::npos) {
            rest = rest.substr(0, end_pos);
        }
        std::string var_name = trim(rest);
        
        int width = 1;
        if (!width_str.empty() && width_str[0] != '[') {
            try {
                width = std::stoi(width_str);
            } catch (const std::exception& e) {
                
                width = 1;
            }
        } else if (width_str[0] == '[') {
            
            size_t colon_pos = width_str.find(':');
            if (colon_pos != std::string::npos) {
                try {
                    int msb = std::stoi(width_str.substr(1, colon_pos - 1));
                    int lsb = std::stoi(width_str.substr(colon_pos + 1, width_str.length() - colon_pos - 2));
                    width = std::abs(msb - lsb) + 1;
                } catch (const std::exception& e) {
                    width = 1;
                }
            }
        }
        process_var(var_type, width, vcd_id, var_name);
    }
    else if (command == "$timescale") {
        std::string timescale_str = parse_timescale_command(file, line);
        process_timescale(timescale_str);
    }
    else if (command == "$date" || command == "$version" || command == "$comment") {
        
        std::string rest;
        std::getline(iss, rest);
        
        size_t end_pos = rest.find("$end");
        if (end_pos == std::string::npos) {
            skip_multiline_command(file, command);
        }
    }
    else if (command == "$enddefinitions") {
        
        return;
    }
}
std::string VcdHierarchy::parse_timescale_command(std::ifstream& file, const std::string& line) {
    std::string timescale_value;
    if (line.find("$end") != std::string::npos) {

        size_t timescale_pos = line.find("$timescale");
        if (timescale_pos != std::string::npos) {
            
            size_t value_start = timescale_pos + 10; 
            while (value_start < line.length() && std::isspace(line[value_start])) {
                value_start++;
            }
            
            size_t end_pos = line.find("$end", value_start);
            if (end_pos != std::string::npos) {
                timescale_value = line.substr(value_start, end_pos - value_start);
            } else {
                
                timescale_value = line.substr(value_start);
            }
        }
        std::cout << "DEBUG: Single-line format, extracted: '" << timescale_value << "'" << std::endl;
    } else {
        
        std::cout << "DEBUG: Multi-line format detected" << std::endl;
        std::string value_line;
        if (std::getline(file, value_line)) {
            timescale_value = trim(value_line);
            std::cout << "DEBUG: Read value line: '" << timescale_value << "'" << std::endl;
            
            std::string end_line;
            if (std::getline(file, end_line)) {
                end_line = trim(end_line);
                if (end_line != "$end") {
                    std::cerr << "WARNING: Expected $end, got: '" << end_line << "'" << std::endl;
                }
            }
        }
    }
    timescale_value = trim(timescale_value);
    return trim(timescale_value);
}
void VcdHierarchy::parse_value_line(const std::string& line) {
    if (line.empty()) return;
    
    if (line[0] == '#') {
        try {
            uint64_t time = std::stoull(line.substr(1));
            process_time(time);
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse time: " << line << std::endl;
        }
        return;
    }
    
    if (line[0] == '0' || line[0] == '1' || line[0] == 'x' || line[0] == 'z' ||
        line[0] == 'X' || line[0] == 'Z') {
        std::string value(1, line[0]);
        std::string vcd_id;
        
        if (line.size() > 1 && line[1] != ' ') {
            vcd_id = line.substr(1);
        } else if (line.size() > 2) {
            vcd_id = trim(line.substr(2));
        }
        if (!vcd_id.empty()) {
            process_value_change(value, vcd_id);
        }
        return;
    }
    
    if (line[0] == 'b') {
        size_t space_pos = line.find(' ');
        if (space_pos != std::string::npos) {
            std::string value = line.substr(1, space_pos - 1);
            std::string vcd_id = trim(line.substr(space_pos + 1));
            process_value_change(value, vcd_id);
        }
        return;
    }
    
    if (line[0] == 'r') {
        size_t space_pos = line.find(' ');
        if (space_pos != std::string::npos) {
            std::string value = line.substr(1, space_pos - 1);
            std::string vcd_id = trim(line.substr(space_pos + 1));
            process_value_change(value, vcd_id);
        }
        return;
    }
    
    if (line[0] == 's') {
        size_t space_pos = line.find(' ');
        if (space_pos != std::string::npos) {
            std::string value = line.substr(1, space_pos - 1);
            std::string vcd_id = trim(line.substr(space_pos + 1));
            process_value_change(value, vcd_id);
        }
        return;
    }
    
    if (line[0] == 'h' || line[0] == 'u' || line[0] == 'w' || line[0] == 'l' || line[0] == '-') {
        std::string value(1, line[0]);
        std::string vcd_id = (line.length() == 2) ? line.substr(1) : trim(line.substr(2));
        process_value_change(value, vcd_id);
        return;
    }
    
    if (line.find("$dumpon") != std::string::npos) {
        
        return;
    }
    if (line.find("$dumpoff") != std::string::npos) {
        
        return;
    }
    if (line.find("$dumpvars") != std::string::npos) {
        
        return;
    }
}
void VcdHierarchy::skip_multiline_command(std::ifstream& file, const std::string& command) {
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.find("$end") != std::string::npos) {
            break;
        }
    }
}
void VcdHierarchy::process_scope(const std::string& scope_type, const std::string& scope_name) {
    ScopeType type = vcd_scope_type_to_scope_type(scope_type);
    ScopeRef parent = scope_stack.back();
    ScopeRef new_scope = add_scope(scope_name, "", type, parent);
    scope_stack.push_back(new_scope);
    
}
void VcdHierarchy::process_var(const std::string& var_type, int width, const std::string& vcd_id,
                               const std::string& var_name) {
    if (scope_stack.empty()) {
        std::cerr << "Error: No scope for variable: " << var_name << std::endl;
        return;
    }
    ScopeRef current_scope = scope_stack.back();
    VarType type = vcd_var_type_to_var_type(var_type);
    
    SignalType signal_type;
    signal_type.is_real = (type == VarType::Real || type == VarType::ShortReal);
    signal_type.is_string = (type == VarType::String);
    signal_type.width = width;
    SignalRef handle;
    if (vcd_id_to_signal_ref.find(vcd_id) != vcd_id_to_signal_ref.end()) {
        handle = vcd_id_to_signal_ref[vcd_id];
        VarRef var_ref = add_var(var_name, type, signal_type, handle, current_scope);
    } else{
        
        handle = static_cast<SignalRef>(get_var_count() + 1);
        
        VarRef var_ref = add_var(var_name, type, signal_type, handle, current_scope);
        
        vcd_id_to_signal_ref[vcd_id] = handle;
    }

}
void VcdHierarchy::process_upscope() {
    if (scope_stack.size() > 1) {
        scope_stack.pop_back();
        
    } else {
        std::cerr << "Warning: Attempt to upscope from root" << std::endl;
    }
}
void VcdHierarchy::process_timescale(const std::string& timescale_str) {
    HierarchyMetaData meta = get_metadata();
    
    std::string number_str;
    std::string unit_str;
    
    for (char c : timescale_str) {
        if (std::isdigit(c)) {
            number_str += c;
        } else if (std::isalpha(c)) {
            unit_str += c;
        }
    }
    try {
        uint32_t factor = number_str.empty() ? 1 : std::stoul(number_str);
        TimescaleUnit unit = TimescaleUnit::Unknown;
        if (unit_str == "s") unit = TimescaleUnit::Seconds;
        else if (unit_str == "ms") unit = TimescaleUnit::MilliSeconds;
        else if (unit_str == "us") unit = TimescaleUnit::MicroSeconds;
        else if (unit_str == "ns") unit = TimescaleUnit::NanoSeconds;
        else if (unit_str == "ps") unit = TimescaleUnit::PicoSeconds;
        else if (unit_str == "fs") unit = TimescaleUnit::FemtoSeconds;
        meta.timescale = Timescale(factor, unit);
        set_metadata(meta);
        std::cout << "Set timescale: " << timescale_str << " -> " << meta.timescale.to_string() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse timescale: " << timescale_str << std::endl;
    }
}
void VcdHierarchy::process_time(uint64_t time) {
    current_time = time;
}
void VcdHierarchy::process_value_change(const std::string& value, const std::string& vcd_id) {
    auto it = vcd_id_to_signal_ref.find(vcd_id);
    if (it != vcd_id_to_signal_ref.end()) {
        SignalRef handle = it->second;
        signal_changes[handle].emplace_back(current_time, value);
    } else {
        
        static std::unordered_set<std::string> warned_ids;
        if (warned_ids.find(vcd_id) == warned_ids.end()) {
            std::cerr << "Warning: Unknown VCD ID: " << vcd_id << std::endl;
            warned_ids.insert(vcd_id);
        }
    }
}
VarType VcdHierarchy::vcd_var_type_to_var_type(const std::string& vcd_type) const {
    if (vcd_type == "wire") return VarType::Wire;
    if (vcd_type == "reg") return VarType::Reg;
    if (vcd_type == "integer") return VarType::Integer;
    if (vcd_type == "real") return VarType::Real;
    if (vcd_type == "time") return VarType::Time;
    if (vcd_type == "parameter") return VarType::Parameter;
    if (vcd_type == "supply0") return VarType::Supply0;
    if (vcd_type == "supply1") return VarType::Supply1;
    if (vcd_type == "tri") return VarType::Tri;
    if (vcd_type == "triand") return VarType::TriAnd;
    if (vcd_type == "trior") return VarType::TriOr;
    if (vcd_type == "trireg") return VarType::TriReg;
    if (vcd_type == "tri0") return VarType::Tri0;
    if (vcd_type == "tri1") return VarType::Tri1;
    if (vcd_type == "wand") return VarType::WAnd;
    if (vcd_type == "wor") return VarType::WOr;
    if (vcd_type == "port") return VarType::Port;
    if (vcd_type == "event") return VarType::Event;
    if (vcd_type == "string") return VarType::String;
    return VarType::Wire; 
}
ScopeType VcdHierarchy::vcd_scope_type_to_scope_type(const std::string& vcd_type) const {
    if (vcd_type == "module") return ScopeType::Module;
    if (vcd_type == "task") return ScopeType::Task;
    if (vcd_type == "function") return ScopeType::Function;
    if (vcd_type == "begin") return ScopeType::Begin;
    if (vcd_type == "fork") return ScopeType::Fork;
    if (vcd_type == "generate") return ScopeType::Generate;
    if (vcd_type == "struct") return ScopeType::Struct;
    if (vcd_type == "union") return ScopeType::Union;
    if (vcd_type == "class") return ScopeType::Class;
    if (vcd_type == "interface") return ScopeType::Interface;
    if (vcd_type == "package") return ScopeType::Package;
    if (vcd_type == "program") return ScopeType::Program;
    return ScopeType::Module; 
}
std::string VcdHierarchy::trim(const std::string& str) const {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}
void VcdHierarchy::clear() {
    Hierarchy::clear();
    vcd_id_to_signal_ref.clear();
    scope_stack.clear();
    signal_changes.clear();
    current_time = 0;
    
    root_scope = add_scope("", "", ScopeType::Module, INVALID_SCOPE_REF);
    scope_stack.push_back(root_scope);
}
