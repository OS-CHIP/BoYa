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

#ifndef SHORTCUTDEFINITIONS_H
#define SHORTCUTDEFINITIONS_H
#include <QString>
namespace Shortcuts {
    
    namespace File {
    const QString IMPORT_DESIGN = "file.import_design";
    const QString OPEN = "file.open";
    const QString SAVE_GROUP = "file.save_group";
    const QString LOAD_GROUP = "file.load_group";
    const QString NEW_WAVEFORM = "file.new_waveform";
    }
    
    namespace Edit {
    const QString ADD_TO_WAVEFORM  = "edit.add_to_waveform";
    }
    
    namespace View {
    const QString INSTANCE = "view.instance";
    const QString EDITOR = "view.editor";
    const QString WAVEFORM = "view.waveform";
    }
    
    namespace Tools {
    const QString GLOBAL_SEARCH = "tools.global_search";
    const QString HIERARCHY_CHANGE = "tools.hierarchy_change";
    const QString FONT = "tools.font";
    const QString GO_TO_LINE = "tools.go_to_line";
    const QString SHORTCUTS = "tools.shortcuts";
    }
    
    namespace Waveform {
        const QString SIGNALADD = "waveform.signal_add";
        const QString HIERARCHY_NAME = "waveform.hierarchy_name";
        const QString GROUPADD = "waveform.group_add";
    }
}
#endif 