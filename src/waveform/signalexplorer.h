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

#ifndef SIGNALEXPLORER_H
#define SIGNALEXPLORER_H
#include <QWidget>
#include <QTreeWidget>
#include <QSet>
#include <QLineEdit>
#include <QPointer>
#include <QSharedPointer>
#include <QPushButton>
#include <QSplitter>
#include <QListView>
#include <QComboBox>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QEvent>
#include <QScrollBar>
#include "waveform.h"
#include "signalsModel.h"
class SignalExplorer : public QWidget
{
    Q_OBJECT
public:
    explicit SignalExplorer(QSharedPointer<Waveform> waveform, QWidget *parent = nullptr);
    ~SignalExplorer();
    void refresh();
    QSet<VarRef> selectedSignals() const { return m_selectedSignals; }
    void updateWaveform(QSharedPointer<Waveform> waveform);
    void setAddedSignals(const QSet<VarRef>& addedSignals);
    void onSignalsAdded(const QVector<VarRef>& addedSignals);
    
    void onSignalsRemoved(const QVector<VarRef>& removedSignals);
signals:

    void applyRequested(const QSet<VarRef>& varRefs);
    void okRequested(const QSet<VarRef>& varRefs);
    void cancelRequested();
public slots:
    
private slots:
    void filterScopes(const QString &text);
    void filterSignals(const QString &text);
    void loadChildren(QTreeWidgetItem *parentItem);
    void displaySignals(QTreeWidgetItem *item);
    void selectAllVisibleSignals();
    void deselectAllVisibleSignals();
    void onSignalItemClicked(const QModelIndex &index);
private:
    QSharedPointer<Waveform> m_waveform;
    
    QPointer<QSplitter> mainSplitter;
    QPointer<QTreeWidget> explorerTreeWidget;
    QPointer<QLineEdit> scopeSearchEdit;
    QPointer<QLineEdit> signalSearchEdit;
    QPointer<QListView> signalListView;
    QPointer<QPushButton> applyButton;
    QPointer<QPushButton> okButton;
    QPointer<QPushButton> cancelButton;
    QTreeWidgetItem *m_currentItem;
    QStringList waveformGroupNames;
    QComboBox *targetAddGroupComboBox;
    
    SignalsModel *signalsModel;
    QSortFilterProxyModel *signalProxyModel;
    
    QSet<VarRef> m_selectedSignals;
    QHash<VarRef, QString> m_signalNames;  
    QVector<VarRef> m_currentScopeSignals; 
    void createUI();
    void populateTree();
    void filterScopesRecursive(QTreeWidgetItem *item, const QString &text);
    void clearExplorer();
    void setupButtonPanel();
    void updateSignalListModel(ScopeRef scope_ref);
    bool eventFilter(QObject *obj, QEvent *event) override;
    QSet<VarRef> m_addedSignals;
    QModelIndex m_lastClickedIndex;  
    bool m_shiftPressed;  
};
#endif 