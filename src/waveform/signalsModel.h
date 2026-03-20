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

#ifndef SIGNALSMODEL_H
#define SIGNALSMODEL_H
#include <QAbstractItemModel>
#include <QSet>
#include <QVector>
#include <QHash>
#include "waveform.h"
class SignalsModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SignalsModel(QObject *parent = nullptr);
    
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    
    void setSignals(const QVector<VarRef> &allSignals, const QHash<VarRef, QString> &signalNames);
    void setSelectedSignals(const QSet<VarRef> &selected);
    QSet<VarRef> getSelectedSignals() const;
    void updateSelection(VarRef varRef, bool selected);
    
    void selectAll();
    void deselectAll();
    void setFilter(const QString &filter);
    void setAddedSignals(const QSet<VarRef>& addedSignals);
    bool isSignalAdded(VarRef varRef) const;
private:
    QVector<VarRef> m_allSignals;
    QVector<VarRef> m_filteredSignals;
    QHash<VarRef, QString> m_signalNames;
    QSet<VarRef> m_selectedSignals;
    QString m_currentFilter;
    QSet<VarRef> m_addedSignals;
    bool matchesFilter(VarRef varRef) const;
};
#endif 