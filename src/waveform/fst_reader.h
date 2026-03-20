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
#include "reader.h"
#include "fstapi.h"
class FstReader : public IWaveformReader {
private:
    void* fst_reader;
    bool is_open;
public:
    FstReader();
    ~FstReader();
    bool open(const std::string& filename) override;
    void close() override;
    QSharedPointer<Waveform> read() override;
    FileType get_file_type() const override { return FileType::FST; }
    bool load_signal_data(QSharedPointer<Waveform>& waveform, SignalRef signal_ref) override;
    void load_signals_data(QSharedPointer<Waveform>& waveform, const std::vector<SignalRef>& signal_refs) override;
private:
    bool build_hierarchy(Waveform& waveform);
    bool build_time_table(Waveform& waveform);
};