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

#include "hierarchy.h"
#include <iostream>
#include <functional>
#include "myutils.h"
#include <cmath>
#include <iostream>
#include <regex>

Timescale convert_timescale(int8_t exponent) {
    if (exponent >= 0) {
        return Timescale(static_cast<uint32_t>(pow(10, exponent)), TimescaleUnit::Seconds);
    } else if (exponent >= -3) {
        return Timescale(static_cast<uint32_t>(pow(10, exponent + 3)), TimescaleUnit::MilliSeconds);
    } else if (exponent >= -6) {
        return Timescale(static_cast<uint32_t>(pow(10, exponent + 6)), TimescaleUnit::MicroSeconds);
    } else if (exponent >= -9) {
        return Timescale(static_cast<uint32_t>(pow(10, exponent + 9)), TimescaleUnit::NanoSeconds);
    } else if (exponent >= -12) {
        return Timescale(static_cast<uint32_t>(pow(10, exponent + 12)), TimescaleUnit::PicoSeconds);
    } else if (exponent >= -15) {
        return Timescale(static_cast<uint32_t>(pow(10, exponent + 15)), TimescaleUnit::FemtoSeconds);
    } else {
        throw std::runtime_error("Unexpected timescale exponent: " + std::to_string(exponent));
    }
}
std::string timescale_unit_to_string(TimescaleUnit unit) {
    switch (unit) {
    case TimescaleUnit::FemtoSeconds: return "fs";
    case TimescaleUnit::PicoSeconds: return "ps";
    case TimescaleUnit::NanoSeconds: return "ns";
    case TimescaleUnit::MicroSeconds: return "us";
    case TimescaleUnit::MilliSeconds: return "ms";
    case TimescaleUnit::Seconds: return "s";
    default: return "unknown";
    }
}
std::string file_type_to_string(FileType type) {
    switch (type) {
    case FileType::VCD: return "VCD";
    case FileType::FST: return "FST";
    default: return "Unknown";
    }
}
Timescale::Timescale(uint32_t factor, TimescaleUnit unit)
    : factor(factor), unit(unit) {}
std::string Timescale::to_string() const {
    return std::to_string(factor) + " " + timescale_unit_to_string(unit);
}
HierarchyMetaData::HierarchyMetaData()
    : timescale(1, TimescaleUnit::Unknown), file_type(FileType::Unknown) {
}
Hierarchy::Hierarchy() : root_scope(INVALID_SCOPE_REF) {}
Hierarchy::~Hierarchy() {
    clear();
}
ScopeRef Hierarchy::add_scope(const std::string& name, const std::string& component_name,
                              ScopeType type, ScopeRef parent) {
    StringId name_id = string_pool.add_string(name);
    StringId component_id = component_name.empty() ?
                                INVALID_STRING_ID : string_pool.add_string(component_name);
    Scope scope;
    scope.name_id = name_id;
    scope.component_name_id = component_id;
    scope.type = type;
    scope.parent_scope = parent;
    scope.first_var = INVALID_VAR_REF;
    scope.first_child = INVALID_SCOPE_REF;
    scope.next_sibling = INVALID_SCOPE_REF;
    scope.last_var = INVALID_VAR_REF;
    scope.last_child = INVALID_SCOPE_REF;
    scopes.push_back(scope);
    ScopeRef ref = static_cast<ScopeRef>(scopes.size());

    if (parent != INVALID_SCOPE_REF && parent <= scopes.size()) {
        Scope& parent_scope = scopes[parent - 1];
        if (parent_scope.first_child == INVALID_SCOPE_REF) {
            parent_scope.first_child = ref;
            parent_scope.last_child = ref;
        } else {
            scopes[parent_scope.last_child - 1].next_sibling = ref;
            parent_scope.last_child = ref;
        }
    }
    return ref;
}
VarRef Hierarchy::add_var(const std::string& name, VarType type, SignalType signal_type,
                          SignalRef handle, ScopeRef parent) {
    size_t firstPos = name.find_first_of('[');
    std::string multi_array;
    
    if (firstPos < std::string::npos) {
        multi_array = name.substr(firstPos);
    }
    StringId name_id = string_pool.add_string(MyUtils::trim(name.substr(0,firstPos)));
    Var var;
    var.name_id = name_id;
    var.type = type;
    var.signal_type = signal_type;
    var.handle = handle;
    var.parent_scope = parent;
    var.next_var = INVALID_VAR_REF;
    var.multi_array = multi_array;
    vars.push_back(var);
    VarRef ref = static_cast<VarRef>(vars.size());

    if (parent != INVALID_SCOPE_REF && parent <= scopes.size()) {
        Scope& scope = scopes[parent - 1];
        if (scope.first_var == INVALID_VAR_REF) {
            scope.first_var = ref;
            scope.last_var = ref;
        } else {
            vars[scope.last_var - 1].next_var = ref;
            scope.last_var = ref;
        }
    }
    return ref;
}
ScopeRef Hierarchy::find_scope_by_fullpath(const std::string& path) const {
    if (path.empty()) {
        return get_root_scope();
    }
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = path.find('.');
    
    while (end != std::string::npos) {
        parts.push_back(path.substr(start, end - start));
        start = end + 1;
        end = path.find('.', start);
    }
    parts.push_back(path.substr(start));
    ScopeRef current = get_root_scope();
    for (const auto& part : parts) {
        if (current == INVALID_SCOPE_REF || current > scopes.size()) {
            return INVALID_SCOPE_REF;
        }
        
        const Scope& current_scope = scopes[current - 1];
        ScopeRef child = current_scope.first_child;
        ScopeRef found = INVALID_SCOPE_REF;
        
        while (child != INVALID_SCOPE_REF) {
            
            const Scope& child_scope = scopes[child - 1];
            std::string child_name = get_string(child_scope.name_id);
            
            if (part.find('[') == std::string::npos) {
                size_t bracket_pos = child_name.find('[');
                if (bracket_pos != std::string::npos) {
                    child_name = child_name.substr(0, bracket_pos);
                }
            }
            if (child_name == part) {
                found = child;
                break;
            }
            child = child_scope.next_sibling;
        }
        if (found == INVALID_SCOPE_REF) {
            return INVALID_SCOPE_REF;
        }
        current = found;
    }
    return current;
}
const Var& Hierarchy::get_var(VarRef ref) const {
    if (ref == INVALID_VAR_REF || ref > vars.size()) {
        static Var invalid_var;
        return invalid_var;
    }
    return vars[ref - 1];
}
const Scope& Hierarchy::get_scope(ScopeRef ref) const {
    if (ref == INVALID_SCOPE_REF || ref > scopes.size()) {
        static Scope invalid_scope;
        return invalid_scope;
    }
    return scopes[ref - 1];
}
const std::string& Hierarchy::get_string(StringId id) const {
    return string_pool.get_string(id);
}
void Hierarchy::traverse_tree() const {
    std::function<void(ScopeRef, const std::string&)> traverse;
    traverse = [&](ScopeRef scope_ref, const std::string& current_path) {
        if (scope_ref == INVALID_SCOPE_REF || scope_ref > scopes.size()) {
            return;
        }
        const Scope& scope = get_scope(scope_ref);
        const std::string& name = get_string(scope.name_id);
        
        std::string full_path;
        if (current_path.empty()) {
            full_path = name;
        } else {
            full_path = current_path + "." + name;
        }
        
        std::cout << "Scope: " << full_path;
        if (scope.component_name_id != INVALID_STRING_ID) {
            std::cout << " (" << get_string(scope.component_name_id) << ")";
        }
        std::cout << std::endl;
        
        VarRef var_ref = scope.first_var;
        while (var_ref != INVALID_VAR_REF && var_ref <= vars.size()) {
            const Var& var = get_var(var_ref);
            const std::string& var_name = get_string(var.name_id);
            
            std::string var_full_path = full_path + "." + var_name;
            std::cout << "  Var: " << var_full_path << " [handle: " << var.handle
                      << ", type: " << static_cast<int>(var.type)
                      << ", width: " << var.signal_type.width << "]" << std::endl;
            var_ref = var.next_var;
        }
        
        ScopeRef child_ref = scope.first_child;
        while (child_ref != INVALID_SCOPE_REF && child_ref <= scopes.size()) {
            traverse(child_ref, full_path);
            child_ref = get_scope(child_ref).next_sibling;
        }
    };
    traverse(root_scope, "");
}
size_t Hierarchy::memory_usage() const {
    size_t total = 0;
    
    total += sizeof(StringPool);
    for (const auto& str : string_pool.get_strings()) {
        total += str.capacity();
    }
    total += string_pool.size() * (sizeof(std::string) + sizeof(StringId));
    
    total += vars.size() * sizeof(Var);
    total += scopes.size() * sizeof(Scope);
    return total;
}
QVector<QString> Hierarchy::get_scope_path_vector(ScopeRef ref) const
{
    QVector<QString> path;
    
    QVector<ScopeRef> reverse_path;
    while (ref != INVALID_SCOPE_REF && ref <= scopes.size()) {
        reverse_path.append(ref);
        ref = get_scope(ref).parent_scope;
    }
    
    for (auto it = reverse_path.rbegin(); it != reverse_path.rend(); ++it) {
        const Scope& scope = get_scope(*it);
        QString name = QString::fromStdString(get_string(scope.name_id));
        
        if (!name.isEmpty()) {
            path.append(name);
        }
    }
    return path;
}
QString Hierarchy::get_full_scope_path(ScopeRef ref) const{
    QVector<QString> scopes = get_scope_path_vector(ref);
    if (scopes.empty())
        return QString();
    QString result = scopes[0];
    for (size_t i = 1; i < scopes.size(); ++i)
    {
        if (!scopes[i].isEmpty())
        {
            result += "." + scopes[i];
        }
    }
    return result;
}
VarRef Hierarchy::get_varRef_by_hierarchy(const std::string& path) const {
    size_t lastDotPos = path.find_last_of('.');
    
    if (lastDotPos == std::string::npos) {
        return INVALID_VAR_REF;
    }
    std::string prefix, suffix;
    if (lastDotPos != std::string::npos) {
        prefix = path.substr(0, lastDotPos);
        suffix = path.substr(lastDotPos + 1);
    } else {
        prefix = "";
        suffix = path;
    }
    
    size_t bracketPos = suffix.find('[');
    std::string name = suffix;
    std::string bracketPart = "";
    if (bracketPos != std::string::npos) {
        name = suffix.substr(0, bracketPos);
        bracketPart = suffix.substr(bracketPos);
    }
    ScopeRef scopeRef = find_scope_by_fullpath(prefix);
    StringId stringId = string_pool.get_stringId(name);
    for (int i = 1; i <= static_cast<int>(vars.size()); ++i) {
        if (vars[i-1].name_id == stringId && vars[i-1].parent_scope == scopeRef) {
            if(!bracketPart.empty()) {
                if(vars[i-1].multi_array == bracketPart){
                    return i;
                }
            }else {
                return i; 
            }
        }
    }
    return INVALID_VAR_REF; 
}
QVector<VarRef> Hierarchy::get_varRefVec_by_hierarchy(const std::string& path) const {
    QVector<VarRef> varRefVec;
    size_t lastDotPos = path.find_last_of('.');

    if (lastDotPos == std::string::npos) {
        return varRefVec;
    }
    std::string prefix, suffix;
    if (lastDotPos != std::string::npos) {
        prefix = path.substr(0, lastDotPos);
        suffix = path.substr(lastDotPos + 1);
    } else {
        prefix = "";
        suffix = path;
    }

    size_t bracketPos = suffix.find('[');
    std::string name = suffix;
    std::string bracketPart = "";
    if (bracketPos != std::string::npos) {
        name = suffix.substr(0, bracketPos);
        bracketPart = suffix.substr(bracketPos);
    }
    ScopeRef scopeRef = find_scope_by_fullpath(prefix);
    StringId stringId = string_pool.get_stringId(name);
    bool flag = false;
    for (int i = 1; i <= static_cast<int>(vars.size()); ++i) {
        if (vars[i-1].name_id == stringId && vars[i-1].parent_scope == scopeRef) {
            varRefVec.append(i);
            flag = true;
        } else if(flag){
            return varRefVec;
        }
    }
    return varRefVec;
}
void Hierarchy::clear() {
    string_pool.clear();
    vars.clear();
    scopes.clear();
    root_scope = INVALID_SCOPE_REF;
}
void Hierarchy::reserve_capacity(size_t expected_scopes, size_t expected_vars) {
    scopes.reserve(expected_scopes);
    vars.reserve(expected_vars);
}
VarRef Hierarchy::add_var_to_root(const std::string& name, VarType type,
                                  SignalType signal_type, SignalRef handle,
                                  const std::string& multi_array) {
    
    if (root_scope == INVALID_SCOPE_REF) {
        root_scope = add_scope("TOP", "TOP", ScopeType::Module, INVALID_SCOPE_REF);
    }
    
    SignalRef new_handle = handle;
    if (handle == 0) {
        SignalRef max_signal_ref = 0;
        for (const auto& var : vars) {
            if (var.handle > max_signal_ref) {
                max_signal_ref = var.handle;
            }
        }
        new_handle = max_signal_ref + 1;
    }
    
    std::string var_name = name;
    std::string var_multi_array = multi_array;
    size_t bracket_pos = name.find('[');
    if (bracket_pos != std::string::npos) {
        var_name = name.substr(0, bracket_pos);
        var_multi_array = name.substr(bracket_pos);
    }
    var_name = MyUtils::trim(var_name);
    StringId name_id = string_pool.add_string(var_name);
    Var var;
    var.name_id = name_id;
    var.type = type;
    var.signal_type = signal_type;
    var.handle = new_handle;
    var.parent_scope = root_scope;
    var.next_var = INVALID_VAR_REF;
    var.multi_array = var_multi_array;
    vars.push_back(var);
    VarRef ref = static_cast<VarRef>(vars.size());
    
    if (root_scope != INVALID_SCOPE_REF && root_scope <= scopes.size()) {
        Scope& scope = scopes[root_scope - 1];
        if (scope.first_var == INVALID_VAR_REF) {
            scope.first_var = ref;
            scope.last_var = ref;
        } else {
            vars[scope.last_var - 1].next_var = ref;
            scope.last_var = ref;
        }
    }
    return ref;
}
VarRef Hierarchy::add_var_by_var_multiarray(const Var& oldVar, int idx,const QString& multiarray,const QString& remainMultiArray,int width) {
    if (root_scope == INVALID_SCOPE_REF) {
        root_scope = add_scope("TOP", "TOP", ScopeType::Module, INVALID_SCOPE_REF);
    }
    
    SignalRef new_handle;
    SignalRef max_signal_ref = 0;
    for (const auto& var : vars) {
        if (var.handle > max_signal_ref) {
            max_signal_ref = var.handle;
        }
    }
    new_handle = max_signal_ref + 1;
    std::string multi_array = multiarray.toStdString()+"[" + std::to_string(idx) + "]" +remainMultiArray.toStdString();
    Var var;
    var.name_id = oldVar.name_id;
    var.type = oldVar.type;
    SignalType signal_type(false,false,width);
    var.signal_type = signal_type;
    var.handle = new_handle;
    var.parent_scope = oldVar.parent_scope;
    var.next_var = INVALID_VAR_REF;
    var.multi_array = multi_array;
    vars.push_back(var);
    VarRef ref = static_cast<VarRef>(vars.size());
    
    if (root_scope != INVALID_SCOPE_REF && root_scope <= scopes.size()) {
        Scope& scope = scopes[root_scope - 1];
        if (scope.first_var == INVALID_VAR_REF) {
            scope.first_var = ref;
            scope.last_var = ref;
        } else {
            vars[scope.last_var - 1].next_var = ref;
            scope.last_var = ref;
        }
    }
    return ref;
}
VarRef Hierarchy::add_var_by_fullPath(const QString& fullPath) {
    QString noMultiArrayPath = fullPath;
    std::string multi_array;
    int last_dot = fullPath.lastIndexOf('.');
    if (last_dot != -1) {
        int bracket_pos = fullPath.indexOf('[', last_dot);
        if (bracket_pos != -1) {
            noMultiArrayPath =  fullPath.left(bracket_pos);
            multi_array = fullPath.mid(bracket_pos).toStdString();
        }
    }

    VarRef var_ref = get_varRef_by_hierarchy(noMultiArrayPath.toStdString());
    if(var_ref > 0) {
        const Var& oldVar = get_var(var_ref);
        bool isExpanded = isFullyExpanded(oldVar.multi_array);
        if (!isExpanded){
            if (root_scope == INVALID_SCOPE_REF) {
                root_scope = add_scope("TOP", "TOP", ScopeType::Module, INVALID_SCOPE_REF);
            }

            SignalRef new_handle;
            SignalRef max_signal_ref = 0;
            for (const auto& var : vars) {
                if (var.handle > max_signal_ref) {
                    max_signal_ref = var.handle;
                }
            }
            new_handle = max_signal_ref + 1;
            Var var;
            var.name_id = oldVar.name_id;
            var.type = oldVar.type;
            int width = calculateTotalWidth(multi_array);
            SignalType signal_type(false,false,width);
            var.signal_type = signal_type;
            var.handle = new_handle;
            var.parent_scope = oldVar.parent_scope;
            var.next_var = INVALID_VAR_REF;
            var.multi_array = multi_array;
            vars.push_back(var);
            VarRef ref = static_cast<VarRef>(vars.size());

            if (root_scope != INVALID_SCOPE_REF && root_scope <= scopes.size()) {
                Scope& scope = scopes[root_scope - 1];
                if (scope.first_var == INVALID_VAR_REF) {
                    scope.first_var = ref;
                    scope.last_var = ref;
                } else {
                    vars[scope.last_var - 1].next_var = ref;
                    scope.last_var = ref;
                }
            }
            return ref;
        }
    }
    return INVALID_VAR_REF;
}

bool Hierarchy::isFullyExpanded(const std::string& multi_array) {
    if (multi_array.empty()) {
        return true;
    }
    size_t pos = 0;
    while ((pos = multi_array.find('[', pos)) != std::string::npos) {
        size_t end = multi_array.find(']', pos);
        if (end == std::string::npos) {
            return true;
        }
        std::string content = multi_array.substr(pos + 1, end - pos - 1);
        if (content.find(':') == std::string::npos) {
            return false;
        }

        pos = end + 1;
    }
    return true;
}
int Hierarchy::calculateTotalWidth(const std::string& expression) {
    std::regex range_regex(R"(\[(\d+):(\d+)\])");
    std::regex single_regex(R"(\[(\d+)\])");

    std::sregex_iterator begin(expression.begin(), expression.end(), range_regex);
    std::sregex_iterator end;
    std::sregex_iterator single_begin(expression.begin(), expression.end(), single_regex);

    std::vector<int> widths;
    for (std::sregex_iterator it = begin; it != end; ++it) {
        std::smatch match = *it;
        int high = std::stoi(match[1].str());
        int low = std::stoi(match[2].str());
        int width = std::abs(high - low) + 1;
        widths.push_back(width);
    }

    for (std::sregex_iterator it = single_begin; it != end; ++it) {
        std::smatch match = *it;
        bool is_range = false;
        for (std::sregex_iterator range_it = begin; range_it != end; ++range_it) {
            if (match.position() == (*range_it).position()) {
                is_range = true;
                break;
            }
        }
        if (!is_range) {
            widths.push_back(1);
        }
    }
    int total_width = 1;
    for (int width : widths) {
        total_width *= width;
    }

    return total_width;
}
