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

#include "shortcutsmanager.h"
#include "shortcutdefinitions.h"
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
ShortcutsManager* ShortcutsManager::m_instance = nullptr;
ShortcutsManager* ShortcutsManager::instance()
{
    if (!m_instance) {
        m_instance = new ShortcutsManager();
    }
    return m_instance;
}
ShortcutsManager::ShortcutsManager(QObject* parent)
    : QObject(parent)
{
    m_settings = new QSettings("BoYa", "BoYa");
    loadConfig();
}
ShortcutsManager::~ShortcutsManager()
{
    saveConfig();
    delete m_settings;
}
bool ShortcutsManager::registerShortcut(const QString& id,
                                        const QString& defaultShortcut,
                                        const QString& description,
                                        const QString& category)
{
    if (m_shortcuts.contains(id)) {
        
        return false;
    }
    ShortcutInfo info;
    info.id = id;
    info.defaultShortcut = defaultShortcut;
    info.description = description;
    info.category = category;
    
    QString savedShortcut = m_settings->value(QString("Shortcuts/%1").arg(id)).toString();
    if (savedShortcut.isEmpty()) {
        
        info.currentShortcut = defaultShortcut;
    } else {
        
        info.currentShortcut = savedShortcut;
    }
    m_shortcuts[id] = info;
    
    QKeySequence shortcut(info.currentShortcut);
    if (!shortcut.isEmpty()) {
        m_shortcutMap[shortcut] = id;
    }
    return true;
}
bool ShortcutsManager::bindToAction(const QString& id, QAction* action)
{
    if (!m_shortcuts.contains(id) || !action) {
        return false;
    }
    ShortcutInfo& info = m_shortcuts[id];
    QKeySequence shortcut(info.currentShortcut);
    action->setShortcut(shortcut);
    action->setToolTip(info.description);
    if (!info.boundActions.contains(action)) {
        info.boundActions.append(action);
    }
    return true;
}
QKeySequence ShortcutsManager::getShortcut(const QString& id) const
{
    if (!m_shortcuts.contains(id)) {
        return QKeySequence();
    }
    return QKeySequence(m_shortcuts[id].currentShortcut);
}
bool ShortcutsManager::setShortcut(const QString& id,
                                   const QKeySequence& shortcut)
{
    if (!m_shortcuts.contains(id)) {
        qWarning() << "Cannot set shortcut: unknown id" << id;
        return false;
    }
    ShortcutInfo& info = m_shortcuts[id];
    QKeySequence oldShortcut(info.currentShortcut);
    qDebug() << "=== setShortcut called ===";
    qDebug() << "ID:" << id;
    qDebug() << "Old shortcut:" << oldShortcut.toString();
    qDebug() << "New shortcut:" << shortcut.toString();
    
    if (shortcut == oldShortcut) {
        qDebug() << "No change needed";
        return true;
    }
    
    QList<QString> conflicts = checkConflicts(id, shortcut);
    if (!conflicts.isEmpty()) {
        qWarning() << "Shortcut conflict detected for" << id << "with" << conflicts;
        emit shortcutConflicted(id, conflicts.first(), shortcut);
        return false;
    }
    
    if (!oldShortcut.isEmpty()) {
        m_shortcutMap.remove(oldShortcut);
        qDebug() << "Removed old mapping:" << oldShortcut.toString();
    }
    
    info.currentShortcut = shortcut.toString();
    qDebug() << "Updated m_shortcuts[" << id << "] to:" << info.currentShortcut;
    
    if (!shortcut.isEmpty()) {
        m_shortcutMap[shortcut] = id;
        qDebug() << "Added new mapping:" << shortcut.toString() << "->" << id;
    }
    
    for (QAction* action : info.boundActions) {
        action->setShortcut(shortcut);
        qDebug() << "Updated action:" << action->text()
                 << "to shortcut:" << shortcut.toString();
    }
    
    QString settingsKey = QString("Shortcuts/%1").arg(id);
    m_settings->setValue(settingsKey, info.currentShortcut);
    m_settings->sync();
    qDebug() << "Saved to settings:" << settingsKey << "=" << info.currentShortcut;
    
    QString savedValue = m_settings->value(settingsKey).toString();
    qDebug() << "Verify from settings:" << savedValue;
    emit shortcutChanged(id, oldShortcut, shortcut);
    return true;
}
bool ShortcutsManager::resetToDefault(const QString& id)
{
    if (!m_shortcuts.contains(id)) {
        return false;
    }
    const ShortcutInfo& info = m_shortcuts[id];
    return setShortcut(id, QKeySequence(info.defaultShortcut));
}
void ShortcutsManager::resetAllShortcuts()
{
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        resetToDefault(it.key());
    }
}
QString ShortcutsManager::getDescription(const QString& id) const
{
    if (m_shortcuts.contains(id)) {
        return m_shortcuts[id].description;
    }
    return QString();
}
QString ShortcutsManager::getCategory(const QString& id) const
{
    if (m_shortcuts.contains(id)) {
        return m_shortcuts[id].category;
    }
    return QString();
}
QString ShortcutsManager::getDefaultShortcut(const QString& id) const
{
    if (m_shortcuts.contains(id)) {
        return m_shortcuts[id].defaultShortcut;
    }
    return QString();
}
QMap<QString, QList<QString>> ShortcutsManager::getShortcutsByCategory() const
{
    QMap<QString, QList<QString>> result;
    for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
        const QString& category = it.value().category;
        if (!category.isEmpty()) {
            result[category].append(it.key());
        }
    }
    return result;
}
QList<QString> ShortcutsManager::getAllShortcutIds() const
{
    return m_shortcuts.keys();
}
QList<QString> ShortcutsManager::checkConflicts(const QString& id,
                                                const QKeySequence& shortcut) const
{
    QList<QString> conflicts;
    if (shortcut.isEmpty()) {
        return conflicts;  
    }
    auto it = m_shortcutMap.find(shortcut);
    if (it != m_shortcutMap.end() && it.value() != id) {
        conflicts.append(it.value());
    }
    return conflicts;
}
void ShortcutsManager::saveConfig()
{
    qDebug() << "=== saveConfig() called ===";
    
    for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
        const QString& id = it.key();
        const QString& shortcut = it.value().currentShortcut;
        qDebug() << "m_shortcuts[" << id << "] =" << shortcut;
        
        m_settings->setValue(QString("Shortcuts/%1").arg(id), shortcut);
    }
    m_settings->sync();
    
    m_settings->beginGroup("Shortcuts");
    QStringList savedKeys = m_settings->allKeys();
    m_settings->endGroup();
    qDebug() << "Saved keys in settings:" << savedKeys;
    for (const QString& key : savedKeys) {
        QString value = m_settings->value(QString("Shortcuts/%1").arg(key)).toString();
        qDebug() << "  " << key << "=" << value;
    }
}
void ShortcutsManager::loadConfig()
{
    
    m_settings->beginGroup("Shortcuts");
    QStringList savedKeys = m_settings->allKeys();
    m_settings->endGroup();
    
    for (const QString& key : savedKeys) {
        QString shortcut = m_settings->value(QString("Shortcuts/%1").arg(key)).toString();
        
        if (m_shortcuts.contains(key)) {
            ShortcutInfo& info = m_shortcuts[key];
            
            if (!info.currentShortcut.isEmpty()) {
                QKeySequence oldShortcut(info.currentShortcut);
                m_shortcutMap.remove(oldShortcut);
            }
            
            info.currentShortcut = shortcut;
            
            QKeySequence newShortcut(shortcut);
            if (!newShortcut.isEmpty()) {
                m_shortcutMap[newShortcut] = key;
            }
            
            for (QAction* action : info.boundActions) {
                action->setShortcut(newShortcut);
            }
            qDebug() << "Updated shortcut for" << key << "to" << shortcut;
        }
    }
}
void ShortcutsManager::exportConfig(const QString& filePath)
{
    QJsonObject root;
    QJsonArray shortcutsArray;
    for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
        QJsonObject shortcutObj;
        shortcutObj["id"] = it.key();
        shortcutObj["shortcut"] = it.value().currentShortcut;
        shortcutObj["description"] = it.value().description;
        shortcutObj["category"] = it.value().category;
        shortcutObj["default"] = it.value().defaultShortcut;
        shortcutsArray.append(shortcutObj);
    }
    root["shortcuts"] = shortcutsArray;
    root["version"] = "1.0";
    root["application"] = QCoreApplication::applicationName();
    QJsonDocument doc(root);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}
void ShortcutsManager::importConfig(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        return;
    }
    QJsonObject root = doc.object();
    QJsonArray shortcutsArray = root["shortcuts"].toArray();
    for (const QJsonValue& value : shortcutsArray) {
        QJsonObject obj = value.toObject();
        QString id = obj["id"].toString();
        QString shortcut = obj["shortcut"].toString();
        if (m_shortcuts.contains(id)) {
            setShortcut(id, QKeySequence(shortcut));
        }
    }
    saveConfig();
}