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
#include "string_pool.h"
#include <vector>
#include <unordered_map>
#include <optional>
#include <QString>
#include <QVector>
struct Var {
    StringId name_id;
    VarType type;
    SignalType signal_type;
    SignalRef  handle; 
    ScopeRef parent_scope;
    VarRef next_var; 
    std::string multi_array; 
};
struct Scope {
    StringId name_id;
    StringId component_name_id; 
    ScopeType type;
    ScopeRef parent_scope;
    VarRef first_var;    
    ScopeRef first_child; 
    ScopeRef next_sibling; 
    VarRef last_var;
    ScopeRef last_child;
};
class Hierarchy {
private:
    StringPool string_pool;
    std::vector<Var> vars;
    std::vector<Scope> scopes;
    HierarchyMetaData meta_data;
    int calculateTotalWidth(const std::string& expression);
public:
    Hierarchy();
    ~Hierarchy();
    
    void reserve_capacity(size_t expected_scopes, size_t expected_vars);
    const std::vector<Var>& get_vars() const { return vars; }
    const std::vector<Scope>& get_scopes() const { return scopes; }
    
    ScopeRef root_scope;
    
    ScopeRef add_scope(const std::string& name, const std::string& component_name,
                       ScopeType type, ScopeRef parent);
    
    VarRef add_var(const std::string& name, VarType type, SignalType signal_type,
                   SignalRef handle, ScopeRef parent);
    
    std::optional<VarRef> find_var_by_path(const std::string& path) const;
    std::optional<ScopeRef> find_scope_by_path(const std::string& path) const;
    ScopeRef find_scope_by_fullpath(const std::string& path) const;
    
    const Var& get_var(VarRef ref) const;
    const Scope& get_scope(ScopeRef ref) const;
    
    const std::string& get_string(StringId id) const;
    
    void traverse_tree() const;
    
    size_t memory_usage() const;
    
    void clear();
    
    ScopeRef get_root_scope() const { return root_scope; }
    
    size_t get_var_count() const { return vars.size(); }
    
    size_t get_scope_count() const { return scopes.size(); }
    
    const HierarchyMetaData& get_metadata() const { return meta_data; }
    void set_metadata(const HierarchyMetaData& metadata) { meta_data = metadata; }
    
    QVector<QString> get_scope_path_vector(ScopeRef ref) const;
    QString get_full_scope_path(ScopeRef ref) const;
    VarRef get_varRef_by_hierarchy(const std::string& path) const;
    VarRef add_var_to_root(const std::string& name, VarType type,
                            SignalType signal_type, SignalRef handle = 0,
                            const std::string& multi_array = "");
    VarRef add_var_by_var_multiarray(const Var& oldVar, int idx,const QString& multiarray,const QString& remainMultiArray,int width);
    bool isFullyExpanded(const std::string& multi_array);
    QVector<VarRef> get_varRefVec_by_hierarchy(const std::string& path) const ;
    VarRef add_var_by_fullPath(const QString& fullPath);
};
