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
#include "waveform.h"
#include <memory>
#include <string>
class IWaveformReader {
public:
    virtual ~IWaveformReader() = default;
    
    virtual bool open(const std::string& filename) = 0;
    
    virtual void close() = 0;
    
    virtual QSharedPointer<Waveform> read() = 0;
    
    virtual FileType get_file_type() const = 0;
    
    virtual bool load_signal_data(QSharedPointer<Waveform>& waveform, SignalRef signal_ref) = 0;
    
    virtual void load_signals_data(QSharedPointer<Waveform>& waveform, const std::vector<SignalRef>& signal_ref) = 0;
};
QSharedPointer<IWaveformReader> create_reader(FileType type);
QSharedPointer<IWaveformReader> create_reader_by_extension(const std::string& filename);