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

#include "signalexplorer.h"
#include "waveform.h"
#include "signalsModel.h"
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSplitter>
#include <QPushButton>
#include <QListView>
#include <QComboBox>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QDebug>
#include <QTimer>
#include <QMouseEvent>
#include <QApplication>
#include "myutils.h"
SignalExplorer::SignalExplorer(QSharedPointer<Waveform> waveform, QWidget *parent)
    : QWidget(parent)
    , m_waveform(waveform)
    , m_currentItem(nullptr)
    , m_shiftPressed(false)
    , signalsModel(new SignalsModel(this))
    , signalProxyModel(new QSortFilterProxyModel(this))
{
    createUI();
    setMinimumSize(800, 600);
}
SignalExplorer::~SignalExplorer()
{
    qDebug() << "SignalExplorer destructor called";
}
void SignalExplorer::updateWaveform(QSharedPointer<Waveform> waveform)
{
    m_waveform = waveform;
    refresh();
}
void SignalExplorer::refresh()
{
    clearExplorer();
    populateTree();
}
void SignalExplorer::clearExplorer()
{
    if (explorerTreeWidget) {
        explorerTreeWidget->clear();
    }
    if (signalsModel) {
        signalsModel->setSignals(QVector<VarRef>(), QHash<VarRef, QString>());
    }
    m_selectedSignals.clear();
    m_signalNames.clear();
    m_currentScopeSignals.clear();
}
void SignalExplorer::createUI()
{
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setChildrenCollapsible(false);
    mainLayout->addWidget(mainSplitter, 1);
    
    QWidget *leftPanel = new QWidget(mainSplitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(5);
    scopeSearchEdit = new QLineEdit(leftPanel);
    scopeSearchEdit->setPlaceholderText("Search Scope...");
    connect(scopeSearchEdit, &QLineEdit::textChanged, this, &SignalExplorer::filterScopes);
    leftLayout->addWidget(scopeSearchEdit);
    explorerTreeWidget = new QTreeWidget(leftPanel);
    explorerTreeWidget->setHeaderHidden(true);
    leftLayout->addWidget(explorerTreeWidget, 1);
    
    QWidget *rightPanel = new QWidget(mainSplitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(5);
    signalSearchEdit = new QLineEdit(rightPanel);
    signalSearchEdit->setPlaceholderText("Search Signals...");
    connect(signalSearchEdit, &QLineEdit::textChanged, this, &SignalExplorer::filterSignals);
    rightLayout->addWidget(signalSearchEdit);
    QHBoxLayout *buttonRowLayout = new QHBoxLayout();
    buttonRowLayout->setContentsMargins(0, 0, 0, 0);
    QPushButton *selectAllButton = new QPushButton("Select All", rightPanel);
    QPushButton *deselectAllButton = new QPushButton("Deselect all", rightPanel);
    selectAllButton->setFixedSize(90, 25);
    deselectAllButton->setFixedSize(90, 25);

    connect(selectAllButton, &QPushButton::clicked, this, &SignalExplorer::selectAllVisibleSignals);
    connect(deselectAllButton, &QPushButton::clicked, this, &SignalExplorer::deselectAllVisibleSignals);
    buttonRowLayout->addWidget(selectAllButton);
    buttonRowLayout->addWidget(deselectAllButton);
    
    buttonRowLayout->addStretch();
    rightLayout->addLayout(buttonRowLayout);
    
    signalListView = new QListView(rightPanel);
    
    signalProxyModel->setSourceModel(signalsModel);
    signalProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    signalProxyModel->setFilterKeyColumn(0); 
    signalListView->setModel(signalProxyModel);
    signalListView->setSelectionMode(QAbstractItemView::NoSelection);
    signalListView->setSelectionBehavior(QAbstractItemView::SelectItems);
    signalListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    signalListView->setUniformItemSizes(true);

    signalListView->setStyleSheet("QListView::item { height: 30px; }");
    
    signalListView->setAttribute(Qt::WA_OpaquePaintEvent);
    signalListView->setAttribute(Qt::WA_NoSystemBackground);
    signalListView->setAutoScroll(false);
    signalListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    signalListView->setStyleSheet(R"(
        QListView::item {
            height: 30px;
            background: transparent;
        }
        QListView::item:hover {
            background: #f0f0f0;
        }
        QListView {
            outline: none;
        }
    )");
    rightLayout->addWidget(signalListView, 1);
    
    mainSplitter->setSizes(QList<int>() << 200 << 600);
    
    setupButtonPanel();
    mainLayout->addSpacing(10);
    
    connect(explorerTreeWidget, &QTreeWidget::itemExpanded, this, &SignalExplorer::loadChildren);
    connect(explorerTreeWidget, &QTreeWidget::itemClicked, this, &SignalExplorer::displaySignals);
    connect(signalListView, &QListView::clicked, this, &SignalExplorer::onSignalItemClicked);
    
    signalListView->viewport()->installEventFilter(this);
    
    populateTree();
}
bool SignalExplorer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == signalListView->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            m_shiftPressed = mouseEvent->modifiers() & Qt::ShiftModifier;
            QModelIndex index = signalListView->indexAt(mouseEvent->pos());
            if (index.isValid()) {
                onSignalItemClicked(index);
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            m_shiftPressed = false;
        }
        else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Shift) {
                m_shiftPressed = true;
            }
        }
        else if (event->type() == QEvent::KeyRelease) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Shift) {
                m_shiftPressed = false;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
void SignalExplorer::setupButtonPanel()
{
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addStretch();
    applyButton = new QPushButton("Apply", this);
    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);
    applyButton->setFixedSize(80, 30);
    okButton->setFixedSize(80, 30);
    cancelButton->setFixedSize(80, 30);
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    qobject_cast<QVBoxLayout*>(layout())->addLayout(buttonLayout);
    connect(applyButton, &QPushButton::clicked, this, [this]() {
        
        emit applyRequested(m_selectedSignals);
        m_selectedSignals.clear();
        if (m_currentItem) {
            displaySignals(m_currentItem);
        }
    });
    connect(okButton, &QPushButton::clicked, this, [this]() {
        
        emit okRequested(m_selectedSignals);
        close();
    });
    connect(cancelButton, &QPushButton::clicked, this, [this]() {
        emit cancelRequested();
        close();
    });
}
void SignalExplorer::populateTree()
{
    if (!explorerTreeWidget || !m_waveform) return;
    explorerTreeWidget->clear();
    ScopeRef root_scope = m_waveform->get_hierarchy().get_root_scope();
    if (root_scope == INVALID_SCOPE_REF) {
        return;
    }
    const Scope& root_scope_obj = m_waveform->get_hierarchy().get_scope(root_scope);
    ScopeRef child_scope = root_scope_obj.first_child;
    while (child_scope != INVALID_SCOPE_REF) {
        const Scope& child = m_waveform->get_hierarchy().get_scope(child_scope);
        QString name = QString::fromStdString(m_waveform->get_hierarchy().get_string(child.name_id));
        QTreeWidgetItem *topLevelItem = new QTreeWidgetItem(explorerTreeWidget);
        topLevelItem->setText(0, name);
        topLevelItem->setData(0, Qt::UserRole, QVariant::fromValue(child_scope));
        QString iconPath = MyUtils::getScopeIconPath(child.type);
        QIcon icon(iconPath);
        if (!icon.isNull()) {
            topLevelItem->setIcon(0, icon);
        }
        bool has_children = (child.first_child != INVALID_SCOPE_REF) || (child.first_var != INVALID_VAR_REF);
        topLevelItem->setChildIndicatorPolicy(has_children ?
                                                  QTreeWidgetItem::ShowIndicator : QTreeWidgetItem::DontShowIndicator);
        child_scope = child.next_sibling;
    }
}
void SignalExplorer::loadChildren(QTreeWidgetItem *parentItem)
{
    if (parentItem->childCount() > 0 || !m_waveform) return;
    QVariant data = parentItem->data(0, Qt::UserRole);
    if (!data.isValid()) {
        return;
    }
    ScopeRef scope_ref = data.toULongLong();
    const Scope& scope = m_waveform->get_hierarchy().get_scope(scope_ref);
    ScopeRef child_scope = scope.first_child;
    while (child_scope != INVALID_SCOPE_REF) {
        const Scope& child = m_waveform->get_hierarchy().get_scope(child_scope);
        QString name = QString::fromStdString(m_waveform->get_hierarchy().get_string(child.name_id));
        QTreeWidgetItem *childItem = new QTreeWidgetItem(parentItem);
        childItem->setText(0, name);
        childItem->setData(0, Qt::UserRole, QVariant::fromValue(child_scope));
        QString iconPath = MyUtils::getScopeIconPath(child.type);
        QIcon icon(iconPath);
        if (!icon.isNull()) {
            childItem->setIcon(0, icon);
        }
        bool has_children = (child.first_child != INVALID_SCOPE_REF) || (child.first_var != INVALID_VAR_REF);
        childItem->setChildIndicatorPolicy(has_children ?
                                               QTreeWidgetItem::ShowIndicator : QTreeWidgetItem::DontShowIndicator);
        child_scope = child.next_sibling;
    }
}
void SignalExplorer::displaySignals(QTreeWidgetItem *item)
{
    if (!signalListView || !m_waveform) return;
    
    m_signalNames.clear();
    m_currentScopeSignals.clear();
    m_currentItem = item;
    QVariant data = item->data(0, Qt::UserRole);
    if (!data.isValid()) {
        return;
    }
    ScopeRef scope_ref = data.toULongLong();
    updateSignalListModel(scope_ref);
}
void SignalExplorer::updateSignalListModel(ScopeRef scope_ref)
{
    const Scope& scope = m_waveform->get_hierarchy().get_scope(scope_ref);
    
    QVector<VarRef> signalRefs;
    QHash<VarRef, QString> signalNames;
    VarRef var_ref = scope.first_var;
    while (var_ref != INVALID_VAR_REF) {
        const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
        QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id));
        if (!var.multi_array.empty()) {
            var_name += " " + QString::fromStdString(var.multi_array);
        }
        
        signalNames[var_ref] = var_name;
        signalRefs.append(var_ref);
        var_ref = var.next_var;
    }
    m_signalNames = signalNames;
    m_currentScopeSignals = signalRefs;
    
    signalsModel->setSignals(signalRefs, signalNames);
    signalsModel->setSelectedSignals(m_selectedSignals);
    signalsModel->setAddedSignals(m_addedSignals);
    
    filterSignals(signalSearchEdit->text());
}
void SignalExplorer::filterScopes(const QString &text)
{
    if (!explorerTreeWidget) return;
    for (int i = 0; i < explorerTreeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = explorerTreeWidget->topLevelItem(i);
        filterScopesRecursive(item, text);
    }
}
void SignalExplorer::filterSignals(const QString &text)
{
    
    signalProxyModel->setFilterFixedString(text);
}
void SignalExplorer::filterScopesRecursive(QTreeWidgetItem *item, const QString &text)
{
    if (!item) return;
    bool match = item->text(0).contains(text, Qt::CaseInsensitive);
    item->setHidden(!match);
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem *child = item->child(i);
        filterScopesRecursive(child, text);
        if (!child->isHidden()) {
            item->setHidden(false);
        }
    }
}
void SignalExplorer::selectAllVisibleSignals()
{
    
    QSet<VarRef> newlySelected;
    for (int i = 0; i < signalProxyModel->rowCount(); ++i) {
        QModelIndex proxyIndex = signalProxyModel->index(i, 0);
        QModelIndex sourceIndex = signalProxyModel->mapToSource(proxyIndex);
        VarRef var_ref = m_currentScopeSignals[sourceIndex.row()];
        if (!m_selectedSignals.contains(var_ref)) {
            newlySelected.insert(var_ref);
        }
    }
    
    m_selectedSignals.unite(newlySelected);
    
    signalsModel->setSelectedSignals(m_selectedSignals);
}
void SignalExplorer::deselectAllVisibleSignals()
{
    
    QSet<VarRef> toRemove;
    for (int i = 0; i < signalProxyModel->rowCount(); ++i) {
        QModelIndex proxyIndex = signalProxyModel->index(i, 0);
        QModelIndex sourceIndex = signalProxyModel->mapToSource(proxyIndex);
        VarRef var_ref = m_currentScopeSignals[sourceIndex.row()];
        if (m_selectedSignals.contains(var_ref)) {
            toRemove.insert(var_ref);
        }
    }
    
    m_selectedSignals.subtract(toRemove);
    
    signalsModel->setSelectedSignals(m_selectedSignals);
}
void SignalExplorer::onSignalItemClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    bool shiftPressed = modifiers.testFlag(Qt::ShiftModifier);
    QModelIndex sourceIndex = signalProxyModel->mapToSource(index);
    VarRef currentVarRef = m_currentScopeSignals[sourceIndex.row()];
    
    bool isSelected = m_selectedSignals.contains(currentVarRef);
    bool newState = !isSelected; 
    if (shiftPressed && m_lastClickedIndex.isValid()) {
        
        int startRow = m_lastClickedIndex.row();
        int endRow = sourceIndex.row();
        if (startRow > endRow) {
            qSwap(startRow, endRow);
        }
        
        for (int row = startRow; row <= endRow; ++row) {
            VarRef varRef = m_currentScopeSignals[row];
            QModelIndex idx = signalsModel->index(row, 0);
            if (newState) {
                m_selectedSignals.insert(varRef);
                signalsModel->setData(idx, Qt::Checked, Qt::CheckStateRole);
            } else {
                m_selectedSignals.remove(varRef);
                signalsModel->setData(idx, Qt::Unchecked, Qt::CheckStateRole);
            }
        }
    } else {
        
        if (newState) {
            m_selectedSignals.insert(currentVarRef);
        } else {
            m_selectedSignals.remove(currentVarRef);
        }
        signalsModel->setData(sourceIndex, newState ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
    }
    
    m_lastClickedIndex = sourceIndex;
}
void SignalExplorer::setAddedSignals(const QSet<VarRef>& addedSignals)
{
    m_addedSignals = addedSignals;
    signalsModel->setAddedSignals(m_addedSignals);
}
void SignalExplorer::onSignalsAdded(const QVector<VarRef>& addedSignals)
{
    for (VarRef varRef : addedSignals) {
        m_addedSignals.insert(varRef);
    }
    signalsModel->setAddedSignals(m_addedSignals);
}
void SignalExplorer::onSignalsRemoved(const QVector<VarRef>& removedSignals)
{
    for (VarRef varRef : removedSignals) {
        m_addedSignals.remove(varRef);
    }
    signalsModel->setAddedSignals(m_addedSignals);
}