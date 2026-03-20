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

#include "wavewindow.h"
#include "ui_wavewindow.h"
#include <QMessageBox>
#include <QAction>
#include <QApplication>
#include "shortcutsmanager.h"
#include "shortcutdefinitions.h"
WaveWindow::WaveWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::WaveWindow)
    , signalExplorer(nullptr)  
    , display(nullptr)
{
    ui->setupUi(this); 
    this->setContextMenuPolicy(Qt::NoContextMenu);
    ui->mainToolBar->setProperty("waveWindowMainToolBar",true);
    ui->mainToolBar->setFixedHeight(34);
    ui->actionNext->setEnabled(false);
    ui->actionPre->setEnabled(false);
    ui->actionZoomIn->setEnabled(false);
    ui->actionZoomOut->setEnabled(false);
    ui->actionSignalAdd->setEnabled(false);
    
    m_searchTypeComboBox = new QComboBox(this);
    m_searchTypeComboBox->setMaximumWidth(200);
    m_searchTypeComboBox->addItem("Any Change", static_cast<int>(SearchType::AnyEdge));
    m_searchTypeComboBox->addItem("Rising Edge", static_cast<int>(SearchType::RisingEdge));
    m_searchTypeComboBox->addItem("Falling Edge", static_cast<int>(SearchType::FallingEdge));
    m_searchTypeComboBox->addItem("Value", static_cast<int>(SearchType::Value));
    m_searchTypeComboBox->addItem("Transition", static_cast<int>(SearchType::Transition));
    m_timeLineEdit = new QLineEdit(this);
    m_timeLineEdit->setMaximumWidth(m_timeLineEditMaxWidth);
    m_timeLineEdit->setPlaceholderText("Time");
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(
        QRegularExpression("^\\d*\\.?\\d*$"),  
        this
        );
    m_timeLineEdit->setValidator(validator);
    m_timeLineEdit->setEnabled(false);
    m_searchLabel = new QLabel("--", this);
    m_valueLineEdit = new QLineEdit(this);
    m_valueLineEdit->setMaximumWidth(m_valueLineEditMaxWidth);
    m_valueLineEdit->setPlaceholderText("Enter the search content");
    m_transitionLineEdit1 = new QLineEdit(this);
    m_transitionLineEdit2 = new QLineEdit(this);
    m_transitionLineEdit1->setMaximumWidth(m_transitionLineEdit1MaxWidth);
    m_transitionLineEdit2->setMaximumWidth(m_transitionLineEdit2MaxWidth);
    m_transitionLabel = new QLabel("->", this);
    m_transitionLineEdit1->setPlaceholderText("from value");
    m_transitionLineEdit2->setPlaceholderText("to value");
    ui->mainToolBar->insertWidget(ui->actionPre, m_timeLineEdit);
    ui->mainToolBar->insertWidget(ui->actionPre, m_searchLabel);
    QAction *searchTypeComboBoxAction = ui->mainToolBar->insertWidget(ui->actionPre, m_searchTypeComboBox);
    searchTypeComboBoxAction->setEnabled(false);
    ui->mainToolBar->insertSeparator(searchTypeComboBoxAction);
    ui->mainToolBar->insertWidget(ui->actionPre, m_valueLineEdit);
    ui->mainToolBar->insertWidget(ui->actionPre, m_transitionLineEdit1);
    ui->mainToolBar->insertWidget(ui->actionPre, m_transitionLabel);
    ui->mainToolBar->insertWidget(ui->actionPre, m_transitionLineEdit2);
    m_valueLineEdit->setMaximumWidth(0);
    m_transitionLineEdit1->setMaximumWidth(0);
    m_transitionLineEdit2->setMaximumWidth(0);
    m_transitionLabel->setMaximumWidth(0);
    
    m_currentSearchType = SearchType::AnyEdge;

    connect(ui->actionOpenWaveFile, &QAction::triggered, this, &WaveWindow::onWaveFileOpenClicked);
    connect(ui->actionSignalAdd, &QAction::triggered, this, &WaveWindow::onSignalAddClicked);
    connect(ui->actionZoomIn, &QAction::triggered, this, &WaveWindow::onZoomInClicked);
    connect(ui->actionZoomOut, &QAction::triggered, this, &WaveWindow::onZoomOutClicked);
    connect(ui->actionPre, &QAction::triggered, this, &WaveWindow::onPreClicked);
    connect(ui->actionNext, &QAction::triggered, this, &WaveWindow::onNextClicked);
    connect(m_searchTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),this, &WaveWindow::onSearchTypeChanged);
    connect(m_timeLineEdit, &QLineEdit::returnPressed, this, &WaveWindow::onTimeTextChanged);
    initializeShortcuts();
    
}
WaveWindow::~WaveWindow()
{
    delete ui;
    delete signalExplorer;
    delete display;
}
void WaveWindow::updateWaveformReader(QSharedPointer<IWaveformReader> reader){
    m_reader = reader;
}
void WaveWindow::saveGroup(){
    display->saveGroup();
}
void WaveWindow::loadGroup(){
    display->loadGroup();
}
void WaveWindow::updateSearchLabel() {
    try {
        auto& hierarchy = m_waveform->get_hierarchy();
        auto& metadata = hierarchy.get_metadata();
        QString text = QString::fromStdString("x " + metadata.timescale.to_string());
        m_searchLabel->setText(text);
    } catch (const std::exception& e) {
        m_searchLabel->setText("--");
        qWarning() << "Error getting timescale:" << e.what();
    }
}
void WaveWindow::updateWaveform(QSharedPointer<Waveform> waveform) {
    m_waveform = waveform;
    if (!signalExplorer) {
        
        signalExplorer = new SignalExplorer(m_waveform);
        connect(signalExplorer, &SignalExplorer::applyRequested, this, &WaveWindow::addSignalsToWaveform);
        connect(signalExplorer, &SignalExplorer::okRequested, this, &WaveWindow::addSignalsToWaveform);
    }
    signalExplorer->updateWaveform(waveform);
    display = nullptr;
    if (!display) {
        
        QWidget *centralWidget = new QWidget(this);
        
        QVBoxLayout *layout = new QVBoxLayout(centralWidget);
        
        display = new WaveformDisplay(waveform->getBeginTime(), waveform->getEndTime(), this->width(), this);
        layout->addWidget(display);
        layout->setContentsMargins(0, 0, 0, 0);
        centralWidget->setContentsMargins(0, 0, 0, 0);
        setCentralWidget(centralWidget);
        connect(display, &WaveformDisplay::signalDoubleClicked, this, &WaveWindow::onWaveformDisplaySignalDoubleClicked);
        connect(display, &WaveformDisplay::addSignalFromEditorToWaveWindow,  this, &WaveWindow::addSignalFromEditor);
        connect(display, &WaveformDisplay::timeValueChanged, this, &WaveWindow::setTimeLineEditValue);
        connect(display, &WaveformDisplay::addSignalFromSource, this, &WaveWindow::addSignalFromSource);
        connect(display, &WaveformDisplay::logicalOperationSignal, this, &WaveWindow::onOpenExpressionDialog);
        connect(display, &WaveformDisplay::getStatusByFullPath, this, &WaveWindow::onGetStatusByFullPath);
        connect(display, &WaveformDisplay::timeChangeForGetSignals, this, &WaveWindow::onTimeChangeForGetSignals);
        display->setTimeRange(waveform->get_time_table().front(),waveform->get_time_table().back());
        display->updateWavefrom(waveform,m_reader);
    } else {
        
        display->setTimeRange(waveform->getBeginTime(), waveform->getEndTime());
        display->autoScaleTime();
    }
    updateSearchLabel();
    ui->actionSignalAdd->setEnabled(true);
    
    onSignalsChanged();
}
void WaveWindow::onSignalsChanged()
{
    
    bool hasSignals = display->hasSignals();
    
    if (m_hasSignals == hasSignals) {
        return;
    }
    m_hasSignals = hasSignals;
    
    if (hasSignals) {
        
        ui->actionNext->setEnabled(true);
        ui->actionPre->setEnabled(true);
        ui->actionZoomIn->setEnabled(true);
        ui->actionZoomOut->setEnabled(true);
        m_searchTypeComboBox->setEnabled(true);
        m_transitionLineEdit1->setEnabled(true);
        m_transitionLineEdit2->setEnabled(true);
        
        QList<QAction*> actions = ui->mainToolBar->actions();
        for (QAction* action : actions) {
            if (action->associatedWidgets().contains(m_searchTypeComboBox)) {
                action->setEnabled(true);
                break;
            }
        }
        
        m_timeLineEdit->setEnabled(true);
    } else {
        
        ui->actionNext->setEnabled(false);
        ui->actionPre->setEnabled(false);
        ui->actionZoomIn->setEnabled(false);
        ui->actionZoomOut->setEnabled(false);
        m_searchTypeComboBox->setEnabled(false);
        m_transitionLineEdit1->setEnabled(false);
        m_transitionLineEdit2->setEnabled(false);
        
        QList<QAction*> actions = ui->mainToolBar->actions();
        for (QAction* action : actions) {
            if (action->associatedWidgets().contains(m_searchTypeComboBox)) {
                action->setEnabled(false);
                break;
            }
        }
        
        m_timeLineEdit->setEnabled(false);
    }
}
void WaveWindow::onWaveformDisplaySignalDoubleClicked(const QString &fullPath) {
    emit waveformDisplaySignalDoubleClicked(fullPath);
}
void WaveWindow::onGetStatusByFullPath(const QString varRefStr, const QString &fullPath) {
    emit getStatusByFullPath(varRefStr,fullPath);
}
void WaveWindow::onWaveFileOpenClicked(){
    emit waveFileOpen();
}
void WaveWindow::onTimeChangeForGetSignals(){
    emit timeChangeForGetSignals();
}
void WaveWindow::onSignalAddClicked()
{
    
    if(display) {
        QSet<VarRef> varRefSet = display->getDisplayVarRefs();
        signalExplorer->setAddedSignals(varRefSet);
    }
    QFont currentFont = this->font();
    qDebug() << "Setting SignalExplorer font on show:"
             << currentFont.family() << currentFont.pointSize() << "pt";
    
    QString styleSheet = QString(
                             "* {"
                             "    font-family: '%1';"
                             "    font-size: %2pt;"
                             "}"
                             ).arg(currentFont.family()).arg(currentFont.pointSize());
    signalExplorer->setStyleSheet(styleSheet);
    signalExplorer->refresh();
    signalExplorer->show();
    signalExplorer->raise();  
    signalExplorer->activateWindow(); 
}
void WaveWindow::onZoomInClicked()
{
    display->zoomIn();
}
void WaveWindow::onZoomOutClicked()
{
    display->zoomOut();
}
void WaveWindow::onPreClicked()
{
    display->setCursor(false);
}
void WaveWindow::onNextClicked()
{
    display->setCursor(true);
}
void WaveWindow::onLastUp()
{
    display->findSearchEdgeType(WaveformDisplay::FindSearchEdgeType::LastUp);
}
void WaveWindow::onLastDown()
{
    display->findSearchEdgeType(WaveformDisplay::FindSearchEdgeType::LastDown);
}
void WaveWindow::onNextUp()
{
    display->findSearchEdgeType(WaveformDisplay::FindSearchEdgeType::NextUp);
}
void WaveWindow::onNextDown()
{
    display->findSearchEdgeType(WaveformDisplay::FindSearchEdgeType::NextDown);
}
void WaveWindow::goToPreviousSearchResult()
{
    display->goToPreviousSearchResult();
}
void WaveWindow::goToNextSearchResult()
{
    display->goToNextSearchResult();
}
void WaveWindow::onHierarchicalNameClicked(){
    bool is_checked = ui->actionHierarchical_Name->isChecked();
    display->setHierarchicalDisplay(is_checked);
}
void WaveWindow:: addSignalsToWaveform(const QSet<VarRef>& varRefs) {
    QVector<VarRef> varlist;
    std::set<VarRef> sortedSet(varRefs.begin(), varRefs.end());
    for (const VarRef& var_ref : sortedSet) {
        const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
        QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id))+QString::fromStdString(var.multi_array);
        SignalRef signal_ref = var.handle;
        m_reader->load_signal_data(m_waveform,signal_ref);
        QSharedPointer<Signal> signal = m_waveform->getSignalShared(signal_ref);
        if (signal) {
            if(display->m_currentSignalPositionInGroup > -2) {
                
                display->addSignal(var_ref, var_name, signal, display->m_currentPositionGroupName, display->m_currentSignalPositionInGroup + 1);
            } else {
                
                display->addSignal(var_ref, var_name, signal,display->m_currentPositionGroupName,-1);
            }
            varlist.append(var_ref);
        } else {
            qWarning() << "Failed to get signal for:" << var_name;
        }
    }
    if(varlist.size() > 0) {
        signalExplorer->onSignalsAdded(varlist);
    }
    
    onSignalsChanged();
}
void WaveWindow::addSignalFromSource(const QString &singalName,const  QString &groupName = "",const int &targetIndex = 0) {
    if(singalName.isEmpty()) return ;
    VarRef var_ref = m_waveform->get_hierarchy().get_varRef_by_hierarchy(singalName.toStdString());
    if(var_ref > 0) {
        const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
        QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id)) + QString::fromStdString(var.multi_array);
        SignalRef signal_ref = var.handle;
        m_reader->load_signal_data(m_waveform,signal_ref);
        QSharedPointer<Signal> signal = m_waveform->getSignalShared(signal_ref);
        if (signal) {
            if(groupName != "" && display->m_currentSignalPositionInGroup <= -2) {
                display->addSignal(var_ref, var_name, signal, groupName, -1);
            } else if(display->m_currentSignalPositionInGroup > -2) {
                
                display->addSignal(var_ref, var_name, signal, display->m_currentPositionGroupName, display->m_currentSignalPositionInGroup + 1);
            } else {
                
                display->addSignal(var_ref, var_name, signal, groupName, -1);
            }
        } else {
            qWarning() << "Failed to get signal for:" << var_name;
        }
    } else {
        ScopeRef scope_ref = m_waveform->get_hierarchy().find_scope_by_fullpath(singalName.toStdString());
        if(scope_ref != INVALID_VAR_REF) {
            const Scope& scope = m_waveform->get_hierarchy().get_scope(scope_ref);
            VarRef var_ref = scope.first_var;
            while (var_ref != INVALID_VAR_REF) {
                const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
                QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id)) + QString::fromStdString(var.multi_array);
                SignalRef signal_ref = var.handle;
                m_reader->load_signal_data(m_waveform,signal_ref);
                QSharedPointer<Signal> signal = m_waveform->getSignalShared(signal_ref);
                if (signal) {
                    if(display->m_currentSignalPosition > -2) {
                        
                        display->addSignal(var_ref, var_name, signal, display->m_currentPositionGroupName, display->m_currentSignalPositionInGroup + 1);
                    } else {

                        display->addSignal(var_ref, var_name, signal, groupName, -1);
                    }
                }
                var_ref = var.next_var;
            }
        }
        else {
            VarRef var_ref = add_var_and_signal_by_signalName(singalName);
            if(var_ref != INVALID_VAR_REF) {
                const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
                QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id)) + QString::fromStdString(var.multi_array);
                SignalRef signal_ref = var.handle;
                m_reader->load_signal_data(m_waveform,signal_ref);
                QSharedPointer<Signal> signal = m_waveform->getSignalShared(signal_ref);
                if (signal) {

                    if(groupName != "" && display->m_currentSignalPositionInGroup <= -2) {

                        display->addSignal(var_ref, var_name, signal, groupName, -1);
                    } else if(display->m_currentSignalPositionInGroup > -2) {

                        display->addSignal(var_ref, var_name, signal, display->m_currentPositionGroupName, display->m_currentSignalPositionInGroup + 1);
                    } else {

                        display->addSignal(var_ref, var_name, signal, groupName, -1);
                    }
                } else {
                    qWarning() << "Failed to get signal for:" << var_name;
                }
            }
        }
    }
    
    onSignalsChanged();
}


void WaveWindow::addSignalFromEditor(const QString &singalName,const  QString &groupName = "",const int &targetIndex = 0) {
    if(singalName.isEmpty()) return ;
    VarRef var_ref = m_waveform->get_hierarchy().get_varRef_by_hierarchy(singalName.toStdString());
    if(var_ref > 0) {
        const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
        QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id)) + QString::fromStdString(var.multi_array);
        SignalRef signal_ref = var.handle;
        m_reader->load_signal_data(m_waveform,signal_ref);
        QSharedPointer<Signal> signal = m_waveform->getSignalShared(signal_ref);
        if (signal) {
            display->addSignal(var_ref, var_name, signal, groupName, targetIndex + 1);
        } else {
            qWarning() << "Failed to get signal for:" << var_name;
        }
    } else {
        ScopeRef scope_ref = m_waveform->get_hierarchy().find_scope_by_fullpath(singalName.toStdString());
        if(scope_ref != INVALID_VAR_REF) {
            const Scope& scope = m_waveform->get_hierarchy().get_scope(scope_ref);
            VarRef var_ref = scope.first_var;
            while (var_ref != INVALID_VAR_REF) {
                const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
                QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id)) + QString::fromStdString(var.multi_array);
                SignalRef signal_ref = var.handle;
                m_reader->load_signal_data(m_waveform,signal_ref);
                QSharedPointer<Signal> signal = m_waveform->getSignalShared(signal_ref);
                if (signal) {
                    display->addSignal(var_ref, var_name, signal, groupName, targetIndex + 1);
                }
                var_ref = var.next_var;
            }
        }
        else {
            VarRef var_ref = add_var_and_signal_by_signalName(singalName);
            if(var_ref != INVALID_VAR_REF) {
                const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
                QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id)) + QString::fromStdString(var.multi_array);
                SignalRef signal_ref = var.handle;
                m_reader->load_signal_data(m_waveform,signal_ref);
                QSharedPointer<Signal> signal = m_waveform->getSignalShared(signal_ref);
                if (signal) {
                    display->addSignal(var_ref, var_name, signal, groupName, targetIndex + 1);
                } else {
                    qWarning() << "Failed to get signal for:" << var_name;
                }
            }
        }
    }

    onSignalsChanged();
}


QString WaveWindow::getSignalValue(QString singalName) {
    VarRef var_ref = m_waveform->get_hierarchy().get_varRef_by_hierarchy(singalName.toStdString());
    if(var_ref != INVALID_VAR_REF) {
        const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
        QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id))+QString::fromStdString(var.multi_array);
        SignalRef signal_ref = var.handle;
        m_reader->load_signal_data(m_waveform,signal_ref);
        QSharedPointer<Signal> signal = m_waveform->getSignalShared(signal_ref);
        if (signal) {
            return display->getSignalValueByVarRef(var_ref,signal);
        }
        return "";
    } else{
        VarRef var_ref = add_var_and_signal_by_signalName(singalName);
        if(var_ref != INVALID_VAR_REF) {
            const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
            QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id)) + QString::fromStdString(var.multi_array);
            SignalRef signal_ref = var.handle;
            m_reader->load_signal_data(m_waveform,signal_ref);
            QSharedPointer<Signal> signal = m_waveform->getSignalShared(signal_ref);
            if (signal) {
                return display->getSignalValueByVarRef(var_ref,signal);
            } else {
                return "";
            }
        }
    }
    return "";
}
void WaveWindow::onTimeTextChanged(){
    QString selectTime = m_timeLineEdit->text();
    display->setSearchTime(selectTime);
}
void WaveWindow::onSearchTypeChanged(int index)
{
    m_valueLineEdit->setMaximumWidth(0);
    m_transitionLineEdit1->setMaximumWidth(0);
    m_transitionLineEdit2->setMaximumWidth(0);
    m_transitionLabel->setMaximumWidth(0);
    ui->actionPre->disconnect();
    ui->actionNext->disconnect();
    m_currentSearchType = static_cast<SearchType>(m_searchTypeComboBox->itemData(index).toInt());
    
    switch (m_currentSearchType) {
    case SearchType::AnyEdge:
        connect(ui->actionPre, &QAction::triggered, this, &WaveWindow::onPreClicked);
        connect(ui->actionNext, &QAction::triggered, this, &WaveWindow::onNextClicked);
        break;
    case SearchType::RisingEdge:
        connect(ui->actionPre, &QAction::triggered, this, &WaveWindow::onLastUp);
        connect(ui->actionNext, &QAction::triggered, this, &WaveWindow::onNextUp);
        break;
    case SearchType::FallingEdge:
        connect(ui->actionPre, &QAction::triggered, this, &WaveWindow::onLastDown);
        connect(ui->actionNext, &QAction::triggered, this, &WaveWindow::onNextDown);
        break;
    case SearchType::Value:
        m_valueLineEdit->setPlaceholderText("Enter the signal value");
        m_valueLineEdit->setMaximumWidth(m_valueLineEditMaxWidth);
        m_valueLineEdit->disconnect();
        connect(m_valueLineEdit, &QLineEdit::textChanged, this, &WaveWindow::onSearchValueChanged);
        connect(ui->actionPre, &QAction::triggered, this, &WaveWindow::goToPreviousSearchResult);
        connect(ui->actionNext, &QAction::triggered, this, &WaveWindow::goToNextSearchResult);
        break;
    case SearchType::Transition:
        m_transitionLineEdit1->setMaximumWidth(m_transitionLineEdit1MaxWidth);
        m_transitionLineEdit2->setMaximumWidth(m_transitionLineEdit2MaxWidth);
        m_transitionLabel->setMaximumWidth(m_transitionLabelMaxWidth);
        m_transitionLineEdit1->disconnect();
        m_transitionLineEdit2->disconnect();
        connect(m_transitionLineEdit1, &QLineEdit::textChanged, this, &WaveWindow::onTransitionLineChanged);
        connect(m_transitionLineEdit2, &QLineEdit::textChanged, this, &WaveWindow::onTransitionLineChanged);
        connect(ui->actionPre, &QAction::triggered, this, &WaveWindow::goToPreviousSearchResult);
        connect(ui->actionNext, &QAction::triggered, this, &WaveWindow::goToNextSearchResult);
        break;
    }
}
void WaveWindow::onTimeChanged()
{
    QString searchText = m_timeLineEdit->text().trimmed();
    if (searchText.isEmpty()) {
        return;
    }
    display->setSearchTime(searchText);
}
void WaveWindow::onSearchValueChanged()
{
    QString searchText = m_valueLineEdit->text().trimmed();
    if (searchText.isEmpty()) {
        return;
    }
    display->searchSignalValue(searchText);
}
void WaveWindow::onTransitionLineChanged()
{
    QString fromValue = m_transitionLineEdit1->text().trimmed();
    QString toValue = m_transitionLineEdit2->text().trimmed();
    if (fromValue.isEmpty() || toValue.isEmpty()) {
        return;
    }
    display->searchSignalTransition(fromValue, toValue);
}
void WaveWindow::setTimeLineEditValue(Time selectTime)
{
    QString timeStr = QString::number(selectTime);
    m_timeLineEdit->setText(timeStr);
}
void WaveWindow::onOpenExpressionDialog(const QString &fullPath)
{
    QString initialSignal = fullPath;
    expressionDialog = new LogicalOperationDialog(initialSignal, m_waveform, this);
    expressionDialog->setWindowTitle("Logical Expression Editor");
    connect(expressionDialog, &LogicalOperationDialog::addToWaveform,
            this, &WaveWindow::addSignalsToWaveform);
    connect(expressionDialog, &LogicalOperationDialog::loadSignalData,
            this, &WaveWindow::loadSignalData);
    expressionDialog->show();
    expressionDialog->raise();
    expressionDialog->activateWindow();
}
void WaveWindow::loadSignalData(SignalRef signal_ref) {
    m_reader->load_signal_data(m_waveform,signal_ref);
}
void WaveWindow::initializeShortcuts()
{
    ShortcutsManager* manager = ShortcutsManager::instance();
    manager->registerShortcut(Shortcuts::Waveform::SIGNALADD,
                              "Alt+A",
                              tr("Get Signals"),
                              tr("Waveform"));
    
    manager->bindToAction(Shortcuts::Waveform::SIGNALADD, ui->actionSignalAdd);
}
bool WaveWindow::hasWaveformLoaded(){
    if(display) return true;
    return false;
}
VarRef WaveWindow::add_var_and_signal_by_signalName(const QString& singalName){
    VarRef var_ref = m_waveform->get_hierarchy().add_var_by_fullPath(singalName);
    if(var_ref != INVALID_VAR_REF) {
        QVector<VarRef> varRefVec = m_waveform->get_hierarchy().get_varRefVec_by_hierarchy(singalName.toStdString());
        auto new_signal = QSharedPointer<Signal>::create();
        QVector<QSharedPointer<Signal>> signalVec;
        std::vector<Time> time_table = m_waveform->get_time_table();
        if (time_table.empty()) {
            return INVALID_VAR_REF;
        }
        for(VarRef var_ref : varRefVec){
            const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
            SignalRef signal_ref = var.handle;
            m_reader->load_signal_data(m_waveform,signal_ref);
            QSharedPointer<Signal> signal = m_waveform->getSignalShared(signal_ref);
            signalVec.append(signal);
        }
        std::string prev_value;

        for (size_t i = 0; i < time_table.size(); i++) {
            Time currentTime = time_table[i];
            QString value="";
            for(QSharedPointer<Signal> signal : signalVec){
                auto time_index_table = signal->get_time_indices();
                auto idx = m_waveform->binary_search_timeindex(time_index_table, i);
                value+=QString::fromStdString(signal->get_signal_values()[idx]);
            }

            const std::string& current_value = value.toStdString();
            if (i == 0 || current_value != prev_value) {
                new_signal->add_value_change(i, current_value);
                prev_value = current_value;
            }
        }
        const Var& var = m_waveform->get_hierarchy().get_var(var_ref);
        m_waveform->add_signal(var.handle, new_signal);
    }
    return var_ref;
}
void WaveWindow::addExpandSignals(QString signalName){
    display->addExpandSignals(signalName);
}
