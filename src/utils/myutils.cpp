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

#include "myutils.h"
#include<QPixmap>
#include<QIcon>
#include<QPainter>
QString MyUtils::getScopeIconPath(ScopeType type) {
    static const std::unordered_map<ScopeType, QString> iconMap = {
        {ScopeType::Root, ":/icons/instance/database.png"},
        {ScopeType::Module, ":/icons/instance/module.png"},
        {ScopeType::Task, ":/icons/instance/task.png"},
        {ScopeType::Function, ":/icons/instance/function.png"},
        {ScopeType::Class, ":/icons/instance/class.png"},
        {ScopeType::Interface, ":/icons/instance/interface.png"},
        {ScopeType::Package, ":/icons/instance/package.png"},
        {ScopeType::Program, ":/icons/instance/program.png"},
        {ScopeType::Struct, ":/icons/instance/struct.png"},
        {ScopeType::Union, ":/icons/instance/union.png"},
        {ScopeType::VhdlArchitecture, ":/icons/instance/vhdl_architecture.png"},
        {ScopeType::VhdlProcedure, ":/icons/instance/vhdl_procedure.png"},
        {ScopeType::VhdlFunction, ":/icons/instance/vhdl_function.png"},
        {ScopeType::VhdlRecord, ":/icons/instance/vhdl_record.png"},
        {ScopeType::VhdlProcess, ":/icons/instance/vhdl_process.png"},
        {ScopeType::VhdlBlock, ":/icons/instance/vhdl_block.png"},
        {ScopeType::VhdlForGenerate, ":/icons/instance/vhdl_for_generate.png"},
        {ScopeType::VhdlIfGenerate, ":/icons/instance/vhdl_if_generate.png"},
        {ScopeType::VhdlGenerate, ":/icons/instance/vhdl_generate.png"},
        {ScopeType::VhdlPackage, ":/icons/instance/vhdl_package.png"}
        
    };
    auto it = iconMap.find(type);
    if (it != iconMap.end()) {
        return it->second;
    }
    return ":/icons/instance/default.png"; 
}
QIcon MyUtils::changeIconColor(const QIcon& originalIcon, const QColor& targetColor) {
    
    QPixmap pixmap = originalIcon.pixmap(QSize(64, 64));
    
    QPainter painter(&pixmap);
    
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    
    painter.fillRect(pixmap.rect(), targetColor);
    
    return QIcon(pixmap);
}
std::string MyUtils::trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(),
                                  [](unsigned char c){ return std::isspace(c); });
    auto end = std::find_if_not(str.rbegin(), str.rend(),
                                [](unsigned char c){ return std::isspace(c); }).base();
    return (start < end) ? std::string(start, end) : "";
}
std::string MyUtils::ltrim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(),
                                  [](unsigned char c){ return std::isspace(c); });
    return std::string(start, str.end());
}
std::string MyUtils::rtrim(const std::string& str) {
    auto end = std::find_if_not(str.rbegin(), str.rend(),
                                [](unsigned char c){ return std::isspace(c); }).base();
    return std::string(str.begin(), end);
}