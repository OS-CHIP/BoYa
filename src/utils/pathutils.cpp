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

#include "pathutils.h"
#include <QCoreApplication>
#include <QDir>
QString PathUtils::applicationDirPath() {
    static const QString path = QCoreApplication::applicationDirPath();
    return path;
}
QString PathUtils::resolvePath(const QString &relativePath) {
    return QDir(applicationDirPath()).absoluteFilePath(relativePath);
}
QString PathUtils::makeAbsolutePath(const QString &relativePath) {
    
    if (relativePath.isEmpty()) {
        return QString(); 
    }

    QString base = QDir::currentPath();
    
    if (QDir::isAbsolutePath(relativePath)) {
        return QDir::cleanPath(relativePath);
    }
    
    QDir baseDir(base);
    
    QString absolutePath = baseDir.absoluteFilePath(relativePath);
    
    return QDir::cleanPath(absolutePath);
}
QSet<QString> GlobalPaths::m_paths;
QMutex GlobalPaths::m_mutex;
void GlobalPaths::addPath(const QString& path) {
    QMutexLocker locker(&m_mutex);
    m_paths.insert(path);
}
bool GlobalPaths::contains(const QString& path) {
    QMutexLocker locker(&m_mutex);
    return m_paths.contains(path);
}
QSet<QString> GlobalPaths::getAllPaths() {
    QMutexLocker locker(&m_mutex);
    return m_paths;
}
void GlobalPaths::clear() {
    QMutexLocker locker(&m_mutex);
    m_paths.clear();
}