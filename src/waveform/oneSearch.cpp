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

#include "oneSearch.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QMessageBox>
#include <QUrlQuery>
OneSearchTab::OneSearchTab(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    searchInput = new QLineEdit(this);
    searchInput->setPlaceholderText("Search...");
    
    searchButton = new QPushButton("search", this);
    
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(searchInput);
    topLayout->addWidget(searchButton);
    
    resultBrowser = new QTextBrowser(this);
    resultBrowser->setOpenLinks(false);
    layout->addLayout(topLayout);
    layout->addWidget(resultBrowser);
    connect(searchButton, &QPushButton::clicked, this, &OneSearchTab::performSearch);
    connect(resultBrowser, &QTextBrowser::anchorClicked, this, &OneSearchTab::handleAnchorClicked);
}
void OneSearchTab::setFileListPath(const QStringList &filelistPath) {
    fileList.clear();
    for (const QString& fileName : filelistPath) {
        QFileInfo fileInfo(fileName);
        QString extension = fileInfo.suffix().toLower();
        if (extension == "f" || extension.isEmpty()) {
            
            QFile file(fileName);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                while (!in.atEnd()) {
                    QString line = in.readLine().trimmed();
                    if (!line.isEmpty()) {
                        fileList.append(line);
                    }
                }
                file.close();
            } else {
                qWarning() << "Cannot open file:" << filelistPath;
            }
        }
        
        else if (extension == "v" || extension == "vh" ||
                 extension == "sv" || extension == "svh") {
            
            fileList.append(filelistPath);
        }
        
        else {
            qWarning() << "Don't support this file:" << filelistPath;
        }
    }
}
void OneSearchTab::performSearch() {
    QString keyword = searchInput->text().trimmed();
    if (keyword.isEmpty()) {
        QMessageBox::warning(this, "Search", "Please input keywords");
        return;
    }
    resultBrowser->clear();
    
    for (const QString &filePath : fileList) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        QTextStream in(&file);
        int lineNumber = 1;
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.contains(keyword)) {
                
                QString highlighted = line;
                highlighted.replace(keyword,
                                    "<b><font color='red'>" + keyword + "</font></b>");
                
                QString link = QString("<a href='?path=%1&line=%2'>%3:%4</a>")
                                   .arg(filePath)
                                   .arg(lineNumber)
                                   .arg(QFileInfo(filePath).fileName())
                                   .arg(lineNumber);
                
                resultBrowser->append(link + "     " + highlighted);
            }
            lineNumber++;
        }
        file.close();
    }
}
void OneSearchTab::handleAnchorClicked(const QUrl &url) {
    QUrlQuery query(url);
    
    QString filePath = query.queryItemValue("path");
    int lineNumber = 1;
    if (query.hasQueryItem("line")) {
        lineNumber = query.queryItemValue("line").toInt();
    }
    
    emit openFileRequested(filePath, lineNumber);
}