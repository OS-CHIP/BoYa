# Copyright 2025 OSCHIP
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.16)
project(BoYa VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

option(BOYA_USE_EDITOR_SOURCE "Use boya-editor source code" OFF)
if(BOYA_USE_EDITOR_SOURCE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../boya-editor/CMakeLists.txt")
        add_subdirectory(../boya-editor boya-editor-build)
    endif()
endif()
if(BOYA_USE_EDITOR_SOURCE)
    set(BOYA_EDITOR_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../boya-editor")
else()
    set(BOYA_EDITOR_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/editor")
endif()

if(WIN32)
    set(BOYA_EDITOR_LIB_EXT "dll.a")
    set(BOYA_EDITOR_SHARED_EXT "dll")
else()
    set(BOYA_EDITOR_LIB_EXT "so")
    set(BOYA_EDITOR_SHARED_EXT "so")
endif()

set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(WIN32)
    set(ZLIB_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib.dll")
endif()
find_package(ZLIB REQUIRED)
message(STATUS "ZLIB found: ${ZLIB_FOUND}")

if(WIN32)
    set(BOYA_EDITOR_IMPORT_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libboya-editor.dll.a")
    set(BOYA_EDITOR_SHARED_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libboya-editor.dll")
else()
    set(BOYA_EDITOR_IMPORT_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libboya-editor.so")
    set(BOYA_EDITOR_SHARED_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libboya-editor.so")

    find_library(BOYA_EDITOR_LIB boya-editor
        PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/lib"
            "${CMAKE_CURRENT_SOURCE_DIR}/../boya-editor/build"
        NO_DEFAULT_PATH
    )

    if(BOYA_EDITOR_LIB)
        set(BOYA_EDITOR_IMPORT_LIB "${BOYA_EDITOR_LIB}")
        set(BOYA_EDITOR_SHARED_LIB "${BOYA_EDITOR_LIB}")
    endif()
endif()

if(NOT EXISTS "${BOYA_EDITOR_IMPORT_LIB}")
    set(SEARCH_PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/../boya-editor/build/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/../boya-editor/build/bin"
        "${CMAKE_CURRENT_SOURCE_DIR}/../boya-editor/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/../build-boya-editor/lib"
        "/usr/local/lib"
        "/usr/lib"
    )

    if(WIN32)
        find_file(BOYA_EDITOR_LIB_FILE "libboya-editor.dll.a"
            PATHS ${SEARCH_PATHS}
            NO_DEFAULT_PATH
        )
        find_file(BOYA_EDITOR_SHARED_FILE "libboya-editor.dll"
            PATHS ${SEARCH_PATHS}
            NO_DEFAULT_PATH
        )
    else()
        find_file(BOYA_EDITOR_LIB_FILE "libboya-editor.so"
            PATHS ${SEARCH_PATHS}
            NO_DEFAULT_PATH
        )
        set(BOYA_EDITOR_SHARED_FILE "${BOYA_EDITOR_LIB_FILE}")
    endif()

    if(BOYA_EDITOR_LIB_FILE)
        set(BOYA_EDITOR_IMPORT_LIB "${BOYA_EDITOR_LIB_FILE}")
    endif()

    if(BOYA_EDITOR_SHARED_FILE)
        set(BOYA_EDITOR_SHARED_LIB "${BOYA_EDITOR_SHARED_FILE}")
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets PrintSupport Sql Concurrent)

set(FST_C_SOURCES
    src/third_party/fst/fastlz.c
    src/third_party/fst/lz4.c
    src/third_party/fst/fstapi.c
    src/third_party/fst/fstext.c
)

set(FST_HEADERS
    src/third_party/fst/config.h
    src/third_party/fst/fastlz.h
    src/third_party/fst/lz4.h
    src/third_party/fst/fstapi.h
    src/third_party/fst/fstext.h
    src/third_party/fst/fst_win_unistd.h
)

add_library(fst STATIC ${FST_C_SOURCES} ${FST_HEADERS})

set_target_properties(fst PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(fst PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/fst
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fst PRIVATE ZLIB::ZLIB)
target_compile_definitions(fst PRIVATE HAVE_ZLIB)

set(MAIN_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/waveform/string_pool.cpp
    src/waveform/hierarchy.cpp
    src/waveform/signal.cpp
    src/waveform/waveform.cpp
    src/waveform/reader.cpp
    src/waveform/fst_hierarchy.cpp
    src/waveform/fst_reader.cpp
    src/waveform/wavewindow.cpp
    src/waveform/signalexplorer.cpp
    src/waveform/waveform_display.cpp
    src/waveform/translators.cpp
    src/waveform/HighlightDialog.cpp
    src/waveform/vcd_hierarchy.cpp
    src/waveform/vcd_reader.cpp
    src/waveform/oneSearch.cpp
    src/waveform/signalsModel.cpp
    src/waveform/logicaloperationdialog.cpp
    src/instance/instance.cpp
    src/utils/myutils.cpp
    src/utils/pathutils.cpp
    src/theme/ThemeManager.cpp
    src/utils/shortcutsmanager.cpp
    src/utils/shortcutsettingsdialog.cpp
    src/utils/hierarchyprefixdialog.cpp
)

set(MAIN_HEADERS
    src/mainwindow.h
    ${BOYA_EDITOR_INCLUDE_DIR}/texteditor.h
    ${BOYA_EDITOR_INCLUDE_DIR}/signallist.h
    src/instance/instance.h
    src/theme/ThemeManager.h
    src/waveform/fst_hierarchy.h
    src/waveform/fst_reader.h
    src/waveform/HighlightDialog.h
    src/waveform/hierarchy.h
    src/waveform/logicaloperationdialog.h
    src/waveform/oneSearch.h
    src/waveform/reader.h
    src/waveform/signal.h
    src/waveform/signalexplorer.h
    src/waveform/signalsModel.h
    src/waveform/string_pool.h
    src/waveform/translators.h
    src/waveform/vcd_hierarchy.h
    src/waveform/vcd_reader.h
    src/waveform/waveform.h
    src/waveform/waveform_display.h
    src/waveform/waveform_types.h
    src/waveform/wavewindow.h
    src/waveform/GlobalCounter.h
    src/utils/myutils.h
    src/utils/pathutils.h
    src/utils/shortcutsmanager.h
    src/utils/shortcutdefinitions.h
    src/utils/shortcutsettingsdialog.h
    src/utils/hierarchyprefixdialog.h
    src/utils/globalState.h
)

set(MAIN_UIS
    src/mainwindow.ui
    src/waveform/wavewindow.ui
    src/instance/instance.ui
    src/utils/shortcutsettingsdialog.ui
)

set(MAIN_RESOURCES
    src/static/myresources.qrc
    src/static/app.rc
)

add_executable(BoYa ${MAIN_SOURCES} ${MAIN_HEADERS} ${MAIN_UIS} ${MAIN_RESOURCES})

if(WIN32)
    set_target_properties(BoYa PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

target_include_directories(BoYa PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/waveform
    ${CMAKE_CURRENT_SOURCE_DIR}/src/instance
    ${CMAKE_CURRENT_SOURCE_DIR}/src/theme
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/fst
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ZLIB_INCLUDE_DIRS}
    ${BOYA_EDITOR_INCLUDE_DIR}
)

if(TARGET boya-editor)
    target_link_libraries(BoYa PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::PrintSupport
        Qt6::Sql
        Qt6::Concurrent
        fst
        boya-editor
        ZLIB::ZLIB
    )

elseif(EXISTS "${BOYA_EDITOR_IMPORT_LIB}")
    target_link_libraries(BoYa PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::PrintSupport
        Qt6::Sql
        Qt6::Concurrent
        fst
        ZLIB::ZLIB
    )

    target_link_directories(BoYa PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../boya-editor/build/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../boya-editor/build/bin
    )

    target_link_libraries(BoYa PRIVATE "${BOYA_EDITOR_IMPORT_LIB}")

    if(UNIX AND NOT APPLE)
        set_target_properties(BoYa PROPERTIES
            INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
        )

        add_custom_command(TARGET BoYa POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${BOYA_EDITOR_SHARED_LIB}"
                $<TARGET_FILE_DIR:BoYa>/
        )
    endif()

    if(WIN32 AND EXISTS "${BOYA_EDITOR_SHARED_LIB}")
        add_custom_command(TARGET BoYa POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${BOYA_EDITOR_SHARED_LIB}"
                $<TARGET_FILE_DIR:BoYa>/
        )
    endif()

else()
    find_library(BOYA_EDITOR_SYSTEM_LIB boya-editor)
    if(BOYA_EDITOR_SYSTEM_LIB)
        target_link_libraries(BoYa PRIVATE ${BOYA_EDITOR_SYSTEM_LIB})
    else()
        message(FATAL_ERROR "boya-editor library not found")
    endif()

    target_link_libraries(BoYa PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::PrintSupport
        Qt6::Sql
        Qt6::Concurrent
        fst
        ZLIB::ZLIB
    )
endif()

if(WIN32 AND EXISTS "${ZLIB_LIBRARY}")
    add_custom_command(TARGET BoYa POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ZLIB_LIBRARY}"
            $<TARGET_FILE_DIR:BoYa>/
    )
endif()

if(WIN32)
    set(SLANG_EXE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows/slang.exe")
    if(EXISTS "${SLANG_EXE_PATH}")
        add_custom_command(TARGET BoYa POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SLANG_EXE_PATH}"
                $<TARGET_FILE_DIR:BoYa>/
        )
    else()
        message(WARNING "slang.exe not found at ${SLANG_EXE_PATH}")
    endif()
endif()

if(UNIX AND NOT APPLE)
    set(LINUX_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin/linux")

    if(EXISTS "${LINUX_BIN_DIR}")
        set(SLANG_FILES
            "${LINUX_BIN_DIR}/ld-linux-x86-64.so.2"
            "${LINUX_BIN_DIR}/libsvlang.so.9"
            "${LINUX_BIN_DIR}/libsvlang.so.9.0.0"
            "${LINUX_BIN_DIR}/slang"
        )

        foreach(SLANG_FILE ${SLANG_FILES})
            if(EXISTS "${SLANG_FILE}")
                get_filename_component(FILE_NAME "${SLANG_FILE}" NAME)

                add_custom_command(TARGET BoYa POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${SLANG_FILE}"
                        $<TARGET_FILE_DIR:BoYa>/

                    COMMAND_EXPAND_LISTS
                )

                if(FILE_NAME STREQUAL "slang")
                    add_custom_command(TARGET BoYa POST_BUILD
                        COMMAND chmod +x $<TARGET_FILE_DIR:BoYa>/slang
                    )
                endif()
            else()
                message(WARNING "Slang file not found: ${SLANG_FILE}")
            endif()
        endforeach()

        set(LIBSVLANG_SO "${LINUX_BIN_DIR}/libsvlang.so.9.0.0")
        if(EXISTS "${LIBSVLANG_SO}")
            add_custom_command(TARGET BoYa POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                    "libsvlang.so.9.0.0"
                    $<TARGET_FILE_DIR:BoYa>/libsvlang.so

                WORKING_DIRECTORY $<TARGET_FILE_DIR:BoYa>
            )
        endif()
    else()
        message(WARNING "Linux binary directory not found: ${LINUX_BIN_DIR}")
    endif()
endif()

qt_finalize_executable(BoYa)

include(GNUInstallDirs)
install(TARGETS BoYa
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(EXISTS "${BOYA_EDITOR_SHARED_LIB}")
    install(FILES "${BOYA_EDITOR_SHARED_LIB}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(WIN32 AND EXISTS "${ZLIB_LIBRARY}")
    install(FILES "${ZLIB_LIBRARY}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(WIN32)
    if(EXISTS "${SLANG_EXE_PATH}")
        install(FILES "${SLANG_EXE_PATH}"
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif()

if(UNIX AND NOT APPLE)
    set(LINUX_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin/linux")

    if(EXISTS "${LINUX_BIN_DIR}")
        add_custom_command(TARGET BoYa POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LINUX_BIN_DIR}/ld-linux-x86-64.so.2"
                $<TARGET_FILE_DIR:BoYa>/
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LINUX_BIN_DIR}/slang"
                $<TARGET_FILE_DIR:BoYa>/
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LINUX_BIN_DIR}/libsvlang.so.9.0.0"
                $<TARGET_FILE_DIR:BoYa>/
            COMMAND chmod 755
                $<TARGET_FILE_DIR:BoYa>/ld-linux-x86-64.so.2
            COMMAND chmod 755
                $<TARGET_FILE_DIR:BoYa>/slang

            COMMAND cd $<TARGET_FILE_DIR:BoYa> &&
                ${CMAKE_COMMAND} -E create_symlink
                libsvlang.so.9.0.0
                libsvlang.so

            COMMENT "Copying and setting up slang binaries"
        )

        install(FILES
            "${LINUX_BIN_DIR}/ld-linux-x86-64.so.2"
            "${LINUX_BIN_DIR}/slang"
            "${LINUX_BIN_DIR}/libsvlang.so.9.0.0"
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif()
