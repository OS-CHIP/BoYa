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

#include "signalsModel.h"
#include <QRegularExpression>
#include <QColor>
SignalsModel::SignalsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}
void SignalsModel::setAddedSignals(const QSet<VarRef>& addedSignals)
{
    beginResetModel();
    m_addedSignals = addedSignals;
    endResetModel();
}
bool SignalsModel::isSignalAdded(VarRef varRef) const
{
    return m_addedSignals.contains(varRef);
}
void SignalsModel::setSignals(const QVector<VarRef> &allSignals, const QHash<VarRef, QString> &signalNames)
{
    beginResetModel();
    m_allSignals = allSignals;
    m_signalNames = signalNames;
    
    if (m_currentFilter.isEmpty()) {
        m_filteredSignals = allSignals;
    } else {
        m_filteredSignals.clear();
        for (VarRef varRef : allSignals) {
            if (matchesFilter(varRef)) {
                m_filteredSignals.append(varRef);
            }
        }
    }
    endResetModel();
}
QModelIndex SignalsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= m_filteredSignals.size() || column != 0)
        return QModelIndex();
    return createIndex(row, column, const_cast<VarRef*>(&m_filteredSignals[row]));
}
QModelIndex SignalsModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}
int SignalsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_filteredSignals.size();
}
int SignalsModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}
QVariant SignalsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_filteredSignals.size())
        return QVariant();
    VarRef varRef = m_filteredSignals[index.row()];
    switch (role) {
    case Qt::DisplayRole:
        return m_signalNames.value(varRef, QString());
    case Qt::CheckStateRole:
        return m_selectedSignals.contains(varRef) ? Qt::Checked : Qt::Unchecked;
    case Qt::ForegroundRole:
        
        if (m_addedSignals.contains(varRef)) {
            return QColor("#0066CC");  
        }
        return QVariant();
    default:
        return QVariant();
    }
}
bool SignalsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_filteredSignals.size())
        return false;
    if (role == Qt::CheckStateRole) {
        VarRef varRef = m_filteredSignals[index.row()];
        Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
        if (state == Qt::Checked) {
            m_selectedSignals.insert(varRef);
        } else {
            m_selectedSignals.remove(varRef);
        }
        emit dataChanged(index, index, {Qt::CheckStateRole});
        return true;
    }
    return false;
}
Qt::ItemFlags SignalsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}
void SignalsModel::setSelectedSignals(const QSet<VarRef> &selected)
{
    beginResetModel();
    m_selectedSignals = selected;
    endResetModel();
}
QSet<VarRef> SignalsModel::getSelectedSignals() const
{
    return m_selectedSignals;
}
void SignalsModel::updateSelection(VarRef varRef, bool selected)
{
    if (selected) {
        m_selectedSignals.insert(varRef);
    } else {
        m_selectedSignals.remove(varRef);
    }
    
    for (int i = 0; i < m_filteredSignals.size(); ++i) {
        if (m_filteredSignals[i] == varRef) {
            QModelIndex index = createIndex(i, 0);
            emit dataChanged(index, index, {Qt::CheckStateRole});
            break;
        }
    }
}
void SignalsModel::selectAll()
{
    beginResetModel();
    for (VarRef varRef : m_filteredSignals) {
        m_selectedSignals.insert(varRef);
    }
    endResetModel();
}
void SignalsModel::deselectAll()
{
    beginResetModel();
    for (VarRef varRef : m_filteredSignals) {
        m_selectedSignals.remove(varRef);
    }
    endResetModel();
}
void SignalsModel::setFilter(const QString &filter)
{
    beginResetModel();
    m_currentFilter = filter;
    if (filter.isEmpty()) {
        m_filteredSignals = m_allSignals;
    } else {
        m_filteredSignals.clear();
        for (VarRef varRef : m_allSignals) {
            if (matchesFilter(varRef)) {
                m_filteredSignals.append(varRef);
            }
        }
    }
    endResetModel();
}
bool SignalsModel::matchesFilter(VarRef varRef) const
{
    QString signalName = m_signalNames.value(varRef);
    return signalName.contains(m_currentFilter, Qt::CaseInsensitive);
}