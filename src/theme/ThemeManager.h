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

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H
#include <QObject>
#include <QString>
#include <QSettings>
class ThemeManager : public QObject
{
    Q_OBJECT
public:
    static ThemeManager& instance();
    void loadTheme(const QString& themeName); 
    
    void saveUserPreference(const QString& themeName);
    QString loadUserPreference();
    QString currentTheme();
signals:
    void themeChanged(const QString& themeName); 
private:
    QString m_currentTheme;
    explicit ThemeManager(QObject *parent = nullptr);
};
#endif 