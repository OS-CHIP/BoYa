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

#include "string_pool.h"
StringId StringPool::add_string(const std::string& str) {
    if (str.empty()) {
        return INVALID_STRING_ID;
    }
    auto it = string_to_id.find(str);
    if (it != string_to_id.end()) {
        return it->second;
    }
    StringId id = static_cast<StringId>(strings.size() + 1);
    strings.push_back(str);
    string_to_id[str] = id;
    return id;
}
StringId StringPool::get_stringId(const std::string& str) const {
    if (str.empty()) {
        return INVALID_STRING_ID;
    }
    auto it = string_to_id.find(str);
    if (it != string_to_id.end()) {
        return it->second;
    }
    return INVALID_STRING_ID;
}
const std::string& StringPool::get_string(StringId id) const {
    if (id == INVALID_STRING_ID || id > strings.size()) {
        static const std::string empty;
        return empty;
    }
    return strings[id - 1];
}
const std::vector<std::string> StringPool::get_strings() const{
    return strings;
}
size_t StringPool::size() const {
    return strings.size();
}
void StringPool::clear() {
    strings.clear();
    string_to_id.clear();
}