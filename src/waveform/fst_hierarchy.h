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
#include "fstapi.h"
class FstHierarchy : public Hierarchy {
public:
    FstHierarchy();
    ~FstHierarchy();
    
    bool build_from_fst(void* fst_reader);
    
    void clear();
    
    std::string get_version_string(void* fst_reader) const;
    std::string get_date_string(void* fst_reader) const;
    FileType get_file_type(void* fst_reader) const;
    uint32_t get_alias_count(void* fst_reader) const;
    uint64_t get_start_time(void* fst_reader) const;
    uint64_t get_end_time(void* fst_reader) const;
    Timescale get_timescale(void* fst_reader) const;
    uint64_t get_timezero(void* fst_reader) const;
    uint32_t get_value_change_section_count(void* fst_reader) const;
    std::vector<Time> get_time_table(void* fst_reader);
    
    HierarchyMetaData get_metadata(void* fst_reader) const;
};