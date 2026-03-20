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

#ifndef WAVEWINDOW_H
#define WAVEWINDOW_H
#include <QMainWindow>
#include "waveform.h"
#include "waveform/signalexplorer.h"
#include "reader.h"
#include "waveform/waveform_display.h"
#include "waveform/logicaloperationdialog.h"
namespace Ui {
class WaveWindow;
}
class WaveWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit WaveWindow(QWidget *parent = nullptr);
    ~WaveWindow();
    enum class SearchType {
        AnyEdge,
        RisingEdge,
        FallingEdge,
        Value,
        Transition
    };
    struct SearchResult {
        QString signalName;
        Time time;
        QString value;
        QString description;
        SearchResult(const QString& name, Time t, const QString& val, const QString& desc = "")
            : signalName(name), time(t), value(val), description(desc) {}
    };
    void updateWaveformReader(QSharedPointer<IWaveformReader> m_reader);
    void updateWaveform(QSharedPointer<Waveform> waveform);
    void updateSearchLabel();
    void addSignalsToWaveform(const QSet<VarRef>& varRefs);
    
    void saveGroup();
    void loadGroup();
    QString getSignalValue(const QString singalName);
    bool hasWaveformLoaded();
    void addExpandSignals(QString signalName);
signals:
    void waveformDisplaySignalDoubleClicked(const QString &fullPath);
    void getStatusByFullPath(const QString varRefStr, const QString &fullPath);
    void waveFileOpen();
    void timeChangeForGetSignals();
private slots:
    void onWaveFileOpenClicked();
    void onSignalAddClicked();    
    void onZoomInClicked();   
    void onZoomOutClicked();   
    void onPreClicked();   
    void onNextClicked();   
    void onLastUp();   
    void onLastDown();   
    void onNextUp();   
    void onNextDown();   
    void goToPreviousSearchResult();   
    void goToNextSearchResult();   
    void onHierarchicalNameClicked(); 
    void onTimeTextChanged();
    void onTransitionLineChanged();
    void setTimeLineEditValue(Time selectTime);
    void onWaveformDisplaySignalDoubleClicked(const QString &fullPath);
    void onGetStatusByFullPath(const QString varRefStr,const QString &fullPath);
    void onOpenExpressionDialog(const QString &fullPath);
    void loadSignalData(SignalRef signal_ref);
    void onSignalsChanged();
    VarRef add_var_and_signal_by_signalName(const QString& singalName);
    void onTimeChangeForGetSignals();
public slots:
    void addSignalFromSource(const QString &singalName,const QString &groupName,const int &targetIndex);
    void addSignalFromEditor(const QString &singalName,const QString &groupName,const int &targetIndex);
    void onSearchTypeChanged(int index);
private:
    Ui::WaveWindow *ui;     
    QSharedPointer<IWaveformReader> m_reader;
    QSharedPointer<Waveform> m_waveform;
    SignalExplorer *signalExplorer = nullptr;
    WaveformDisplay *display = nullptr;
    QLineEdit* m_timeLineEdit;
    QLineEdit* m_valueLineEdit;
    QLineEdit* m_transitionLineEdit1;
    QLineEdit* m_transitionLineEdit2;
    QLabel * m_searchLabel;
    QLabel * m_transitionLabel;
    LogicalOperationDialog *expressionDialog = nullptr;
    int m_timeLineEditMaxWidth = 80;
    int m_valueLineEditMaxWidth = 300;
    int m_searchLabelMaxWidth = 40;
    int m_transitionLineEdit1MaxWidth = 140;
    int m_transitionLineEdit2MaxWidth = 140;
    int m_transitionLabelMaxWidth = 20;
    QString wavefromFileName;
    QComboBox* m_searchTypeComboBox;
    SearchType currentSearchType;
    bool m_hasSignals = false;
    
    void onSearchTextChanged();
    void initializeShortcuts();
    SearchType m_currentSearchType;
    void onTimeChanged();
    void onSearchValueChanged();
};
#endif 
