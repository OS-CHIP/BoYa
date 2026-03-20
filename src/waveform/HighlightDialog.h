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

#ifndef HIGHLIGHTDIALOG_H
#define HIGHLIGHTDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QPushButton>
#include <QMap>
#include <QColor>
#include <QLabel>
#include <QSet>
#include <QStyledItemDelegate>


struct SignalHighlightConfig {
    QString signalKey;
    QColor nameColor;
    QColor valueColor;
    QColor lineColor;
    QColor backgroundColor;

    SignalHighlightConfig() = default;

    SignalHighlightConfig(const QColor& name, const QColor& value,
                          const QColor& line, const QColor& background)
        : nameColor(name), valueColor(value), lineColor(line), backgroundColor(background) {}

    bool isValid() const {
        return nameColor.isValid() || valueColor.isValid() ||
               lineColor.isValid() || backgroundColor.isValid();
    }
};

class HighlightTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit HighlightTreeDelegate(QObject *parent = nullptr);

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class HighlightDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HighlightDialog(QWidget *parent = nullptr);
    ~HighlightDialog();
    void addSignal(const QString& signalKey, const QString& path, const SignalHighlightConfig& config);
    int findSignalRow(const QString& path);
    void selectSignalRow(const QString& path);
    QTreeWidgetItem* findSignalItem(const QString& path);
    QTreeWidgetItem* findSignalItemInGroup(QTreeWidgetItem* groupItem, const QString& path);

    QMap<QString, SignalHighlightConfig> getSignalHighlights() const;

signals:
    void highlightsApplied(const QMap<QString, SignalHighlightConfig>& highlights);

private slots:
    void onNameColorButtonClicked(const QColor& color);
    void onValueColorButtonClicked(const QColor& color);
    void onLineColorButtonClicked(const QColor& color);
    void onBackgroundColorButtonClicked(const QColor& color);
    void cleanupEmptyGroups(QTreeWidgetItem* groupItem);
    void onRemoveHighlight();
    void onRemoveAllHighlight();
    void onApply();
    void onSave();
    void saveGroupSignals(QTreeWidgetItem* groupItem, QJsonArray& signalsArray);
    void onLoad();
    void onSignalSelectionChanged();

private:
    void setupUI();
    void createColorButtons();
    void updateSelectedSignalsColor(const QString& colorType, const QColor& color);
    QString extractGroupName(const QString& path);
    QTreeWidgetItem* findOrCreateGroupItem(const QString& groupName);
    QSet<QTreeWidgetItem*> getSelectedSignalItems() const;
    void updateSignalItemColor(QTreeWidgetItem* signalItem, const QString& colorType, const QColor& color);
    void updateColorButtonIcons();

private:
    QTreeWidget *m_signalTree;
    HighlightTreeDelegate *m_treeDelegate;
    QWidget *m_colorButtonsWidget;
    QPushButton *m_nameColorButton;
    QPushButton *m_valueColorButton;
    QPushButton *m_lineColorButton;
    QPushButton *m_backgroundColorButton;
    QPushButton *m_removeButton;
    QPushButton *m_removeAllButton;
    QPushButton *m_saveButton;
    QPushButton *m_loadButton;
    QPushButton *m_applyButton;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    QList<QColor> m_colorPalette;
    QMap<QString, SignalHighlightConfig> m_signalHighlights;

    QString m_currentColorType;
    QColor m_currentNameColor;
    QColor m_currentValueColor;
    QColor m_currentLineColor;
    QColor m_currentBackgroundColor;
};

#endif
