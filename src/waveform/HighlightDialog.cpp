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

#include "HighlightDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QColorDialog>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QSplitter>

HighlightTreeDelegate::HighlightTreeDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void HighlightTreeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QColor originalBgColor = opt.backgroundBrush.color();
    if (!originalBgColor.isValid()) {
        originalBgColor = opt.palette.color(QPalette::Base);
    }

    QColor originalTextColor = opt.palette.color(QPalette::Text);
    bool isSelected = opt.state & QStyle::State_Selected;
    bool isHovered = opt.state & QStyle::State_MouseOver;
    if (isSelected) {
        originalBgColor.setAlphaF(0.8);
    } else if (isHovered) {
        originalBgColor.setAlphaF(0.7);
    }
    painter->fillRect(opt.rect, originalBgColor);
    painter->setPen(originalTextColor);
    painter->drawText(opt.rect, Qt::AlignLeft, opt.text);
}

HighlightDialog::~HighlightDialog()
{
}

HighlightDialog::HighlightDialog(QWidget *parent)
    : QDialog(parent)
    , m_signalTree(new QTreeWidget(this))
    , m_treeDelegate(new HighlightTreeDelegate(this))
    , m_colorButtonsWidget(new QWidget(this))
    , m_nameColorButton(new QPushButton(this))
    , m_valueColorButton(new QPushButton(this))
    , m_lineColorButton(new QPushButton(this))
    , m_backgroundColorButton(new QPushButton(this))
    , m_removeButton(new QPushButton(tr("Remove"), this))
    , m_removeAllButton(new QPushButton(tr("Remove All"), this))
    , m_saveButton(new QPushButton(tr("Save"), this))
    , m_loadButton(new QPushButton(tr("Load"), this))
    , m_applyButton(new QPushButton(tr("Apply"), this))
    , m_okButton(new QPushButton(tr("OK"), this))
    , m_cancelButton(new QPushButton(tr("Cancel"), this))
{
    setupUI();
    createColorButtons();
    connect(m_nameColorButton, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(m_currentNameColor, this, tr("Select Name Color"));
        if (color.isValid()) {
            onNameColorButtonClicked(color);
        }
    });

    connect(m_valueColorButton, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(m_currentValueColor, this, tr("Select Value Color"));
        if (color.isValid()) {
            onValueColorButtonClicked(color);
        }
    });

    connect(m_lineColorButton, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(m_currentLineColor, this, tr("Select Line Color"));
        if (color.isValid()) {
            onLineColorButtonClicked(color);
        }
    });

    connect(m_backgroundColorButton, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(m_currentBackgroundColor, this, tr("Select Background Color"));
        if (color.isValid()) {
            onBackgroundColorButtonClicked(color);
        }
    });

    connect(m_removeButton, &QPushButton::clicked, this, &HighlightDialog::onRemoveHighlight);
    connect(m_removeAllButton, &QPushButton::clicked, this, &HighlightDialog::onRemoveAllHighlight);
    connect(m_saveButton, &QPushButton::clicked, this, &HighlightDialog::onSave);
    connect(m_loadButton, &QPushButton::clicked, this, &HighlightDialog::onLoad);
    connect(m_applyButton, &QPushButton::clicked, this, &HighlightDialog::onApply);
    connect(m_okButton, &QPushButton::clicked, this, [this]() {
        onApply();
        accept();
    });
    connect(m_cancelButton, &QPushButton::clicked, this, &HighlightDialog::reject);
    connect(m_signalTree, &QTreeWidget::itemSelectionChanged, this, &HighlightDialog::onSignalSelectionChanged);
    m_currentNameColor = Qt::black;
    m_currentValueColor = Qt::black;
    m_currentLineColor = Qt::black;
    m_currentBackgroundColor = Qt::white;

    updateColorButtonIcons();
}

void HighlightDialog::setupUI()
{
    setWindowTitle(tr("Signal Highlight Configuration"));
    setMinimumSize(600, 400);
    resize(800, 500);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    m_signalTree->setHeaderLabels(QStringList() << tr("Signal") << tr("Name") << tr("Value") << tr("Line") << tr("Background"));
    m_signalTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_signalTree->setRootIsDecorated(true);
    m_signalTree->setAlternatingRowColors(false);
    m_signalTree->setMouseTracking(true);
    m_signalTree->setItemDelegate(m_treeDelegate);
    m_signalTree->setStyleSheet(
        "QTreeWidget {"
        "  outline: 0;"
        "  border: 1px solid #666666;"
        "  border-radius: 0px;"
        "}"
        "QTreeWidget::item {"
        "  border: 0px;"
        "  padding: 1px;"
        "  border-bottom: 1px solid #444444;"
        "}"
        "QTreeWidget::item:selected {"
        "  background: transparent;"
        "}"
        "QTreeWidget::item:hover {"
        "  background: transparent;"
        "}"
        "QHeaderView::section {"
        "  padding: 2px;"
        "  border: 0px solid transparent;"
        "  border-bottom: 1px solid #666666;"
        "  border-right: 0px solid transparent;"
        "  font-weight: bold;"
        "  background-color: transparent;"
        "}"
        );

    m_signalTree->header()->setMinimumHeight(25);
    m_signalTree->header()->setDefaultSectionSize(25);
    m_signalTree->header()->setStretchLastSection(false);
    m_signalTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_signalTree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_signalTree->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_signalTree->header()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_signalTree->header()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_signalTree->setColumnWidth(0, 350);
    m_signalTree->setColumnWidth(1, 80);
    m_signalTree->setColumnWidth(2, 80);
    m_signalTree->setColumnWidth(3, 80);
    m_signalTree->setColumnWidth(4, 100);
    m_signalTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QWidget *signalTreeContainer = new QWidget(this);
    QVBoxLayout *signalTreeLayout = new QVBoxLayout(signalTreeContainer);
    signalTreeLayout->setContentsMargins(0, 0, 0, 0);
    signalTreeLayout->setSpacing(0);
    signalTreeLayout->addWidget(m_signalTree);
    mainLayout->addWidget(signalTreeContainer, 1);
    QWidget *colorSelectorWidget = new QWidget(this);
    QHBoxLayout *colorSelectorLayout = new QHBoxLayout(colorSelectorWidget);
    colorSelectorLayout->setSpacing(15);
    colorSelectorLayout->setContentsMargins(10, 10, 10, 10);
    colorSelectorLayout->setAlignment(Qt::AlignCenter);
    auto createColorSelector = [this](const QString& labelText, QPushButton* button) -> QWidget* {
        QWidget *widget = new QWidget(this);
        QHBoxLayout *hLayout = new QHBoxLayout(widget);
        hLayout->setSpacing(5);
        hLayout->setContentsMargins(0, 0, 0, 0);

        QLabel *label = new QLabel(labelText + ":", this);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setMinimumWidth(60);
        button->setMinimumSize(100, 30);
        button->setMaximumSize(120, 30);

        // 移除按钮边框，让它更像颜色显示区域而不是按钮
        button->setStyleSheet("QPushButton {"
                              "  border: 1px solid #666;"
                              "  border-radius: 3px;"
                              "  padding: 2px;"
                              "  font-family: monospace;"
                              "  font-size: 10pt;"
                              "}");

        hLayout->addWidget(label);
        hLayout->addWidget(button);

        return widget;
    };
    QWidget *nameSelector = createColorSelector(tr("Name"), m_nameColorButton);
    QWidget *valueSelector = createColorSelector(tr("Value"), m_valueColorButton);
    QWidget *lineSelector = createColorSelector(tr("Line"), m_lineColorButton);
    QWidget *bgSelector = createColorSelector(tr("Background"), m_backgroundColorButton);
    colorSelectorLayout->addStretch();
    colorSelectorLayout->addWidget(nameSelector);
    colorSelectorLayout->addWidget(valueSelector);
    colorSelectorLayout->addWidget(lineSelector);
    colorSelectorLayout->addWidget(bgSelector);
    colorSelectorLayout->addStretch();
    mainLayout->addWidget(colorSelectorWidget, 0);
    QWidget *buttonWidget = new QWidget(this);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(10, 10, 10, 10);
    QWidget *leftButtonGroup = new QWidget(this);
    QHBoxLayout *leftButtonLayout = new QHBoxLayout(leftButtonGroup);
    leftButtonLayout->setSpacing(10);
    leftButtonLayout->setContentsMargins(0, 0, 0, 0);
    m_removeButton->setFixedSize(80, 30);
    m_removeAllButton->setFixedSize(80, 30);
    leftButtonLayout->addWidget(m_removeButton);
    leftButtonLayout->addWidget(m_removeAllButton);
    QWidget *centerButtonGroup = new QWidget(this);
    QHBoxLayout *centerButtonLayout = new QHBoxLayout(centerButtonGroup);
    centerButtonLayout->setSpacing(10);
    centerButtonLayout->setContentsMargins(0, 0, 0, 0);
    m_saveButton->setFixedSize(60, 30);
    m_loadButton->setFixedSize(60, 30);
    m_applyButton->setFixedSize(60, 30);
    centerButtonLayout->addWidget(m_saveButton);
    centerButtonLayout->addWidget(m_loadButton);
    centerButtonLayout->addWidget(m_applyButton);
    QWidget *rightButtonGroup = new QWidget(this);
    QHBoxLayout *rightButtonLayout = new QHBoxLayout(rightButtonGroup);
    rightButtonLayout->setSpacing(10);
    rightButtonLayout->setContentsMargins(0, 0, 0, 0);
    m_okButton->setFixedSize(60, 30);
    m_cancelButton->setFixedSize(60, 30);
    rightButtonLayout->addWidget(m_okButton);
    rightButtonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(leftButtonGroup);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(centerButtonGroup);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(rightButtonGroup);
    mainLayout->addWidget(buttonWidget, 0);
    m_removeButton->setText(tr("Remove"));
    m_removeAllButton->setText(tr("Remove All"));
    m_saveButton->setText(tr("Save"));
    m_loadButton->setText(tr("Load"));
    m_applyButton->setText(tr("Apply"));
    m_okButton->setText(tr("OK"));
    m_cancelButton->setText(tr("Cancel"));
    m_signalTree->setMinimumHeight(250);
}


void HighlightDialog::createColorButtons()
{
    m_nameColorButton->setFixedSize(30, 30);
    m_valueColorButton->setFixedSize(30, 30);
    m_lineColorButton->setFixedSize(30, 30);
    m_backgroundColorButton->setFixedSize(30, 30);
}
void HighlightDialog::addSignal(const QString& signalKey, const QString& path, const SignalHighlightConfig& config)
{
    QString groupName = extractGroupName(path);
    QTreeWidgetItem* groupItem = findOrCreateGroupItem(groupName);
    for (int i = 0; i < groupItem->childCount(); ++i) {
        QTreeWidgetItem* child = groupItem->child(i);
        QString itemSignalKey = child->data(0, Qt::UserRole).toString();
        if (itemSignalKey == signalKey) {
            updateSignalItemColor(child, "name", config.nameColor);
            updateSignalItemColor(child, "value", config.valueColor);
            updateSignalItemColor(child, "line", config.lineColor);
            updateSignalItemColor(child, "background", config.backgroundColor);
            m_signalHighlights[signalKey] = config;
            return;
        }
    }

    QTreeWidgetItem* signalItem = new QTreeWidgetItem(groupItem);
    signalItem->setText(0, path);
    signalItem->setData(0, Qt::UserRole, signalKey);
    signalItem->setFlags(signalItem->flags() | Qt::ItemIsSelectable);
    updateSignalItemColor(signalItem, "name", config.nameColor);
    updateSignalItemColor(signalItem, "value", config.valueColor);
    updateSignalItemColor(signalItem, "line", config.lineColor);
    updateSignalItemColor(signalItem, "background", config.backgroundColor);

    m_signalHighlights[signalKey] = config;
}

int HighlightDialog::findSignalRow(const QString& signalKey)
{
    for (int i = 0; i < m_signalTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* groupItem = m_signalTree->topLevelItem(i);
        for (int j = 0; j < groupItem->childCount(); ++j) {
            QTreeWidgetItem* signalItem = groupItem->child(j);
            QString itemSignalKey = signalItem->data(0, Qt::UserRole).toString();
            if (itemSignalKey == signalKey) {
                return j;
            }
        }
    }
    return -1;
}

QTreeWidgetItem* HighlightDialog::findSignalItemInGroup(QTreeWidgetItem* groupItem, const QString& signalKey)
{
    if (!groupItem) return nullptr;
    for (int i = 0; i < groupItem->childCount(); ++i) {
        QTreeWidgetItem* child = groupItem->child(i);
        QString itemSignalKey = child->data(0, Qt::UserRole).toString();
        if (itemSignalKey == signalKey) {
            return child;
        }
        if (child->childCount() > 0) {
            QTreeWidgetItem* signalItem = findSignalItemInGroup(child, signalKey);
            if (signalItem) {
                return signalItem;
            }
        }
    }
    return nullptr;
}

QTreeWidgetItem* HighlightDialog::findSignalItem(const QString& signalKey)
{
    for (int i = 0; i < m_signalTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* groupItem = m_signalTree->topLevelItem(i);
        for (int j = 0; j < groupItem->childCount(); ++j) {
            QTreeWidgetItem* signalItem = groupItem->child(j);
            QString itemSignalKey = signalItem->data(0, Qt::UserRole).toString();
            if (itemSignalKey == signalKey) {
                return signalItem;
            }
        }
    }
    return nullptr;
}

void HighlightDialog::selectSignalRow(const QString& signalKey)
{
    for (int i = 0; i < m_signalTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* groupItem = m_signalTree->topLevelItem(i);
        for (int j = 0; j < groupItem->childCount(); ++j) {
            QTreeWidgetItem* signalItem = groupItem->child(j);
            QString itemSignalKey = signalItem->data(0, Qt::UserRole).toString();
            if (itemSignalKey == signalKey) {
                m_signalTree->clearSelection();
                signalItem->setSelected(true);
                m_signalTree->scrollToItem(signalItem);
                m_signalTree->setCurrentItem(signalItem);
                return;
            }
        }
    }
}

QMap<QString, SignalHighlightConfig> HighlightDialog::getSignalHighlights() const
{
    return m_signalHighlights;
}

void HighlightDialog::onNameColorButtonClicked(const QColor& color)
{
    m_currentNameColor = color;
    updateSelectedSignalsColor("name", color);
    updateColorButtonIcons();
}

void HighlightDialog::onValueColorButtonClicked(const QColor& color)
{
    m_currentValueColor = color;
    updateSelectedSignalsColor("value", color);
    updateColorButtonIcons();
}

void HighlightDialog::onLineColorButtonClicked(const QColor& color)
{
    m_currentLineColor = color;
    updateSelectedSignalsColor("line", color);
    updateColorButtonIcons();
}

void HighlightDialog::onBackgroundColorButtonClicked(const QColor& color)
{
    m_currentBackgroundColor = color;
    updateSelectedSignalsColor("background", color);
    updateColorButtonIcons();
}

void HighlightDialog::cleanupEmptyGroups(QTreeWidgetItem* groupItem)
{
    if (!groupItem) return;
    if (groupItem->childCount() == 0) {
        QTreeWidgetItem* parent = groupItem->parent();
        if (parent) {
            parent->removeChild(groupItem);
            delete groupItem;
            cleanupEmptyGroups(parent);
        } else {
            int index = m_signalTree->indexOfTopLevelItem(groupItem);
            if (index >= 0) {
                delete m_signalTree->takeTopLevelItem(index);
            }
        }
    }
}

void HighlightDialog::onRemoveHighlight()
{
    QSet<QTreeWidgetItem*> selectedItems = getSelectedSignalItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    updateSelectedSignalsColor("name",QColor());
    updateSelectedSignalsColor("value",QColor());
    updateSelectedSignalsColor("line",QColor());
    updateSelectedSignalsColor("background",QColor());
    onApply();

    for (QTreeWidgetItem* item : selectedItems) {
        QString signalKey = item->data(0, Qt::UserRole).toString();
        m_signalHighlights.remove(signalKey);

        QTreeWidgetItem* parent = item->parent();
        if (parent) {
            parent->removeChild(item);
            delete item;
            if (parent->childCount() == 0) {
                int index = m_signalTree->indexOfTopLevelItem(parent);
                if (index >= 0) {
                    delete m_signalTree->takeTopLevelItem(index);
                }
            }
        }
    }
}

void HighlightDialog::onRemoveAllHighlight()
{
    int ret = QMessageBox::question(this, tr("Confirm Removal"),
                                    tr("Are you sure you want to remove all highlights?"));
    if (ret == QMessageBox::Yes) {
        for (const QString &key : m_signalHighlights.keys()) {
            qDebug() << "Key:" << key;
            m_signalHighlights[key].nameColor = QColor();
            m_signalHighlights[key].valueColor = QColor();
            m_signalHighlights[key].lineColor = QColor();
            m_signalHighlights[key].backgroundColor = QColor();
        }
        onApply();
        m_signalHighlights.clear();
        m_signalTree->clear();
    }
}

void HighlightDialog::onApply()
{
    emit highlightsApplied(m_signalHighlights);
}

void HighlightDialog::saveGroupSignals(QTreeWidgetItem* groupItem, QJsonArray& signalsArray)
{
    if (!groupItem) return;
    for (int i = 0; i < groupItem->childCount(); ++i) {
        QTreeWidgetItem* child = groupItem->child(i);
        if (child->childCount() == 0) {
            QString signalKey = child->data(0, Qt::UserRole).toString();
            QString path = child->text(0);
            if (m_signalHighlights.contains(signalKey)) {
                QJsonObject signalObj;
                signalObj["signalKey"] = signalKey;
                signalObj["path"] = path;
                const SignalHighlightConfig& config = m_signalHighlights[signalKey];
                if (config.nameColor.isValid()) {
                    signalObj["nameColor"] = config.nameColor.name();
                }
                if (config.valueColor.isValid()) {
                    signalObj["valueColor"] = config.valueColor.name();
                }
                if (config.lineColor.isValid()) {
                    signalObj["lineColor"] = config.lineColor.name();
                }
                if (config.backgroundColor.isValid()) {
                    signalObj["backgroundColor"] = config.backgroundColor.name();
                }

                signalsArray.append(signalObj);
            }
        } else {
            saveGroupSignals(child, signalsArray);
        }
    }
}


void HighlightDialog::onSave()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Highlight Configuration"),
                                                    "", tr("JSON Files (*.json)"));
    if (fileName.isEmpty()) {
        return;
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Save Error"), tr("Cannot save file."));
        return;
    }
    QJsonObject root;
    QJsonArray signalsArray;
    for (int i = 0; i < m_signalTree->topLevelItemCount(); ++i) {
        saveGroupSignals(m_signalTree->topLevelItem(i), signalsArray);
    }
    root["signals"] = signalsArray;
    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
}

void HighlightDialog::onLoad()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Highlight Configuration"),
                                                    "", tr("JSON Files (*.json)"));
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Load Error"), tr("Cannot open file."));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        QMessageBox::warning(this, tr("Load Error"), tr("Invalid file format."));
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray signalsArray = root["signals"].toArray();

    m_signalHighlights.clear();
    m_signalTree->clear();

    for (const QJsonValue& value : signalsArray) {
        QJsonObject signalObj = value.toObject();
        QString signalKey = signalObj["signalKey"].toString();
        QString path = signalObj.contains("path") ?
                                 signalObj["path"].toString() : signalKey;

        SignalHighlightConfig config;
        if (signalObj.contains("nameColor")) {
            config.nameColor = QColor(signalObj["nameColor"].toString());
        }
        if (signalObj.contains("valueColor")) {
            config.valueColor = QColor(signalObj["valueColor"].toString());
        }
        if (signalObj.contains("lineColor")) {
            config.lineColor = QColor(signalObj["lineColor"].toString());
        }
        if (signalObj.contains("backgroundColor")) {
            config.backgroundColor = QColor(signalObj["backgroundColor"].toString());
        }

        addSignal(signalKey, path, config);
    }
}

void HighlightDialog::onSignalSelectionChanged()
{
    QSet<QTreeWidgetItem*> selectedItems = getSelectedSignalItems();
    if (!selectedItems.isEmpty()) {
        QTreeWidgetItem* firstItem = *selectedItems.begin();
        QColor nameColor = QColor(firstItem->text(1));
        QColor valueColor = QColor(firstItem->text(2));
        QColor lineColor = QColor(firstItem->text(3));
        QColor backgroundColor = QColor(firstItem->text(4));
        m_currentNameColor = nameColor.isValid() ? nameColor : Qt::black;
        m_currentValueColor = valueColor.isValid() ? valueColor : Qt::black;
        m_currentLineColor = lineColor.isValid() ? lineColor : Qt::black;
        m_currentBackgroundColor = backgroundColor.isValid() ? backgroundColor : Qt::white;
        updateColorButtonIcons();
    }
}

void HighlightDialog::updateSelectedSignalsColor(const QString& colorType, const QColor& color)
{
    QSet<QTreeWidgetItem*> selectedItems = getSelectedSignalItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    for (QTreeWidgetItem* item : selectedItems) {
        updateSignalItemColor(item, colorType, color);
        QString signalKey = item->data(0, Qt::UserRole).toString();
        SignalHighlightConfig config = m_signalHighlights.value(signalKey);
        if (colorType == "name") {
            config.nameColor = color;
        } else if (colorType == "value") {
            config.valueColor = color;
        } else if (colorType == "line") {
            config.lineColor = color;
        } else if (colorType == "background") {
            config.backgroundColor = color;
        }
        m_signalHighlights[signalKey] = config;
    }
}

QString HighlightDialog::extractGroupName(const QString& path)
{
    QStringList parts = path.split('.');
    if (parts.size() >= 1) {
        return parts[0];
    }
    return tr("Ungrouped");
}

QTreeWidgetItem* HighlightDialog::findOrCreateGroupItem(const QString& groupName)
{
    for (int i = 0; i < m_signalTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_signalTree->topLevelItem(i);
        if (item->text(0) == groupName) {
            return item;
        }
    }
    QTreeWidgetItem* groupItem = new QTreeWidgetItem(m_signalTree);
    groupItem->setText(0, groupName);
    groupItem->setFlags(groupItem->flags() & ~Qt::ItemIsSelectable);
    groupItem->setExpanded(true);
    return groupItem;
}

QSet<QTreeWidgetItem*> HighlightDialog::getSelectedSignalItems() const
{
    QSet<QTreeWidgetItem*> selectedItems;
    QList<QTreeWidgetItem*> items = m_signalTree->selectedItems();

    for (QTreeWidgetItem* item : items) {
        if (item->parent()) {
            selectedItems.insert(item);
        }
    }

    return selectedItems;
}

void HighlightDialog::updateSignalItemColor(QTreeWidgetItem* signalItem, const QString& colorType, const QColor& color)
{
    if (!signalItem) return;

    int column = -1;
    if (colorType == "name") {
        column = 1;
    } else if (colorType == "value") {
        column = 2;
    } else if (colorType == "line") {
        column = 3;
    } else if (colorType == "background") {
        column = 4;
    }

    if (column >= 0) {
        if (color.isValid()) {
            signalItem->setBackground(column, color);
            int brightness = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
            signalItem->setForeground(column, brightness > 128 ? Qt::black : Qt::white);
            signalItem->setText(column, color.name());
        } else {
            signalItem->setBackground(column, QBrush());
            signalItem->setForeground(column, QBrush());
            signalItem->setText(column, "");
        }
    }

    QString signalKey = signalItem->data(0, Qt::UserRole).toString();
    SignalHighlightConfig config = m_signalHighlights.value(signalKey);

    if (colorType == "name") {
        config.nameColor = color;
    } else if (colorType == "value") {
        config.valueColor = color;
    } else if (colorType == "line") {
        config.lineColor = color;
    } else if (colorType == "background") {
        config.backgroundColor = color;
    }
    m_signalHighlights[signalKey] = config;
}

void HighlightDialog::updateColorButtonIcons()
{
    auto createColorIcon = [](const QColor& color) -> QIcon {
        QPixmap pixmap(20, 20);
        pixmap.fill(color.isValid() ? color : Qt::gray);
        QPainter painter(&pixmap);
        painter.setPen(Qt::black);
        painter.drawRect(0, 0, 19, 19);

        return QIcon(pixmap);
    };

    m_nameColorButton->setIcon(createColorIcon(m_currentNameColor));
    m_valueColorButton->setIcon(createColorIcon(m_currentValueColor));
    m_lineColorButton->setIcon(createColorIcon(m_currentLineColor));
    m_backgroundColorButton->setIcon(createColorIcon(m_currentBackgroundColor));
}
