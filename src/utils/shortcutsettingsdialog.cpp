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

#include "shortcutsettingsdialog.h"
#include "ui_shortcutsettingsdialog.h"
#include "shortcutsmanager.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QKeySequenceEdit>
#include <QDebug>
ShortcutSettingsDialog::ShortcutSettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ShortcutSettingsDialog)
{
    ui->setupUi(this);
    setupUI();
    loadShortcuts();
    
    connect(ui->categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ShortcutSettingsDialog::onCategoryChanged);

    connect(ui->resetButton, &QPushButton::clicked,
            this, &ShortcutSettingsDialog::onResetClicked);
    connect(ui->resetAllButton, &QPushButton::clicked,
            this, &ShortcutSettingsDialog::onResetAllClicked);
    connect(ui->filterEdit, &QLineEdit::textChanged,
            this, &ShortcutSettingsDialog::onFilterTextChanged);
    connect(ui->clearFilterButton, &QPushButton::clicked,
            this, &ShortcutSettingsDialog::onClearFilterClicked);
    
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &ShortcutSettingsDialog::onAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &ShortcutSettingsDialog::reject);
}
ShortcutSettingsDialog::~ShortcutSettingsDialog()
{
    delete ui;
}
void ShortcutSettingsDialog::setupUI()
{
    ui->shortcutTable->setColumnCount(3);
    ui->shortcutTable->setHorizontalHeaderLabels(
        QStringList() << tr("Description") << tr("Shortcut") << tr("Default"));
    
    QHeaderView* header = ui->shortcutTable->horizontalHeader();
    
    header->setSectionResizeMode(QHeaderView::Stretch);
    
    header->setMinimumSectionSize(100);
    
    int tableWidth = ui->shortcutTable->width();
    if (tableWidth > 0) {
        ui->shortcutTable->setColumnWidth(0, tableWidth * 0.4);  
        ui->shortcutTable->setColumnWidth(1, tableWidth * 0.3);  
        ui->shortcutTable->setColumnWidth(2, tableWidth * 0.3);  
    }
    
    ui->shortcutTable->setAlternatingRowColors(true);
    ui->shortcutTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->shortcutTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->shortcutTable->setShowGrid(true);
    ui->shortcutTable->setGridStyle(Qt::SolidLine);
    ui->shortcutTable->horizontalHeader()->setStretchLastSection(true);
    ui->shortcutTable->verticalHeader()->setVisible(false);
    ui->shortcutTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}
void ShortcutSettingsDialog::loadShortcuts()
{
    ShortcutsManager* manager = ShortcutsManager::instance();
    m_shortcutsByCategory = manager->getShortcutsByCategory();
    ui->categoryCombo->clear();
    ui->categoryCombo->addItem(tr("All"));
    for (const QString& category : m_shortcutsByCategory.keys()) {
        ui->categoryCombo->addItem(category);
    }
    updateTable();
}
void ShortcutSettingsDialog::updateTable()
{
    
    int currentRow = ui->shortcutTable->currentRow();
    QString selectedId;
    if (currentRow >= 0) {
        QTableWidgetItem* item = ui->shortcutTable->item(currentRow, 0);
        if (item) {
            selectedId = item->data(Qt::UserRole).toString();
        }
    }
    ui->shortcutTable->setRowCount(0);
    ShortcutsManager* manager = ShortcutsManager::instance();
    QString category = ui->categoryCombo->currentText();
    QString filter = ui->filterEdit->text().toLower();
    QList<QString> shortcutIds;
    if (category == tr("All")) {
        shortcutIds = manager->getAllShortcutIds();
    } else if (m_shortcutsByCategory.contains(category)) {
        shortcutIds = m_shortcutsByCategory[category];
    }
    for (const QString& id : shortcutIds) {
        QString description = manager->getDescription(id);
        QString cat = manager->getCategory(id);
        
        if (!filter.isEmpty()) {
            bool match = description.toLower().contains(filter) ||
                         cat.toLower().contains(filter) ||
                         id.toLower().contains(filter);
            if (!match) {
                continue;
            }
        }
        addShortcutToTable(id);
    }
    
    if (!selectedId.isEmpty()) {
        for (int row = 0; row < ui->shortcutTable->rowCount(); ++row) {
            QTableWidgetItem* item = ui->shortcutTable->item(row, 0);
            if (item && item->data(Qt::UserRole).toString() == selectedId) {
                ui->shortcutTable->setCurrentCell(row, 0);
                break;
            }
        }
    }
}
void ShortcutSettingsDialog::addShortcutToTable(const QString& id)
{
    ShortcutsManager* manager = ShortcutsManager::instance();
    int row = ui->shortcutTable->rowCount();
    ui->shortcutTable->insertRow(row);
    
    QTableWidgetItem* descItem = new QTableWidgetItem(manager->getDescription(id));
    descItem->setData(Qt::UserRole, id);
    descItem->setFlags(descItem->flags() & ~Qt::ItemIsEditable);
    ui->shortcutTable->setItem(row, 0, descItem);
    
    QKeySequenceEdit* shortcutEdit = new QKeySequenceEdit(this);
    shortcutEdit->setKeySequence(manager->getShortcut(id));
    shortcutEdit->setProperty("shortcutId", id);
    ui->shortcutTable->setCellWidget(row, 1, shortcutEdit);
    
    connect(shortcutEdit, &QKeySequenceEdit::keySequenceChanged,
            this, [this, id, row](const QKeySequence& keySequence) {
                qDebug() << "Shortcut changed in table:" << id
                         << "to" << keySequence.toString();
                
                ShortcutsManager::instance()->setShortcut(id, keySequence);
            });
    
    QTableWidgetItem* defaultItem = new QTableWidgetItem(
        manager->getDefaultShortcut(id));
    defaultItem->setFlags(defaultItem->flags() & ~Qt::ItemIsEditable);
    ui->shortcutTable->setItem(row, 2, defaultItem);
}
void ShortcutSettingsDialog::onAccepted()
{
    qDebug() << "=== OK Button Clicked ===";
    
    for (int row = 0; row < ui->shortcutTable->rowCount(); ++row) {
        QKeySequenceEdit* edit = qobject_cast<QKeySequenceEdit*>(
            ui->shortcutTable->cellWidget(row, 1));
        if (edit) {
            QString id = ui->shortcutTable->item(row, 0)->data(Qt::UserRole).toString();
            QKeySequence currentShortcut = edit->keySequence();
            
            ShortcutsManager* manager = ShortcutsManager::instance();
            QKeySequence savedShortcut = manager->getShortcut(id);
            if (currentShortcut != savedShortcut) {
                qDebug() << "  -> Setting to manager";
                manager->setShortcut(id, currentShortcut);
            }
        }
    }
    
    ShortcutsManager::instance()->saveConfig();
    ShortcutsManager* manager = ShortcutsManager::instance();
    for (int row = 0; row < ui->shortcutTable->rowCount(); ++row) {
        QString id = ui->shortcutTable->item(row, 0)->data(Qt::UserRole).toString();
        QKeySequence saved = manager->getShortcut(id);
    }
    accept();
}
void ShortcutSettingsDialog::onCategoryChanged(int index)
{
    Q_UNUSED(index);
    updateTable();
}
void ShortcutSettingsDialog::onResetClicked()
{
    int row = ui->shortcutTable->currentRow();
    if (row >= 0) {
        QString id = ui->shortcutTable->item(row, 0)->data(Qt::UserRole).toString();
        ShortcutsManager::instance()->resetToDefault(id);
        
        QKeySequenceEdit* edit = qobject_cast<QKeySequenceEdit*>(
            ui->shortcutTable->cellWidget(row, 1));
        if (edit) {
            edit->setKeySequence(ShortcutsManager::instance()->getShortcut(id));
        }
    }
}
void ShortcutSettingsDialog::onResetAllClicked()
{
    if (QMessageBox::question(this, tr("Reset All Shortcuts"),
                              tr("Reset all shortcuts to default values?"))
        == QMessageBox::Yes) {
        ShortcutsManager::instance()->resetAllShortcuts();
        updateTable();
    }
}
void ShortcutSettingsDialog::onImportClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    tr("Import Shortcuts"),
                                                    "",
                                                    tr("JSON Files (*.json);;All Files (*.*)"));
    if (!filePath.isEmpty()) {
        ShortcutsManager::instance()->importConfig(filePath);
        updateTable();
    }
}
void ShortcutSettingsDialog::onExportClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    tr("Export Shortcuts"),
                                                    "",
                                                    tr("JSON Files (*.json);;All Files (*.*)"));
    if (!filePath.isEmpty()) {
        ShortcutsManager::instance()->exportConfig(filePath);
    }
}
void ShortcutSettingsDialog::onFilterTextChanged(const QString& text)
{
    Q_UNUSED(text);
    updateTable();
}
void ShortcutSettingsDialog::onClearFilterClicked()
{
    ui->filterEdit->clear();
    updateTable();
}
void ShortcutSettingsDialog::onShortcutCellChanged(int row, int column)
{
    
    if (column == 1) {  
        QKeySequenceEdit* edit = qobject_cast<QKeySequenceEdit*>(
            ui->shortcutTable->cellWidget(row, column));
        if (edit) {
            QString id = ui->shortcutTable->item(row, 0)->data(Qt::UserRole).toString();
            QKeySequence newShortcut = edit->keySequence();
            
            ShortcutsManager::instance()->setShortcut(id, newShortcut);
        }
    }
    
    Q_UNUSED(row);
    Q_UNUSED(column);
    
}