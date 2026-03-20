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
#include <cstdint>
#include <string>
#include <vector>
using StringId = uint32_t;
using VarRef = uint32_t;
using ScopeRef = uint32_t;
using Time = uint64_t;
using TimeTableIdx = uint32_t;
using SignalRef = uint32_t;
constexpr VarRef INVALID_VAR_REF = 0;
constexpr ScopeRef INVALID_SCOPE_REF = 0;
constexpr StringId INVALID_STRING_ID = 0;
enum class VarType {
    
    Event,
    Integer,
    Parameter,
    Real,
    Reg,
    Supply0,
    Supply1,
    Time,
    Tri,
    TriAnd,
    TriOr,
    TriReg,
    Tri0,
    Tri1,
    WAnd,
    Wire,
    WOr,
    String,
    Port,
    SparseArray,
    RealTime,
    
    Bit,
    Logic,
    Int,
    ShortInt,
    LongInt,
    Byte,
    Enum,
    ShortReal,
    
    Boolean,
    BitVector,
    StdLogic,
    StdLogicVector,
    StdULogic,
    StdULogicVector
};
enum class ScopeType {
    
    Module,
    Task,
    Function,
    Begin,
    Fork,
    Generate,
    Struct,
    Union,
    Class,
    Interface,
    Package,
    Program,
    
    VhdlArchitecture,
    VhdlProcedure,
    VhdlFunction,
    VhdlRecord,
    VhdlProcess,
    VhdlBlock,
    VhdlForGenerate,
    VhdlIfGenerate,
    VhdlGenerate,
    VhdlPackage,
    Root
};
struct SignalType {
    bool is_real;
    bool is_string;
    uint32_t width;
    SignalType() : is_real(false), is_string(false), width(1) {}
    SignalType(bool real, bool str, uint32_t w) : is_real(real), is_string(str), width(w) {}
};
enum class TimescaleUnit {
    FemtoSeconds,
    PicoSeconds,
    NanoSeconds,
    MicroSeconds,
    MilliSeconds,
    Seconds,
    Unknown
};
struct Timescale {
    uint32_t factor;
    TimescaleUnit unit;
    Timescale(uint32_t factor, TimescaleUnit unit);
    std::string to_string() const;
};
enum class FileType {
    VCD,
    FST,
    Unknown
};
struct HierarchyMetaData {
    Timescale timescale;
    std::string date;
    std::string version;
    std::vector<std::string> comments;
    FileType file_type;
    HierarchyMetaData();
};
enum Endianness {
    ENDIAN_BIG,    
    ENDIAN_LITTLE  
};
struct DimInfo {
    int left;           
    int right;          
    Endianness endian;  
    bool is_range;      
    DimInfo() : left(0), right(0), is_range(false), endian(ENDIAN_BIG) {}
    DimInfo(int l, int r, bool range = true)
        : left(l), right(r), is_range(range) {
        if (is_range) {
            endian = (left >= right) ? ENDIAN_BIG : ENDIAN_LITTLE;
        } else {
            endian = ENDIAN_BIG;  
        }
    }
    int width() const {
        if (is_range) {
            return std::abs(left - right) + 1;
        }
        return 1;
    }
    std::string to_string() const {
        if (is_range) {
            return "[" + std::to_string(left) + ":" + std::to_string(right) + "]";
        } else {
            return "[" + std::to_string(left) + "]";
        }
    }
    
    int get_logical_min() const {
        return std::min(left, right);
    }
    
    int get_logical_max() const {
        return std::max(left, right);
    }
    
    int get_physical_min() const {
        return 0;
    }
    
    int get_physical_max() const {
        return width() - 1;
    }
    
    int logical_to_physical(int logical_idx) const {
        if (!is_range) return 0;
        int min_val = get_logical_min();
        int physical_idx = logical_idx - min_val;
        return physical_idx;
    }
    
    int physical_to_logical(int physical_idx) const {
        if (!is_range) return left;  
        int min_val = get_logical_min();
        if (endian == ENDIAN_BIG) {
            
            return min_val + physical_idx;
        } else {
            
            return min_val + (width() - 1 - physical_idx);
        }
    }
    
    bool is_valid_logical(int logical_idx) const {
        int min_val = get_logical_min();
        int max_val = get_logical_max();
        return logical_idx >= min_val && logical_idx <= max_val;
    }
    
    bool is_valid_physical(int physical_idx) const {
        return physical_idx >= 0 && physical_idx < width();
    }
};
Timescale convert_timescale(int8_t exponent);
std::string timescale_unit_to_string(TimescaleUnit unit);
std::string file_type_to_string(FileType type);