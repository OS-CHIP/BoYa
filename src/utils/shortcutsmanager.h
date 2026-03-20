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

#ifndef SHORTCUTSMANAGER_H
#define SHORTCUTSMANAGER_H
#include <QObject>
#include <QMap>
#include <QHash>
#include <QKeySequence>
#include <QAction>
#include <QSettings>
class ShortcutsManager : public QObject
{
    Q_OBJECT
public:
    
    static ShortcutsManager* instance();
    
    bool registerShortcut(const QString& id,
                          const QString& defaultShortcut,
                          const QString& description = "",
                          const QString& category = "");
    
    bool bindToAction(const QString& id, QAction* action);
    
    QKeySequence getShortcut(const QString& id) const;
    bool setShortcut(const QString& id, const QKeySequence& shortcut);
    bool resetToDefault(const QString& id);
    void resetAllShortcuts();
    
    QString getDescription(const QString& id) const;
    QString getCategory(const QString& id) const;
    QString getDefaultShortcut(const QString& id) const;
    
    QMap<QString, QList<QString>> getShortcutsByCategory() const;
    QList<QString> getAllShortcutIds() const;
    
    void saveConfig();
    void loadConfig();
    void exportConfig(const QString& filePath);
    void importConfig(const QString& filePath);
    
    QList<QString> checkConflicts(const QString& id,
                                  const QKeySequence& shortcut) const;
signals:
    void shortcutChanged(const QString& id,
                         const QKeySequence& oldShortcut,
                         const QKeySequence& newShortcut);
    void shortcutConflicted(const QString& id1,
                            const QString& id2,
                            const QKeySequence& shortcut);
private:
    explicit ShortcutsManager(QObject* parent = nullptr);
    ~ShortcutsManager();
    struct ShortcutInfo {
        QString id;
        QString defaultShortcut;
        QString currentShortcut;
        QString description;
        QString category;
        QList<QAction*> boundActions;
    };
    QMap<QString, ShortcutInfo> m_shortcuts;
    QMap<QKeySequence, QString> m_shortcutMap;  
    QSettings* m_settings;
    static ShortcutsManager* m_instance;
};
#endif 