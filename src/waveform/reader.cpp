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

#include "reader.h"
#include <algorithm>
#include "fst_reader.h"
#include "vcd_reader.h"
QSharedPointer<IWaveformReader> create_reader(FileType type) {
    switch (type) {
    case FileType::FST:

        return QSharedPointer<FstReader>::create();
        break;
    case FileType::VCD:

        return QSharedPointer<VcdReader>::create();
        break;
    default:
        return nullptr;
    }
    return nullptr;
}
QSharedPointer<IWaveformReader> create_reader_by_extension(const std::string& filename) {
    
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return nullptr;
    }
    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == "fst") {
        return create_reader(FileType::FST);
    } else if (ext == "vcd") {
        return create_reader(FileType::VCD);
    }
    return nullptr;
}