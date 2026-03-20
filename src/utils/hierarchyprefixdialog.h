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

#ifndef HIERARCHYPREFIXDIALOG_H
#define HIERARCHYPREFIXDIALOG_H
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
class HierarchyPrefixDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HierarchyPrefixDialog(QWidget *parent = nullptr);
    ~HierarchyPrefixDialog();
    
    QString getOriginalPrefix() const;
    QString getNewPrefix() const;
    
    void setOriginalPrefix(const QString& prefix);
    void setNewPrefix(const QString& prefix);
    
    void clearInputs();
signals:
    void prefixChanged(const QString& original, const QString& newPrefix);
    void prefixReset();
private slots:
    void onConfirmClicked();
    void onCancelClicked();
    void onResetClicked();
    void updatePreview();
private:
    void setupUI();
    void setupConnections();
    QLineEdit *m_originalPrefixEdit;
    QLineEdit *m_newPrefixEdit;
    QLabel *m_previewLabel;
    QPushButton *m_confirmButton;
    QPushButton *m_cancelButton;
    QPushButton *m_resetButton;
};
#endif 