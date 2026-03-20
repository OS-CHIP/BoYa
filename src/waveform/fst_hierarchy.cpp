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

#include "fst_hierarchy.h"
#include <iostream>
#include <cmath>
#include <stdexcept>
#include "fstext.h"
FstHierarchy::FstHierarchy() : Hierarchy() {}
FstHierarchy::~FstHierarchy() {
    clear();
}
bool FstHierarchy::build_from_fst(void* fst_reader) {
    if (!fst_reader) {
        return false;
    }
    
    clear();
    set_metadata(get_metadata(fst_reader));
    uint32_t estimated_scopes = fstReaderGetScopeCount(fst_reader)+10;
    uint32_t estimated_vars = fstReaderGetVarCount(fst_reader)+100;
    reserve_capacity(estimated_scopes, estimated_vars);
    
    root_scope = add_scope("", "", ScopeType::Module, INVALID_SCOPE_REF);
    
    std::vector<ScopeRef> scope_stack;
    scope_stack.push_back(root_scope);
    
    fstReaderIterateHierRewind(fst_reader);
    
    while (true) {
        fstHier* fst_hier = fstReaderIterateHier(fst_reader);
        if (!fst_hier) {
            break;
        }
        switch (fst_hier->htyp) {
        case FST_HT_SCOPE: {
            std::string scope_name = fst_hier->u.scope.name ? fst_hier->u.scope.name : "";
            std::string component_name = fst_hier->u.scope.component ? fst_hier->u.scope.component : "";
            ScopeType scope_type;
            switch (fst_hier->u.scope.typ) {
            case FST_ST_VCD_FUNCTION:  scope_type = ScopeType::Function; break;
            case FST_ST_VHDL_FUNCTION:   scope_type = ScopeType::Function; break;
            case FST_ST_VCD_TASK :   scope_type = ScopeType::Task; break;
            case FST_ST_VCD_STRUCT:     scope_type = ScopeType::Struct; break;
            case FST_ST_VCD_GENERATE: scope_type = ScopeType::Generate; break;
            case FST_ST_VHDL_GENERATE:   scope_type = ScopeType::Generate; break;
            case FST_ST_VHDL_PACKAGE:    scope_type = ScopeType::Package; break;
            case FST_ST_VCD_PACKAGE :    scope_type = ScopeType::Package; break;
            default:                     scope_type = ScopeType::Module; break;
            }
            ScopeRef parent_scope = scope_stack.back();
            ScopeRef new_scope = add_scope(scope_name, component_name, scope_type, parent_scope);
            scope_stack.push_back(new_scope);
            break;
        }
        case FST_HT_UPSCOPE: {
            
            if (scope_stack.size() > 1) {
                scope_stack.pop_back();
            }
            break;
        }
        case FST_HT_VAR: {
            
            if (scope_stack.empty()) {
                break;
            }
            ScopeRef current_scope = scope_stack.back();
            std::string var_name = fst_hier->u.var.name ? fst_hier->u.var.name : "";
            
            VarType var_type;
            switch (fst_hier->u.var.typ) {
            case FST_VT_VCD_EVENT:         var_type = VarType::Event; break;
            case FST_VT_VCD_INTEGER:       var_type = VarType::Integer; break;
            case FST_VT_VCD_PARAMETER:     var_type = VarType::Parameter; break;
            case FST_VT_VCD_REAL:          var_type = VarType::Real; break;
            case FST_VT_VCD_REG:           var_type = VarType::Reg; break;
            case FST_VT_VCD_SUPPLY0:       var_type = VarType::Supply0; break;
            case FST_VT_VCD_SUPPLY1:       var_type = VarType::Supply1; break;
            case FST_VT_VCD_TIME:          var_type = VarType::Time; break;
            case FST_VT_VCD_TRI:           var_type = VarType::Tri; break;
            case FST_VT_VCD_TRIAND:        var_type = VarType::TriAnd; break;
            case FST_VT_VCD_TRIOR:         var_type = VarType::TriOr; break;
            case FST_VT_VCD_TRIREG:        var_type = VarType::TriReg; break;
            case FST_VT_VCD_TRI0:          var_type = VarType::Tri0; break;
            case FST_VT_VCD_TRI1:          var_type = VarType::Tri1; break;
            case FST_VT_VCD_WAND:          var_type = VarType::WAnd; break;
            case FST_VT_VCD_WIRE:          var_type = VarType::Wire; break;
            case FST_VT_VCD_WOR:           var_type = VarType::WOr; break;
            case FST_VT_VCD_PORT:          var_type = VarType::Port; break;
            case FST_VT_VCD_REALTIME:      var_type = VarType::RealTime; break;
            case FST_VT_SV_BIT:            var_type = VarType::Bit; break;
            case FST_VT_SV_LOGIC:          var_type = VarType::Logic; break;
            case FST_VT_SV_INT:            var_type = VarType::Int; break;
            case FST_VT_SV_SHORTINT:       var_type = VarType::ShortInt; break;
            case FST_VT_SV_LONGINT:        var_type = VarType::LongInt; break;
            case FST_VT_SV_BYTE:           var_type = VarType::Byte; break;
            case FST_VT_SV_ENUM:           var_type = VarType::Enum; break;
            case FST_VT_SV_SHORTREAL:      var_type = VarType::ShortReal; break;
            case FST_VT_GEN_STRING:        var_type = VarType::String; break;
            default:                       var_type = VarType::Wire; break;
            }
            
            SignalType signal_type;
            signal_type.is_real = (var_type == VarType::Real || var_type == VarType::ShortReal);
            signal_type.is_string = (var_type == VarType::String);
            signal_type.width = fst_hier->u.var.length;
            
            add_var(var_name, var_type, signal_type, fst_hier->u.var.handle, current_scope);
            break;
        }
        case FST_HT_ATTRBEGIN:
        case FST_HT_ATTREND:
        case FST_HT_TREEBEGIN:
        case FST_HT_TREEEND:
            
            break;
        default:
            std::cerr << "Unknown hierarchy type: " << fst_hier->htyp << std::endl;
            break;
        }
    }
    return true;
}
void FstHierarchy::clear() {
    Hierarchy::clear();
}
std::string FstHierarchy::get_version_string(void* fst_reader) const {
    const char* ver = fstReaderGetVersionString(fst_reader);
    return ver ? std::string(ver) : "";
}
std::string FstHierarchy::get_date_string(void* fst_reader) const {
    const char* date = fstReaderGetDateString(fst_reader);
    return date ? std::string(date) : "";
}
FileType FstHierarchy::get_file_type(void* fst_reader) const {
    int type = fstReaderGetFileType(fst_reader);
    switch (type) {
    case 0: return FileType::VCD;
    case 1: return FileType::FST;
    default: return FileType::Unknown;
    }
}
uint32_t FstHierarchy::get_alias_count(void* fst_reader) const {
    return fstReaderGetAliasCount(fst_reader);
}
uint64_t FstHierarchy::get_start_time(void* fst_reader) const {
    return fstReaderGetStartTime(fst_reader);
}
uint64_t FstHierarchy::get_end_time(void* fst_reader) const {
    return fstReaderGetEndTime(fst_reader);
}
Timescale FstHierarchy::get_timescale(void* fst_reader) const {
    int8_t exponent = fstReaderGetTimescale(fst_reader);
    return convert_timescale(static_cast<int>(exponent));
}
uint64_t FstHierarchy::get_timezero(void* fst_reader) const {
    return fstReaderGetTimezero(fst_reader);
}
uint32_t FstHierarchy::get_value_change_section_count(void* fst_reader) const {
    return fstReaderGetValueChangeSectionCount(fst_reader);
}
std::vector<Time> FstHierarchy::get_time_table(void* fst_reader) {
    uint64_t start_time = fstReaderGetStartTime(fst_reader);
    uint64_t end_time = fstReaderGetEndTime(fst_reader);
    fstReaderSetFacProcessMaskAll(fst_reader);
    fstTsList* tslist = fstReaderGetTimestamps(fst_reader);
    std::vector<Time> table;
    if (tslist && tslist->nvals > 0) {
        
        table.reserve(tslist->nvals + 2);
        
        if (start_time < tslist->val[0]) {
            table.push_back(start_time);
        }
        
        table.insert(table.end(), tslist->val, tslist->val + tslist->nvals);
        
        if (end_time > tslist->val[tslist->nvals - 1]) {
            table.push_back(end_time);
        }
    } else {
        table = {start_time, end_time};
    }
    if (tslist) {
        fstReaderFreeTimestamps(tslist);
    }
    return table;
}
HierarchyMetaData FstHierarchy::get_metadata(void* fst_reader) const {
    HierarchyMetaData meta;
    meta.version = get_version_string(fst_reader);
    meta.date = get_date_string(fst_reader);
    meta.timescale = get_timescale(fst_reader);
    meta.file_type = get_file_type(fst_reader);
    
    return meta;
}
