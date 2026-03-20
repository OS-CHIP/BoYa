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

#include "hierarchyprefixdialog.h"
HierarchyPrefixDialog::HierarchyPrefixDialog(QWidget *parent)
    : QDialog(parent)
    , m_originalPrefixEdit(nullptr)
    , m_newPrefixEdit(nullptr)
    , m_previewLabel(nullptr)
    , m_confirmButton(nullptr)
    , m_cancelButton(nullptr)
    , m_resetButton(nullptr)
{
    setupUI();
    setupConnections();
    
    setWindowTitle("Hierarchy Prefix Configuration");
    setMinimumWidth(450);
    setModal(true);
}
HierarchyPrefixDialog::~HierarchyPrefixDialog()
{
    
}
void HierarchyPrefixDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QLabel *descriptionLabel = new QLabel(
        "If your analysis starts from a specific hierarchy (not the top level), change the hierarchy prefix accordingly.",
        this
        );
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet("QLabel { color: #666; padding: 5px; }");
    
    QLabel *originalLabel = new QLabel("Original Hierarchy Prefix:", this);
    originalLabel->setToolTip("The current prefix that will be replaced");
    m_originalPrefixEdit = new QLineEdit(this);
    m_originalPrefixEdit->setPlaceholderText("e.g., top.module.submodule");
    m_originalPrefixEdit->setToolTip("Leave empty to add prefix to all signals");
    
    QLabel *newLabel = new QLabel("New Hierarchy Prefix:", this);
    newLabel->setToolTip("The new prefix that will replace the original");
    m_newPrefixEdit = new QLineEdit(this);
    m_newPrefixEdit->setPlaceholderText("e.g., new_top.new_module");
    m_newPrefixEdit->setToolTip("Enter the new hierarchy prefix");
    
    m_previewLabel = new QLabel("Preview: No changes", this);
    m_previewLabel->setStyleSheet("QLabel { font-style: italic; color: #888; }");
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_confirmButton = new QPushButton("Confirm", this);
    m_confirmButton->setDefault(true);
    m_confirmButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");
    m_cancelButton = new QPushButton("Cancel", this);
    m_resetButton = new QPushButton("Reset", this);
    buttonLayout->addWidget(m_confirmButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    
    mainLayout->addWidget(descriptionLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(originalLabel);
    mainLayout->addWidget(m_originalPrefixEdit);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(newLabel);
    mainLayout->addWidget(m_newPrefixEdit);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_previewLabel);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(buttonLayout);
    
    setStyleSheet(
        "QDialog { background-color: #f5f5f5; }"
        "QLabel { font-weight: bold; color: #333; }"
        "QLineEdit { padding: 5px; border: 1px solid #ccc; border-radius: 3px; }"
        "QPushButton { padding: 8px 15px; border: none; border-radius: 3px; }"
        "QPushButton:hover { background-color: #e0e0e0; }"
        );
}
void HierarchyPrefixDialog::setupConnections()
{
    connect(m_confirmButton, &QPushButton::clicked, this, &HierarchyPrefixDialog::onConfirmClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &HierarchyPrefixDialog::onCancelClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &HierarchyPrefixDialog::onResetClicked);
    
    connect(m_originalPrefixEdit, &QLineEdit::textChanged, this, &HierarchyPrefixDialog::updatePreview);
    connect(m_newPrefixEdit, &QLineEdit::textChanged, this, &HierarchyPrefixDialog::updatePreview);
    
    connect(m_newPrefixEdit, &QLineEdit::returnPressed, m_confirmButton, &QPushButton::click);
}
QString HierarchyPrefixDialog::getOriginalPrefix() const
{
    return m_originalPrefixEdit->text().trimmed();
}
QString HierarchyPrefixDialog::getNewPrefix() const
{
    return m_newPrefixEdit->text().trimmed();
}
void HierarchyPrefixDialog::setOriginalPrefix(const QString& prefix)
{
    m_originalPrefixEdit->setText(prefix);
}
void HierarchyPrefixDialog::setNewPrefix(const QString& prefix)
{
    m_newPrefixEdit->setText(prefix);
}
void HierarchyPrefixDialog::clearInputs()
{
    m_originalPrefixEdit->clear();
    m_newPrefixEdit->clear();
}
void HierarchyPrefixDialog::onConfirmClicked()
{
    QString originalPrefix = getOriginalPrefix();
    QString newPrefix = getNewPrefix();

    emit prefixChanged(originalPrefix, newPrefix);
    
    accept();

}
void HierarchyPrefixDialog::onCancelClicked()
{
    
    clearInputs();
    
    reject();
}
void HierarchyPrefixDialog::onResetClicked()
{
    
    m_newPrefixEdit->clear();
    
    emit prefixReset();
    
    QMessageBox::information(this, "Reset", "Hierarchy prefix settings have been reset.");
}
void HierarchyPrefixDialog::updatePreview()
{
    QString original = getOriginalPrefix();
    QString newPrefix = getNewPrefix();
    if (original.isEmpty() && newPrefix.isEmpty()) {
        m_previewLabel->setText("Preview: No changes");
    } else if (original.isEmpty()) {
        m_previewLabel->setText(QString("Preview: All signals will get prefix '%1'").arg(newPrefix));
    } else if (newPrefix.isEmpty()) {
        m_previewLabel->setText(QString("Preview: Prefix '%1' will be removed").arg(original));
    } else {
        m_previewLabel->setText(QString("Preview: '%1' → '%2'").arg(original).arg(newPrefix));
    }
}