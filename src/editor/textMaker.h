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

#ifndef TEXTMAKER_H
#define TEXTMAKER_H
#include <QString>
struct TextMarker {
    QString name;
    int source_line;
    int source_column;
    int name_length;
    QString kind;
    QString multi_array="";

    TextMarker() : source_line(-1), source_column(-1), name_length(0) {}

    bool isEmpty() const {
        return source_line == -1;
    }

    void clear() {
        source_line = -1;
        source_column = -1;
        name_length = 0;
        name.clear();
        kind.clear();
        multi_array.clear();
    }
};
#endif
