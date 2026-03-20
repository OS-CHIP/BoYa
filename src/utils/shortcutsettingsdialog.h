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

#ifndef SHORTCUTSETTINGSDIALOG_H
#define SHORTCUTSETTINGSDIALOG_H
#include <QDialog>
#include <QKeySequenceEdit>
QT_BEGIN_NAMESPACE
namespace Ui {
class ShortcutSettingsDialog;
}
QT_END_NAMESPACE
class ShortcutSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ShortcutSettingsDialog(QWidget* parent = nullptr);
    ~ShortcutSettingsDialog();  
private slots:
    
    void onCategoryChanged(int index);
    void onShortcutCellChanged(int row, int column);
    void onResetClicked();
    void onResetAllClicked();
    void onImportClicked();
    void onExportClicked();
    void onFilterTextChanged(const QString& text);
    void onClearFilterClicked();
    
    void onAccepted();
private:
    void setupUI();
    void loadShortcuts();
    void updateTable();
    void applyChanges();  
    void addShortcutToTable(const QString& id);
private:
    Ui::ShortcutSettingsDialog* ui;
    QMap<QString, QList<QString>> m_shortcutsByCategory;
};
#endif 