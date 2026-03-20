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

#include <iostream>
#include <string>
#include <QApplication>
#include "mainwindow.h"
#include "third_party/cxxopts.hpp"
#include <vector>
#include <QString>
#include <QDebug> 
int main(int argc, char** argv) {
    
    cxxopts::Options options("BoYa", "An advanced waveform viewer and analysis tool for chip/FPGA designers and verification engineers. Created by OSCHIP team.");
    
    options.add_options()
        ("f,filelist", "Filelist input file path (can be specified multiple times)",
         cxxopts::value<std::vector<std::string>>()) 
        ("w,waveform", "Waveform input file path", cxxopts::value<std::string>())
        ("h,help", "Print usage")
        ("source-files", "Direct source files (.v, .sv)",
         cxxopts::value<std::vector<std::string>>()) 
        ;
    
    options.parse_positional({"source-files"});
    options.positional_help("[source files...]");
    
    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << std::endl;
        std::cerr << options.help() << std::endl;
        return 1;
    }
    
    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/icons/icon.ico"));
    MainWindow viewer;
    viewer.show();
    QStringList allFiles;
    
    if (result.count("filelist")) {
        auto filelists = result["filelist"].as<std::vector<std::string>>();
        for (const auto& filelist : filelists) {
            QString fileListFile = QString::fromStdString(filelist);
            qDebug() << "Processing filelist: " << fileListFile;
            allFiles.append(fileListFile);
        }
    }
    
    if (result.count("source-files")) {
        auto sourceFiles = result["source-files"].as<std::vector<std::string>>();
        for (const auto& sourceFile : sourceFiles) {
            QString filePath = QString::fromStdString(sourceFile);
            qDebug() << "Processing direct source file: " << filePath;
            
            if (filePath.endsWith(".v") || filePath.endsWith(".sv") ||
                filePath.endsWith(".vh") || filePath.endsWith(".svh")) {
                allFiles.append(filePath);
            } else {
                qWarning() << "Skipping unsupported file type: " << filePath;
            }
        }
    }
    if(allFiles.size() >0) {
        emit viewer.fileListFileReady(allFiles);
    }
    
    if (result.count("waveform")) {
        std::string waveformFileStr = result["waveform"].as<std::string>();
        QString waveformFile = QString::fromStdString(waveformFileStr);
        qDebug() << "Processing waveform file: " << waveformFile;
        emit viewer.instance->openFile(waveformFile);
    }
    return app.exec();
}
