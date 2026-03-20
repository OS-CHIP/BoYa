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

#ifndef SIGNALLIST_H
#define SIGNALLIST_H

#include <QDockWidget>
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QVector>
#include <QShortcut>
#include <QSet>
#include <QMap>

class QTableWidgetItem;

class SignalListDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit SignalListDock(QWidget *parent = nullptr);
    ~SignalListDock();

    void loadSampleData();
    void loadSignalsFromWaveform(const QVector<QStringList> &signalData);
    void updateSignalValueFromWaveform(const QVector<QStringList> &signalData);
    void clearAllSignals();
    void addSignal(const QString &name, const QString &value, const QString &type);
    void addSignal(const QString &name, const QString &value, const QString &type,
                   const QString &len, const QString &line, const QString &column);

    void setPageSize(int size);
    QString getSelectedSignal() const;
    void showPagination(bool show);
    QVector<QStringList> getSignalLists() const;
    QVector<QStringList> getSelectedSignals() const;
    void expandSignal(int signalIndex, const QString& groupName = "");
    void collapseSignal(int signalIndex);
    void toggleSignalExpansion(int signalIndex);
    void setScope(QString scope);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void signalClicked(const QStringList &signalData);
    void signalDoubleClicked(const QStringList &signalData);
    void multipleSignalsSelected(const QVector<QStringList> &signallists);
    void addSignalsToWaveform(const QVector<QStringList> &signalList);
    void requestSignalExpansion(const QString& signalName, int expansionId);
    void requestSignalCollapse(const QString& signalName, int expansionId);
    void addAndShowExpandSignals(const QString& signalName);

private slots:
    void onSearchTextChanged(const QString &text);
    void onSignalItemClicked(QTableWidgetItem *item);
    void onSignalCellDoubleClicked(int row, int column);
    void onPrevPageClicked();
    void onNextPageClicked();
    void onCustomContextMenuRequested(const QPoint &pos);
    void onAddToWaveform();
    void onAddToWaveformShortcut();
    void onSignalCellClicked(int row, int column);

private:
    struct SignalInfo {
        QString name;
        QString value;
        QString type;
        int index;
        int len;
        int source_line;
        int source_column;
        int parent_index;
        int expansion_id;
        int parent_expansion_id;
        bool is_expansion;
        bool expanded;
        int indent_level;
        bool visible;

        int expanded_dimension;
        QString remaining_dims;

        SignalInfo(const QString& n = "", const QString& v = "", const QString& t = "", int i = 0,
                   int l = 0, int sl = 0, int sc = 0,
                   int pi = -1, int eid = 0, int peid = 0,
                   bool exp = false, bool expd = false,
                   int indent = 0, bool vis = true,
                   int expDim = -1, const QString& remDims = "")
            : name(n), value(v), type(t), index(i), len(l),
            source_line(sl), source_column(sc), parent_index(pi),
            expansion_id(eid), parent_expansion_id(peid),
            is_expansion(exp), expanded(expd), indent_level(indent),
            visible(vis), expanded_dimension(expDim), remaining_dims(remDims)
        {}
        bool operator==(const SignalInfo& other) const {
            return index == other.index &&
                   name == other.name &&
                   parent_index == other.parent_index;
        }
    };

    QWidget *m_mainWidget = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QTableWidget *m_tableWidget = nullptr;
    QLabel *m_pageInfoLabel = nullptr;
    QPushButton *m_prevPageButton = nullptr;
    QPushButton *m_nextPageButton = nullptr;
    QShortcut *m_shortcutAddToWaveform = nullptr;

    QList<SignalInfo> m_allSignals;
    QList<SignalInfo> m_filteredSignals;
    QSet<QString> m_selectedSignalNames;

    int m_currentPage = 0;
    int m_pageSize = 20;
    int m_totalPages = 1;
    bool m_showPagination = true;
    int m_lastSelectedRow = -1;

    QPoint m_dragStartPosition;
    bool m_isDragging = false;
    static const int DRAG_THRESHOLD = 5;
    int m_expansionIdCounter = 1;
    QMap<int, QSet<int>> m_expansionMap;

    QString m_scope;


    void initUI();
    void initConnections();
    void updateTable();
    void saveSelectionState();
    void updatePageInfo();
    void applySorting();
    void clearTable();
    void adjustColumnWidths();

    void startDrag();
    QString getDragData() const;
    QString getSignalNamesForText() const;
    QPixmap createDragIcon(int signalCount) const;

    QIcon getExpandCollapseIcon(bool expanded) const;
    void rebuildFilteredSignals();
    bool isMultiArraySignal(const QString& signalName) const;
    QString getBaseSignalName(const QString& signalName) const;
    QString getMultiArrayString(const QString& signalName) const;

    void expandCurrentDimension(int signalIndex);
    int findSignalByName(const QString& signalName) const;
    void addChildSignals(int parentIndex, const QString& childName,
                         const QString& remainingDims, int indentLevel);

    void handleShiftClick(int clickedRow);
    void handleCtrlClick(int clickedRow);

    QVector<QStringList> getDragSignals() const;
};
#endif
