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

#include "waveform_display.h"
#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QToolTip>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QSplitter>
#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QDrag>
#include <QMimeData>
#include <cmath>
#include <algorithm>
#include <QDebug>
#include <QFormLayout>
#include <QComboBox>
#include <QColorDialog>
#include <QShortcut>
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDir>
#include <QGroupBox>
#include "theme/ThemeManager.h"
#include "shortcutsmanager.h"
#include "shortcutdefinitions.h"
WaveformDisplay::WaveformDisplay(double begin_time, double end_time, double width, QWidget *parent)
    : QWidget(parent),
      m_markers(),
      m_nextMarkerId(1),
      m_minTime(begin_time),
      m_maxTime(end_time),
      m_timeScale(1.0),
      m_timeOffset(0),
      m_minZoomRange(1.0),
      m_signalSpacing(6),
      m_signalHeight(20),
      m_timeRulerHeight(22),
      m_signalNameWidth(0.1 * width),
      m_valueWidth(0.08 * width),
      m_groupHeaderHeight(10),
      m_gridVisible(false),
      m_timeRulerVisible(true),
      m_signalNamesVisible(true),
      m_hoverTime(0),
      m_zoomMode(ZoomNormal),
      m_verticalOffset(0),
      m_horizontalOffset(0),
      m_contentHeight(0),
      m_nameAreaWidth(0.1 * width),
      m_valueAreaWidth(0.08 * width),
      m_waveAreaWidth(0.82 * width),
      m_resizingAreas(false),
      m_syncingScroll(false),
      m_selectTime(0),
      m_signalNameLongestWidth(0),
      m_signalValueLongestWidth(0),
      m_draggingMarkerId(-1),
      m_markerBeforeDragTime(0),
      m_translatorManager(new TranslatorManager()),
      m_goToMarkerMenu(nullptr),
      m_lineStyleCombo(nullptr),
      m_colorButton(nullptr),
      m_markerManagerDialog(nullptr),
      m_hoveredMarkerId(-1),
      m_detailAction(nullptr),
      m_contextMenu(nullptr),
      m_addGroupAction(nullptr),
      m_renameGroupAction(nullptr),
      m_removeGroupAction(nullptr),
      m_collapseGroupAction(nullptr),
      m_expandGroupAction(nullptr),
      m_contextMenuGroup(""),
      m_removeSignalAction(nullptr),
      m_unpinnedSignalAction(nullptr),
      m_copyFullPathAction(nullptr),
      m_copyValueAction(nullptr),
      m_highlightAction(nullptr),
      m_pinTopAction(nullptr),
      m_radixMenu(nullptr),
      m_logicalOperationAction(nullptr),
      m_saveGroupAction(nullptr),
      m_loadGroupAction(nullptr),
      m_zoomInAction(nullptr),
      m_zoomOutAction(nullptr),
      m_zoomAllAction(nullptr),
      m_toggleGridAction(nullptr),
      m_toggleRulerAction(nullptr),
      m_addMarkerAction(nullptr),
      m_highlightDialog(nullptr),
      m_selectionMode(SingleSelection)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    setAcceptDrops(true);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &WaveformDisplay::handleThemeChange);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_horizontalSplitter = new QSplitter(Qt::Horizontal, this);
    m_horizontalSplitter->setHandleWidth(0);
    m_horizontalSplitter->setChildrenCollapsible(false);

    m_nameArea = new QScrollArea(this);
    m_valueArea = new QScrollArea(this);
    m_waveArea = new QScrollArea(this);
    m_nameArea->setStyleSheet("QScrollArea { background-color: transparent; border: 0px; } QScrollBar:vertical {padding-top: 22px;} QScrollBar::sub-line:vertical {position:relative;top:22px;height:10px;width:10px;subcontrol-position: top;subcontrol-origin:margin;}");
    m_valueArea->setStyleSheet("QScrollArea { background-color: transparent; border: 0px; }");
    m_waveArea->setStyleSheet("QScrollArea { background-color: transparent; border: 0px; }");

    m_nameArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_nameArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_valueArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_valueArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_waveArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_waveArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_firstShow = true;

    QWidget *nameWidget = new QWidget;
    nameWidget->setProperty("no-style", true);

    m_nameArea->setWidget(nameWidget);
    QWidget *valueWidget = new QWidget;
    valueWidget->setProperty("no-style", true);

    QVBoxLayout *valueLayout = new QVBoxLayout(valueWidget);
    valueLayout->setContentsMargins(0, 0, 0, 0);
    valueWidget->setStyleSheet("margin: 5px 10px 15px 20px;");
    m_valueArea->setWidget(valueWidget);
    QWidget *waveWidget = new QWidget;
    waveWidget->setProperty("no-style", true);
    QVBoxLayout *waveLayout = new QVBoxLayout(waveWidget);
    waveLayout->setContentsMargins(0, 0, 0, 0);

    m_waveArea->setWidget(waveWidget);

    m_searchLineEdit = new QLineEdit(this);
    m_searchLineEdit->setFixedSize(m_nameAreaWidth - 2, m_timeRulerHeight);

    m_searchLineEdit->setPlaceholderText("search..");

    QAction *searchAction = new QAction(m_searchLineEdit);
    searchAction->setIcon(QIcon(":/icons/icons/search.png"));
    m_searchLineEdit->addAction(searchAction, QLineEdit::LeadingPosition);
    connect(m_searchLineEdit, &QLineEdit::textChanged, this, &WaveformDisplay::handleSearch);
    connect(searchAction, &QAction::triggered, this, &WaveformDisplay::handleSearch);

    m_horizontalSplitter->setProperty("no-style", true);
    m_horizontalSplitter->addWidget(m_nameArea);
    m_horizontalSplitter->addWidget(m_valueArea);
    m_horizontalSplitter->addWidget(m_waveArea);

    connect(m_horizontalSplitter, &QSplitter::splitterMoved, this, &WaveformDisplay::handleSplitterMoved);

    connect(m_nameArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &WaveformDisplay::syncVerticalScroll);
    connect(m_valueArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &WaveformDisplay::syncVerticalScroll);
    connect(m_waveArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &WaveformDisplay::syncVerticalScroll);
    connect(m_waveArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, &WaveformDisplay::syncHorizontalScroll);

    m_waveArea->viewport()->installEventFilter(this);
    m_waveArea->verticalScrollBar()->installEventFilter(this);

    connect(m_nameArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this](int value)
    { update(); });
    connect(m_valueArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this](int value)
    { update(); });
    mainLayout->addWidget(m_horizontalSplitter);

    addGroup("G1", QColor(200, 230, 255));

    createMenuActions();

    createContextMenu();

    updateScrollBars();
    handleThemeChange(ThemeManager::instance().currentTheme());
    m_deleteShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(m_deleteShortcut, &QShortcut::activated, this, &WaveformDisplay::onDeleteKeyPressed);

    if (m_addGroupAction)
    {
        m_addGroupAction->setVisible(true);

        if (!this->actions().contains(m_addGroupAction))
        {
            this->addAction(m_addGroupAction);
        }
    }
    if (m_hierarchicalNameAction)
    {
        m_hierarchicalNameAction->setVisible(true);

        if (!this->actions().contains(m_hierarchicalNameAction))
        {
            this->addAction(m_hierarchicalNameAction);
        }
    }
    initializeShortcuts();
}

void WaveformDisplay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
    {
        m_ctrlPressed = true;
        updateSelectionMode();
    }
    else if (event->key() == Qt::Key_Shift)
    {
        m_shiftPressed = true;
        updateSelectionMode();
    }
    QWidget::keyPressEvent(event);
}

void WaveformDisplay::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
    {
        m_ctrlPressed = false;
        updateSelectionMode();
    }
    else if (event->key() == Qt::Key_Shift)
    {
        m_shiftPressed = false;
        updateSelectionMode();
    }
    QWidget::keyReleaseEvent(event);
}

void WaveformDisplay::updateSelectionMode()
{
    if (m_ctrlPressed)
    {
        m_selectionMode = CtrlMultiSelection;
    }
    else if (m_shiftPressed)
    {
        m_selectionMode = ShiftRangeSelection;
    }
    else
    {
        m_selectionMode = SingleSelection;
    }
}

WaveformDisplay::~WaveformDisplay()
{
    if (m_markerManagerDialog)
    {
        delete m_markerManagerDialog;
        m_markerManagerDialog = nullptr;
    }
    if (m_contextMenu)
    {
        delete m_contextMenu;
    }
}
void WaveformDisplay::onDeleteKeyPressed()
{
    if (!m_selectedSignalKeys.isEmpty())
    {
        QList<QString> signalsToRemove = m_selectedSignalKeys;
        for (const QString &signalKey : signalsToRemove)
        {
            if (m_signals.contains(signalKey))
            {
                removeSignal(signalKey);
            }
        }
        m_selectedSignalKeys.clear();
        m_lastSelectedSignalKey.clear();
        update();
    }
}
void WaveformDisplay::createMenuActions()
{

    if (!m_addGroupAction)
    {
        m_addGroupAction = new QAction(tr("Add Group"), this);
        connect(m_addGroupAction, &QAction::triggered, this, [this]()
        {
            bool ok;
            QString groupName = QInputDialog::getText(this, tr("Add Group"),
                                                      tr("Group name:"), QLineEdit::Normal, "", &ok);
            if (ok && !groupName.isEmpty()) {
                addGroup(groupName, QColor(200, 230, 255));
            } });
    }
    if (!m_renameGroupAction)
    {
        m_renameGroupAction = new QAction(tr("Rename Group"), this);
        connect(m_renameGroupAction, &QAction::triggered, this, [this]()
        {
            
            if (m_contextMenuGroup.isEmpty()) {
                QMessageBox::warning(this, tr("Rename Group"),
                                     tr("Please select a group first."));
                return;
            }
            bool ok;
            QString newName = QInputDialog::getText(this, tr("Rename Group"),
                                                    tr("New name:"), QLineEdit::Normal,
                                                    m_contextMenuGroup, &ok);
            if (ok && !newName.isEmpty()) {
                renameGroup(m_contextMenuGroup, newName);
            } });
    }
    if (!m_removeGroupAction)
    {
        m_removeGroupAction = new QAction(tr("Remove Group"), this);
        connect(m_removeGroupAction, &QAction::triggered, this, [this]()
        {
            if (m_contextMenuGroup != "") {
                removeGroup(m_contextMenuGroup);
            } });
    }
    if (!m_collapseGroupAction)
    {
        m_collapseGroupAction = new QAction(tr("Collapse Group"), this);
        connect(m_collapseGroupAction, &QAction::triggered, this, [this]()
        {
            if (m_contextMenuGroup != "") {
                setGroupCollapsed(m_contextMenuGroup, !isGroupCollapsed(m_contextMenuGroup));
            } });
    }
    if (!m_expandGroupAction)
    {
        m_expandGroupAction = new QAction(tr("Expand All Groups"), this);
        connect(m_expandGroupAction, &QAction::triggered, this, [this]()
        {
            for (const QString &groupName : m_groupOrder) {
                setGroupCollapsed(groupName, false);
            } });
    }

    if (!m_removeSignalAction)
    {
        m_removeSignalAction = new QAction(tr("Remove Signal"), this);
        connect(m_removeSignalAction, &QAction::triggered, this, [this]()
        {
            if (!m_selectedSignalKeys.isEmpty()) {
                QList<QString> signalsToRemove = m_selectedSignalKeys;
                for (const QString &signalKey : signalsToRemove) {
                    removeSignal(signalKey);
                }
                m_selectedSignalKeys.clear();
                m_lastSelectedSignalKey.clear();
            } });
    }
    if (!m_copyFullPathAction)
    {
        m_copyFullPathAction = new QAction(tr("Copy Full Path(s)"), this);
        connect(m_copyFullPathAction, &QAction::triggered, this, [this]()
        {
            if (!m_selectedSignalKeys.isEmpty()) {
                QStringList fullPaths;
                for (const QString &signalKey : m_selectedSignalKeys) {
                    if (m_signals.contains(signalKey)) {
                        QString fullPath = join_path_with_dot(m_signals[signalKey].scopes) + "." + m_signals[signalKey].signal_name;
                        fullPaths.append(fullPath);
                    }
                }
                QApplication::clipboard()->setText(fullPaths.join("\n"));
            } });
    }
    if (!m_logicalOperationAction)
    {
        m_logicalOperationAction = new QAction(tr("Logical Operation"), this);
        connect(m_logicalOperationAction, &QAction::triggered, this, [this]()
        {
            if (!m_selectedSignalKeys.isEmpty()) {
                QStringList fullPaths;
                for (const QString &signalKey : m_selectedSignalKeys) {
                    if (m_signals.contains(signalKey)) {
                        QString fullPath = join_path_with_dot(m_signals[signalKey].scopes) + "." + m_signals[signalKey].signal_name;
                        fullPaths.append(fullPath);
                    }
                }
                QStringList quotedPaths;
                for (const QString &path : fullPaths) {
                    quotedPaths.append("\"" + path + "\"");
                }

                emit logicalOperationSignal(quotedPaths.join("\n"));
            } });
    }
    if (!m_copyValueAction)
    {
        m_copyValueAction = new QAction(tr("Copy Value"), this);
        connect(m_copyValueAction, &QAction::triggered, this, [this]()
        {
            if (!m_selectedSignalKeys.isEmpty()) {
                QStringList values;
                for (const QString &signalKey : m_selectedSignalKeys) {
                    if (m_signals.contains(signalKey)) {
                        QString value = getSignalValueAtTime(m_signals[signalKey], m_selectTime);
                        values.append(value);
                    }
                }
                QApplication::clipboard()->setText(values.join("\n"));
            } });
    }
    if (!m_highlightAction)
    {
        m_highlightAction = new QAction(tr("Highlight Signal"), this);
        connect(m_highlightAction, &QAction::triggered, this, [this]()
        {
            if (!m_selectedSignalKeys.isEmpty()) {
                onHighlightButtonClicked();
            } });
    }
    if (!m_pinTopAction)
    {
        m_pinTopAction = new QAction(tr("Pin to Top"), this);
        connect(m_pinTopAction, &QAction::triggered, this, [this]()
        {
            if (!m_selectedSignalKeys.isEmpty()) {
                for (const QString &signalKey : m_selectedSignalKeys) {
                    if (m_signals.contains(signalKey)) {
                        onPinTopButtonClicked(m_signals[signalKey]);
                    }
                }
            } });
    }
    if (!m_unpinnedSignalAction)
    {
        m_unpinnedSignalAction = new QAction(tr("Unpin from Top"), this);
        connect(m_unpinnedSignalAction, &QAction::triggered, this, [this]()
        {
            if (!m_selectedSignalKeys.isEmpty()) {
                for (const QString &signalKey : m_selectedSignalKeys) {
                    if (m_signals.contains(signalKey)) {
                        m_signals[signalKey].visible = true;
                        m_pinTopSignals.removeAll(signalKey);
                    }
                }
                update();
            } });
    }

    if (!m_radixMenu)
    {
        m_radixMenu = new QMenu(tr("Radix"), this);
    }

    if (!m_hierarchicalNameAction)
    {
        m_hierarchicalNameAction = new QAction(tr("Hierarchical Name"), this);
        m_hierarchicalNameAction->setCheckable(true);
        m_hierarchicalNameAction->setChecked(m_hierarchicalDisplay);
        connect(m_hierarchicalNameAction, &QAction::triggered, this, [this]()
        {
            setHierarchicalDisplay(!m_hierarchicalDisplay);
            m_hierarchicalNameAction->setChecked(m_hierarchicalDisplay); });
    }

    if (!m_detailAction)
    {
        m_detailAction = new QAction(tr("Detail"), this);
        connect(m_detailAction, &QAction::triggered, this, &WaveformDisplay::onDetailActionTriggered);
    }

    if (!m_saveGroupAction)
    {
        m_saveGroupAction = new QAction(tr("Save Group Configuration"), this);
        connect(m_saveGroupAction, &QAction::triggered, this, &WaveformDisplay::saveGroup);
    }
    if (!m_loadGroupAction)
    {
        m_loadGroupAction = new QAction(tr("Load Group Configuration"), this);
        connect(m_loadGroupAction, &QAction::triggered, this, &WaveformDisplay::loadGroup);
    }

    if (!m_zoomInAction)
    {
        m_zoomInAction = new QAction(tr("Zoom In"), this);
        connect(m_zoomInAction, &QAction::triggered, this, &WaveformDisplay::zoomIn);
    }
    if (!m_zoomOutAction)
    {
        m_zoomOutAction = new QAction(tr("Zoom Out"), this);
        connect(m_zoomOutAction, &QAction::triggered, this, &WaveformDisplay::zoomOut);
    }
    if (!m_zoomAllAction)
    {
        m_zoomAllAction = new QAction(tr("Zoom All"), this);
        connect(m_zoomAllAction, &QAction::triggered, this, &WaveformDisplay::zoomAll);
    }

    if (!m_addMarkerAction)
    {

        m_addMarkerAction = new QAction(tr("Add Marker at Cursor"), this);
        m_manageMarkersAction = new QAction(tr("Manage Markers..."), this);
        m_clearMarkersAction = new QAction(tr("Clear All Markers"), this);
    }
}
void WaveformDisplay::saveGroup()
{

    QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Save Group Configuration"),
                QDir::homePath(),
                tr("Group Configuration Files (*.grp);;All Files (*)"));
    if (fileName.isEmpty())
    {
        return;
    }

    if (!fileName.endsWith(".grp", Qt::CaseInsensitive))
    {
        fileName += ".grp";
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, tr("Save Failed"),
                             tr("Cannot save file: %1").arg(file.errorString()));
        return;
    }
    QJsonObject rootObject;

    QJsonArray groupsArray;
    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
        {
            continue;
        }
        const SignalGroup &group = m_groups[groupName];
        QJsonObject groupObject;

        groupObject["name"] = groupName;
        groupObject["color"] = group.color.name();
        groupObject["collapsed"] = group.collapsed;

        QJsonArray signalsArray;
        for (const QString &signalName : group.v_signals)
        {
            if (!m_signals.contains(signalName))
            {
                continue;
            }
            const DisplaySignal &displaySignal = m_signals[signalName];
            QJsonObject signalObject;

            signalObject["var_ref"] = static_cast<qint64>(displaySignal.var_ref);
            signalObject["signal_name"] = displaySignal.signal_name;
            signalObject["display_name"] = displaySignal.name;

            QJsonArray scopesArray;
            for (const QString &scope : displaySignal.scopes)
            {
                scopesArray.append(scope);
            }
            signalObject["scopes"] = scopesArray;

            if (displaySignal.nameColor.isValid())
            {
                signalObject["nameColor"] = displaySignal.nameColor.name();
            }

            if (displaySignal.valueColor.isValid())
            {
                signalObject["valueColor"] = displaySignal.valueColor.name();
            }

            if (displaySignal.lineColor.isValid())
            {
                signalObject["lineColor"] = displaySignal.lineColor.name();
            }

            if (displaySignal.backgroundColor.isValid())
            {
                signalObject["backgroundColor"] = displaySignal.backgroundColor.name();
            }

            if (displaySignal.translator)
            {
                signalObject["translator"] = displaySignal.translator->name();
            }
            signalObject["visible"] = displaySignal.visible;
            signalsArray.append(signalObject);
        }
        groupObject["signals"] = signalsArray;
        groupsArray.append(groupObject);
    }
    rootObject["groups"] = groupsArray;

    QJsonArray pinTopArray;
    for (const QString &signalName : m_pinTopSignals)
    {
        if (m_signals.contains(signalName))
        {
            pinTopArray.append(signalName);
        }
    }
    rootObject["pin_top_signals"] = pinTopArray;

    QJsonObject displaySettings;
    displaySettings["hierarchical_display"] = m_hierarchicalDisplay;
    displaySettings["signal_height"] = m_signalHeight;
    displaySettings["signal_spacing"] = m_signalSpacing;
    displaySettings["time_ruler_height"] = m_timeRulerHeight;
    displaySettings["grid_visible"] = m_gridVisible;
    displaySettings["time_ruler_visible"] = m_timeRulerVisible;
    displaySettings["signal_names_visible"] = m_signalNamesVisible;
    rootObject["display_settings"] = displaySettings;

    QJsonObject timeSettings;
    timeSettings["min_time"] = m_minTime;
    timeSettings["max_time"] = m_maxTime;
    timeSettings["global_min_time"] = m_globalMinTime;
    timeSettings["global_max_time"] = m_globalMaxTime;

    rootObject["time_settings"] = timeSettings;

    QJsonObject areaSettings;
    areaSettings["name_area_width"] = m_nameAreaWidth;
    areaSettings["value_area_width"] = m_valueAreaWidth;
    areaSettings["wave_area_width"] = m_waveAreaWidth;
    rootObject["area_settings"] = areaSettings;

    QJsonDocument doc(rootObject);
    file.write(doc.toJson());
    file.close();
    QMessageBox::information(this, tr("Save Successful"),
                             tr("Group configuration saved successfully to:\n%1").arg(fileName));
    qDebug() << "Group configuration saved to:" << fileName;
}
void WaveformDisplay::loadGroup()
{

    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Load Group Configuration"),
                QDir::homePath(),
                tr("Group Configuration Files (*.grp);;All Files (*)"));
    if (fileName.isEmpty())
    {
        return;
    }
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, tr("Load Failed"),
                             tr("Cannot open file: %1").arg(file.errorString()));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        QMessageBox::warning(this, tr("Load Failed"),
                             tr("Invalid configuration file: %1").arg(error.errorString()));
        return;
    }
    if (!doc.isObject())
    {
        QMessageBox::warning(this, tr("Load Failed"),
                             tr("Invalid configuration file format"));
        return;
    }
    QJsonObject rootObject = doc.object();

    clearCurrentPosition();
    clearGroups();
    m_signals.clear();
    m_pinTopSignals.clear();
    m_groupOrder.clear();

    signal_idx = 0;

    if (rootObject.contains("groups") && rootObject["groups"].isArray())
    {
        QJsonArray groupsArray = rootObject["groups"].toArray();
        for (const QJsonValue &groupValue : groupsArray)
        {
            if (!groupValue.isObject())
                continue;
            QJsonObject groupObject = groupValue.toObject();

            QString groupName = groupObject["name"].toString();
            QColor groupColor = QColor(groupObject["color"].toString());
            bool collapsed = groupObject["collapsed"].toBool(false);

            addGroup(groupName, groupColor);
            setGroupCollapsed(groupName, collapsed);

            if (groupObject.contains("signals") && groupObject["signals"].isArray())
            {
                QJsonArray signalsArray = groupObject["signals"].toArray();
                for (const QJsonValue &signalValue : signalsArray)
                {
                    if (!signalValue.isObject())
                        continue;
                    QJsonObject signalObject = signalValue.toObject();
                    QString signal_name = signalObject["signal_name"].toString();

                    QString fullPath = "";
                    if (signalObject.contains("scopes") && signalObject["scopes"].isArray())
                    {
                        QJsonArray scopesArray = signalObject["scopes"].toArray();
                        QVector<QString> scopes;
                        for (const QJsonValue &scopeValue : scopesArray)
                        {
                            scopes.append(scopeValue.toString());
                        }

                        fullPath = scopes.join(".");
                    }
                    fullPath += "." + signal_name.remove(QRegularExpression("\\[[0-9:]+\\]"));
                    emit addSignalFromSource(fullPath, groupName, m_currentSignalPositionInGroup);

                    QString signalKey = QString::number(signal_idx - 1);
                    if (m_signals.contains(signalKey))
                    {
                        DisplaySignal &displaySignal = m_signals[signalKey];

                        if (displaySignal.nameColor.isValid())
                        {
                            signalObject["nameColor"] = displaySignal.nameColor.name();
                        }

                        if (displaySignal.valueColor.isValid())
                        {
                            signalObject["valueColor"] = displaySignal.valueColor.name();
                        }

                        if (displaySignal.lineColor.isValid())
                        {
                            signalObject["lineColor"] = displaySignal.lineColor.name();
                        }

                        if (displaySignal.backgroundColor.isValid())
                        {
                            signalObject["backgroundColor"] = displaySignal.backgroundColor.name();
                        }

                        if (signalObject.contains("translator"))
                        {
                            QString translatorName = signalObject["translator"].toString();

                            QSharedPointer<Translator> translator = nullptr;
                            for (auto &t : m_translatorManager->getAllTranslators())
                            {
                                if (t->name() == translatorName)
                                {
                                    translator = t;
                                    break;
                                }
                            }
                            if (translator)
                            {
                                displaySignal.translator = translator;
                            }
                        }
                        displaySignal.visible = signalObject["visible"].toBool(true);
                    }
                }
            }
        }
    }

    if (rootObject.contains("pin_top_signals") && rootObject["pin_top_signals"].isArray())
    {
        QJsonArray pinTopArray = rootObject["pin_top_signals"].toArray();
        for (const QJsonValue &signalValue : pinTopArray)
        {
            QString signalKey = signalValue.toString();
            if (m_signals.contains(signalKey))
            {

                m_pinTopSignals.append(signalKey);
            }
        }
    }

    if (rootObject.contains("display_settings") && rootObject["display_settings"].isObject())
    {
        QJsonObject displaySettings = rootObject["display_settings"].toObject();
        if (displaySettings.contains("hierarchical_display"))
        {
            m_hierarchicalDisplay = displaySettings["hierarchical_display"].toBool();
        }
        if (displaySettings.contains("signal_height"))
        {
            m_signalHeight = displaySettings["signal_height"].toInt();
        }
        if (displaySettings.contains("signal_spacing"))
        {
            m_signalSpacing = displaySettings["signal_spacing"].toInt();
        }
        if (displaySettings.contains("grid_visible"))
        {
            m_gridVisible = displaySettings["grid_visible"].toBool();
        }
        if (displaySettings.contains("time_ruler_visible"))
        {
            m_timeRulerVisible = displaySettings["time_ruler_visible"].toBool();
        }
        if (displaySettings.contains("signal_names_visible"))
        {
            m_signalNamesVisible = displaySettings["signal_names_visible"].toBool();
        }
    }

    if (rootObject.contains("time_settings") && rootObject["time_settings"].isObject())
    {
        QJsonObject timeSettings = rootObject["time_settings"].toObject();
        if (timeSettings.contains("min_time") && timeSettings.contains("max_time"))
        {
            double minTime = timeSettings["min_time"].toDouble();
            double maxTime = timeSettings["max_time"].toDouble();

            setTimeRange(minTime, maxTime);
        }
        if (timeSettings.contains("select_time"))
        {
            setSelectTime(timeSettings["select_time"].toDouble());
        }
        if (timeSettings.contains("global_min_time"))
        {
            m_globalMinTime = timeSettings["global_min_time"].toDouble();
        }
        if (timeSettings.contains("global_max_time"))
        {
            m_globalMaxTime = timeSettings["global_max_time"].toDouble();
        }
    }

    if (rootObject.contains("area_settings") && rootObject["area_settings"].isObject())
    {
        QJsonObject areaSettings = rootObject["area_settings"].toObject();
        if (areaSettings.contains("name_area_width"))
        {
            m_nameAreaWidth = areaSettings["name_area_width"].toInt();
        }
        if (areaSettings.contains("value_area_width"))
        {
            m_valueAreaWidth = areaSettings["value_area_width"].toInt();
        }
        if (areaSettings.contains("wave_area_width"))
        {
            m_waveAreaWidth = areaSettings["wave_area_width"].toInt();
        }

        QList<int> sizes;
        sizes << m_nameAreaWidth << m_valueAreaWidth << m_waveAreaWidth;
        m_horizontalSplitter->setSizes(sizes);
    }

    recalculateMaxWidths();
    updateContentWidth();
    updateScrollBars();
    update();
    QMessageBox::information(this, tr("Load Successful"),
                             tr("Group configuration loaded successfully from:\n%1").arg(fileName));
}
void WaveformDisplay::addMarker(double time, const QString &name, const QColor &color, Qt::PenStyle lineStyle)
{
    Marker marker(m_nextMarkerId++,
                  name.isEmpty() ? tr("Marker%1=%2ns").arg(m_nextMarkerId).arg(time) : name,
                  time, color, lineStyle);
    m_markers.insert(marker.id, marker);
    update();
    emit markerAdded(marker.id);
}
void WaveformDisplay::removeMarker(int markerId)
{
    if (m_markers.contains(markerId))
    {
        m_markers.remove(markerId);
        update();
        emit markerRemoved(markerId);
    }
}
void WaveformDisplay::clearAllMarkers()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
                this, tr("Clear Markers"),
                tr("Are you sure you want to clear all markers?"),
                QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        m_markers.clear();
        update();
        emit markersCleared();
    }
}
void WaveformDisplay::showMarkerManagerDialog()
{
    if (!m_markerManagerDialog)
    {
        createMarkerManagerDialog();
    }
    updateMarkerList();
    m_markerManagerDialog->show();
}
void WaveformDisplay::updateMarkerList()
{
    m_markerListWidget->clear();
    for (const auto &marker : m_markers)
    {
        QString itemText = QString("%1: %2 (Time: %3)")
                .arg(marker.name)
                .arg(marker.id)
                .arg(marker.time, 0, 'f', 3);
        QListWidgetItem *item = new QListWidgetItem(itemText, m_markerListWidget);
        item->setData(Qt::UserRole, marker.id);
        item->setForeground(marker.color);
        if (!marker.visible)
        {
            QFont font = item->font();
            font.setItalic(true);
            item->setFont(font);
            item->setForeground(Qt::gray);
        }
    }
}
void WaveformDisplay::onMarkerSelected()
{
    QList<QListWidgetItem *> selectedItems = m_markerListWidget->selectedItems();
    if (selectedItems.isEmpty())
    {
        m_markerNameEdit->clear();
        m_markerTimeEdit->setValue(0);
        m_lineStyleCombo->setCurrentIndex(0);
        updateColorButton(m_colorButton, Qt::red);
        m_applyMarkerBtn->setEnabled(false);
        m_deleteMarkerBtn->setEnabled(false);
        return;
    }
    int markerId = selectedItems.first()->data(Qt::UserRole).toInt();
    if (m_markers.contains(markerId))
    {
        const Marker &marker = m_markers[markerId];
        m_markerNameEdit->setText(marker.name);
        m_markerTimeEdit->setValue(marker.time);

        int index = m_lineStyleCombo->findData(static_cast<int>(marker.lineStyle));
        if (index >= 0)
        {
            m_lineStyleCombo->setCurrentIndex(index);
        }

        updateColorButton(m_colorButton, marker.color);
        m_currentMarkerColor = marker.color;
        m_applyMarkerBtn->setEnabled(true);
        m_deleteMarkerBtn->setEnabled(true);
    }
}
void WaveformDisplay::applyMarkerChanges()
{
    QList<QListWidgetItem *> selectedItems = m_markerListWidget->selectedItems();
    if (selectedItems.isEmpty())
        return;
    int markerId = selectedItems.first()->data(Qt::UserRole).toInt();
    if (m_markers.contains(markerId))
    {
        Marker &marker = m_markers[markerId];
        marker.name = m_markerNameEdit->text();
        marker.time = m_markerTimeEdit->value();
        marker.lineStyle = static_cast<Qt::PenStyle>(m_lineStyleCombo->currentData().toInt());
        marker.color = m_currentMarkerColor;
        updateMarkerList();
        update();
        emit markerUpdated(markerId);
    }
}
void WaveformDisplay::deleteSelectedMarker()
{
    QList<QListWidgetItem *> selectedItems = m_markerListWidget->selectedItems();
    if (selectedItems.isEmpty())
        return;
    int markerId = selectedItems.first()->data(Qt::UserRole).toInt();
    removeMarker(markerId);
    updateMarkerList();
}
void WaveformDisplay::setMarkerVisible(int markerId, bool visible)
{
    if (m_markers.contains(markerId))
    {
        m_markers[markerId].visible = visible;
        update();
    }
}
void WaveformDisplay::updateColorButton(QPushButton *button, const QColor &color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(color);
    button->setIcon(QIcon(pixmap));
}
void WaveformDisplay::createMarkerManagerDialog()
{
    m_markerManagerDialog = new QDialog(this);
    m_markerManagerDialog->setWindowTitle(tr("Marker Manager"));
    m_markerManagerDialog->setMinimumSize(400, 300);
    QVBoxLayout *mainLayout = new QVBoxLayout(m_markerManagerDialog);

    QLabel *listLabel = new QLabel(tr("Markers:"), m_markerManagerDialog);
    m_markerListWidget = new QListWidget(m_markerManagerDialog);
    m_markerListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    QLabel *editLabel = new QLabel(tr("Edit Marker:"), m_markerManagerDialog);
    QFormLayout *formLayout = new QFormLayout();

    QLabel *lineStyleLabel = new QLabel(tr("Line Style:"), m_markerManagerDialog);
    QComboBox *lineStyleCombo = new QComboBox(m_markerManagerDialog);
    lineStyleCombo->addItem(tr("Solid"), static_cast<int>(Qt::SolidLine));
    lineStyleCombo->addItem(tr("Dash"), static_cast<int>(Qt::DashLine));
    lineStyleCombo->addItem(tr("Dot"), static_cast<int>(Qt::DotLine));
    lineStyleCombo->addItem(tr("Dash Dot"), static_cast<int>(Qt::DashDotLine));
    lineStyleCombo->addItem(tr("Dash Dot Dot"), static_cast<int>(Qt::DashDotDotLine));

    QLabel *colorLabel = new QLabel(tr("Color:"), m_markerManagerDialog);
    QPushButton *colorButton = new QPushButton(m_markerManagerDialog);
    colorButton->setFixedSize(24, 24);

    formLayout->addRow(lineStyleLabel, lineStyleCombo);
    formLayout->addRow(colorLabel, colorButton);

    connect(colorButton, &QPushButton::clicked, this, [this, colorButton]()
    {
        QColor color = QColorDialog::getColor(m_currentMarkerColor, this, tr("Select Marker Color"));
        if (color.isValid()) {
            m_currentMarkerColor = color;
            updateColorButton(colorButton, color);
        } });

    m_lineStyleCombo = lineStyleCombo;
    m_colorButton = colorButton;
    m_markerNameEdit = new QLineEdit(m_markerManagerDialog);
    m_markerTimeEdit = new QDoubleSpinBox(m_markerManagerDialog);
    m_markerTimeEdit->setRange(0, 1000000);
    m_markerTimeEdit->setDecimals(3);
    formLayout->addRow(tr("Name:"), m_markerNameEdit);
    formLayout->addRow(tr("Time:"), m_markerTimeEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *goToBtn = new QPushButton(tr("Go to Marker"), m_markerManagerDialog);
    m_applyMarkerBtn = new QPushButton(tr("Apply"), m_markerManagerDialog);
    m_deleteMarkerBtn = new QPushButton(tr("Delete"), m_markerManagerDialog);
    QPushButton *closeBtn = new QPushButton(tr("Close"), m_markerManagerDialog);
    buttonLayout->addWidget(goToBtn);
    buttonLayout->addWidget(m_applyMarkerBtn);
    buttonLayout->addWidget(m_deleteMarkerBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);

    connect(goToBtn, &QPushButton::clicked, this, [this]()
    {
        QList<QListWidgetItem*> selectedItems = m_markerListWidget->selectedItems();
        if (!selectedItems.isEmpty()) {
            int markerId = selectedItems.first()->data(Qt::UserRole).toInt();
            goToMarker(markerId);
        } });

    connect(m_markerListWidget, &QListWidget::itemSelectionChanged,
            this, &WaveformDisplay::onMarkerSelected);
    connect(m_applyMarkerBtn, &QPushButton::clicked,
            this, &WaveformDisplay::applyMarkerChanges);
    connect(m_deleteMarkerBtn, &QPushButton::clicked,
            this, &WaveformDisplay::deleteSelectedMarker);
    connect(closeBtn, &QPushButton::clicked,
            m_markerManagerDialog, &QDialog::accept);

    mainLayout->addWidget(listLabel);
    mainLayout->addWidget(m_markerListWidget);
    mainLayout->addWidget(editLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
}
void WaveformDisplay::updateWavefrom(QSharedPointer<Waveform> waveform, QSharedPointer<IWaveformReader> reader)
{
    m_waveform = waveform;
    m_reader = reader;
}
void WaveformDisplay::updateContentWidth()
{

    updateScrollBars();
}
void WaveformDisplay::handleSearch()
{
    QString searchText = m_searchLineEdit->text().trimmed();
    if (!searchText.isEmpty())
    {

        for (auto &signal : m_signals)
        {
            QString displayName = signal.signal_name;
            if (m_hierarchicalDisplay)
            {
                displayName = join_path_with_slash(signal.scopes) + "/" + signal.signal_name;
            }

            bool isVisible = displayName.contains(searchText, Qt::CaseInsensitive);
            signal.visible = isVisible;
        }

        update();
        updateScrollBars();
    }
    else
    {
        for (auto &signal : m_signals)
        {
            signal.visible = true;
        }
        update();
        updateScrollBars();
    }
}
void WaveformDisplay::addSignal(const VarRef var_ref, const QString &name, const QSharedPointer<Signal> &signal, const QString &group, const int &index,
                                const int parent_expansion_id, const int indent_level)
{

    DisplaySignal displaySignal;
    displaySignal.var_ref = var_ref;
    displaySignal.name = QString::number(signal_idx);
    displaySignal.signal_name = name;
    displaySignal.signal = signal;
    ScopeRef scopeRef = m_waveform->get_hierarchy().get_var(var_ref).parent_scope;
    displaySignal.scopes = m_waveform->get_hierarchy().get_scope_path_vector(scopeRef);
    auto meta = m_waveform->var_to_meta(var_ref);
    QSharedPointer<Translator> defaultTranslator = m_translatorManager->findTranslator(meta);
    displaySignal.translator = defaultTranslator;
    displaySignal.visible = true;
    displaySignal.parent_expansion_id = parent_expansion_id;
    displaySignal.indent_level = indent_level;
    Var var = m_waveform->get_hierarchy().get_var(var_ref);
    if (!var.multi_array.empty() && var.signal_type.width > 1)
    {
        displaySignal.canExpand = true;
    }
    signal_idx++;
    m_signals[displaySignal.name] = displaySignal;

    QFontMetrics fm(this->font());
    QString displayName = name;
    if (m_hierarchicalDisplay)
    {
        displayName = join_path_with_slash(displaySignal.scopes) + "/" + name;
    }
    int nameWidth = fm.horizontalAdvance(displayName) + 20;
    if (nameWidth > m_signalNameLongestWidth)
    {
        m_signalNameLongestWidth = nameWidth;
    }

    QString initialValue = getSignalValueAtTime(displaySignal, 0);
    int valueWidth = fm.horizontalAdvance(initialValue) + 15;
    if (valueWidth > m_signalValueLongestWidth)
    {
        m_signalValueLongestWidth = valueWidth;
    }

    if (group != "" && m_groups.contains(group))
    {
        if (index >= 0 && index <= m_groups[group].v_signals.size())
        {
            m_groups[group].v_signals.insert(index, displaySignal.name);
        }
        else
        {
            m_groups[group].v_signals.append(displaySignal.name);
        }
    }
    else
    {

        if (m_groups.contains("G1"))
        {
            if (index >= 0 && index <= m_groups[group].v_signals.size())
            {
                m_groups["G1"].v_signals.insert(index, displaySignal.name);
            }
            else
            {
                m_groups["G1"].v_signals.append(displaySignal.name);
            }
        }
        else
        {
            addGroup("G1", QColor(200, 230, 255));
            m_groups["G1"].v_signals.append(displaySignal.name);
        }
    }

    autoScaleTime();
    updateContentWidth();
    updateScrollBars();
    update();
}
void WaveformDisplay::addSignalToPinTopSignals(const VarRef var_ref, const QString &name, const QSharedPointer<Signal> &signal, const QString &idx, const int &index)
{

    DisplaySignal displaySignal;
    displaySignal.var_ref = var_ref;
    displaySignal.name = idx;
    displaySignal.signal_name = name;
    displaySignal.signal = signal;
    ScopeRef scopeRef = m_waveform->get_hierarchy().get_var(var_ref).parent_scope;
    displaySignal.scopes = m_waveform->get_hierarchy().get_scope_path_vector(scopeRef);
    auto meta = m_waveform->var_to_meta(var_ref);
    QSharedPointer<Translator> defaultTranslator = m_translatorManager->findTranslator(meta);
    displaySignal.translator = defaultTranslator;
    displaySignal.visible = true;
    m_signals[displaySignal.name] = displaySignal;

    QFontMetrics fm(this->font());
    QString displayName = name;
    if (m_hierarchicalDisplay)
    {
        displayName = join_path_with_slash(displaySignal.scopes) + "/" + name;
    }
    int nameWidth = fm.horizontalAdvance(displayName) + 20;
    if (nameWidth > m_signalNameLongestWidth)
    {
        m_signalNameLongestWidth = nameWidth;
    }

    QString initialValue = getSignalValueAtTime(displaySignal, 0);
    int valueWidth = fm.horizontalAdvance(initialValue) + 15;
    if (valueWidth > m_signalValueLongestWidth)
    {
        m_signalValueLongestWidth = valueWidth;
    }

    if (index >= 0 && index < m_pinTopSignals.size())
    {
        m_pinTopSignals.insert(index, displaySignal.name);
    }
    else
    {
        m_pinTopSignals.append(displaySignal.name);
    }

    autoScaleTime();
    updateContentWidth();
    updateScrollBars();
    update();
}
void WaveformDisplay::recalculateMaxWidths()
{
    QFontMetrics fm(this->font());
    m_signalNameLongestWidth = 0;
    m_signalValueLongestWidth = 0;
    for (const auto &displaySignal : m_signals)
    {

        QString displayName = displaySignal.signal_name;
        if (m_hierarchicalDisplay)
        {
            displayName = join_path_with_slash(displaySignal.scopes) + "/" + displaySignal.signal_name;
        }
        int nameWidth = fm.horizontalAdvance(displayName) + 20;
        if (nameWidth > m_signalNameLongestWidth)
        {
            m_signalNameLongestWidth = nameWidth;
        }

        QString value = getSignalValueAtTime(displaySignal, m_selectTime);
        int valueWidth = fm.horizontalAdvance(value) + 15;
        if (valueWidth > m_signalValueLongestWidth)
        {
            m_signalValueLongestWidth = valueWidth;
        }
    }

    if (m_signalNameLongestWidth < 100)
        m_signalNameLongestWidth = 100;
    if (m_signalValueLongestWidth < 80)
        m_signalValueLongestWidth = 80;
}
void WaveformDisplay::removeSignal(const QString &name, const bool &deep)
{
    if (m_signals.contains(name))
    {
        QFontMetrics fm(this->font());
        QString displayName = m_signals[name].signal_name;
        if (m_hierarchicalDisplay)
        {
            displayName = join_path_with_slash(m_signals[name].scopes) + "/" + m_signals[name].signal_name;
        }
        int removedNameWidth = fm.horizontalAdvance(displayName) + 20;
        QString removedValue = getSignalValueAtTime(m_signals[name], m_selectTime);
        int removedValueWidth = fm.horizontalAdvance(removedValue) + 15;

        if (removedNameWidth >= m_signalNameLongestWidth || removedValueWidth >= m_signalValueLongestWidth)
        {
            recalculateMaxWidths();
        }

        for (auto &group : m_groups)
        {
            group.v_signals.removeAll(name);
        }
        if (deep)
        {
            m_signals.remove(name);
        }
        m_pinTopSignals.removeAll(name);
        updateContentWidth();
        update();
    }
}
void WaveformDisplay::clearSignals()
{
    m_signals.clear();
    for (auto &group : m_groups)
    {
        group.v_signals.clear();
    }
    update();
}
void WaveformDisplay::addGroup(const QString &name, const QColor &color)
{
    if (m_groups.contains(name))
    {
        qDebug() << "m_groups.contains(name):" << name;
        return;
    }
    SignalGroup group;

    group.name = name;
    group.color = color;
    group.collapsed = false;
    m_groups[name] = group;
    m_groupOrder.append(name);
    emit groupAdded(name);

    update();
}
void WaveformDisplay::renameGroup(const QString &oldName, const QString &newName)
{
    if (oldName == newName)
    {
        return;
    }
    if (!m_groups.contains(oldName))
    {
        QMessageBox::warning(this, tr("Rename Group"),
                             tr("Group '%1' does not exist.").arg(oldName));
        return;
    }
    if (m_groups.contains(newName))
    {
        QMessageBox::warning(this, tr("Rename Group"),
                             tr("Group '%1' already exists.").arg(newName));
        return;
    }
    if (newName.trimmed().isEmpty())
    {
        QMessageBox::warning(this, tr("Rename Group"),
                             tr("Group name cannot be empty."));
        return;
    }

    SignalGroup group = m_groups[oldName];
    group.name = newName;

    m_groups.remove(oldName);
    m_groups[newName] = group;

    int index = m_groupOrder.indexOf(oldName);
    if (index != -1)
    {
        m_groupOrder.replace(index, newName);
    }

    if (m_contextMenuGroup == oldName)
    {
        m_contextMenuGroup = newName;
    }

    update();
    qDebug() << "Group renamed from" << oldName << "to" << newName;
}
void WaveformDisplay::removeGroup(const QString &name)
{
    if (m_groups.contains(name))
    {
        m_groups.remove(name);
        m_groupOrder.removeAll(name);
        emit groupRemoved(name);
        update();
    }
}
void WaveformDisplay::clearGroups()
{

    QStringList groupsToRemove;
    for (const QString &groupName : m_groupOrder)
    {

        groupsToRemove.append(groupName);
    }
    for (const QString &groupName : groupsToRemove)
    {
        removeGroup(groupName);
    }

    if (m_groups.contains("G1"))
    {
        m_groups["G1"].v_signals.clear();
    }
}
void WaveformDisplay::setGroupCollapsed(const QString &name, bool collapsed)
{
    if (m_groups.contains(name))
    {
        m_groups[name].collapsed = collapsed;
        update();
    }
}
bool WaveformDisplay::isGroupCollapsed(const QString &name) const
{
    return m_groups.contains(name) ? m_groups[name].collapsed : false;
}
void WaveformDisplay::setGroupColor(const QString &name, const QColor &color)
{
    if (m_groups.contains(name))
    {
        m_groups[name].color = color;
        update();
    }
}
QColor WaveformDisplay::groupColor(const QString &name) const
{
    return m_groups.contains(name) ? m_groups[name].color : Qt::gray;
}
QString WaveformDisplay::signalGroup(const QString &signalName) const
{
    for (const auto &group : m_groups)
    {
        if (group.v_signals.contains(signalName))
        {
            return group.name;
        }
    }
    return QString();
}
void WaveformDisplay::moveSignalToGroup(const QString &signalName, const QString &groupName, const int &targetIndex)
{
    if (!m_signals.contains(signalName) || !m_groups.contains(groupName))
    {
        return;
    }

    QString oldGroup = signalGroup(signalName);
    if (!oldGroup.isEmpty() && m_groups.contains(oldGroup))
    {
        m_groups[oldGroup].v_signals.removeAll(signalName);
    }

    m_groups[groupName].v_signals.insert(targetIndex, signalName);
    emit signalMoved(signalName, oldGroup, groupName);
    update();
}
void WaveformDisplay::setTimeRange(double minTime, double maxTime)
{
    if (minTime >= maxTime)
    {
        return;
    }

    minTime = std::max(minTime, m_globalMinTime);
    maxTime = std::min(maxTime, m_globalMaxTime);

    if (maxTime - minTime < m_minZoomRange)
    {

        return;
    }
    int viewWidth = m_waveAreaWidth;
    if (m_waveArea->verticalScrollBar()->isVisible())
    {
        viewWidth -= m_waveArea->verticalScrollBar()->width();
    }
    m_pixelsPerTimeUnit = viewWidth / (maxTime - minTime);
    m_minTime = minTime;
    m_maxTime = maxTime;
    update();
    emit timeRangeChanged(minTime, maxTime);
}
void WaveformDisplay::autoScaleTime()
{
    if (m_signals.isEmpty())
    {
        setTimeRange(0, 1000);
        return;
    }
    double minTime = std::numeric_limits<double>::max();
    double maxTime = std::numeric_limits<double>::lowest();

    for (const auto &displaySignal : m_signals)
    {
        const auto &signal = displaySignal.signal;
        if (signal.isNull() || signal->get_change_count() == 0)
        {
            continue;
        }

        auto plotData = m_waveform->to_plot_data(signal);
        if (!plotData.first.empty())
        {
            minTime = std::min(minTime, plotData.first.front());
            maxTime = std::max(maxTime, plotData.first.back());
        }
    }
    if (minTime < maxTime)
    {

        double margin = (maxTime - minTime) * 0.05;

        m_globalMaxTime = maxTime + margin;

        m_globalMinTime = minTime;

        setTimeRange(m_globalMinTime, m_globalMaxTime);
    }
    else
    {
        setTimeRange(0, 1000);
        m_globalMinTime = 0;
        m_globalMaxTime = 1000;
    }
}
void WaveformDisplay::setSignalSpacing(int spacing)
{
    m_signalSpacing = spacing;
    update();
}
void WaveformDisplay::setSignalHeight(int height)
{
    m_signalHeight = height;
    update();
}
void WaveformDisplay::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    update();
}
void WaveformDisplay::setTimeRulerVisible(bool visible)
{
    m_timeRulerVisible = visible;
    update();
}
void WaveformDisplay::setSignalNamesVisible(bool visible)
{
    m_signalNamesVisible = visible;
    update();
}
QPair<double, double> WaveformDisplay::timeRange() const
{
    return qMakePair(m_minTime, m_maxTime);
}
int WaveformDisplay::signalCount() const
{
    return m_signals.size();
}
void WaveformDisplay::zoomIn()
{

    double currentRange = m_maxTime - m_minTime;
    double center = (m_minTime + m_maxTime) / 2.0;

    double newRange = currentRange * 0.8;

    if (newRange < m_minZoomRange)
    {
        newRange = m_minZoomRange;

        if (newRange >= (m_globalMaxTime - m_globalMinTime))
        {

            return;
        }
    }

    double newMinTime = center - newRange / 2.0;
    double newMaxTime = center + newRange / 2.0;

    if (newMinTime < m_globalMinTime)
    {
        newMinTime = m_globalMinTime;
        newMaxTime = newMinTime + newRange;

        if (newMaxTime > m_globalMaxTime)
        {
            newMaxTime = m_globalMaxTime;
            newRange = newMaxTime - newMinTime;
        }
    }
    else if (newMaxTime > m_globalMaxTime)
    {
        newMaxTime = m_globalMaxTime;
        newMinTime = newMaxTime - newRange;
        if (newMinTime < m_globalMinTime)
        {
            newMinTime = m_globalMinTime;
            newRange = newMaxTime - newMinTime;
        }
    }

    if (newMinTime >= newMaxTime || newRange < m_minZoomRange)
    {
        return;
    }
    setTimeRange(newMinTime, newMaxTime);
    updateScrollBars();
}
void WaveformDisplay::zoomOut()
{

    double currentRange = m_maxTime - m_minTime;
    double center = (m_minTime + m_maxTime) / 2.0;

    double newRange = currentRange * 1.25;

    if (newRange > (m_globalMaxTime - m_globalMinTime))
    {
        newRange = m_globalMaxTime - m_globalMinTime;
    }

    double newMinTime = center - newRange / 2.0;
    double newMaxTime = center + newRange / 2.0;

    if (newMinTime < m_globalMinTime)
    {
        double offset = m_globalMinTime - newMinTime;
        newMinTime = m_globalMinTime;
        newMaxTime += offset;

        if (newMaxTime > m_globalMaxTime)
        {
            newMaxTime = m_globalMaxTime;
            newMinTime = newMaxTime - newRange;

            if (newMinTime < m_globalMinTime)
            {
                newMinTime = m_globalMinTime;
                newRange = newMaxTime - newMinTime;
            }
        }
    }
    else if (newMaxTime > m_globalMaxTime)
    {
        double offset = newMaxTime - m_globalMaxTime;
        newMaxTime = m_globalMaxTime;
        newMinTime -= offset;

        if (newMinTime < m_globalMinTime)
        {
            newMinTime = m_globalMinTime;
            newRange = newMaxTime - newMinTime;
        }
    }

    if (newMinTime >= newMaxTime || newRange < m_minZoomRange)
    {
        return;
    }
    setTimeRange(newMinTime, newMaxTime);
    updateScrollBars();
}
void WaveformDisplay::zoomAll()
{

    setTimeRange(m_globalMinTime, m_globalMaxTime);
}
bool WaveformDisplay::currentPositionIsTime() const
{
    SignalCrusorTimeAndPosition x;
    x.clickTime = m_clickTime;
    x.clickTimeXPosition = timeToX(m_clickTime);
    if (!m_lastSelectedSignalKey.isNull() && !m_lastSelectedSignalKey.trimmed().isEmpty() && m_signals.contains(m_lastSelectedSignalKey))
    {
        QSharedPointer<Signal> signal = m_signals[m_lastSelectedSignalKey].signal;
        Time last_cursor_time = m_waveform->get_next_time(signal, m_clickTime, false);
        Time last_next_cursor_time = m_waveform->get_next_time(signal, last_cursor_time, true);
        if (m_clickTime == last_next_cursor_time)
            return true;
        return false;
    }
    return false;
}
WaveformDisplay::SignalCrusorTimeAndPosition WaveformDisplay::getDistanceFromLastTimeAndNextTime() const
{
    SignalCrusorTimeAndPosition x;
    x.clickTime = m_clickTime;
    x.clickTimeXPosition = timeToX(m_clickTime);
    if (!m_lastSelectedSignalKey.isNull() && !m_lastSelectedSignalKey.trimmed().isEmpty() && m_signals.contains(m_lastSelectedSignalKey))
    {
        QSharedPointer<Signal> signal = m_signals[m_lastSelectedSignalKey].signal;
        Time last_cursor_time = m_waveform->get_next_time(signal, m_clickTime, false);
        Time next_cursor_time = m_waveform->get_next_time(signal, m_clickTime, true);
        x.lastTime = last_cursor_time;
        x.nextTime = next_cursor_time;
        x.lastTimeXPosition = timeToX(last_cursor_time);
        x.nextTimeXPosition = timeToX(next_cursor_time);
        x.lastTimeDistance = x.clickTime - x.lastTime;
        x.nextTimeDistance = x.nextTime - x.clickTime;
    }
    else
    {
        x.lastTime = -1;
        x.nextTime = -1;
        x.lastTimeXPosition = -1;
        x.nextTimeXPosition = -1;
        x.lastTimeDistance = -1;
        x.nextTimeDistance = -1;
    }
    return x;
}
void WaveformDisplay::setCursor(bool isNext)
{
    if (!m_lastSelectedSignalKey.isNull() && !m_lastSelectedSignalKey.trimmed().isEmpty() && m_signals.contains(m_lastSelectedSignalKey))
    {
        QSharedPointer<Signal> signal = m_signals[m_lastSelectedSignalKey].signal;
        Time cursor_time = m_waveform->get_next_time(signal, m_selectTime, isNext);
        setSelectTime(cursor_time);
        emit timeValueChanged(m_selectTime);
        emit timeChangeForGetSignals();
        ensureCursorAtMiddle();
        update();
    }
}
void WaveformDisplay::findSearchEdgeType(FindSearchEdgeType edgeType)
{
    if (!m_lastSelectedSignalKey.isNull() && !m_lastSelectedSignalKey.trimmed().isEmpty() && m_signals.contains(m_lastSelectedSignalKey))
    {
        QSharedPointer<Signal> signal = m_signals[m_lastSelectedSignalKey].signal;
        Time current_time = 0;
        if (m_selectTime)
        {
            current_time = m_selectTime;
        }
        else if (m_clickTime)
        {
            current_time = m_clickTime;
        }
        if (signal)
        {
            Time targetTime = -1;
            bool found = false;
            if (edgeType == FindSearchEdgeType::LastUp)
            {
                auto time = m_waveform->get_previous_rising_edge(signal, current_time);

                if (time.has_value())
                {
                    targetTime = time.value();
                    found = true;
                }
            }
            else if (edgeType == FindSearchEdgeType::LastDown)
            {
                auto time = m_waveform->get_previous_falling_edge(signal, current_time);

                if (time.has_value())
                {
                    targetTime = time.value();
                    found = true;
                }
            }
            else if (edgeType == FindSearchEdgeType::NextUp)
            {
                auto time = m_waveform->get_next_rising_edge(signal, current_time);

                if (time.has_value())
                {
                    targetTime = time.value();
                    found = true;
                }
            }
            else if (edgeType == FindSearchEdgeType::NextDown)
            {
                auto time = m_waveform->get_next_falling_edge(signal, current_time);

                if (time.has_value())
                {
                    targetTime = time.value();
                    found = true;
                }
            }
            if (found)
            {

                setSelectTime(targetTime);
                ensureCursorAtMiddle();
                update();
            }
            else
            {
                qDebug() << "There was no rising edge before the current time.";
            }
        }
        else
        {
            qDebug() << "Not found signal.";
        }
    }
    else
    {
        qDebug() << "Unselected signal.";
    }
}
void WaveformDisplay::ensureCursorAtMiddle()
{

    double x = timeToX(m_selectTime);
    int visibleLeft = m_waveArea->horizontalScrollBar()->value() + m_nameAreaWidth + m_valueAreaWidth;
    int visibleRight = visibleLeft + m_waveAreaWidth;
    const int margin = m_waveAreaWidth / 2;
    if (x < visibleLeft + margin || x > visibleRight - margin)
    {

        int newScrollPos;
        if (x < visibleLeft + margin)
        {
            newScrollPos = x - margin - (m_nameAreaWidth + m_valueAreaWidth);
        }
        else if (x > visibleRight - margin)
        {
            newScrollPos = x - m_waveAreaWidth + margin - (m_nameAreaWidth + m_valueAreaWidth);
        }

        newScrollPos = std::max(m_waveArea->horizontalScrollBar()->minimum(), std::min(newScrollPos, m_waveArea->horizontalScrollBar()->maximum()));
        m_waveArea->horizontalScrollBar()->setValue(newScrollPos);
    }
}
void WaveformDisplay::ensureCursorVisible()
{

    double x = timeToX(m_selectTime);
    int visibleLeft = m_waveArea->horizontalScrollBar()->value() + m_nameAreaWidth + m_valueAreaWidth;
    int visibleRight = visibleLeft + m_waveAreaWidth;

    const int margin = 100;
    if (x < visibleLeft + margin || x > visibleRight - margin)
    {

        int newScrollPos;
        if (x < visibleLeft + margin)
        {
            newScrollPos = x - margin - (m_nameAreaWidth + m_valueAreaWidth);
        }
        else if (x > visibleRight - margin)
        {
            newScrollPos = x - m_waveAreaWidth + margin - (m_nameAreaWidth + m_valueAreaWidth);
        }

        newScrollPos = std::max(m_waveArea->horizontalScrollBar()->minimum(), std::min(newScrollPos, m_waveArea->horizontalScrollBar()->maximum()));
        m_waveArea->horizontalScrollBar()->setValue(newScrollPos);
    }
}
void WaveformDisplay::panLeft()
{
    double range = m_maxTime - m_minTime;
    setTimeRange(m_minTime - range * 0.1, m_maxTime - range * 0.1);
}
void WaveformDisplay::panRight()
{
    double range = m_maxTime - m_minTime;
    setTimeRange(m_minTime + range * 0.1, m_maxTime + range * 0.1);
}
void WaveformDisplay::setHierarchicalDisplay(bool enabled)
{

    m_hierarchicalDisplay = !m_hierarchicalDisplay;

    recalculateMaxWidths();
    updateContentWidth();
    update();
}
void WaveformDisplay::drawArrowHead(QPainter &painter, const QPoint &point, bool directionUp, int size)
{
    QPointF arrowHead[3];
    int halfSize = size / 2;
    if (directionUp)
    {

        arrowHead[0] = QPointF(point.x(), point.y() - halfSize);
        arrowHead[1] = QPointF(point.x() - halfSize, point.y() + halfSize);
        arrowHead[2] = QPointF(point.x() + halfSize, point.y() + halfSize);
    }
    else
    {

        arrowHead[0] = QPointF(point.x(), point.y() + halfSize);
        arrowHead[1] = QPointF(point.x() - halfSize, point.y() - halfSize);
        arrowHead[2] = QPointF(point.x() + halfSize, point.y() - halfSize);
    }
    painter.drawPolygon(arrowHead, 3);
}
void paintImageWithDragging(QPainter &painter, int iconX, int iconY)
{
    QPixmap indicatorIcon(":/icons/icons/drag-hand.png");
    if (indicatorIcon.isNull())
    {
        qDebug() << "Failed to load image from resource.";
    }
    else
    {

        QRect backgroundRect(iconX, iconY, 24, 24);

        painter.fillRect(backgroundRect, Qt::white);

        painter.drawPixmap(iconX, iconY, 24, 24, indicatorIcon);
    }
}
void WaveformDisplay::handleThemeChange(const QString &themeName)
{
    if (themeName == "light")
    {
        m_pen = QPen(Qt::black);
        m_groupRectColor = QColor("#c8e6ff");
        m_signalPen = QPen(Qt::darkBlue);

        m_mainRulerLinePen = QPen(Qt::black, 1);
        m_mainRulerTextPen = QPen(Qt::black, 2);
        m_splitPen = QPen(Qt::black, 2);
        m_dragPreviewLinePen = QPen(Qt::blue, 2, Qt::DashLine);
        m_verticalLinePen = QPen(QColor("#003e00"), 1, Qt::DashLine);
        m_horizontalDraggingBorderPen = QPen(Qt::blue);
        m_rulerRectColor = QColor(220, 220, 220);
        m_arrowColor = QColor(255, 100, 0);
        m_highLightColor = QColor("#ffffb5");
        m_highLightHighColor = QColor("#40fd1c");
        m_highLightHighToLowColor = QColor("#5ddb4f");
        m_highLightLowColor = QColor("#57d54b");

        m_highHexagonalHighColor = QColor("#58d74c");
        m_highHexagonalLowColor = QColor("#57d54b");
        m_selectionColor = QColor(100, 100, 255, 50);
        m_highLevelColor = QColor("#698e68");
        m_highToLowColor = QColor("#5a9a52");
        m_lowLevelColor = QColor("#38a82b");
        m_segmentXColor = QColor("#df1f25");
        m_segmentZColor = QColor("orange");
        m_hexagonalHighColor = QColor("#3bc235");
        m_hexagonalLowColor = QColor("#2ac223");
        m_waveformSignalTextColor = QColor("#333");
    }
    else if (themeName == "dark" || themeName == "gray")
    {
        m_pen = QPen(QColor("#e0e0e0"));
        m_groupRectColor = QColor("#313131");
        m_signalPen = QPen(QColor("#4FE9C4"));
        m_mainRulerLinePen = QPen(QColor("#E0E0E0"), 1);
        m_mainRulerTextPen = QPen(QColor("#E0E0E0"), 2);
        m_splitPen = QPen(Qt::white, 2);
        m_dragPreviewLinePen = QPen(Qt::yellow, 2, Qt::DashLine);
        m_verticalLinePen = QPen(Qt::yellow, 1, Qt::DashLine);
        m_horizontalDraggingBorderPen = QPen(Qt::yellow);
        m_rulerRectColor = QColor("#313131");
        m_arrowColor = QColor(255, 100, 0);
        m_highLightColor = QColor("#606060");
        m_highLightHighColor = QColor("#00ff00");
        m_highLightHighToLowColor = QColor("#00ff00");
        m_highLightLowColor = QColor("#00ff00");
        m_highHexagonalHighColor = QColor("#00ff00");
        m_highHexagonalLowColor = QColor("#00ff00");
        m_selectionColor = QColor(100, 100, 255, 50);
        m_highLevelColor = QColor("#00ff00");
        m_highToLowColor = QColor("#00ff00");
        m_lowLevelColor = QColor("#00ff00");
        m_segmentXColor = QColor("#df1f25");
        m_segmentZColor = QColor("orange");
        m_hexagonalHighColor = QColor("#00ff00");
        m_hexagonalLowColor = QColor("#00ff00");
        m_waveformSignalTextColor = QColor("#01ffff");
    }
}
void WaveformDisplay::drawPinTopSignalNames(QPainter &painter)
{
    painter.setPen(m_pen);
    int currentY = m_timeRulerHeight;
    int nameScrollX = m_nameArea->horizontalScrollBar()->value();
    for (const QString &signalKey : m_pinTopSignals)
    {
        QRect signalRect(5 - nameScrollX, currentY, m_nameAreaWidth - 5 + nameScrollX, m_signalHeight);

        if(m_signals[signalKey].nameColor.isValid()) {
            painter.setPen(m_signals[signalKey].nameColor);
        } else {
            painter.setPen(m_pen);
        }
        QString signalName = m_signals[signalKey].signal_name;
        if (m_hierarchicalDisplay)
        {
            signalName = join_path_with_slash(m_signals[signalKey].scopes) + "/" + signalName;
        }

        if (m_signals[signalKey].backgroundColor.isValid())
        {
            QColor backgroundColor = m_signals[signalKey].backgroundColor;
            QRect signalBgRect3(5 - nameScrollX + m_nameAreaWidth + m_valueAreaWidth - 5,
                                currentY, m_waveAreaWidth - 10, m_signalHeight);
            painter.fillRect(signalBgRect3, backgroundColor);
        }
        bool isSelected = m_selectedSignalKeys.contains(signalKey);
        if (isSelected)
        {
            QColor backgroundColor =  m_highLightColor;
            QRect signalBgRect1(5 - nameScrollX, currentY, m_nameAreaWidth - 10, m_signalHeight);
            QRect signalBgRect2(5 - nameScrollX + m_nameAreaWidth + 5, currentY, m_valueAreaWidth - 20, m_signalHeight);
            painter.fillRect(signalBgRect1, backgroundColor);
            painter.fillRect(signalBgRect2, backgroundColor);
        }

        painter.drawText(signalRect, Qt::AlignLeft | Qt::AlignVCenter, signalName);
        currentY += m_signalHeight + m_signalSpacing;
    }
}

void WaveformDisplay::drawFixedPinTopArea(QPainter &painter)
{

    painter.save();
    painter.setClipRect(0, m_timeRulerHeight, width(),
                        getTopPinSignalsAreaHeight());
    drawPinTopSignalNames(painter);
    painter.restore();

    painter.save();
    painter.setClipRect(m_nameAreaWidth, m_timeRulerHeight, m_valueAreaWidth,
                        getTopPinSignalsAreaHeight());
    drawPinTopSignalValues(painter);
    painter.restore();

    painter.save();
    painter.setClipRect(m_nameAreaWidth + m_valueAreaWidth, m_timeRulerHeight,
                        m_waveAreaWidth, getTopPinSignalsAreaHeight());
    painter.translate(-m_horizontalOffset, 0);
    drawSignalsArr(painter, m_timeRulerHeight, m_pinTopSignals, true);
    painter.restore();
}

void WaveformDisplay::drawScrollableSignalValues(QPainter &painter)
{

    QFont font = painter.font();
    font.setBold(false);
    painter.setFont(font);

    int currentY = m_timeRulerHeight + getTopPinSignalsAreaHeight();

    int valueScrollX = m_valueArea->horizontalScrollBar()->value();
    int valueScrollMax = m_valueArea->horizontalScrollBar()->maximum();
    int valueViewportWidth = m_valueArea->viewport()->width();
    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
        {
            continue;
        }
        const SignalGroup &group = m_groups[groupName];

        currentY += m_groupHeaderHeight + m_signalSpacing;

        if (!group.collapsed)
        {
            for (const QString &signalName : group.v_signals)
            {
                if (!m_signals[signalName].visible)
                {
                    continue;
                }

                const auto &displaySignal = m_signals[signalName];
                if (displaySignal.valueColor.isValid())
                {
                    painter.setPen(displaySignal.valueColor);
                }
                else
                {
                    painter.setPen(m_signalPen);
                }

                QString value = getSignalValueAtTime(displaySignal, m_selectTime);
                value += displaySignal.translator->prefixName();
                QFontMetrics fm(painter.font());
                int textWidth = fm.horizontalAdvance(value);
                if (m_signalValueLongestWidth > m_valueAreaWidth)
                {
                    double pixelsPerXValueUnit = double(m_signalValueLongestWidth - m_valueAreaWidth) / double(valueScrollMax);
                    int textX = m_nameAreaWidth + valueViewportWidth - textWidth + (valueScrollMax - valueScrollX) * pixelsPerXValueUnit;
                    QRect valueRect(textX, currentY, textWidth, m_signalHeight);
                    painter.drawText(valueRect, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, value);
                }
                else
                {
                    int textX = m_nameAreaWidth + valueViewportWidth - textWidth;
                    QRect valueRect(textX, currentY, textWidth, m_signalHeight);
                    painter.drawText(valueRect, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, value);
                }
                currentY += m_signalHeight + m_signalSpacing;
            }
        }
    }
}
void WaveformDisplay::drawScrollableSignalsWaveform(QPainter &painter)
{

    int currentY = m_timeRulerHeight + getTopPinSignalsAreaHeight();

    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
        {
            continue;
        }
        const SignalGroup &group = m_groups[groupName];
        currentY += m_groupHeaderHeight + m_signalSpacing;
        if (!group.collapsed)
        {
            currentY = drawSignalsArr(painter, currentY, group.v_signals, false);
        }
    }
}
void WaveformDisplay::drawScrollableGroupArea(QPainter &painter)
{
    int scrollAreaStartY = m_timeRulerHeight + getTopPinSignalsAreaHeight();
    int scrollAreaHeight = height() - scrollAreaStartY;

    painter.save();

    painter.setClipRect(0, scrollAreaStartY, width(), scrollAreaHeight);
    painter.translate(0, -m_verticalOffset);
    drawScrollableSignalNamesAndGroups(painter);
    painter.restore();

    painter.save();
    painter.setClipRect(m_nameAreaWidth, scrollAreaStartY, m_valueAreaWidth, scrollAreaHeight);
    painter.translate(0, -m_verticalOffset);
    drawScrollableSignalValues(painter);
    painter.restore();

    painter.save();
    painter.setClipRect(m_nameAreaWidth + m_valueAreaWidth, scrollAreaStartY,
                        m_waveAreaWidth, scrollAreaHeight);
    painter.translate(-m_horizontalOffset, -m_verticalOffset);
    drawScrollableSignalsWaveform(painter);
    painter.restore();
}
void WaveformDisplay::drawTimeRulerArea(QPainter &painter)
{
    painter.save();
    painter.setClipRect(m_nameAreaWidth + m_valueAreaWidth, 0, m_waveAreaWidth, height());
    painter.translate(-m_horizontalOffset, 0);
    if (m_timeRulerVisible)
    {
        drawTimeRuler(painter);
    }
    painter.restore();
}
int WaveformDisplay::getTopPinSignalsAreaHeight() const
{
    int height = 0;
    for (const QString &name : m_pinTopSignals)
    {
        height += m_signalHeight + m_signalSpacing;
    }
    return height;
}
int WaveformDisplay::getTopPinSignalsAreaBottomY() const
{
    return m_timeRulerHeight + getTopPinSignalsAreaHeight();
}
void WaveformDisplay::drawInteractiveElements(QPainter &painter)
{
    painter.save();

    if (m_draggingSignalName || m_addingSignalFromEditor)
    {
        painter.translate(0, -m_verticalOffset);
        int targetGroupIndex = getViewSignalGroupIndexAtY(m_dragCurrentY);
        int targetSignalInGroupIndex = getViewSignalIndexInGroupAtY(m_dragCurrentY);
        int topBottomY = getTopPinSignalsAreaBottomY();
        if (topBottomY < m_dragCurrentYWithoutOffset && targetGroupIndex >= 0)
        {
            int y = getTopPinSignalsAreaBottomY();

            y = y + (targetGroupIndex + 1) * (m_groupHeaderHeight + m_signalSpacing);

            if (targetGroupIndex > 0)
            {
                for (int i = 0; i < targetGroupIndex; i++)
                {
                    const QString &groupName = m_groupOrder.at(i);
                    const SignalGroup &group = m_groups[groupName];
                    if (group.collapsed)
                    {
                        continue;
                    }

                    int size = getGroupSignalVisibleSize(groupName);
                    y = y + (m_signalHeight + m_signalSpacing) * size;
                }
            }

            y = y + (targetSignalInGroupIndex + 1) * (m_signalHeight + m_signalSpacing);

            painter.setPen(m_dragPreviewLinePen);
            painter.drawLine(0, y, m_nameAreaWidth, y);
        }
    }

    if (m_isDraggingSignalInTopArea)
    {
        int topAreaHeight = getTopPinSignalsAreaHeight();
        painter.setClipRect(0, m_timeRulerHeight, m_nameAreaWidth, topAreaHeight);
        painter.translate(-m_horizontalOffset, -m_verticalOffset);

        QString targetSignalKey = getSignalKeyInTopArea(m_dragCurrentYWithoutOffset);
        if (!targetSignalKey.isEmpty())
        {
            int targetSignalIndex = m_pinTopSignals.indexOf(targetSignalKey);
            int y = m_timeRulerHeight + (targetSignalIndex + 1) * (m_signalHeight + m_signalSpacing) + m_verticalOffset;
            painter.setPen(m_dragPreviewLinePen);
            painter.drawLine(0, y, m_nameAreaWidth, y);
        }
    }

    if (m_draggingGroupName)
    {
        int targetIndex = getGroupIndexAtY(m_dragCurrentY);
        if (m_originalGroupIndex < targetIndex)
        {
            targetIndex += 1;
        }
        if (targetIndex >= 0)
        {
            int y = getTopPinSignalsAreaBottomY();
            for (int i = 0; i < targetIndex; i++)
            {
                const QString &groupName = m_groupOrder.at(i);
                const SignalGroup &group = m_groups[groupName];
                if (group.collapsed)
                {
                    y = y + m_groupHeaderHeight + m_signalSpacing;
                }
                else
                {
                    y = y + m_groupHeaderHeight + m_signalSpacing;
                    int size = getGroupSignalVisibleSize(groupName);
                    y = y + (m_signalHeight + m_signalSpacing) * size;
                }
            }
            painter.setPen(m_dragPreviewLinePen);
            painter.drawLine(0, y, m_nameAreaWidth, y);

            int iconX = m_dragCurrentX - m_horizontalOffset + 5;
            int iconY = m_dragCurrentY - m_verticalOffset + 5;
            paintImageWithDragging(painter, iconX, iconY);
        }
    }

    if (m_showClickIndicator)
    {
        painter.save();
        painter.setClipRect(m_nameAreaWidth + m_valueAreaWidth, 0, m_waveAreaWidth - 15, height());
        painter.translate(-m_horizontalOffset, 0);
        painter.setPen(m_verticalLinePen);

        double x = (m_selectTime != NULL) || (m_selectTime == 0) ? timeToX(m_selectTime) : timeToX(m_clickTime);

        painter.drawLine(x, 0, x, height());
        painter.restore();
    }

    if (m_middlePressClickTime != 0)
    {
        painter.save();
        painter.setClipRect(m_nameAreaWidth + m_valueAreaWidth, 0, m_waveAreaWidth - 15, height());
        painter.translate(-m_horizontalOffset, 0);
        painter.setPen(m_verticalLineMiddlePressPen);

        double x = timeToX(m_middlePressClickTime);

        painter.drawLine(x, 0, x, height());
        painter.restore();
    }

    if (m_horizontalDragging)
    {

        painter.save();
        painter.setClipRect(m_nameAreaWidth + m_valueAreaWidth, m_timeRulerHeight, m_waveAreaWidth - 15, height());
        painter.translate(-m_horizontalOffset, -m_verticalOffset);

        int startX = timeToX(m_dragStartTime);
        int endX = timeToX(m_dragEndTime);

        if (startX > endX)
        {
            std::swap(startX, endX);
        }

        int yPosition = m_horizontalDraggingLineStartPositionY;

        QRect selectionRect(startX, yPosition, endX - startX, m_signalHeight);
        painter.fillRect(selectionRect, m_selectionColor);

        painter.setPen(m_horizontalDraggingBorderPen);
        painter.drawLine(startX, yPosition, startX, yPosition + m_signalHeight);
        painter.drawLine(endX, yPosition, endX, yPosition + m_signalHeight);
        painter.restore();
    }

    if (m_verticalDragging)
    {

        int arrowBaseSize = 15;
        int arrowLength = std::min(std::abs(m_verticalDragDistance) / 2, 100);
        bool directionUp = m_verticalDragDistance < 0;
        painter.setPen(QPen(m_arrowColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(m_arrowColor);

        QPoint startPoint(m_dragStartPosWithoutOffset.x(), m_dragStartPosWithoutOffset.y());
        QPoint endPoint(m_dragStartPosWithoutOffset.x(), m_dragStartPosWithoutOffset.y() + m_verticalDragDistance);
        painter.drawLine(startPoint, endPoint);

        drawArrowHead(painter, endPoint, directionUp, arrowBaseSize);

        QFont hintFont = font();
        hintFont.setPointSize(10);
        hintFont.setBold(true);
        painter.setFont(hintFont);
        painter.setPen(m_pen);
        QRect textRect(m_dragStartPosWithoutOffset.x() - 40,
                       m_dragStartPosWithoutOffset.y() + m_verticalDragDistance / 2 - 20,
                       80, 20);

        painter.setBrush(QColor(255, 255, 255, 200));
        painter.drawRoundedRect(textRect, 5, 5);
        painter.setPen(QPen(Qt::black));

        painter.drawText(textRect, Qt::AlignCenter, m_verticalHintText);
    }

    if (m_isPressSignalSpacing)
    {
        if (m_currentSignalPosition > -2)
        {
            int y = m_timeRulerHeight + (m_currentGroupPosition + 1) * (m_groupHeaderHeight + m_signalSpacing) + (m_currentSignalPosition + 1) * (m_signalHeight + m_signalSpacing);

            painter.setPen(m_dragPreviewLinePen);
            painter.translate(0, -m_verticalOffset);
            painter.drawLine(0, y, m_nameAreaWidth, y);
        }
    }

    painter.restore();
}
void WaveformDisplay::drawMarkersLine(QPainter &painter)
{
    painter.save();

    painter.setClipRect(m_nameAreaWidth + m_valueAreaWidth, 0,
                        m_waveAreaWidth, height());
    painter.translate(-m_horizontalOffset, 0);
    for (const auto &marker : m_markers)
    {
        if (!marker.visible)
            continue;
        double x = timeToX(marker.time);

        QPen pen(marker.color, 1, marker.lineStyle);
        painter.setPen(pen);

        painter.drawLine(x, m_timeRulerHeight, x, height());
    }
    painter.restore();
}
void WaveformDisplay::drawMarkersRect(QPainter &painter)
{
    painter.save();

    painter.setClipRect(m_nameAreaWidth + m_valueAreaWidth, 0,
                        m_waveAreaWidth, m_timeRulerHeight);
    painter.translate(-m_horizontalOffset, 0);

    QList<Marker> sortedMarkers = m_markers.values();
    std::sort(sortedMarkers.begin(), sortedMarkers.end(),
              [](const Marker &a, const Marker &b)
    { return a.time < b.time; });

    QSet<double> usedTimes;
    for (const auto &marker : sortedMarkers)
    {
        if (!marker.visible)
            continue;
        double x = timeToX(marker.time);

        if (usedTimes.contains(marker.time))
        {
            continue;
        }

        usedTimes.insert(marker.time);

        QPen pen(marker.color, 1, marker.lineStyle);
        painter.setPen(pen);

        painter.drawLine(x, 0, x, m_timeRulerHeight);

        QFontMetrics fm(painter.font());
        int textWidth = fm.horizontalAdvance(marker.name + " = " + QString::number(marker.time)) + 40;
        int rectHeight = m_timeRulerHeight - 4;

        int verticalOffset = 0;
        int rectX = x;
        int rectY = 2 + verticalOffset;
        int rectWidth = textWidth;

        if (rectX + rectWidth > timeToX(m_maxTime))
        {
            rectWidth = timeToX(m_maxTime) - rectX - 2;
        }
        if (rectWidth < 20)
        {
            rectWidth = 20;
        }

        QColor bgColor = marker.color;
        if (marker.id == m_hoveredMarkerId)
        {
            bgColor.setAlpha(255);
        }
        else
        {
            bgColor.setAlpha(80);
        }

        painter.setBrush(bgColor);
        painter.setPen(QPen(marker.color.darker(), 1));
        QRect markerRect(rectX, rectY, rectWidth, rectHeight);
        painter.drawRect(markerRect);

        painter.setPen(Qt::white);
        QRect textRect = markerRect.adjusted(5, 0, -5, 0);

        QString displayText = marker.name + " = " + QString::number(marker.time);
        if (fm.horizontalAdvance(displayText) > textRect.width() - 10)
        {
            displayText = fm.elidedText(displayText, Qt::ElideRight, textRect.width() - 10);
        }
        painter.drawText(textRect, Qt::AlignCenter, displayText);
    }
    painter.restore();
}
QList<int> WaveformDisplay::getMarkerIds() const
{
    return m_markers.keys();
}
WaveformDisplay::Marker WaveformDisplay::getMarker(int markerId) const
{
    return m_markers.value(markerId, Marker());
}
void WaveformDisplay::clearMarkers()
{
    clearAllMarkers();
}
void WaveformDisplay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    calculateLayout();

    drawMarkersLine(painter);

    drawFixedPinTopArea(painter);

    drawScrollableGroupArea(painter);

    drawTimeRulerArea(painter);
    drawMarkersRect(painter);

    drawSearchHighlights(painter);

    drawInteractiveElements(painter);
    update();
}
void WaveformDisplay::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateAreaWidth();
    updateScrollBars();
    update();
}
void WaveformDisplay::drawPinTopSignalValues(QPainter &painter)
{

    int currentY = m_timeRulerHeight;
    int valueScrollX = m_valueArea->horizontalScrollBar()->value();
    int valueScrollMax = m_valueArea->horizontalScrollBar()->maximum();
    int valueViewportWidth = m_valueArea->viewport()->width();
    for (const QString &signalKey : m_pinTopSignals)
    {
        const auto &displaySignal = m_signals[signalKey];
        if(displaySignal.valueColor.isValid()){
            painter.setPen(QPen(displaySignal.valueColor));
        } else {
            painter.setPen(m_signalPen);
        }

        QString value = getSignalValueAtTime(displaySignal, m_selectTime);
        value += displaySignal.translator->prefixName();
        QFontMetrics fm(painter.font());
        int textWidth = fm.horizontalAdvance(value);
        if (m_signalValueLongestWidth > m_valueAreaWidth)
        {
            double pixelsPerXValueUnit = double(m_signalValueLongestWidth - m_valueAreaWidth) / double(valueScrollMax);
            int textX = m_nameAreaWidth + valueViewportWidth - textWidth + (valueScrollMax - valueScrollX) * pixelsPerXValueUnit;
            QRect valueRect(textX, currentY, textWidth, m_signalHeight);
            painter.drawText(valueRect, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, value);
        }
        else
        {
            int textX = m_nameAreaWidth + valueViewportWidth - textWidth;
            QRect valueRect(textX, currentY, textWidth, m_signalHeight);
            painter.drawText(valueRect, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, value);
        }
        currentY += m_signalHeight + m_signalSpacing;
    }
}
int WaveformDisplay::getMarkerAtPosition(const QPoint &pos) const
{

    if (pos.y() > m_timeRulerHeight || pos.x() < m_nameAreaWidth + m_valueAreaWidth)
    {
        return -1;
    }

    double x = pos.x() + m_horizontalOffset;

    for (const auto &marker : m_markers)
    {
        if (!marker.visible)
            continue;
        double markerX = timeToX(marker.time);

        QFontMetrics fm(font());
        int textWidth = fm.horizontalAdvance(marker.name + " = " + QString::number(marker.time)) + 40;

        QRect markerRect(markerX, 2, textWidth, m_timeRulerHeight - 4);

        if (markerRect.contains(pos.x(), pos.y()))
        {
            return marker.id;
        }
    }
    return -1;
}

void WaveformDisplay::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    m_dragStartPosWithoutOffset = pos;

    bool isInTopNameArea = (pos.y() <= getTopPinSignalsAreaBottomY() && pos.x() <= m_nameAreaWidth);
    if (isInTopNameArea)
    {
        m_dragStartPosWithOffset = pos;
        m_isPotentialDragSignalInTopArea = true;
        handleSignalSelection(event, true);
        return;
    }
    else
    {
        m_dragStartPosWithOffset = QPoint(pos.x(), pos.y() + m_verticalOffset);
    }
    if (pos.x() >= m_nameAreaWidth + m_valueAreaWidth)
    {
        m_dragStartPosWithOffset.setX(m_dragStartPosWithOffset.x() + m_horizontalOffset);
    }
    clearCurrentPosition();
    if (event->button() == Qt::LeftButton)
    {
        if (m_dragStartPosWithOffset.y() <= m_timeRulerHeight && m_dragStartPosWithOffset.x() > (m_nameAreaWidth + m_valueAreaWidth))
        {
            int markerId = getMarkerAtPosition(m_dragStartPosWithOffset);
            if (markerId != -1)
            {
                m_draggingMarkerId = markerId;
                m_markerBeforeDragTime = m_markers[markerId].time;
                m_markerDragStartTime = xToTime(m_dragStartPosWithOffset.x());
                m_markers[markerId].isDragging = true;
                return;
            }
        }

        if (abs(m_dragStartPosWithoutOffset.x() - m_nameAreaWidth) < 3 ||
                abs(m_dragStartPosWithoutOffset.x() - (m_nameAreaWidth + m_valueAreaWidth)) < 3)
        {
            m_resizingAreas = true;
            m_resizeStartX = m_dragStartPosWithoutOffset.x();
            m_nameAreaStartWidth = m_nameAreaWidth;
            m_valueAreaStartWidth = m_valueAreaWidth;
            return;
        }
        if (m_dragStartPosWithoutOffset.x() < (m_nameAreaWidth + m_valueAreaWidth) &&
                m_dragStartPosWithoutOffset.y() > m_timeRulerHeight)
        {
            int groupIndex = getGroupIndexAtY(m_dragStartPosWithOffset.y());
            if (groupIndex >= 0)
            {
                QString groupName = m_groupOrder[groupIndex];
                int groupY = getGroupYPosition(groupName);
                if (m_dragStartPosWithoutOffset.x() < 15 && m_dragStartPosWithOffset.y() >= groupY &&
                        m_dragStartPosWithOffset.y() < groupY + m_groupHeaderHeight)
                {
                    setGroupCollapsed(groupName, !isGroupCollapsed(groupName));
                    updateScrollBars();
                    return;
                }

                if (m_dragStartPosWithOffset.y() >= groupY && m_dragStartPosWithOffset.y() < groupY + m_groupHeaderHeight)
                {
                    m_isPotentialDragGroupName = true;
                    return;
                }
            }

            int signalIndex = getViewSignalIndexInGroupAtY(m_dragStartPosWithOffset.y());
            if (signalIndex >= 0)
            {
                m_isPotentialDragSignalName = true;
                handleSignalSelection(event);
                return;
            }
        }
        if (m_dragStartPosWithOffset.y() > m_timeRulerHeight &&
                m_dragStartPosWithOffset.x() > (m_nameAreaWidth + m_valueAreaWidth))
        {
            m_isPotentialDragSignalLine = true;
            m_dragStartTime = xToTime(m_dragStartPosWithOffset.x());
            m_clickTime = NULL;
            m_selectTime = NULL;
            return;
        }
    }
    else if (event->button() == Qt::MiddleButton)
    {
        if (m_dragStartPosWithOffset.x() <= m_nameAreaWidth)
        {
            m_currentSignalPosition = getViewSignalSpacingIndexAndGroupAtY(m_dragStartPosWithOffset.y(), m_currentGroupPosition, m_currentSignalPositionInGroup, m_currentPositionGroupName);
            if (m_currentSignalPosition > -2)
            {
                m_isPressSignalSpacing = true;
                m_lastSelectedSignalKey = "";
            }
            else
            {
                clearCurrentPosition();
            }
        }
        else if (m_dragStartPosWithOffset.x() >= m_nameAreaWidth + m_valueAreaWidth)
        {
            if (xToTime(m_dragStartPosWithOffset.x()) == m_middlePressClickTime)
            {
                m_middlePressClickTime = 0;
            }
            else
            {
                m_middlePressClickTime = xToTime(m_dragStartPosWithOffset.x());
            }
        }
        else
        {
            clearCurrentPosition();
        }
    }
}

void WaveformDisplay::clearCurrentPosition()
{
    m_currentGroupPosition = -2;
    m_currentSignalPosition = -2;
    m_currentSignalPositionInGroup = -2;
    m_currentPositionGroupName = "";
    m_isPressSignalSpacing = false;
}

QStringList WaveformDisplay::getAllSignalsInDisplayOrder() const
{
    QStringList allSignals;

    for (const QString &signalKey : m_pinTopSignals)
    {
        allSignals.append(signalKey);
    }

    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
            continue;
        const SignalGroup &group = m_groups[groupName];
        if (!group.collapsed)
        {
            for (const QString &signalKey : group.v_signals)
            {
                if (m_signals.contains(signalKey) && m_signals[signalKey].visible)
                {
                    allSignals.append(signalKey);
                }
            }
        }
    }

    return allSignals;
}

void WaveformDisplay::sortSelectedSignalsByDisplayOrder(const QStringList &displayOrder)
{
    QList<QString> sortedSelection;
    for (const QString &signalKey : displayOrder)
    {
        if (m_selectedSignalKeys.contains(signalKey))
        {
            sortedSelection.append(signalKey);
        }
    }
    m_selectedSignalKeys = sortedSelection;
}

void WaveformDisplay::handleSignalSelection(QMouseEvent *event, bool isPinTop)
{
    QPoint adjustedPos = event->pos();
    if (!isPinTop)
    {
        adjustedPos.setY(adjustedPos.y() + m_verticalOffset);
        adjustedPos.setX(adjustedPos.x() + m_horizontalOffset);
    }
    QString clickedSignalKey;
    clickedSignalKey = getSignalKeyInTopArea(adjustedPos.y());

    if (clickedSignalKey.isEmpty())
    {
        clickedSignalKey = getSignalKeyInScrollerArea(adjustedPos.y());
    }
    else
    {
        m_originalTopSignalKey = clickedSignalKey;
    }

    if (clickedSignalKey.isEmpty())
        return;

    QStringList displayOrder = getAllSignalsInDisplayOrder();

    switch (m_selectionMode)
    {
    case SingleSelection:
        if (!m_selectedSignalKeys.contains(clickedSignalKey))
        {
            m_selectedSignalKeys.clear();
            m_selectedSignalKeys.append(clickedSignalKey);
            sortSelectedSignalsByDisplayOrder(displayOrder);
            m_lastSelectedSignalKey = clickedSignalKey;
        }
        break;
    case CtrlMultiSelection:
        if (m_selectedSignalKeys.contains(clickedSignalKey))
        {
            m_selectedSignalKeys.removeAll(clickedSignalKey);
        }
        else
        {
            m_selectedSignalKeys.append(clickedSignalKey);
            sortSelectedSignalsByDisplayOrder(displayOrder);
        }
        m_lastSelectedSignalKey = clickedSignalKey;
        break;

    case ShiftRangeSelection:
        if (m_lastSelectedSignalKey.isEmpty())
        {
            m_selectedSignalKeys.clear();
            m_selectedSignalKeys.append(clickedSignalKey);
            sortSelectedSignalsByDisplayOrder(displayOrder);
            m_lastSelectedSignalKey = clickedSignalKey;
        }
        else
        {
            QStringList allSignals = getAllSignalsInDisplayOrder();
            int startIndex = allSignals.indexOf(m_lastSelectedSignalKey);
            int endIndex = allSignals.indexOf(clickedSignalKey);
            if (startIndex == -1 || endIndex == -1)
            {
                m_selectedSignalKeys.clear();
                m_selectedSignalKeys.append(clickedSignalKey);
                sortSelectedSignalsByDisplayOrder(displayOrder);
                m_lastSelectedSignalKey = clickedSignalKey;
            }
            else
            {
                if (startIndex > endIndex)
                {
                    std::swap(startIndex, endIndex);
                }
                if (!m_selectedSignalKeys.contains(clickedSignalKey))
                {
                    for (int i = startIndex; i <= endIndex; i++)
                    {
                        m_selectedSignalKeys.append(allSignals[i]);
                        sortSelectedSignalsByDisplayOrder(displayOrder);
                    }
                }
                else
                {
                    for (int i = startIndex; i <= endIndex; i++)
                    {
                        m_selectedSignalKeys.removeAll(allSignals[i]);
                    }
                }
            }
            m_lastSelectedSignalKey = clickedSignalKey;
        }
        break;
    }

    update();
}

void WaveformDisplay::performDrag(const QString &dragType)
{
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    if (dragType == "signal")
    {
        QString groupName;
        QString signalKey = getSignalKeyInScrollerArea(m_dragStartPosWithOffset.y());
        if (signalKey.isEmpty())
        {
            return;
        }

        QString fullPath = join_path_with_dot(m_signals[signalKey].scopes) + "." + m_signals[signalKey].signal_name;
        mimeData->setData("application/waveform_display-signal-full-path-data", fullPath.toUtf8());
    }
    else if (dragType == "group")
    {
        mimeData->setData("application/waveform_display-group-move-data", "");
    }
    else if (dragType == "topSignal")
    {
        if (m_originalTopSignalKey != "")
        {
            QString fullPath = join_path_with_dot(m_signals[m_originalTopSignalKey].scopes) + "." + m_signals[m_originalTopSignalKey].signal_name;
            mimeData->setData("application/waveform_display-topSignal-move-data", fullPath.toUtf8());
        }
    }

    drag->setMimeData(mimeData);

    QPixmap pixmap(":/icons/icons/drag-hand.png");
    pixmap = pixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    drag->setPixmap(pixmap);

    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

    Qt::DropAction dropAction = drag->exec(Qt::MoveAction | Qt::CopyAction);
    m_draggingSignalName = false;
    m_draggingSignalName = false;
    update();
}

void WaveformDisplay::dragEnterEvent(QDragEnterEvent *event)
{

    if (event->mimeData()->hasFormat("application/waveform_display-signal-full-path-data") ||
            event->mimeData()->hasFormat("application/waveform_display-group-move-data") ||
            event->mimeData()->hasFormat("application/texteditor") ||
            event->mimeData()->hasFormat("application/waveform_display-topSignal-move-data"))
    {

        event->accept();
    }
    else
    {
        event->ignore();
    }
}
void WaveformDisplay::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/waveform_display-signal-full-path-data") ||
            event->mimeData()->hasFormat("application/waveform_display-group-move-data") ||
            event->mimeData()->hasFormat("application/texteditor") ||
            event->mimeData()->hasFormat("application/waveform_display-topSignal-move-data"))
    {
        event->accept();
        clearCurrentPosition();
        if (m_draggingSignalName || m_draggingGroupName || m_isDraggingSignalInTopArea)
        {
            m_dragCurrentY = event->pos().y() + m_verticalOffset;
            m_dragCurrentX = event->pos().x() + m_horizontalOffset;

            m_dragCurrentYWithoutOffset = event->pos().y();
            m_dragCurrentXWithoutOffset = event->pos().x();

            update();
            return;
        }
        if (event->mimeData()->hasFormat("application/texteditor"))
        {
            m_addingSignalFromEditor = true;
            m_dragCurrentY = event->pos().y() + m_verticalOffset;
            m_dragCurrentX = event->pos().x() + m_horizontalOffset;

            update();
            return;
        }
    }
    else
    {
        event->ignore();
    }
}

void WaveformDisplay::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/waveform_display-signal-full-path-data") ||
            event->mimeData()->hasFormat("application/waveform_display-group-move-data") ||
            event->mimeData()->hasFormat("application/texteditor") ||
            event->mimeData()->hasFormat("application/waveform_display-topSignal-move-data"))
    {
        event->accept();

        if (m_draggingSignalName)
        {
            QString targetGroupName = getSignalGroupNameAtY(m_dragCurrentY);
            QString targetSignalKey = getSignalKeyInScrollerArea(m_dragCurrentY);
            int topBottomY = getTopPinSignalsAreaBottomY();

            if (!targetGroupName.isEmpty() && targetSignalKey.isEmpty() && topBottomY < m_dragCurrentYWithoutOffset)
            {
                if (!m_selectedSignalKeys.isEmpty())
                {
                    QList<QString> signalsToRemove = m_selectedSignalKeys;
                    for (const QString &signalKey : signalsToRemove)
                    {
                        if (m_signals.contains(signalKey))
                        {
                            removeSignal(signalKey, false);
                        }
                    }
                }
                int targetSignalIndex = 0;
                for (const QString &signalKey : m_selectedSignalKeys)
                {
                    if (m_signals.contains(signalKey))
                    {
                        m_groups[targetGroupName].v_signals.insert(targetSignalIndex, signalKey);
                        targetSignalIndex++;
                    }
                }
            }
            else if (!targetSignalKey.isEmpty() && topBottomY < m_dragCurrentYWithoutOffset)
            {
                if (!m_selectedSignalKeys.isEmpty())
                {
                    QList<QString> signalsToRemove = m_selectedSignalKeys;
                    for (const QString &signalKey : signalsToRemove)
                    {
                        if (m_signals.contains(signalKey))
                        {
                            removeSignal(signalKey, false);
                        }
                    }
                }
                int targetSignalIndex = m_groups[targetGroupName].v_signals.indexOf(targetSignalKey);
                for (const QString &signalKey : m_selectedSignalKeys)
                {
                    if (m_signals.contains(signalKey))
                    {
                        m_groups[targetGroupName].v_signals.insert(targetSignalIndex + 1, signalKey);
                        targetSignalIndex++;
                    }
                }
            }

            m_draggingSignalName = false;
            return;
        }

        if (m_draggingGroupName)
        {
            m_draggingGroupName = false;

            QPoint adjustedPos = event->pos();
            adjustedPos.setY(adjustedPos.y() + m_verticalOffset);
            int targetIndex = getGroupIndexAtY(adjustedPos.y());
            if (targetIndex >= 0 && targetIndex != m_originalGroupIndex)
            {

                updateGroupOrder(m_originalGroupIndex, targetIndex);
            }
            update();

            return;
        }

        if (m_addingSignalFromEditor)
        {
            m_addingSignalFromEditor = false;
            QString targetGroupName = getSignalGroupNameAtY(m_dragCurrentY);
            QString targetSignalKey = getSignalKeyInScrollerArea(m_dragCurrentY);
            int targetSignalIndex = getViewSignalIndexInGroupAtY(m_dragCurrentY);
            m_currentSignalPositionInGroup = targetSignalIndex;
            if (!targetGroupName.isEmpty())
            {
                QString signalFullPath = event->mimeData()->data("application/texteditor");
                QStringList signalList = signalFullPath.split(',', Qt::SkipEmptyParts);
                for (const QString &signal : signalList)
                {
                    QString trimmedSignal = signal.trimmed();
                    if (!trimmedSignal.isEmpty())
                    {
                        emit addSignalFromEditorToWaveWindow(trimmedSignal, targetGroupName, targetSignalIndex);
                    }
                }
            }
            return;
        }

        if (m_isDraggingSignalInTopArea)
        {
            m_isDraggingSignalInTopArea = false;
            QPoint adjustedPos = event->pos();
            QString targetSignalKey = getSignalKeyInTopArea(adjustedPos.y());

            if (!targetSignalKey.isEmpty())
            {
                QList<QString> signalsToRemove = m_selectedSignalKeys;
                for (const QString &signalKey : signalsToRemove)
                {
                    m_pinTopSignals.removeAll(signalKey);
                }
                int targetSignalIndex = m_pinTopSignals.indexOf(targetSignalKey);
                for (const QString &signalKey : signalsToRemove)
                {
                    if (m_signals.contains(signalKey))
                    {
                        m_pinTopSignals.insert(targetSignalIndex + 1, signalKey);
                        targetSignalIndex++;
                    }
                }
            }
            update();
            return;
        }
    }
    else
    {
        event->ignore();
    }
}

void WaveformDisplay::mouseMoveEvent(QMouseEvent *event)
{
    if (!event)
    {
        qDebug() << "!mouseMoveEvent event";
        return;
    }

    if (event->pos().y() <= m_timeRulerHeight &&
            event->pos().x() >= m_nameAreaWidth + m_valueAreaWidth)
    {
        QPoint positionWithOffset = event->pos();
        positionWithOffset.setX(positionWithOffset.x() + m_horizontalOffset);

        int markerId = getMarkerAtPosition(positionWithOffset);
        if (markerId != m_hoveredMarkerId)
        {
            m_hoveredMarkerId = markerId;
            update();
        }
    }
    else if (m_hoveredMarkerId != -1)
    {
        m_hoveredMarkerId = -1;
        update();
    }

    if (m_draggingMarkerId != -1 && m_markers.contains(m_draggingMarkerId) && m_markers[m_draggingMarkerId].isDragging)
    {
        QPoint currentPos = event->pos();
        Time deltaTime = xToTime(currentPos.x() + m_horizontalOffset) - m_markerDragStartTime;
        double newTime = m_markerBeforeDragTime + deltaTime;

        newTime = qMax(m_globalMinTime, qMin(newTime, m_globalMaxTime));

        m_markers[m_draggingMarkerId].time = newTime;
        update();
        return;
    }

    if (m_resizingAreas)
    {
        int deltaX = event->pos().x() - m_resizeStartX;

        if (abs(m_resizeStartX - m_nameAreaWidth) < 3)
        {

            m_nameAreaWidth = std::max(50 + 1, m_nameAreaStartWidth + deltaX + 1);
        }
        else if (abs(m_resizeStartX - (m_nameAreaWidth + m_valueAreaWidth)) < 3)
        {

            m_valueAreaWidth = std::max(50 + 1, m_valueAreaStartWidth + deltaX + 1);
        }
        updateScrollBars();
        update();
        return;
    }

    if (m_isPotentialDragSignalName)
    {
        QPoint delta = event->pos() - m_dragStartPosWithoutOffset;
        if (delta.manhattanLength() >= DRAG_THRESHOLD)
        {
            m_isPotentialDragSignalName = false;
            m_draggingSignalName = true;
            performDrag("signal");
        }
    }

    if (m_isPotentialDragGroupName)
    {
        QPoint delta = event->pos() - m_dragStartPosWithoutOffset;

        if (delta.manhattanLength() >= DRAG_THRESHOLD)
        {
            m_isPotentialDragGroupName = false;
            m_draggingGroupName = true;

            int groupIndex = getGroupIndexAtY(m_dragStartPosWithOffset.y());
            if (groupIndex > -1)
            {
                QString groupName = m_groupOrder[groupIndex];
                m_dragStartY = m_dragStartPosWithOffset.y();
                m_originalGroupIndex = groupIndex;
                m_draggedGroupName = groupName;
                performDrag("group");
            }
        }
    }
    if (m_isPotentialDragSignalInTopArea)
    {
        QPoint delta = event->pos() - m_dragStartPosWithoutOffset;

        if (delta.manhattanLength() >= DRAG_THRESHOLD)
        {
            m_isPotentialDragSignalInTopArea = false;
            m_isDraggingSignalInTopArea = true;

            QString signalKey = getSignalKeyInTopArea(m_dragStartPosWithOffset.y());
            if (!signalKey.isEmpty())
            {
                m_originalSignalIndex = m_pinTopSignals.indexOf(signalKey);
                m_dragStartY = m_dragStartPosWithOffset.y();
                performDrag("topSignal");
            }
        }
    }

    if (m_isPotentialDragSignalLine)
    {
        QPoint delta = event->pos() - m_dragStartPosWithoutOffset;
        if (delta.manhattanLength() >= DRAG_THRESHOLD)
        {
            m_isPotentialDragSignalLine = false;
            if (abs(delta.x()) > abs(delta.y()))
            {
                m_horizontalDragging = true;
                QPoint pos = event->pos();
                int topBottomY = getTopPinSignalsAreaBottomY();
                if (pos.y() < topBottomY)
                {
                    QString targetSignalKey = getSignalKeyInTopArea(pos.y());
                    if (!targetSignalKey.isEmpty())
                    {
                        int targetSignalIndex = m_pinTopSignals.indexOf(targetSignalKey);
                        m_horizontalDraggingLineStartPositionY = m_timeRulerHeight + targetSignalIndex * (m_signalHeight + m_signalSpacing) + m_verticalOffset;
                    }
                }
                else if (pos.y() >= topBottomY)
                {
                    pos.setY(pos.y() + m_verticalOffset);
                    pos.setX(pos.x() + m_horizontalOffset);
                    m_dragEndTime = xToTime(pos.x());

                    int groupIndex = 0;
                    int currentSignalPositionInGroup = -1;
                    QString currentPositionGroupName = "";
                    int currentSignalPosition = getViewSignalSpacingIndexAndGroupAtY(pos.y(), groupIndex, currentSignalPositionInGroup, currentPositionGroupName);
                    m_horizontalDraggingLineStartPositionY = getTopPinSignalsAreaBottomY() + (m_groupHeaderHeight + m_signalSpacing) * groupIndex + (currentSignalPosition + 1) * (m_signalHeight + m_signalSpacing);
                }
                update();
                return;
            }
            else if (abs(delta.y()) > abs(delta.x()))
            {
                m_verticalDragging = true;
                m_verticalDragDistance = delta.y();
                m_verticalHintText = "zoom out";
                update();
                return;
            }
        }
        return;
    }

    if (m_horizontalDragging || m_verticalDragging)
    {
        QPoint delta = event->pos() - m_dragStartPosWithoutOffset;
        if (m_horizontalDragging)
        {
            if (abs(delta.x()) < DRAG_THRESHOLD)
            {
                m_horizontalDragging = false;
                m_isPotentialDragSignalLine = true;
                return;
            }
        }
        if (m_verticalDragging)
        {
            if (abs(delta.y()) < DRAG_THRESHOLD)
            {
                m_verticalDragging = false;
                m_isPotentialDragSignalLine = true;
                return;
            }
        }
    }
    if (m_horizontalDragging)
    {

        QPoint currentAdjustedPos = event->pos();
        currentAdjustedPos.setY(currentAdjustedPos.y() + m_verticalOffset);
        currentAdjustedPos.setX(currentAdjustedPos.x() + m_horizontalOffset);
        m_dragEndTime = xToTime(currentAdjustedPos.x());
        update();
        return;
    }
    if (m_verticalDragging)
    {

        QPoint delta = event->pos() - m_dragStartPosWithoutOffset;
        m_verticalDragDistance = delta.y();
        update();
        return;
    }
}
void WaveformDisplay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {

        if (m_draggingMarkerId != -1)
        {
            if (m_markers.contains(m_draggingMarkerId))
            {
                m_markers[m_draggingMarkerId].isDragging = false;
            }
            m_draggingMarkerId = -1;
            m_markerBeforeDragTime = 0;

            update();
        }
        if (m_resizingAreas)
        {
            m_resizingAreas = false;

            return;
        }
        if (m_isPotentialDragGroupName)
        {
            m_isPotentialDragGroupName = false;
            return;
        }
        if (m_isPotentialDragSignalName)
        {

            m_isPotentialDragSignalName = false;
            QPoint adjustedPos = event->pos();
            adjustedPos.setY(adjustedPos.y() + m_verticalOffset);
            adjustedPos.setX(adjustedPos.x() + m_horizontalOffset);
            // m_selectedSignalKeys.size()
            int groupIndex = 0;
            int currentSignalPositionInGroup = -1;
            QString currentPositionGroupName = "";
            int currentSignalPosition = getViewSignalSpacingIndexAndGroupAtY(adjustedPos.y(), groupIndex, currentSignalPositionInGroup, currentPositionGroupName);

            if (currentSignalPosition > -1)
            {
                m_lastSelectedSignalKey = m_groups[currentPositionGroupName].v_signals[currentSignalPositionInGroup];
                update();
            }
        }

        if (m_isPotentialDragSignalInTopArea)
        {
            m_isPotentialDragSignalInTopArea = false;
            QPoint adjustedPos = event->pos();
            adjustedPos.setY(adjustedPos.y() + m_verticalOffset);
            adjustedPos.setX(adjustedPos.x() + m_horizontalOffset);

            QString signalKey = getSignalKeyInTopArea(m_dragStartPosWithoutOffset.y());
            if (!signalKey.isEmpty())
            {
                m_lastSelectedSignalKey = signalKey;
                update();
            }
        }
        if (m_isPotentialDragSignalLine)
        {

            m_isPotentialDragSignalLine = false;

            Time clickTime = xToTime(m_dragStartPosWithOffset.x());
            m_clickTime = clickTime;
            setSelectTimeWithDistance();
            ensureCursorVisible();

            m_showClickIndicator = true;
            m_clickPosition = m_dragStartPosWithOffset;
            if (m_selectTime)
            {
                emit timeValueChanged(m_selectTime);
                emit timeChangeForGetSignals();
            } else {
                emit timeValueChanged(clickTime);
                emit timeChangeForGetSignals();
            }

            update();
        }
        else if (m_horizontalDragging)
        {

            m_horizontalDragging = false;

            Time startTime = std::min(m_dragStartTime, m_dragEndTime);
            Time endTime = std::max(m_dragStartTime, m_dragEndTime);

            setTimeRange(startTime, endTime);
            updateScrollBars();
            update();
        }
        else if (m_verticalDragging)
        {
            m_verticalDragging = false;
            setTimeRange(m_globalMinTime, m_globalMaxTime);
            updateScrollBars();
            update();
        }
    }
}
void WaveformDisplay::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {

        QPoint doubleClickPos = event->pos();
        QPoint doubleClickPosWithOffset = event->pos();
        doubleClickPosWithOffset.setY(doubleClickPosWithOffset.y() + m_verticalOffset);

        if (abs(doubleClickPos.x() - m_nameAreaWidth) < 3 || abs(doubleClickPos.x() - (m_nameAreaWidth + m_valueAreaWidth)) < 3)
        {
            return;
        }

        if (doubleClickPos.y() > m_timeRulerHeight)
        {

            int groupIndex = getGroupIndexAtY(doubleClickPosWithOffset.y());
            if (groupIndex >= 0)
            {
                return;
            }

            QString groupName = getSignalGroupNameAtY(doubleClickPosWithOffset.y());
            int signalIndex = getRealSignalIndexInGroupAtY(doubleClickPosWithOffset.y());

            if (signalIndex >= 0)
            {
                m_lastSelectedSignalKey = m_groups[groupName].v_signals[signalIndex];
                if (doubleClickPos.x() < m_nameAreaWidth)
                {
                    m_currentSignalPositionInGroup = signalIndex;
                    m_currentPositionGroupName = groupName;
                    expandMultiArray(m_signals[m_lastSelectedSignalKey], groupName, signalIndex);
                    m_currentSignalPositionInGroup = -2;
                    m_currentPositionGroupName.clear();
                }
                if (doubleClickPos.x() >= m_nameAreaWidth + m_valueAreaWidth)
                {
                    QString fullPath = join_path_with_dot(m_signals[m_lastSelectedSignalKey].scopes) + "." + m_signals[m_lastSelectedSignalKey].signal_name;
                    emit signalDoubleClicked(fullPath);
                }
            }
        }
    }
}
void WaveformDisplay::zoomInAtPoint(double mouseX)
{

    int savedVerticalScroll = m_waveArea->verticalScrollBar()->value();

    double mouseTime = xToTime(mouseX + m_nameAreaWidth + m_valueAreaWidth + m_horizontalOffset);

    double currentRange = m_maxTime - m_minTime;
    double newRange = currentRange * 0.8;

    if (newRange < m_minZoomRange)
    {
        newRange = m_minZoomRange;
        if (newRange >= (m_globalMaxTime - m_globalMinTime))
        {
            return;
        }
    }

    double zoomFactor = currentRange / newRange;
    double newMinTime = mouseTime - (mouseTime - m_minTime) / zoomFactor;
    double newMaxTime = mouseTime + (m_maxTime - mouseTime) / zoomFactor;

    if (newMinTime < m_globalMinTime)
    {
        newMinTime = m_globalMinTime;
        newMaxTime = newMinTime + newRange;
        if (newMaxTime > m_globalMaxTime)
        {
            newMaxTime = m_globalMaxTime;
            newRange = newMaxTime - newMinTime;
        }
    }
    else if (newMaxTime > m_globalMaxTime)
    {
        newMaxTime = m_globalMaxTime;
        newMinTime = newMaxTime - newRange;
        if (newMinTime < m_globalMinTime)
        {
            newMinTime = m_globalMinTime;
            newRange = newMaxTime - newMinTime;
        }
    }

    if (newMinTime >= newMaxTime || newRange < m_minZoomRange)
    {
        return;
    }
    setTimeRange(newMinTime, newMaxTime);
    updateScrollBars();

    m_waveArea->verticalScrollBar()->setValue(savedVerticalScroll);
}
void WaveformDisplay::zoomOutAtPoint(double mouseX)
{

    int savedVerticalScroll = m_waveArea->verticalScrollBar()->value();

    double mouseTime = xToTime(mouseX + m_nameAreaWidth + m_valueAreaWidth + m_horizontalOffset);

    double currentRange = m_maxTime - m_minTime;
    double newRange = currentRange * 1.25;

    if (newRange > (m_globalMaxTime - m_globalMinTime))
    {
        newRange = m_globalMaxTime - m_globalMinTime;
    }

    double zoomFactor = newRange / currentRange;
    double newMinTime = mouseTime - (mouseTime - m_minTime) * zoomFactor;
    double newMaxTime = mouseTime + (m_maxTime - mouseTime) * zoomFactor;

    if (newMinTime < m_globalMinTime)
    {
        double offset = m_globalMinTime - newMinTime;
        newMinTime = m_globalMinTime;
        newMaxTime += offset;
        if (newMaxTime > m_globalMaxTime)
        {
            newMaxTime = m_globalMaxTime;
            newMinTime = newMaxTime - newRange;
            if (newMinTime < m_globalMinTime)
            {
                newMinTime = m_globalMinTime;
                newRange = newMaxTime - newMinTime;
            }
        }
    }
    else if (newMaxTime > m_globalMaxTime)
    {
        double offset = newMaxTime - m_globalMaxTime;
        newMaxTime = m_globalMaxTime;
        newMinTime -= offset;
        if (newMinTime < m_globalMinTime)
        {
            newMinTime = m_globalMinTime;
            newRange = newMaxTime - newMinTime;
        }
    }

    if (newMinTime >= newMaxTime || newRange < m_minZoomRange)
    {
        return;
    }

    m_waveArea->verticalScrollBar()->setValue(savedVerticalScroll);
    setTimeRange(newMinTime, newMaxTime);
    updateScrollBars();
}
bool WaveformDisplay::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);

        if (obj == m_waveArea->viewport() || obj == m_waveArea->verticalScrollBar())
        {

            if (wheelEvent->modifiers() & Qt::ControlModifier)
            {
                QPoint numDegrees = wheelEvent->angleDelta() / 8;
                if (!numDegrees.isNull())
                {
                    if (numDegrees.y() > 0)
                    {
                        zoomInAtPoint(wheelEvent->position().x());
                    }
                    else
                    {
                        zoomOutAtPoint(wheelEvent->position().x());
                    }
                }
                return true;
            }

            return QWidget::eventFilter(obj, event);
        }
    }
    if (m_highlightDialog && obj == m_highlightDialog)
    {
        if (event->type() == QEvent::Move)
        {
            QMoveEvent *moveEvent = static_cast<QMoveEvent *>(event);
            m_highlightDialogOldPos = moveEvent->pos();
        }
    }

    return QWidget::eventFilter(obj, event);
}
void WaveformDisplay::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (m_hoveredMarkerId != -1)
    {
        m_hoveredMarkerId = -1;
        update();
    }
    m_hoverSignal.clear();
    update();
}
void WaveformDisplay::goToMarker(int markerId)
{
    if (m_markers.contains(markerId))
    {
        Marker marker = m_markers[markerId];
        setSelectTime(marker.time);
        ensureCursorVisible();
        update();
    }
}
void WaveformDisplay::updateGoToMarkerMenu()
{
    if (!m_goToMarkerMenu)
        return;

    m_goToMarkerMenu->clear();
    if (m_markers.isEmpty())
    {
        QAction *noMarkersAction = new QAction(tr("No markers available"), this);
        noMarkersAction->setEnabled(false);
        m_goToMarkerMenu->addAction(noMarkersAction);
        return;
    }

    QList<Marker> sortedMarkers = m_markers.values();
    std::sort(sortedMarkers.begin(), sortedMarkers.end(),
              [](const Marker &a, const Marker &b)
    { return a.time < b.time; });

    for (const auto &marker : sortedMarkers)
    {
        if (!marker.visible)
            continue;
        QString actionText = QString("%1 (Time: %2)")
                .arg(marker.name)
                .arg(marker.time, 0, 'f', 3);
        QAction *markerAction = new QAction(actionText, this);
        markerAction->setData(marker.id);

        QPixmap pixmap(16, 16);
        pixmap.fill(marker.color);
        markerAction->setIcon(QIcon(pixmap));
        connect(markerAction, &QAction::triggered, this, [this, marker]()
        { goToMarker(marker.id); });
        m_goToMarkerMenu->addAction(markerAction);
    }
}
void WaveformDisplay::contextMenuEvent(QContextMenuEvent *event)
{

    if (!m_contextMenu)
    {
        createContextMenu();
    }
    if (!m_contextMenu)
    {
        qDebug() << "Context menu creation failed";
        return;
    }
    QPoint pos = event->pos();
    QPoint adjustedPos = pos;

    bool isInTopArea = (pos.y() <= getTopPinSignalsAreaBottomY());
    if (isInTopArea)
    {
        adjustedPos.setY(pos.y());
    }
    else
    {
        adjustedPos.setY(pos.y() + m_verticalOffset);
    }

    resetContextMenuVisibility();

    ContextMenuType menuType = determineContextMenuType(adjustedPos, isInTopArea);

    setupContextMenuForType(menuType, adjustedPos, isInTopArea);

    bool hasVisibleItems = false;
    QList<QAction *> actions = m_contextMenu->actions();
    for (QAction *action : actions)
    {
        if (action->isVisible())
        {
            hasVisibleItems = true;
            break;
        }
    }
    if (hasVisibleItems)
    {

        m_contextMenu->exec(event->globalPos());
    }
    else
    {
        qDebug() << "No visible items in context menu, not showing";
    }
}
void WaveformDisplay::resetContextMenuVisibility()
{

    if (m_addGroupAction)
        m_addGroupAction->setVisible(false);
    if (m_renameGroupAction)
        m_renameGroupAction->setVisible(false);
    if (m_removeGroupAction)
        m_removeGroupAction->setVisible(false);
    if (m_collapseGroupAction)
        m_collapseGroupAction->setVisible(false);
    if (m_expandGroupAction)
        m_expandGroupAction->setVisible(false);

    if (m_removeSignalAction)
        m_removeSignalAction->setVisible(false);
    if (m_copyFullPathAction)
        m_copyFullPathAction->setVisible(false);
    if (m_logicalOperationAction)
        m_logicalOperationAction->setVisible(false);
    if (m_copyValueAction)
        m_copyValueAction->setVisible(false);
    if (m_highlightAction)
        m_highlightAction->setVisible(false);
    if (m_pinTopAction)
        m_pinTopAction->setVisible(false);
    if (m_unpinnedSignalAction)
        m_unpinnedSignalAction->setVisible(false);

    if (m_saveGroupAction)
        m_saveGroupAction->setVisible(false);
    if (m_loadGroupAction)
        m_loadGroupAction->setVisible(false);

    if (m_zoomInAction)
        m_zoomInAction->setVisible(false);
    if (m_zoomOutAction)
        m_zoomOutAction->setVisible(false);
    if (m_zoomAllAction)
        m_zoomAllAction->setVisible(false);
    if (m_detailAction)
        m_detailAction->setVisible(false);

    if (m_addMarkerAction)
        m_addMarkerAction->setVisible(false);

    if (m_radixMenu)
        m_radixMenu->menuAction()->setVisible(false);
    if (m_hierarchicalNameAction)
        m_hierarchicalNameAction->setVisible(false);

    if (m_markerMenu)
    {
        m_markerMenu->menuAction()->setVisible(false);
    }

    clearDynamicMenuActions();
}
void WaveformDisplay::clearDynamicMenuActions()
{
    if (!m_contextMenu)
        return;

    QList<QAction *> actions = m_contextMenu->actions();
    QList<QAction *> actionsToRemove;

    for (QAction *action : actions)
    {
        bool isPredefined = false;

        if (action == m_addGroupAction || action == m_renameGroupAction || action == m_removeGroupAction ||
                action == m_collapseGroupAction || action == m_expandGroupAction ||
                action == m_removeSignalAction || action == m_copyFullPathAction ||
                action == m_copyValueAction || action == m_highlightAction ||
                action == m_detailAction || action == m_logicalOperationAction ||
                action == m_pinTopAction || action == m_unpinnedSignalAction ||
                action == m_saveGroupAction || action == m_loadGroupAction ||
                action == m_zoomInAction || action == m_zoomOutAction ||
                action == m_zoomAllAction || action == m_addMarkerAction ||
                (m_radixMenu && action == m_radixMenu->menuAction()) ||
                action == m_hierarchicalNameAction ||
                (m_markerMenu && action == m_markerMenu->menuAction()))
        {
            isPredefined = true;
        }

        if (action->isSeparator() || !isPredefined)
        {
            actionsToRemove.append(action);
        }
    }

    for (QAction *action : actionsToRemove)
    {
        m_contextMenu->removeAction(action);

        if (!action->isSeparator())
        {
            delete action;
        }
    }
}
WaveformDisplay::ContextMenuType WaveformDisplay::determineContextMenuType(const QPoint &adjustedPos, bool isInTopArea)
{

    int groupIndex = getGroupIndexAtY(adjustedPos.y());
    if (groupIndex >= 0 && groupIndex < m_groupOrder.size())
    {
        return ContextMenuType::GroupHeader;
    }
    int signalIndex = -1;
    if (isInTopArea)
    {
        QString signalKey = getSignalKeyInTopArea(adjustedPos.y());
        if (!signalKey.isEmpty())
        {
            return ContextMenuType::TopSignal;
        }
    }
    else
    {
        signalIndex = getViewSignalIndexInGroupAtY(adjustedPos.y());
        if (signalIndex >= 0)
        {
            return ContextMenuType::ScrollableSignal;
        }
    }

    if (adjustedPos.y() <= m_timeRulerHeight &&
            adjustedPos.x() >= m_nameAreaWidth + m_valueAreaWidth)
    {
        return ContextMenuType::TimeRuler;
    }

    if (adjustedPos.y() > m_timeRulerHeight &&
            adjustedPos.x() >= m_nameAreaWidth + m_valueAreaWidth)
    {
        return ContextMenuType::WaveformArea;
    }

    if (adjustedPos.x() < m_nameAreaWidth + m_valueAreaWidth)
    {
        return ContextMenuType::NameValueArea;
    }

    return ContextMenuType::BlankArea;
}
void WaveformDisplay::setupBlankAreaMenu(const QPoint &adjustedPos, bool isInTopArea)
{
    qDebug() << "setupBlankAreaMenu";
    Q_UNUSED(adjustedPos)

    m_addGroupAction->setVisible(true);
    m_expandGroupAction->setVisible(true);
    m_hierarchicalNameAction->setVisible(true);

    m_saveGroupAction->setVisible(true);
    m_loadGroupAction->setVisible(true);

    m_contextMenu->addSeparator();

    if (isInTopArea)
    {
        QAction *manageTopSignalsAction = new QAction(tr("Manage Pinned Signals"), this);
        connect(manageTopSignalsAction, &QAction::triggered, this, [this]()
        { QMessageBox::information(this, tr("Manage Pinned Signals"),
                                   tr("Pinned signals management dialog would open here.")); });
        m_contextMenu->addAction(manageTopSignalsAction);
    }
}
void WaveformDisplay::setupContextMenuForType(ContextMenuType menuType, const QPoint &adjustedPos, bool isInTopArea)
{
    switch (menuType)
    {
    case ContextMenuType::GroupHeader:
        setupGroupHeaderMenu(adjustedPos);
        break;
    case ContextMenuType::TopSignal:
        setupTopSignalMenu(adjustedPos);
        break;
    case ContextMenuType::ScrollableSignal:
        setupScrollableSignalMenu(adjustedPos);
        break;
    case ContextMenuType::TimeRuler:
        setupTimeRulerMenu(adjustedPos);
        break;
    case ContextMenuType::WaveformArea:
        setupWaveformAreaMenu(adjustedPos);
        break;
    case ContextMenuType::NameValueArea:
        setupNameValueAreaMenu(adjustedPos, isInTopArea);
        break;
    case ContextMenuType::BlankArea:
        setupBlankAreaMenu(adjustedPos, isInTopArea);
        break;
    case ContextMenuType::General:
        setupGeneralMenu(adjustedPos);
        break;
    }
}
void WaveformDisplay::setupGroupHeaderMenu(const QPoint &adjustedPos)
{
    int groupIndex = getGroupIndexAtY(adjustedPos.y());
    if (groupIndex >= 0 && groupIndex < m_groupOrder.size())
    {
        m_contextMenuGroup = m_groupOrder[groupIndex];
        m_renameGroupAction->setVisible(true);
        m_removeGroupAction->setVisible(true);
        m_collapseGroupAction->setVisible(true);
        m_collapseGroupAction->setText(
                    isGroupCollapsed(m_contextMenuGroup) ? tr("UnFold Group") : tr("Fold Group"));

        m_contextMenu->addSeparator();
        m_addGroupAction->setVisible(true);
        m_expandGroupAction->setVisible(true);
        m_hierarchicalNameAction->setVisible(true);
    }
}
void WaveformDisplay::setupTopSignalMenu(const QPoint &adjustedPos)
{
    QString signalKey = getSignalKeyInTopArea(adjustedPos.y());
    if (!signalKey.isEmpty())
    {
        m_lastSelectedSignalKey = signalKey;
        setupSignalSpecificMenuItems();
        m_unpinnedSignalAction->setVisible(true);
        m_radixMenu->menuAction()->setVisible(true);

        m_contextMenu->addSeparator();
        m_addGroupAction->setVisible(true);
        m_hierarchicalNameAction->setVisible(true);
    }
}
void WaveformDisplay::setupScrollableSignalMenu(const QPoint &adjustedPos)
{
    QString signalKey = getSignalKeyInScrollerArea(adjustedPos.y());
    if (!signalKey.isEmpty())
    {
        m_lastSelectedSignalKey = signalKey;
        setupSignalSpecificMenuItems();
        m_pinTopAction->setVisible(true);
        m_radixMenu->menuAction()->setVisible(true);

        updateRadixMenu();

        m_addMarkerAction->setVisible(true);
        m_addMarkerAction->disconnect();
        connect(m_addMarkerAction, &QAction::triggered, this, [this]()
        {
            if(m_selectTime) {
                addMarkerAtSpecificTime(m_selectTime);
            } else if(m_clickTime) {
                addMarkerAtSpecificTime(m_clickTime);
            } });

        if (m_markerMenu)
        {
            m_contextMenu->addSeparator();
            m_markerMenu->menuAction()->setVisible(true);
        }

        m_contextMenu->addSeparator();
        m_addGroupAction->setVisible(true);
        m_hierarchicalNameAction->setVisible(true);
    }
}
void WaveformDisplay::setupSignalSpecificMenuItems()
{
    m_removeSignalAction->setVisible(true);
    m_copyFullPathAction->setVisible(true);
    m_copyValueAction->setVisible(true);
    m_highlightAction->setVisible(true);
    m_detailAction->setVisible(true);
    m_logicalOperationAction->setVisible(true);
}
void WaveformDisplay::setupTimeRulerMenu(const QPoint &adjustedPos)
{

    if (m_markerMenu)
    {
        m_markerMenu->menuAction()->setVisible(true);
        updateGoToMarkerMenu();
    }

    m_addMarkerAction->setVisible(true);
    double clickTime = xToTime(adjustedPos.x() + m_horizontalOffset);

    m_addMarkerAction->disconnect();
    connect(m_addMarkerAction, &QAction::triggered, this, [this, clickTime]()
    { addMarkerAtSpecificTime(clickTime); });

    m_contextMenu->addSeparator();
    m_hierarchicalNameAction->setVisible(true);
}
void WaveformDisplay::setupWaveformAreaMenu(const QPoint &adjustedPos)
{

    m_zoomInAction->setVisible(true);
    m_zoomOutAction->setVisible(true);
    m_zoomAllAction->setVisible(true);

    m_addMarkerAction->setVisible(true);
    double clickTime = xToTime(adjustedPos.x() + m_horizontalOffset);
    m_addMarkerAction->disconnect();
    connect(m_addMarkerAction, &QAction::triggered, this, [this, clickTime]()
    { addMarkerAtSpecificTime(clickTime); });

    if (m_markerMenu)
    {
        m_contextMenu->addSeparator();
        m_markerMenu->menuAction()->setVisible(true);
    }

    m_contextMenu->addSeparator();
    m_hierarchicalNameAction->setVisible(true);
}
void WaveformDisplay::clearSearchResults()
{
    m_searchResults.clear();
    m_currentSearchIndex = -1;
    update();
    emit searchResultsUpdated(0);
}
int WaveformDisplay::searchResultCount() const
{
    return m_searchResults.size();
}
int WaveformDisplay::currentSearchIndex() const
{
    return m_currentSearchIndex;
}
void WaveformDisplay::searchSignalValue(const QString &value)
{
    if (value.trimmed().isEmpty())
    {
        clearSearchResults();
        return;
    }
    m_currentSearchType = SearchType::Value;
    m_currentSearchValue = value.trimmed();
    performValueSearch(value.trimmed());
}
void WaveformDisplay::searchSignalTransition(const QString &oldValue, const QString &newValue)
{
    if (oldValue.trimmed().isEmpty() || newValue.trimmed().isEmpty())
    {
        clearSearchResults();
        return;
    }
    m_currentSearchType = SearchType::Transition;
    m_currentSearchOldValue = oldValue.trimmed();
    m_currentSearchNewValue = newValue.trimmed();
    performTransitionSearch(oldValue.trimmed(), newValue.trimmed());
}
void WaveformDisplay::performValueSearch(const QString &value)
{
    clearSearchResults();
    if (m_signals.isEmpty())
    {
        emit searchResultsUpdated(0);
        return;
    }
    int resultCount = 0;

    for (auto it = m_signals.constBegin(); it != m_signals.constEnd(); ++it)
    {
        const QString &signalKey = it.key();
        const DisplaySignal &displaySignal = it.value();

        if (!displaySignal.visible)
        {
            continue;
        }

        auto plotData = m_waveform->to_plot_data(displaySignal.signal);
        const auto &times = plotData.first;
        const auto &values = plotData.second;
        if (times.empty() || values.empty())
        {
            continue;
        }

        const Var &var = m_waveform->get_hierarchy().get_var(displaySignal.var_ref);
        bool isMultiBit = var.signal_type.is_real || var.signal_type.is_string || var.signal_type.width > 1;

        for (size_t i = 0; i < times.size(); i++)
        {
            QString currentValue;
            if (isMultiBit)
            {

                auto signalValues = displaySignal.signal->get_signal_values();
                if (i < signalValues.size())
                {
                    currentValue = QString::fromStdString(signalValues[i]);

                    if (displaySignal.translator)
                    {
                        VariableMeta meta = m_waveform->var_to_meta(displaySignal.var_ref);
                        TranslatedValue translated = displaySignal.translator->translate(meta, currentValue);
                        currentValue = translated.value;
                    }
                }
            }
            else
            {

                currentValue = QString::number(values[i]);

                if (values[i] == 0.5)
                    currentValue = "x";
                else if (values[i] == 0.625)
                    currentValue = "z";
            }

            if (currentValue.contains(value, Qt::CaseInsensitive))
            {
                Time foundTime = times[i];
                QString description = QString("信号 %1 在时间 %2 ns 处值为 %3")
                        .arg(displaySignal.signal_name)
                        .arg(foundTime)
                        .arg(currentValue);
                m_searchResults.append(SearchResult(
                                           displaySignal.signal_name,
                                           foundTime,
                                           currentValue,
                                           description));
                resultCount++;
            }
        }
    }

    std::sort(m_searchResults.begin(), m_searchResults.end(),
              [](const SearchResult &a, const SearchResult &b)
    {
        return a.time < b.time;
    });
    if (!m_searchResults.isEmpty())
    {
        m_currentSearchIndex = 0;
        highlightSearchResult(0);
    }
    emit searchResultsUpdated(resultCount);
    update();
}
void WaveformDisplay::performTransitionSearch(const QString &oldValue, const QString &newValue)
{
    clearSearchResults();
    if (m_signals.isEmpty())
    {
        emit searchResultsUpdated(0);
        return;
    }
    int resultCount = 0;

    for (auto it = m_signals.constBegin(); it != m_signals.constEnd(); ++it)
    {
        const QString &signalKey = it.key();
        const DisplaySignal &displaySignal = it.value();
        if (!displaySignal.visible)
        {
            continue;
        }

        auto plotData = m_waveform->to_plot_data(displaySignal.signal);
        const auto &times = plotData.first;
        const auto &values = plotData.second;
        if (times.size() < 2)
        {
            continue;
        }

        const Var &var = m_waveform->get_hierarchy().get_var(displaySignal.var_ref);
        bool isMultiBit = var.signal_type.is_real || var.signal_type.is_string || var.signal_type.width > 1;

        for (size_t i = 1; i < times.size(); i++)
        {
            QString prevValueStr, currentValueStr;
            if (isMultiBit)
            {

                auto signalValues = displaySignal.signal->get_signal_values();
                if (i - 1 < signalValues.size() && i < signalValues.size())
                {
                    prevValueStr = QString::fromStdString(signalValues[i - 1]);
                    currentValueStr = QString::fromStdString(signalValues[i]);

                    if (displaySignal.translator)
                    {
                        VariableMeta meta = m_waveform->var_to_meta(displaySignal.var_ref);
                        TranslatedValue translatedPrev = displaySignal.translator->translate(meta, prevValueStr);
                        TranslatedValue translatedCurrent = displaySignal.translator->translate(meta, currentValueStr);
                        prevValueStr = translatedPrev.value;
                        currentValueStr = translatedCurrent.value;
                    }
                }
            }
            else
            {

                prevValueStr = QString::number(values[i - 1]);
                currentValueStr = QString::number(values[i]);

                if (values[i - 1] == 0.5)
                    prevValueStr = "x";
                else if (values[i - 1] == 0.625)
                    prevValueStr = "z";
                if (values[i] == 0.5)
                    currentValueStr = "x";
                else if (values[i] == 0.625)
                    currentValueStr = "z";
            }

            bool matches = false;

            if (oldValue == "*" && newValue == "*")
            {

                matches = (prevValueStr != currentValueStr);
            }
            else if (oldValue == "*")
            {

                matches = currentValueStr.contains(newValue, Qt::CaseInsensitive);
            }
            else if (newValue == "*")
            {

                matches = prevValueStr.contains(oldValue, Qt::CaseInsensitive);
            }
            else
            {

                matches = (prevValueStr.contains(oldValue, Qt::CaseInsensitive) &&
                           currentValueStr.contains(newValue, Qt::CaseInsensitive));
            }
            if (matches)
            {
                Time foundTime = times[i];
                QString description = QString("信号 %1 在时间 %2 ns 处从 %3 跳变到 %4")
                        .arg(displaySignal.signal_name)
                        .arg(foundTime)
                        .arg(prevValueStr)
                        .arg(currentValueStr);
                m_searchResults.append(SearchResult(
                                           displaySignal.signal_name,
                                           foundTime,
                                           currentValueStr,
                                           description));
                resultCount++;
            }
        }
    }

    std::sort(m_searchResults.begin(), m_searchResults.end(),
              [](const SearchResult &a, const SearchResult &b)
    {
        return a.time < b.time;
    });
    if (!m_searchResults.isEmpty())
    {
        m_currentSearchIndex = 0;
        highlightSearchResult(0);
    }
    emit searchResultsUpdated(resultCount);
    update();
}
void WaveformDisplay::goToNextSearchResult()
{
    if (m_searchResults.isEmpty())
    {
        return;
    }
    m_currentSearchIndex = (m_currentSearchIndex + 1) % m_searchResults.size();
    highlightSearchResult(m_currentSearchIndex);
    emit searchResultChanged(m_currentSearchIndex + 1, m_searchResults.size());
}
void WaveformDisplay::goToPreviousSearchResult()
{
    if (m_searchResults.isEmpty())
    {
        return;
    }
    m_currentSearchIndex = (m_currentSearchIndex - 1 + m_searchResults.size()) % m_searchResults.size();
    highlightSearchResult(m_currentSearchIndex);
    emit searchResultChanged(m_currentSearchIndex + 1, m_searchResults.size());
}
void WaveformDisplay::highlightSearchResult(int index)
{
    if (index < 0 || index >= m_searchResults.size())
    {
        return;
    }
    const SearchResult &result = m_searchResults[index];

    setSelectTime(result.time);

    QString selectedSignalKey = "";
    for (auto it = m_signals.constBegin(); it != m_signals.constEnd(); ++it)
    {
        if (it.value().signal_name == result.signalName)
        {
            selectedSignalKey = it.key();
            break;
        }
    }
    if (!selectedSignalKey.isEmpty())
    {
        m_lastSelectedSignalKey = selectedSignalKey;

        ensureSignalVisible(selectedSignalKey);
    }
    else
    {

        if (m_selectTime >= 0)
        {

            double x = timeToX(m_selectTime);

            int waveAreaLeft = m_nameAreaWidth + m_valueAreaWidth;
            int waveAreaRight = waveAreaLeft + m_waveAreaWidth;

            if (m_waveArea->verticalScrollBar()->isVisible())
            {
                waveAreaRight -= m_waveArea->verticalScrollBar()->width();
            }

            int visibleLeft = waveAreaLeft + m_horizontalOffset;
            int visibleRight = waveAreaRight + m_horizontalOffset;

            int desiredHorizontalOffset = m_horizontalOffset;

            const int margin = 50;
            if (x < visibleLeft + margin)
            {

                desiredHorizontalOffset = x - waveAreaLeft - margin;
            }
            else if (x > visibleRight - margin)
            {

                desiredHorizontalOffset = x - waveAreaRight + margin;
            }

            double timeRange = m_globalMaxTime - m_globalMinTime;
            int maxHorizontalOffset = std::max(0, static_cast<int>(timeRange * m_pixelsPerTimeUnit) - m_waveAreaWidth);
            desiredHorizontalOffset = std::max(0, std::min(desiredHorizontalOffset, maxHorizontalOffset));

            if (desiredHorizontalOffset != m_horizontalOffset)
            {
                m_horizontalOffset = desiredHorizontalOffset;

                m_waveArea->horizontalScrollBar()->setValue(m_horizontalOffset);
            }
        }
    }
    update();

    QToolTip::showText(mapToGlobal(QPoint(width() / 2, 10)),
                       result.description,
                       this,
                       QRect(),
                       3000);
}
int WaveformDisplay::getSignalAbsoluteTop(const QString &signalKey) const
{
    int currentY = m_timeRulerHeight;

    for (const QString &topSignalKey : m_pinTopSignals)
    {
        if (topSignalKey == signalKey)
        {
            return currentY;
        }
        currentY += m_signalHeight + m_signalSpacing;
    }

    currentY = getTopPinSignalsAreaBottomY();
    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
            continue;
        const SignalGroup &group = m_groups[groupName];
        currentY += m_groupHeaderHeight + m_signalSpacing;
        if (!group.collapsed)
        {
            for (const QString &groupSignalKey : group.v_signals)
            {
                if (groupSignalKey == signalKey)
                {
                    return currentY;
                }
                currentY += m_signalHeight + m_signalSpacing;
            }
        }
    }
    return -1;
}
void WaveformDisplay::ensureSignalVisible(const QString &signalKey)
{
    if (!m_signals.contains(signalKey))
    {
        return;
    }

    int signalTop = getSignalAbsoluteTop(signalKey);
    if (signalTop == -1)
    {
        return;
    }
    int signalBottom = signalTop + m_signalHeight;

    int topAreaBottom = getTopPinSignalsAreaBottomY();

    if (signalTop >= topAreaBottom)
    {

        int scrollAreaStartY = topAreaBottom;
        int scrollAreaHeight = height() - scrollAreaStartY;

        int signalPositionInContent = signalTop - scrollAreaStartY;

        int desiredVerticalOffset = m_verticalOffset;
        if (signalPositionInContent < m_verticalOffset)
        {

            desiredVerticalOffset = signalPositionInContent - 2 * m_signalHeight;
        }
        else if (signalPositionInContent + m_signalHeight > m_verticalOffset + scrollAreaHeight)
        {

            desiredVerticalOffset = signalPositionInContent + m_signalHeight - scrollAreaHeight + 2 * m_signalHeight;
        }
        else
        {
        }

        if (desiredVerticalOffset != m_verticalOffset)
        {
            m_verticalOffset = desiredVerticalOffset;

            updateScrollBars();

            m_nameArea->verticalScrollBar()->setValue(m_verticalOffset);
            m_valueArea->verticalScrollBar()->setValue(m_verticalOffset);
            m_waveArea->verticalScrollBar()->setValue(m_verticalOffset);
        }
    }

    if (m_selectTime >= 0)
    {

        double x = timeToX(m_selectTime);

        int waveAreaLeft = m_nameAreaWidth + m_valueAreaWidth;
        int waveAreaRight = waveAreaLeft + m_waveAreaWidth;

        if (m_waveArea->verticalScrollBar()->isVisible())
        {
            waveAreaRight -= m_waveArea->verticalScrollBar()->width();
        }

        int visibleLeft = waveAreaLeft + m_horizontalOffset;
        int visibleRight = waveAreaRight + m_horizontalOffset;

        int desiredHorizontalOffset = m_horizontalOffset;

        const int margin = 50;
        if (x < visibleLeft + margin)
        {

            desiredHorizontalOffset = x - waveAreaLeft - margin;
        }
        else if (x > visibleRight - margin)
        {

            desiredHorizontalOffset = x - waveAreaRight + margin;
        }

        double timeRange = m_globalMaxTime - m_globalMinTime;
        int maxHorizontalOffset = std::max(0, static_cast<int>(timeRange * m_pixelsPerTimeUnit) - m_waveAreaWidth);
        desiredHorizontalOffset = std::max(0, std::min(desiredHorizontalOffset, maxHorizontalOffset));

        if (desiredHorizontalOffset != m_horizontalOffset)
        {
            m_horizontalOffset = desiredHorizontalOffset;

            m_waveArea->horizontalScrollBar()->setValue(m_horizontalOffset);
        }
    }
}
void WaveformDisplay::drawSearchHighlights(QPainter &painter)
{
    if (m_searchResults.isEmpty())
    {
        return;
    }
    painter.save();

    painter.setClipRect(m_nameAreaWidth + m_valueAreaWidth, 0,
                        m_waveAreaWidth, height());
    painter.translate(-m_horizontalOffset, 0);
    for (int i = 0; i < m_searchResults.size(); i++)
    {
        const SearchResult &result = m_searchResults[i];
        double x = timeToX(result.time);

        QString signalKey = "";
        for (auto it = m_signals.constBegin(); it != m_signals.constEnd(); ++it)
        {
            if (it.value().signal_name == result.signalName)
            {
                signalKey = it.key();
                break;
            }
        }
        if (signalKey.isEmpty())
        {
            continue;
        }

        QColor signalColor = Qt::yellow;
        if (i == m_currentSearchIndex)
        {

            signalColor = signalColor.lighter(150);
        }

        painter.setPen(QPen(signalColor, i == m_currentSearchIndex ? 2 : 1));
        painter.setBrush(signalColor);

        int triangleTop = 2;
        int triangleHeight = 8;
        QPolygonF triangle;
        triangle << QPointF(x - 6, triangleTop + triangleHeight)
                 << QPointF(x + 6, triangleTop + triangleHeight)
                 << QPointF(x, triangleTop);
        painter.drawPolygon(triangle);

        painter.setPen(Qt::black);
        QRect textRect(x - 10, triangleTop + triangleHeight + 2, 20, 15);
        painter.setBrush(Qt::white);
        painter.drawRect(textRect.adjusted(-1, -1, 1, 1));
        painter.drawText(textRect, Qt::AlignCenter, QString::number(i + 1));

        QPair<int, int> yRange = getSignalYPositionInView(signalKey);
        if (yRange.first == -1 || yRange.second == -1)
        {
            continue;
        }

        painter.setPen(QPen(signalColor, i == m_currentSearchIndex ? 3 : 2));
        painter.drawLine(x, yRange.first, x, yRange.second);

        if (i == m_currentSearchIndex)
        {

            painter.setBrush(signalColor);
            painter.setPen(QPen(signalColor.darker(), 2));
            int centerY = (yRange.first + yRange.second) / 2;
            painter.drawEllipse(QPointF(x, centerY), 6, 6);

            painter.setPen(QPen(signalColor.lighter(150), 1, Qt::DashLine));
            painter.drawLine(x - 3, yRange.first, x - 3, yRange.second);
            painter.drawLine(x + 3, yRange.first, x + 3, yRange.second);
        }
    }
    painter.restore();
}
QPair<int, int> WaveformDisplay::getSignalYPositionInView(const QString &signalKey) const
{
    if (!m_signals[signalKey].visible)
    {
        return qMakePair(-1, -1);
    }

    int topAreaBottom = getTopPinSignalsAreaBottomY();
    int currentY = m_timeRulerHeight;

    for (const QString &topSignalKey : m_pinTopSignals)
    {
        if (topSignalKey == signalKey)
        {

            return qMakePair(currentY, currentY + m_signalHeight);
        }
        if (m_signals[topSignalKey].visible)
        {
            currentY += m_signalHeight + m_signalSpacing;
        }
    }

    currentY = topAreaBottom;
    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
            continue;
        const SignalGroup &group = m_groups[groupName];
        currentY += m_groupHeaderHeight + m_signalSpacing;
        if (!group.collapsed)
        {
            for (const QString &groupSignalKey : group.v_signals)
            {
                if (groupSignalKey == signalKey)
                {

                    int signalTop = currentY - m_verticalOffset;
                    int signalBottom = signalTop + m_signalHeight;

                    if (signalTop >= 0 && signalBottom <= height())
                    {
                        return qMakePair(signalTop, signalBottom);
                    }
                    return qMakePair(-1, -1);
                }
                if (m_signals[groupSignalKey].visible)
                {
                    currentY += m_signalHeight + m_signalSpacing;
                }
            }
        }
    }
    return qMakePair(-1, -1);
}
void WaveformDisplay::setupNameValueAreaMenu(const QPoint &adjustedPos, bool isInTopArea)
{
    if (isInTopArea)
    {

        m_addGroupAction->setVisible(true);
        m_expandGroupAction->setVisible(true);
    }
    else
    {

        m_addGroupAction->setVisible(true);
        m_expandGroupAction->setVisible(true);

        m_saveGroupAction->setVisible(true);
        m_loadGroupAction->setVisible(true);
    }
    m_contextMenu->addSeparator();
    m_hierarchicalNameAction->setVisible(true);
}
void WaveformDisplay::setupGeneralMenu(const QPoint &adjustedPos)
{

    m_addGroupAction->setVisible(true);
    m_expandGroupAction->setVisible(true);
    m_hierarchicalNameAction->setVisible(true);
}
bool WaveformDisplay::shouldShowContextMenu(ContextMenuType menuType) const
{

    return menuType != ContextMenuType::General ||
            !m_contextMenu->actions().isEmpty();
}
void WaveformDisplay::addMarkerAtSpecificTime(double time)
{
    QString name = tr("Marker%1").arg(m_nextMarkerId);
    addMarker(time, name, Qt::red, Qt::SolidLine);
}
void WaveformDisplay::createContextMenu()
{

    if (m_contextMenu)
    {
        delete m_contextMenu;
        m_contextMenu = nullptr;
    }
    m_contextMenu = new QMenu(this);

    createMenuActions();

    m_contextMenu->addAction(m_addGroupAction);
    m_contextMenu->addAction(m_renameGroupAction);
    m_contextMenu->addAction(m_removeGroupAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_collapseGroupAction);
    m_contextMenu->addAction(m_expandGroupAction);
    m_contextMenu->addSeparator();

    m_contextMenu->addAction(m_removeSignalAction);
    m_contextMenu->addAction(m_copyFullPathAction);
    m_contextMenu->addAction(m_copyValueAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_highlightAction);
    m_contextMenu->addAction(m_detailAction);
    m_contextMenu->addAction(m_pinTopAction);
    m_contextMenu->addAction(m_unpinnedSignalAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_logicalOperationAction);

    m_contextMenu->addMenu(m_radixMenu);

    m_contextMenu->addAction(m_hierarchicalNameAction);
    m_contextMenu->addSeparator();

    m_contextMenu->addAction(m_saveGroupAction);
    m_contextMenu->addAction(m_loadGroupAction);
    m_contextMenu->addSeparator();

    m_contextMenu->addAction(m_zoomInAction);
    m_contextMenu->addAction(m_zoomOutAction);
    m_contextMenu->addAction(m_zoomAllAction);
    m_contextMenu->addSeparator();

    m_markerMenu = new QMenu(tr("Markers"), this);
    m_goToMarkerMenu = new QMenu(tr("Go to Marker"), this);
    connect(m_manageMarkersAction, &QAction::triggered, this, &WaveformDisplay::showMarkerManagerDialog);
    connect(m_goToMarkerMenu, &QMenu::aboutToShow, this, &WaveformDisplay::updateGoToMarkerMenu);
    connect(m_clearMarkersAction, &QAction::triggered, this, &WaveformDisplay::clearAllMarkers);
    m_markerMenu->addAction(m_addMarkerAction);
    m_markerMenu->addSeparator();
    m_markerMenu->addAction(m_manageMarkersAction);
    m_markerMenu->addMenu(m_goToMarkerMenu);
    m_markerMenu->addAction(m_clearMarkersAction);

    m_contextMenu->addSeparator();
    m_contextMenu->addMenu(m_markerMenu);

    createMarkerManagerDialog();

    resetContextMenuVisibility();
}
void WaveformDisplay::onDetailActionTriggered()
{
    if (!m_lastSelectedSignalKey.isEmpty() && m_signals.contains(m_lastSelectedSignalKey))
    {
        showSignalDetailDialog();
    }
}
int WaveformDisplay::calculateSignalTransitions(const DisplaySignal &displaySignal) const
{
    if (displaySignal.signal.isNull())
    {
        return 0;
    }
    int transitionCount = 0;
    const auto &signal = displaySignal.signal;
    auto plotData = m_waveform->to_plot_data(signal);
    if (plotData.second.size() < 2)
    {
        return 0;
    }

    const Var &var = m_waveform->get_hierarchy().get_var(displaySignal.var_ref);
    bool isMultiBit = var.signal_type.is_real || var.signal_type.is_string || var.signal_type.width > 1;
    if (!isMultiBit)
    {

        double prevValue = plotData.second[0];
        for (size_t i = 1; i < plotData.second.size(); i++)
        {
            if (plotData.second[i] != prevValue)
            {
                transitionCount++;
                prevValue = plotData.second[i];
            }
        }
    }
    else
    {

        auto values = signal->get_signal_values();
        if (values.size() < 2)
        {
            return 0;
        }
        std::string prevValue = values[0];
        for (size_t i = 1; i < values.size(); i++)
        {
            if (values[i] != prevValue)
            {
                transitionCount++;
                prevValue = values[i];
            }
        }
    }
    return transitionCount;
}
void WaveformDisplay::showSignalDetailDialog()
{
    if (!m_signals.contains(m_lastSelectedSignalKey))
    {
        return;
    }
    const DisplaySignal &displaySignal = m_signals[m_lastSelectedSignalKey];

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Signal Details"));
    dialog.setMinimumSize(500, 400);
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    QGroupBox *timeRangeGroup = new QGroupBox(tr("Time Range Statistics"), &dialog);
    QFormLayout *timeRangeLayout = new QFormLayout(timeRangeGroup);
    Time minTime = 0;
    Time maxTime = 0;
    if (m_middlePressClickTime && m_selectTime)
    {
        minTime = m_selectTime > m_middlePressClickTime ? m_middlePressClickTime : m_selectTime;
        maxTime = m_selectTime > m_middlePressClickTime ? m_selectTime : m_middlePressClickTime;
    }
    else if (m_middlePressClickTime && m_clickTime)
    {
        minTime = m_clickTime > m_middlePressClickTime ? m_middlePressClickTime : m_clickTime;
        maxTime = m_clickTime > m_middlePressClickTime ? m_clickTime : m_middlePressClickTime;
    }
    else if (m_selectTime)
    {

        minTime = 0;
        maxTime = m_selectTime;
    }
    else if (m_clickTime)
    {
        minTime = 0;
        maxTime = m_clickTime;
    }
    else
    {
        minTime = m_minTime;
        maxTime = m_maxTime;
    }
    QDoubleSpinBox *startTimeEdit = new QDoubleSpinBox(&dialog);
    startTimeEdit->setRange(m_globalMinTime, m_globalMaxTime);
    startTimeEdit->setDecimals(3);

    startTimeEdit->setValue(minTime);
    startTimeEdit->setSuffix(" ns");
    QDoubleSpinBox *endTimeEdit = new QDoubleSpinBox(&dialog);
    endTimeEdit->setRange(m_globalMinTime, m_globalMaxTime);
    endTimeEdit->setDecimals(3);

    endTimeEdit->setValue(maxTime);
    endTimeEdit->setSuffix(" ns");
    QPushButton *updateStatsBtn = new QPushButton(tr("Update Statistics"), &dialog);
    timeRangeLayout->addRow(tr("Start Time:"), startTimeEdit);
    timeRangeLayout->addRow(tr("End Time:"), endTimeEdit);
    timeRangeLayout->addRow(updateStatsBtn);

    QGroupBox *statsGroup = new QGroupBox(tr("Statistics Results"), &dialog);
    QFormLayout *statsLayout = new QFormLayout(statsGroup);

    QLabel *transitionsLabel = new QLabel("0", &dialog);
    QLabel *risingEdgesLabel = new QLabel("0", &dialog);
    QLabel *fallingEdgesLabel = new QLabel("0", &dialog);
    QLabel *uniqueValuesLabel = new QLabel("0", &dialog);
    QLabel *durationLabel = new QLabel("0 ns", &dialog);
    QLabel *frequencyLabel = new QLabel("0 Hz", &dialog);
    statsLayout->addRow(tr("Total Transitions:"), transitionsLabel);
    QLabel *risingEdgesTitle = new QLabel(tr("Rising Edges:"), &dialog);
    QLabel *fallingEdgesTitle = new QLabel(tr("Falling Edges:"), &dialog);
    statsLayout->addRow(risingEdgesTitle, risingEdgesLabel);
    statsLayout->addRow(fallingEdgesTitle, fallingEdgesLabel);
    statsLayout->addRow(tr("Unique Values:"), uniqueValuesLabel);
    statsLayout->addRow(tr("Duration:"), durationLabel);
    statsLayout->addRow(tr("Transition Frequency:"), frequencyLabel);

    QGroupBox *infoGroup = new QGroupBox(tr("Signal Information"), &dialog);
    QFormLayout *infoLayout = new QFormLayout(infoGroup);

    QLabel *nameLabel = new QLabel(displaySignal.signal_name, &dialog);
    infoLayout->addRow(tr("Signal Name:"), nameLabel);

    QString fullPath = join_path_with_dot(displaySignal.scopes) + "." + displaySignal.signal_name;
    QLabel *pathLabel = new QLabel(fullPath, &dialog);
    pathLabel->setWordWrap(true);
    infoLayout->addRow(tr("Full Path:"), pathLabel);

    QString currentValue = getSignalValueAtTime(displaySignal, m_selectTime);
    QLabel *valueLabel = new QLabel(currentValue, &dialog);
    infoLayout->addRow(tr("Current Value:"), valueLabel);

    mainLayout->addWidget(infoGroup);
    mainLayout->addWidget(timeRangeGroup);
    mainLayout->addWidget(statsGroup);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    mainLayout->addWidget(buttonBox);

    auto calculateStatistics = [&](double startTime, double endTime)
    {
        if (displaySignal.signal.isNull())
        {
            return;
        }
        SignalStats stats = calculateSignalStatistics(displaySignal, startTime, endTime);
        transitionsLabel->setText(QString::number(stats.totalTransitions));

        if (stats.risingEdges == -1)
        {
            risingEdgesLabel->setText("N/A");
            fallingEdgesLabel->setText("N/A");
            risingEdgesTitle->setText(tr("Rising Edges:") + " (N/A for multi-bit)");
            fallingEdgesTitle->setText(tr("Falling Edges:") + " (N/A for multi-bit)");
        }
        else
        {
            risingEdgesLabel->setText(QString::number(stats.risingEdges));
            fallingEdgesLabel->setText(QString::number(stats.fallingEdges));
            risingEdgesTitle->setText(tr("Rising Edges:"));
            fallingEdgesTitle->setText(tr("Falling Edges:"));
        }
        uniqueValuesLabel->setText(QString::number(stats.uniqueValues.size()));
        double duration = endTime - startTime;
        durationLabel->setText(QString("%1 ns").arg(duration, 0, 'f', 3));
        if (duration > 0)
        {
            double freq = stats.totalTransitions * 1e9 / duration;
            frequencyLabel->setText(QString("%1 Hz").arg(freq, 0, 'f', 1));
        }
        else
        {
            frequencyLabel->setText("N/A");
        }
    };

    calculateStatistics(minTime, maxTime);

    connect(updateStatsBtn, &QPushButton::clicked, [&]()
    { calculateStatistics(startTimeEdit->value(), endTimeEdit->value()); });

    connect(startTimeEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [&](double value)
    {
        if (value < endTimeEdit->value())
        {
            calculateStatistics(value, endTimeEdit->value());
        }
    });
    connect(endTimeEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [&](double value)
    {
        if (value > startTimeEdit->value())
        {
            calculateStatistics(startTimeEdit->value(), value);
        }
    });
    dialog.exec();
}
WaveformDisplay::SignalStats WaveformDisplay::calculateSignalStatistics(const DisplaySignal &displaySignal,
                                                                        double startTime, double endTime) const
{
    SignalStats stats;
    if (displaySignal.signal.isNull() || startTime >= endTime)
    {
        return stats;
    }
    const auto &signal = displaySignal.signal;
    auto plotData = m_waveform->to_plot_data(signal);
    if (plotData.first.empty() || plotData.second.empty())
    {
        return stats;
    }

    const Var &var = m_waveform->get_hierarchy().get_var(displaySignal.var_ref);
    bool isSingleBit = !var.signal_type.is_real && !var.signal_type.is_string && var.signal_type.width == 1;

    size_t startIndex = 0;
    size_t endIndex = plotData.first.size() - 1;

    size_t left = 0, right = plotData.first.size() - 1;
    while (left <= right)
    {
        size_t mid = left + (right - left) / 2;
        if (plotData.first[mid] < startTime)
        {
            left = mid + 1;
        }
        else
        {
            startIndex = mid;
            if (mid == 0)
                break;
            right = mid - 1;
        }
    }

    left = 0;
    right = plotData.first.size() - 1;
    while (left <= right)
    {
        size_t mid = left + (right - left) / 2;
        if (plotData.first[mid] > endTime)
        {
            if (mid == 0)
                break;
            right = mid - 1;
        }
        else
        {
            endIndex = mid;
            left = mid + 1;
        }
    }

    if (startIndex >= plotData.first.size() || endIndex >= plotData.first.size())
    {
        return stats;
    }
    if (isSingleBit)
    {

        double prevValue = -1;
        for (size_t i = startIndex; i <= endIndex && i < plotData.second.size(); i++)
        {
            double time = plotData.first[i];
            if (time < startTime || time > endTime)
                continue;
            double value = plotData.second[i];

            QString strValue = QString::number(value);
            stats.uniqueValues.insert(strValue);
            if (prevValue != -1)
            {
                if (value != prevValue)
                {
                    stats.totalTransitions++;
                    if (value > prevValue)
                    {
                        stats.risingEdges++;
                    }
                    else
                    {
                        stats.fallingEdges++;
                    }
                }
            }
            prevValue = value;
        }
    }
    else
    {

        auto values = signal->get_signal_values();

        if (values.size() != plotData.first.size())
        {
            qDebug() << "Warning: Signal values size doesn't match time points size";

            QString prevValue;
            for (size_t i = startIndex; i <= endIndex && i < plotData.first.size(); i++)
            {
                double time = plotData.first[i];
                if (time < startTime || time > endTime)
                    continue;

                double value = plotData.second[i];
                QString currentValue = QString::number(value);

                if (displaySignal.translator)
                {
                    VariableMeta meta = m_waveform->var_to_meta(displaySignal.var_ref);
                    TranslatedValue translated = displaySignal.translator->translate(meta, currentValue);
                    currentValue = translated.value;
                }
                stats.uniqueValues.insert(currentValue);
                if (!prevValue.isEmpty() && currentValue != prevValue)
                {
                    stats.totalTransitions++;
                }
                prevValue = currentValue;
            }
        }
        else
        {

            QString prevValue;
            for (size_t i = startIndex; i <= endIndex && i < values.size(); i++)
            {
                double time = plotData.first[i];
                if (time < startTime || time > endTime)
                    continue;
                QString currentValue = QString::fromStdString(values[i]);

                if (displaySignal.translator)
                {
                    VariableMeta meta = m_waveform->var_to_meta(displaySignal.var_ref);
                    TranslatedValue translated = displaySignal.translator->translate(meta, currentValue);
                    currentValue = translated.value;
                }
                stats.uniqueValues.insert(currentValue);
                if (!prevValue.isEmpty() && currentValue != prevValue)
                {
                    stats.totalTransitions++;
                }
                prevValue = currentValue;
            }
        }

        stats.risingEdges = -1;
        stats.fallingEdges = -1;
    }
    return stats;
}
void WaveformDisplay::updateSignalColors(const QMap<QString, SignalHighlightConfig> &highlights)
{
    for (auto it = highlights.begin(); it != highlights.end(); ++it)
    {
        m_signals[it->signalKey].nameColor = it->nameColor;
        m_signals[it->signalKey].valueColor = it->valueColor;
        m_signals[it->signalKey].lineColor = it->lineColor;
        m_signals[it->signalKey].backgroundColor = it->backgroundColor;
    }

    update();
}

void WaveformDisplay::onHighlightButtonClicked()
{
    if (!m_highlightDialog)
    {
        m_highlightDialog = new HighlightDialog(this);
        connect(m_highlightDialog, &HighlightDialog::highlightsApplied,
                this, &WaveformDisplay::applySignalHighlights);
        m_highlightDialog->setModal(false);
        connect(m_highlightDialog, &QDialog::accepted, this, [this]()
        {
            QMap<QString, SignalHighlightConfig> currentHighlights = m_highlightDialog->getSignalHighlights();
            updateSignalColors(currentHighlights); });
        m_highlightDialog->installEventFilter(this);
    }

    if (!m_highlightDialogOldPos.isNull())
    {
        m_highlightDialog->move(m_highlightDialogOldPos);
    }
    else
    {
        m_highlightDialogOldPos = m_highlightDialog->pos();
    }

    for (const QString &signalKey : m_selectedSignalKeys)
    {
        if (m_signals.contains(signalKey))
        {

            DisplaySignal &signal = m_signals[signalKey];

            QString groupName = getGroupNameBySignalKey(signalKey);
            QString path = groupName + "." + signal.signal_name;
            auto currentHighlights = m_highlightDialog->getSignalHighlights();
            if (!currentHighlights.keys().contains(signalKey))
            {
                SignalHighlightConfig config;
                config.nameColor = m_pen.color();
                config.valueColor = m_pen.color();
                config.lineColor = m_highLevelColor;
                config.backgroundColor = Qt::transparent;
                config.signalKey = signalKey;
                m_highlightDialog->addSignal(signalKey, path, config);
            }
            else
            {
                m_highlightDialog->selectSignalRow(path);
            }
        }
    }

    m_highlightDialog->show();
    m_highlightDialog->raise();
    m_highlightDialog->activateWindow();
}


QString WaveformDisplay::getGroupNameBySignalKey(const QString &signalKey) {
    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
        {
            continue;
        }
        const SignalGroup &group = m_groups[groupName];

        QJsonArray signalsArray;
        for (const QString &key : group.v_signals)
        {
            if(signalKey == key) {return groupName; }
        }
    }
    return QString();
}

void WaveformDisplay::onPinTopButtonClicked(DisplaySignal &signal)
{
    if (!m_pinTopSignals.contains(signal.name))
    {
        signal.visible = false;
        m_pinTopSignals.insert(m_pinTopSignals.size(), signal.name);
        update();
    }
}

void WaveformDisplay::applySignalHighlights(const QMap<QString, SignalHighlightConfig> &highlights)
{
    m_signalHighlights = highlights;
    updateSignalColors(highlights);
}

void WaveformDisplay::calculateLayout()
{
    m_timeRulerHeight = m_timeRulerVisible ? 22 : 0;
    m_signalNameWidth = m_signalNamesVisible ? m_nameAreaWidth - 5 : 0;
    m_valueWidth = m_valueAreaWidth - 5;
    m_groupHeaderHeight = 20;

    m_waveAreaWidth = width() - m_nameAreaWidth - m_valueAreaWidth - 5;
}
void WaveformDisplay::drawTimeRuler(QPainter &painter)
{

    double timeRange = m_maxTime - m_minTime;
    if (timeRange <= 0)
    {
        return;
    }

    int optimalTickCount = 8;
    double rawInterval = timeRange / optimalTickCount;
    double magnitude = pow(10, floor(log10(rawInterval)));
    double interval = ceil(rawInterval / magnitude) * magnitude;

    if (interval < 1.0)
    {
        interval = 1.0;
    }

    double firstTick = floor(m_minTime / interval) * interval;

    int tickCount = floor((m_maxTime - firstTick) / interval) + 1;
    static int lastDisplayedValue = -1;

    double endX = timeToX(m_maxTime);
    QRect rulerRect(m_nameAreaWidth + m_valueAreaWidth, 0, endX, m_timeRulerHeight);
    painter.fillRect(rulerRect, m_rulerRectColor);
    painter.setPen(m_pen);
    painter.drawLine(m_nameAreaWidth + m_valueAreaWidth, m_timeRulerHeight,
                     endX, m_timeRulerHeight);
    for (int i = 0; i < tickCount; i++)
    {
        double time = firstTick + i * interval;

        if (time < m_minTime)
        {
            continue;
        }

        if (time > m_maxTime)
        {
            break;
        }
        double x = timeToX(time);

        if (i > 0)
        {
            double prevTime = firstTick + (i - 1) * interval;

            if (prevTime >= m_minTime)
            {
                double prevX = timeToX(prevTime);
                int prevValue = round(prevTime);
                int currentValue = round(time);
                int valueDiff = currentValue - prevValue;

                if (abs(valueDiff) >= 10)
                {
                    double segmentStep = (x - prevX) / 10;
                    for (int j = 1; j < 10; j++)
                    {
                        double midX = prevX + j * segmentStep;
                        int tickHeight = (j % 5 == 0) ? 8 : 4;
                        painter.setPen(m_pen);
                        painter.drawLine(midX, m_timeRulerHeight, midX, m_timeRulerHeight - tickHeight);
                    }
                }

                if (abs(valueDiff) > 1 && abs(valueDiff) < 10)
                {
                    double segmentStep = (x - prevX) / valueDiff;
                    for (int j = 1; j < valueDiff; j++)
                    {
                        double midX = prevX + j * segmentStep;
                        painter.setPen(m_pen);
                        painter.drawLine(midX, m_timeRulerHeight, midX, m_timeRulerHeight - 4);
                    }
                }
            }
        }

        int currentValue = round(time);
        if (currentValue != lastDisplayedValue)
        {

            painter.setPen(m_mainRulerLinePen);

            painter.drawLine(x, m_timeRulerHeight, x, m_timeRulerHeight - 8);
            QString label = QString::number(currentValue);
            QRect textRect(x + 5, 0, 100, m_timeRulerHeight);
            painter.setPen(m_mainRulerTextPen);
            painter.drawText(textRect, Qt::AlignLeft, label);
            lastDisplayedValue = currentValue;
        }
    }

    if (tickCount == 0 || (firstTick + (tickCount - 1) * interval) < m_maxTime)
    {
        double x = timeToX(m_maxTime);
        int currentValue = round(m_maxTime);
        if (currentValue != lastDisplayedValue)
        {
            painter.setPen(m_mainRulerLinePen);
            painter.drawLine(x, 0, x, height() - 10);
            QString label = QString::number(currentValue);
            QRect textRect(x + 5, 0, 100, m_timeRulerHeight);
            painter.setPen(m_mainRulerTextPen);
            painter.drawText(textRect, Qt::AlignLeft, label);
        }
    }
}
int WaveformDisplay::drawSignalsArr(QPainter &painter, int currentY, QVector<QString> signalsArr, bool isPinTop)
{
    for (const QString &signalName : signalsArr)
    {
        double lineWidth = 1.0;
        if (!m_signals.contains(signalName))
        {
            continue;
        }

        const auto &displaySignal = m_signals[signalName];

        if (!isPinTop && !displaySignal.visible)
        {
            continue;
        }

        const Var &var = m_waveform->get_hierarchy().get_var(displaySignal.var_ref);
        bool isMulBit = var.signal_type.is_real || var.signal_type.is_string || var.signal_type.width > 1;
        const auto &signal = displaySignal.signal;
        auto plotData = m_waveform->to_plot_data(signal);
        if (plotData.first.empty() || plotData.second.empty())
        {
            currentY += m_signalHeight + m_signalSpacing;
            continue;
        }

        QPainterPath path;

        double prevTime = plotData.first[0];
        double prevValue = plotData.second[0];
        double prevX = timeToX(prevTime);
        double prevY = valueToY(currentY, signalName, QString::number(prevValue));

        const double cornerRadius = 1.0;
        path.moveTo(prevX, prevY);
        bool isSelected = m_selectedSignalKeys.contains(signalName);
        if (!isMulBit)
        {
            QColor currentColor = QColor();

            if (isSelected)
            {
                lineWidth = 2.0;
                if (prevValue == 0)
                {
                    currentColor = m_highLightLowColor;
                }
                else if (prevValue == 1)
                {
                    currentColor = m_highLightHighColor;
                }
                else if (prevValue == 0.5)
                {
                    currentColor = m_segmentXColor;
                }
                else if (prevValue == 0.625)
                {
                    currentColor = m_segmentZColor;
                }
                else if (displaySignal.color.isValid())
                {
                    currentColor = displaySignal.color;
                }
                else
                {
                    currentColor = Qt::black;
                }
            }
            else
            {
                lineWidth = 1.0;
                if (m_signals[signalName].lineColor.isValid())
                {
                    currentColor = m_signals[signalName].lineColor;
                }
                else if (prevValue == 0)
                {
                    currentColor = m_lowLevelColor;
                }
                else if (prevValue == 1)
                {
                    currentColor = m_highLevelColor;
                }
                else if (prevValue == 0.5)
                {
                    currentColor = m_segmentXColor;
                }
                else if (prevValue == 0.625)
                {
                    currentColor = m_segmentZColor;
                }
                else if (displaySignal.color.isValid())
                {
                    currentColor = displaySignal.color;
                }
                else
                {
                    currentColor = Qt::black;
                }
            }
            int minCornerRadiusNumberDistance = 0;

            for (size_t i = 1; i < plotData.first.size(); i++)
            {

                double time = plotData.first[i];
                double value = plotData.second[i];
                double x = timeToX(time);
                double y = valueToY(currentY, signalName, QString::number(value));
                QColor newColor = currentColor;
                if (isSelected)
                {
                    lineWidth = 2.0;
                    if (value == 0)
                    {
                        newColor = m_highLightLowColor;
                    }
                    else if (value == 1)
                    {
                        newColor = m_highLightHighColor;
                    }
                    else if (value == 0.5)
                    {
                        newColor = m_segmentXColor;
                    }
                    else if (value == 0.625)
                    {
                        newColor = m_segmentZColor;
                    }
                    else if (displaySignal.color.isValid())
                    {
                        newColor = displaySignal.color;
                    }
                    else
                    {
                        newColor = Qt::black;
                    }
                }
                else
                {
                    lineWidth = 1.0;
                    if (displaySignal.lineColor.isValid())
                    {
                        newColor = displaySignal.lineColor;
                    }
                    else if (value == 0)
                    {
                        newColor = m_lowLevelColor;
                    }
                    else if (value == 1)
                    {
                        newColor = m_highLevelColor;
                    }
                    else if (value == 0.5)
                    {
                        newColor = m_segmentXColor;
                    }
                    else if (value == 0.625)
                    {
                        newColor = m_segmentZColor;
                    }
                    else if (displaySignal.color.isValid())
                    {
                        newColor = displaySignal.color;
                    }
                    else
                    {
                        newColor = Qt::black;
                    }
                }
                if (m_signals[signalName].lineColor.isValid())
                {
                    currentColor = m_signals[signalName].lineColor;
                }
                if (value != prevValue)
                {
                    bool risingEdge = (value > prevValue);
                    double transitionX = x;
                    double horizontalEndX = transitionX - cornerRadius;

                    path.lineTo(horizontalEndX, prevY);
                    painter.setPen(QPen(currentColor, lineWidth));
                    painter.drawPath(path);
                    currentColor = newColor;

                    QPainterPath cornerPath1;
                    cornerPath1.moveTo(horizontalEndX, prevY);
                    if (risingEdge)
                    {
                        cornerPath1.moveTo(transitionX - cornerRadius, prevY);
                        QRectF arcRect(transitionX - cornerRadius * 2, prevY - cornerRadius * 2,
                                       cornerRadius * 2, cornerRadius * 2);
                        cornerPath1.arcTo(arcRect, 270, 90);
                    }
                    else
                    {

                        cornerPath1.moveTo(transitionX, prevY + cornerRadius);
                        QRectF arcRect(transitionX - cornerRadius * 2, prevY,
                                       cornerRadius * 2, cornerRadius * 2);
                        cornerPath1.arcTo(arcRect, 0, 90);
                    }

                    if (m_signals[signalName].lineColor.isValid())
                    {
                        currentColor = m_signals[signalName].lineColor;
                    }

                    painter.setPen(QPen(currentColor, lineWidth));
                    painter.drawPath(cornerPath1);

                    path = QPainterPath();
                    if (risingEdge)
                    {
                        path.moveTo(transitionX, prevY - cornerRadius);
                        path.lineTo(transitionX, y + cornerRadius);
                    }
                    else
                    {
                        path.moveTo(transitionX, prevY + cornerRadius);
                        path.lineTo(transitionX, y - cornerRadius);
                    }

                    painter.setPen(QPen(currentColor, lineWidth));
                    painter.drawPath(path);

                    path = QPainterPath();
                    path.moveTo(transitionX, y);

                    QPainterPath cornerPath2;
                    cornerPath2.moveTo(transitionX + cornerRadius, y);
                    if (risingEdge)
                    {

                        cornerPath2.moveTo(transitionX + cornerRadius, y);
                        QRectF arcRect(transitionX, y, cornerRadius * 2, cornerRadius * 2);
                        cornerPath2.arcTo(arcRect, 90, 90);
                    }
                    else
                    {

                        cornerPath2.moveTo(transitionX, y - cornerRadius);
                        QRectF arcRect(transitionX, y - cornerRadius * 2, cornerRadius * 2, cornerRadius * 2);
                        cornerPath2.arcTo(arcRect, 180, 90);
                    }

                    painter.setPen(QPen(currentColor, lineWidth));
                    painter.drawPath(cornerPath2);
                }

                else
                {
                    path.lineTo(x - cornerRadius, y);
                    painter.setPen(QPen(currentColor, lineWidth));
                    painter.drawPath(path);
                }
                currentColor = newColor;
                path = QPainterPath();
                if (x > prevX + (minCornerRadiusNumberDistance + 1) * cornerRadius)
                {
                    path.moveTo(x + cornerRadius, y);
                }
                else
                {
                    path.moveTo(x, y);
                }

                prevValue = value;
                prevX = x;
                prevY = y;
            }

            painter.setPen(QPen(currentColor, lineWidth));
            painter.drawPath(path);
        }
        else
        {
            if (isSelected)
            {
                lineWidth = 2.0;
            }
            else
            {
                lineWidth = 1.0;
            }

            prevY = valueToY(currentY, signalName, 0);
            auto values = signal->get_signal_values();

            const double hexHeight = m_signalHeight - 2;
            const double hexRadius = hexHeight / 2.0;

            QString rawValue;
            QString displayValue;
            double x;
            for (size_t i = 1; i < plotData.first.size(); i++)
            {
                double time = plotData.first[i];

                rawValue = QString::fromStdString(values[i - 1]);
                x = timeToX(time);

                double distance = x - prevX;

                if (displaySignal.translator)
                {

                    VariableMeta meta = m_waveform->var_to_meta(displaySignal.var_ref);
                    TranslatedValue translated = displaySignal.translator->translate(meta, rawValue);
                    displayValue = translated.value;
                }
                else
                {
                    displayValue = rawValue;
                }

                QColor highColor = m_hexagonalHighColor;
                QColor lowColor = m_hexagonalLowColor;
                if (displayValue.contains('x'))
                {
                    highColor = m_segmentXColor;
                    lowColor = m_segmentXColor;
                }
                else if (displayValue.contains('z'))
                {
                    highColor = m_segmentZColor;
                    lowColor = m_segmentZColor;
                }
                else if (isSelected)
                {
                    highColor = m_highHexagonalHighColor;
                    lowColor = m_highHexagonalLowColor;
                }
                else if (displaySignal.lineColor.isValid())
                {
                    highColor = displaySignal.lineColor;
                    lowColor = displaySignal.lineColor;
                }
                if (m_signals[signalName].lineColor.isValid())
                {
                    highColor = m_signals[signalName].lineColor;
                    lowColor = m_signals[signalName].lineColor;
                }
                drawHorizontalHexagon(painter, highColor, lowColor, prevX, x, prevY, hexHeight, displayValue, lineWidth);
                prevX = x;
            }

            rawValue = QString::fromStdString(values[values.size() - 1]);
            if (displaySignal.translator)
            {

                VariableMeta meta = m_waveform->var_to_meta(displaySignal.var_ref);
                TranslatedValue translated = displaySignal.translator->translate(meta, rawValue);
                displayValue = translated.value;
            }
            else
            {
                displayValue = rawValue;
            }
            QColor highColor = m_hexagonalHighColor;
            QColor lowColor = m_hexagonalLowColor;
            if (isSelected)
            {
                lineWidth = 2.0;
                highColor = m_highHexagonalHighColor;
                lowColor = m_highHexagonalLowColor;
            }
            else if (displaySignal.lineColor.isValid())
            {
                highColor = displaySignal.lineColor;
                lowColor = displaySignal.lineColor;
            }
            else if (displayValue.contains('x'))
            {
                highColor = m_segmentXColor;
                lowColor = m_segmentXColor;
            }
            else if (displayValue.contains('z'))
            {
                highColor = m_segmentZColor;
                lowColor = m_segmentZColor;
            }

            if (m_signals[signalName].lineColor.isValid())
            {
                highColor = m_signals[signalName].lineColor;
                lowColor = m_signals[signalName].lineColor;
            }
            drawHorizontalHexagon(painter, highColor, lowColor, prevX, x, prevY, hexHeight, displayValue, lineWidth);
        }
        currentY += m_signalHeight + m_signalSpacing;
    }
    return currentY;
}
int WaveformDisplay::drawSignalNamesArr(QPainter &painter, QVector<QString> signalsArr, int currentY, bool isPinTop)
{
    int nameScrollX = m_nameArea->horizontalScrollBar()->value();

    const int iconSize = 8;
    const int iconMargin = 4;
    const int indentPerLevel = 8;
    const int lineWidth = 1;
    for (int i = 0; i < signalsArr.size(); i++)
    {
        const QString &signalKey = signalsArr[i];
        if (!isPinTop && !m_signals[signalKey].visible)
        {
            continue;
        }
        DisplaySignal &signal = m_signals[signalKey];
        int indentLevel = signal.indent_level;
        QRect signalRect(5 - nameScrollX, currentY, m_nameAreaWidth - 5 + nameScrollX, m_signalHeight);

        if (m_signals[signalKey].nameColor.isValid())
        {
            painter.setPen(m_signals[signalKey].nameColor);
        }
        else
        {
            painter.setPen(m_pen);
        }

        QString signalName = signal.signal_name;
        if (m_hierarchicalDisplay)
        {
            signalName = join_path_with_slash(signal.scopes) + "/" + signalName;
        }
        bool isSelected = m_selectedSignalKeys.contains(signalKey);
        if (signal.backgroundColor.isValid())
        {
            QColor backgroundColor = signal.backgroundColor;
            QRect signalBgRect3(5 - nameScrollX + m_nameAreaWidth + m_valueAreaWidth - 5,
                                currentY, m_waveAreaWidth - 10, m_signalHeight);
            painter.fillRect(signalBgRect3, backgroundColor);
        }
        if (isSelected)
        {
            QColor backgroundColor = m_highLightColor;
            QRect signalBgRect1(5 - nameScrollX, currentY, m_nameAreaWidth - 10, m_signalHeight);
            QRect signalBgRect2(5 - nameScrollX + m_nameAreaWidth + 5, currentY, m_valueAreaWidth - 20, m_signalHeight);
            painter.fillRect(signalBgRect1, backgroundColor);
            painter.fillRect(signalBgRect2, backgroundColor);
        }

        int baseIndent = 5;

        int levelIndent = baseIndent + (indentLevel * indentPerLevel);

        if (indentLevel > 0)
        {
            painter.save();

            QPen linePen(QColor(120, 120, 120));
            linePen.setWidth(lineWidth);
            linePen.setStyle(Qt::DashLine);
            painter.setPen(linePen);

            for (int level = 1; level <= indentLevel; level++)
            {
                int lineX = baseIndent + (level - 1) * indentPerLevel + (indentPerLevel / 2) - nameScrollX;

                bool isLastChild = true;
                for (int j = i + 1; j < signalsArr.size(); j++)
                {
                    if (m_signals[signalsArr[j]].indent_level >= level)
                    {
                        if (m_signals[signalsArr[j]].indent_level == level)
                        {
                            isLastChild = false;
                        }
                        break;
                    }
                }

                int signalCenterY = currentY + m_signalHeight / 2;
                if (level == indentLevel)
                {

                    int lineTopY = currentY;
                    int lineBottomY = currentY + m_signalHeight;

                    if (isLastChild)
                    {
                        lineBottomY = signalCenterY;
                    }

                    painter.drawLine(lineX, lineTopY, lineX, lineBottomY);
                }
                else
                {

                    int lineTopY = currentY;
                    int lineBottomY = currentY + m_signalHeight;

                    if (!isLastChild)
                    {

                        for (int j = i + 1; j < signalsArr.size(); j++)
                        {
                            if (m_signals[signalsArr[j]].indent_level == level)
                            {

                                int nextY = currentY;
                                for (int k = i; k < j; k++)
                                {
                                    nextY += m_signalHeight + m_signalSpacing;
                                }
                                lineBottomY = nextY + m_signalHeight;
                                break;
                            }
                        }
                    }

                    painter.drawLine(lineX, lineTopY, lineX, lineBottomY);
                }
            }
            painter.restore();
        }

        int iconX = levelIndent - nameScrollX;

        int textStartX = levelIndent - nameScrollX;

        if (signal.canExpand)
        {
            int iconY = currentY + (m_signalHeight - iconSize) / 2;
            QRect iconRect(iconX, iconY, iconSize, iconSize);

            painter.save();

            painter.setPen(Qt::NoPen);

            painter.setBrush(QBrush(m_pen.color()));

            QPolygon triangle;
            if (signal.is_expansion)
            {

                triangle << QPoint(iconRect.left(), iconRect.top())
                         << QPoint(iconRect.right(), iconRect.top())
                         << QPoint(iconRect.center().x(), iconRect.bottom());
            }
            else
            {

                triangle << QPoint(iconRect.left(), iconRect.top())
                         << QPoint(iconRect.left(), iconRect.bottom())
                         << QPoint(iconRect.right(), iconRect.center().y());
            }

            painter.drawPolygon(triangle);

            painter.restore();

            textStartX += iconSize + iconMargin;
        }
        else
        {

            textStartX += iconSize + iconMargin;
        }

        QRect textRect(textStartX, signalRect.top(),
                       signalRect.width() - (textStartX - signalRect.left()),
                       signalRect.height());

        QString displayText = signalName;
        int textWidth = painter.fontMetrics().horizontalAdvance(displayText);
        int availableWidth = textRect.width() - 2;
        if (textWidth > availableWidth)
        {

            displayText = painter.fontMetrics().elidedText(signalName, Qt::ElideRight, availableWidth);
        }

        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, displayText);
        currentY += m_signalHeight + m_signalSpacing;
    }
    return currentY;
}
void WaveformDisplay::drawScrollableSignalNamesAndGroups(QPainter &painter)
{
    int startY = getTopPinSignalsAreaBottomY();
    painter.translate(0, startY);

    painter.setPen(m_pen);
    QFont font = painter.font();
    font.setBold(true);
    painter.setFont(font);
    int nameScrollX = m_nameArea->horizontalScrollBar()->value();
    int currentY = 0;
    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
        {
            continue;
        }
        const SignalGroup &group = m_groups[groupName];
        QRect groupRect(-nameScrollX, currentY, m_nameAreaWidth + nameScrollX - 16, m_groupHeaderHeight);
        painter.fillRect(groupRect, m_groupRectColor);
        painter.setPen(m_pen);

        int triangleSize = 8;
        int triangleX = 5 - nameScrollX;
        int triangleY = currentY + m_groupHeaderHeight / 2;
        if (group.collapsed)
        {
            QPointF points[3] = {
                QPointF(triangleX, triangleY - triangleSize / 2),
                QPointF(triangleX + triangleSize, triangleY),
                QPointF(triangleX, triangleY + triangleSize / 2)};
            painter.drawPolygon(points, 3);
        }
        else
        {
            QPointF points[3] = {
                QPointF(triangleX, triangleY - triangleSize / 2),
                QPointF(triangleX + triangleSize, triangleY - triangleSize / 2),
                QPointF(triangleX + triangleSize / 2, triangleY + triangleSize / 2)};
            painter.drawPolygon(points, 3);
        }
        QRect textRect(20 - nameScrollX, currentY, m_nameAreaWidth - 20 + nameScrollX - 16, m_groupHeaderHeight);
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, groupName);
        painter.drawRect(groupRect);
        currentY += m_groupHeaderHeight + m_signalSpacing;
        if (!group.collapsed)
        {
            currentY = drawSignalNamesArr(painter, group.v_signals, currentY);
        }
    }
}

double WaveformDisplay::timeToX(double time) const
{
    double timeRange = m_maxTime - m_minTime;
    if (timeRange <= 0)
    {
        return m_nameAreaWidth + m_valueAreaWidth;
    }

    int waveAreaAvailableWidth = m_waveAreaWidth;

    if (m_waveArea && m_waveArea->verticalScrollBar() &&
            m_waveArea->verticalScrollBar()->isVisible())
    {
        waveAreaAvailableWidth -= m_waveArea->verticalScrollBar()->width();
    }
    return m_nameAreaWidth + m_valueAreaWidth + m_horizontalOffset +
            (time - m_minTime) * waveAreaAvailableWidth / timeRange;
}
Time WaveformDisplay::xToTime(double x) const
{
    double timeRange = m_maxTime - m_minTime;
    if (timeRange <= 0)
    {
        return 0;
    }

    int waveAreaAvailableWidth = m_waveAreaWidth;
    if (m_waveArea && m_waveArea->verticalScrollBar() &&
            m_waveArea->verticalScrollBar()->isVisible())
    {
        waveAreaAvailableWidth -= m_waveArea->verticalScrollBar()->width();
    }
    double time = m_minTime + (x - m_nameAreaWidth - m_valueAreaWidth - m_horizontalOffset) * timeRange /
            waveAreaAvailableWidth;
    return (Time)time;
}
int WaveformDisplay::getGroupPosition(const QString &groupName) const
{
    return m_groupOrder.indexOf(groupName);
}
int WaveformDisplay::getSignalPositionInGroup(const QString &signalName, const QString &groupName) const
{
    if (m_groups.contains(groupName))
    {
        return m_groups[groupName].v_signals.indexOf(signalName);
    }
    return -1;
}
void WaveformDisplay::updateMouseTracking(const QPoint &pos)
{
    qDebug() << "updateMouseTracking";

    QPoint adjustedPos = pos;
    adjustedPos.setY(adjustedPos.y() + m_verticalOffset);
    adjustedPos.setX(adjustedPos.x() + m_horizontalOffset);
    if (adjustedPos.x() < m_nameAreaWidth || adjustedPos.y() < m_timeRulerHeight)
    {
        m_hoverSignal.clear();
        return;
    }

    m_hoverTime = xToTime(adjustedPos.x());

    int groupIndex = 0;
    int currentSignalPositionInGroup = -1;
    QString currentPositionGroupName = "";
    int currentSignalPosition = getViewSignalSpacingIndexAndGroupAtY(adjustedPos.y(), groupIndex, currentSignalPositionInGroup, currentPositionGroupName);
    if (currentSignalPosition < 0)
    {
        m_hoverSignal.clear();
        return;
    }

    QStringList allSignals = getAllSignalsInOrder();
    if (currentSignalPosition >= allSignals.size())
    {
        m_hoverSignal.clear();
        return;
    }
    QString signalName = allSignals[currentSignalPosition];
    if (!m_signals.contains(signalName))
    {
        m_hoverSignal.clear();
        return;
    }
    const auto &displaySignal = m_signals[signalName];
    if (displaySignal.signal.isNull())
    {
        m_hoverSignal.clear();
        return;
    }
    m_hoverSignal = signalName;
    m_hoverValue = getSignalValueAtTime(displaySignal, m_hoverTime);
    update();
}

int WaveformDisplay::getGroupSignalVisibleSize(QString groupName) const
{
    if (!m_groups.contains(groupName))
    {
        return 0;
    }

    int visibleSignalSize = 0;
    const SignalGroup &group = m_groups[groupName];
    for (int i = 0; i < group.v_signals.size(); i++)
    {
        if (!m_signals[group.v_signals[i]].visible)
        {
            continue;
        }
        visibleSignalSize++;
    }
    return visibleSignalSize;
}

int WaveformDisplay::getViewSignalSpacingIndexAndGroupAtY(int y, int &groupIndex, int &currentSignalPositionInGroup, QString &currentPositionGroupName) const
{

    if (y < m_timeRulerHeight || y >= height() + m_verticalOffset)
    {
        return -2;
    }

    if (y < getTopPinSignalsAreaBottomY())
    {
        return -2;
    }

    int scrollY = y - getTopPinSignalsAreaBottomY();
    int currentY = 0;
    int signalIndex = -1;
    currentSignalPositionInGroup = -1;
    groupIndex = 0;

    for (const QString &groupName : m_groupOrder)
    {
        currentSignalPositionInGroup = -1;
        if (!m_groups.contains(groupName))
            continue;
        currentPositionGroupName = groupName;
        const SignalGroup &group = m_groups[groupName];
        currentY += m_groupHeaderHeight + m_signalSpacing;

        if (!group.collapsed)
        {

            if (scrollY >= currentY - m_signalSpacing && scrollY <= currentY)
            {
                return signalIndex;
            }
            for (int i = 0; i < group.v_signals.size(); i++)
            {
                if (!m_signals[group.v_signals[i]].visible)
                {
                    continue;
                }
                signalIndex++;
                currentSignalPositionInGroup++;
                if (scrollY >= currentY && scrollY < currentY + m_signalHeight + m_signalSpacing)
                {
                    return signalIndex;
                }
                currentY += m_signalHeight + m_signalSpacing;
            }
        }
        groupIndex++;
    }

    groupIndex--;
    if (currentSignalPositionInGroup == -1)
    {
        currentSignalPositionInGroup = 0;
    }

    if (scrollY > currentY)
    {
        return signalIndex;
    }
    return -2;
}

int WaveformDisplay::getViewSignalIndexInGroupAtY(int y)
{

    if (y < m_timeRulerHeight || y >= height() + m_verticalOffset)
    {
        return -1;
    }
    if (y < getTopPinSignalsAreaBottomY())
    {
        return -1;
    }
    int scrollY = y - getTopPinSignalsAreaBottomY();
    int currentY = 0;
    int signalIndex = -1;

    for (const QString &groupName : m_groupOrder)
    {
        signalIndex = -1;

        if (!m_groups.contains(groupName))
            continue;
        const SignalGroup &group = m_groups[groupName];
        currentY += m_groupHeaderHeight + m_signalSpacing;

        if (scrollY <= currentY)
        {
            return signalIndex;
        }

        if (!group.collapsed)
        {

            for (int i = 0; i < group.v_signals.size(); i++)
            {
                if (!m_signals[group.v_signals[i]].visible)
                {
                    continue;
                }
                signalIndex++;
                if (scrollY < currentY + m_signalHeight + m_signalSpacing)
                {
                    return signalIndex;
                }
                currentY += m_signalHeight + m_signalSpacing;
            }
        }
    }
    if (scrollY > currentY)
    {
        return signalIndex;
    }
    return -1;
}

QString WaveformDisplay::getSignalGroupNameAtY(int y) const
{
    if (y < m_timeRulerHeight || y >= height() + m_verticalOffset || y < getTopPinSignalsAreaBottomY())
    {
        return QString();
    }

    int scrollY = y - getTopPinSignalsAreaBottomY();
    int currentY = 0;
    QString signalGroupName = "";

    for (const QString &groupName : m_groupOrder)
    {
        signalGroupName = groupName;
        if (!m_groups.contains(groupName))
            continue;
        const SignalGroup &group = m_groups[groupName];

        if (scrollY >= currentY && scrollY <= currentY + m_groupHeaderHeight + m_signalSpacing)
        {
            return signalGroupName;
        }

        currentY += m_groupHeaderHeight + m_signalSpacing;

        if (!group.collapsed)
        {
            if (scrollY >= currentY - m_signalSpacing && scrollY <= currentY)
            {
                return signalGroupName;
            }
            for (int i = 0; i < group.v_signals.size(); i++)
            {
                if (!m_signals[group.v_signals[i]].visible)
                {
                    continue;
                }
                if (scrollY >= currentY && scrollY < currentY + m_signalHeight + m_signalSpacing)
                {
                    return signalGroupName;
                }
                currentY += m_signalHeight + m_signalSpacing;
            }
        }
    }
    if (scrollY > currentY)
    {
        return signalGroupName;
    }
    return QString();
}

int WaveformDisplay::getRealSignalIndexInGroupAtY(int y) const
{
    if (y < m_timeRulerHeight || y >= height() + m_verticalOffset)
    {
        return -1;
    }
    if (y < getTopPinSignalsAreaBottomY())
    {
        return -1;
    }
    int scrollY = y - getTopPinSignalsAreaBottomY();
    int currentY = 0;
    int signalIndex = -1;

    for (const QString &groupName : m_groupOrder)
    {
        signalIndex = -1;

        if (!m_groups.contains(groupName))
            continue;
        const SignalGroup &group = m_groups[groupName];
        currentY += m_groupHeaderHeight + m_signalSpacing;

        if (!group.collapsed)
        {
            if (scrollY >= currentY - m_signalSpacing && scrollY <= currentY)
            {
                return signalIndex;
            }
            for (int i = 0; i < group.v_signals.size(); i++)
            {
                if (!m_signals[group.v_signals[i]].visible)
                {
                    signalIndex++;
                    continue;
                }
                signalIndex++;
                if (scrollY >= currentY && scrollY < currentY + m_signalHeight + m_signalSpacing)
                {
                    return signalIndex;
                }
                currentY += m_signalHeight + m_signalSpacing;
            }
        }
    }
    if (scrollY > currentY)
    {
        return signalIndex;
    }
    return -1;
}

int WaveformDisplay::getViewSignalGroupIndexAtY(int y) const
{
    if (y < m_timeRulerHeight || y >= height() + m_verticalOffset)
    {
        return -1;
    }
    if (y < getTopPinSignalsAreaBottomY())
    {
        return -1;
    }
    int scrollY = y - getTopPinSignalsAreaBottomY();
    int currentY = 0;
    int groupIndex = -1;

    for (const QString &groupName : m_groupOrder)
    {
        groupIndex++;
        if (!m_groups.contains(groupName))
            continue;

        const SignalGroup &group = m_groups[groupName];
        currentY += m_groupHeaderHeight + m_signalSpacing;
        if (scrollY <= currentY)
        {
            return groupIndex;
        }
        if (!group.collapsed)
        {
            for (int i = 0; i < group.v_signals.size(); i++)
            {
                if (!m_signals[group.v_signals[i]].visible)
                {
                    continue;
                }

                if (scrollY < currentY + m_signalHeight + m_signalSpacing)
                {
                    return groupIndex;
                }
                currentY += m_signalHeight + m_signalSpacing;
            }
        }
    }
    return groupIndex;
}

QString WaveformDisplay::getSignalKeyInScrollerArea(int y) const
{
    if (y < m_timeRulerHeight || y >= height() + m_verticalOffset || y < getTopPinSignalsAreaBottomY())
    {
        return QString();
    }
    int realSignalIndexInGroup = getRealSignalIndexInGroupAtY(y);
    QString groupName = getSignalGroupNameAtY(y);
    if (groupName.isEmpty())
    {
        return QString();
    }
    if (realSignalIndexInGroup >= 0)
    {
        QString targetSignalKey = m_groups[groupName].v_signals[realSignalIndexInGroup];
        return targetSignalKey;
    }
    return QString();
}

QString WaveformDisplay::getSignalKeyInTopArea(int y) const
{
    if (y < m_timeRulerHeight || y >= height() + m_verticalOffset || y >= getTopPinSignalsAreaBottomY())
    {
        return QString();
    }
    int currentY = m_timeRulerHeight;

    for (const QString &signalKey : m_pinTopSignals)
    {
        if (y >= currentY && y < currentY + m_signalHeight)
        {
            return signalKey;
        }
        currentY += m_signalHeight + m_signalSpacing;
    }
    return QString();
}

int WaveformDisplay::getSignalCurrentY(int y) const
{
    if (y < m_timeRulerHeight || y >= height() + m_verticalOffset)
    {
        return -1;
    }
    int currentY = m_timeRulerHeight;
    int groupIndex = 0;
    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
        {
            groupIndex++;
            continue;
        }
        const SignalGroup &group = m_groups[groupName];
        currentY += m_groupHeaderHeight + m_signalSpacing;
        if (!group.collapsed)
        {
            for (int signalIdx = 0; signalIdx < group.v_signals.size(); signalIdx++)
            {
                if (!m_signals[group.v_signals[signalIdx]].visible)
                {
                    continue;
                }
                if (y >= currentY && y < currentY + m_signalHeight)
                {
                    return currentY;
                }
                currentY += m_signalHeight + m_signalSpacing;
            }
        }
        groupIndex++;
    }
    return currentY;
}
int WaveformDisplay::getGroupIndexAtY(int y) const
{

    if (y < m_timeRulerHeight || y >= height() + m_verticalOffset)
    {
        return -1;
    }

    if (y < getTopPinSignalsAreaBottomY())
    {
        return -1;
    }

    int scrollY = y - getTopPinSignalsAreaBottomY();
    int currentY = 0;
    int groupIndex = 0;
    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
        {
            continue;
        }
        const SignalGroup &group = m_groups[groupName];

        if (scrollY >= currentY && scrollY < currentY + m_groupHeaderHeight)
        {
            return groupIndex;
        }
        currentY += m_groupHeaderHeight + m_signalSpacing;

        if (!group.collapsed)
        {
            int size = getGroupSignalVisibleSize(groupName);
            currentY += size * (m_signalHeight + m_signalSpacing);
        }
        groupIndex++;
    }
    return -1;
}
int WaveformDisplay::getGroupYPosition(const QString &groupName) const
{
    int position = getGroupPosition(groupName);
    if (position < 0)
    {
        return -1;
    }
    int y = m_timeRulerHeight;
    for (const QString &name : m_pinTopSignals)
    {
        y = y + m_signalHeight + m_signalSpacing;
    }
    for (int i = 0; i < position; i++)
    {
        if (i < m_groupOrder.size())
        {
            QString currentGroup = m_groupOrder[i];
            if (m_groups.contains(currentGroup))
            {
                y += m_groupHeaderHeight + m_signalSpacing;
                if (!m_groups[currentGroup].collapsed)
                {
                    int size = getGroupSignalVisibleSize(currentGroup);
                    y += size * (m_signalHeight + m_signalSpacing);
                }
            }
        }
    }
    return y;
}
void WaveformDisplay::updateGroupOrder(int fromIndex, int toIndex)
{
    if (fromIndex == toIndex ||
            fromIndex < 0 || fromIndex >= m_groupOrder.size() ||
            toIndex < 0 || toIndex >= m_groupOrder.size())
    {
        return;
    }

    m_groupOrder.move(fromIndex, toIndex);
    update();
}
QStringList WaveformDisplay::getAllSignalsInOrder() const
{
    QStringList allSignals;
    for (const QString &groupName : m_groupOrder)
    {
        if (m_groups.contains(groupName) && !m_groups[groupName].collapsed)
        {
            allSignals.append(m_groups[groupName].v_signals);
        }
    }
    return allSignals;
}
double WaveformDisplay::valueToY(const int currentY, const QString &signalName, const QString &value, bool isMulBit) const
{

    QString groupName = signalGroup(signalName);
    if (groupName.isEmpty())
    {
        return 0;
    }
    int groupIndex = getGroupPosition(groupName);
    int signalIndexInGroup = getSignalPositionInGroup(signalName, groupName);
    if (groupIndex < 0 || signalIndexInGroup < 0)
    {
        return currentY;
    }
    int baseY = currentY;

    baseY += m_signalHeight / 2;
    if (isMulBit)
    {
        return baseY + m_signalHeight / 3;
    }

    if (value == "1" || value.toLower() == "h" || value.toLower() == "high")
    {
        return baseY - m_signalHeight / 2 + 2;
    }
    else if (value == "0" || value.toLower() == "l" || value.toLower() == "low")
    {
        return baseY + m_signalHeight / 2 - 2;
    }
    else if (value.toLower() == "x" || value.toLower() == "unknown")
    {

        return baseY;
    }
    else if (value.toLower() == "z" || value.toLower() == "highz")
    {

        return baseY - m_signalHeight / 8;
    }

    return baseY;
}
void WaveformDisplay::drawHorizontalHexagon(QPainter &painter, const QColor &highColor, const QColor &lowColor,
                                            double startX, double endX, double centerY, double height,
                                            const QString &valueText, double lineWidth)
{
    if (startX >= endX)
        return;

    double waveAreaStartX = m_nameAreaWidth + m_valueAreaWidth;
    if (endX < waveAreaStartX)
        return;

    startX = std::max(startX, waveAreaStartX);
    double radius = height / 2.0;

    double angle = 80.0;
    double angleRad = qDegreesToRadians(angle);
    double slopeWidth = radius / tan(angleRad);

    double maxSlopeWidth = (endX - startX) / 2.0;
    if (slopeWidth > maxSlopeWidth)
    {
        slopeWidth = maxSlopeWidth;
    }

    QPainterPath path = QPainterPath();
    path.moveTo(startX, centerY);
    path.lineTo(startX + slopeWidth, centerY - radius);
    path.lineTo(endX - slopeWidth, centerY - radius);
    path.lineTo(endX, centerY);
    painter.setPen(QPen(highColor, lineWidth));
    painter.drawPath(path);
    path = QPainterPath();
    path.moveTo(endX, centerY);
    path.lineTo(endX - slopeWidth, centerY + radius);
    path.lineTo(startX + slopeWidth, centerY + radius);
    path.lineTo(startX, centerY);
    painter.setPen(QPen(lowColor, lineWidth));
    painter.drawPath(path);

    double textWidth = (endX - slopeWidth) - (startX + slopeWidth);

    if (textWidth <= 0)
        return;

    QRectF textRect(startX + slopeWidth, centerY - radius, textWidth, height);
    painter.setPen(QColor(m_waveformSignalTextColor));

    QFontMetrics fm(painter.font());
    int fullTextWidth = fm.horizontalAdvance(valueText);

    QString displayText;
    if (textWidth >= fullTextWidth)
    {

        displayText = valueText;
    }
    else if (textWidth >= fm.horizontalAdvance(".."))
    {

        int maxChars = valueText.length();
        int displayChars = 0;

        for (int i = maxChars; i >= 2; i--)
        {
            QString candidate = valueText.left(i) + "..";
            if (fm.horizontalAdvance(candidate) <= textWidth)
            {
                displayChars = i;
                break;
            }
        }
        if (displayChars >= 2)
        {
            displayText = valueText.left(displayChars) + "..";
        }
        else
        {

            return;
        }
    }
    else
    {

        return;
    }

    painter.drawText(textRect, Qt::AlignCenter, displayText);
}
void WaveformDisplay::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (m_firstShow)
    {
        m_firstShow = false;
        QList<int> initialSizes;
        initialSizes << 0.1 * width() << 0.08 * width() << 0.82 * width();
        m_horizontalSplitter->setSizes(initialSizes);
        updateAreaWidth();
    }
}
void WaveformDisplay::updateAreaWidth()
{

    QList<int> sizes = m_horizontalSplitter->sizes();
    if (sizes.size() >= 3)
    {
        m_nameAreaWidth = sizes[0] + 1;
        m_valueAreaWidth = sizes[1] + 1;
        m_waveAreaWidth = sizes[2];
    }

    m_searchLineEdit->setFixedSize(m_nameAreaWidth, m_timeRulerHeight);
}
void WaveformDisplay::updateScrollBars()
{

    m_contentHeight = m_timeRulerHeight;

    for (const QString &name : m_pinTopSignals)
    {
        m_contentHeight += m_signalHeight + m_signalSpacing;
    }

    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
            continue;
        const SignalGroup &group = m_groups[groupName];
        m_contentHeight += m_groupHeaderHeight + m_signalSpacing;
        if (!group.collapsed)
        {
            for (const QString &signalName : group.v_signals)
            {
                if (m_signals.contains(signalName) && m_signals[signalName].visible)
                {
                    m_contentHeight += m_signalHeight + m_signalSpacing;
                }
            }
        }
    }

    int viewportHeight = height() - getTopPinSignalsAreaBottomY();
    int maxVertical = std::max(0, m_contentHeight - viewportHeight);

    int nameContentWidth = m_signalNameLongestWidth;
    int nameAreaViewportWidth = m_nameArea->viewport()->width();
    int nameAreaHorizontalMax = std::max(0, nameContentWidth - nameAreaViewportWidth);
    m_nameArea->horizontalScrollBar()->setRange(0, nameAreaHorizontalMax);
    m_nameArea->horizontalScrollBar()->setPageStep(nameAreaViewportWidth);
    m_nameArea->horizontalScrollBar()->setSingleStep(10);

    int valueContentWidth = m_signalValueLongestWidth;
    int valueAreaViewportWidth = m_valueArea->viewport()->width();
    int valueAreaHorizontalMax = std::max(0, valueContentWidth - valueAreaViewportWidth);
    m_valueArea->horizontalScrollBar()->setRange(0, valueAreaHorizontalMax);
    m_valueArea->horizontalScrollBar()->setPageStep(valueAreaViewportWidth);
    m_valueArea->horizontalScrollBar()->setSingleStep(5);

    m_nameArea->verticalScrollBar()->setRange(0, maxVertical);
    m_valueArea->verticalScrollBar()->setRange(0, maxVertical);
    m_waveArea->verticalScrollBar()->setRange(0, maxVertical);

    m_nameArea->verticalScrollBar()->setPageStep(viewportHeight);
    m_valueArea->verticalScrollBar()->setPageStep(viewportHeight);
    m_waveArea->verticalScrollBar()->setPageStep(viewportHeight);

    m_nameArea->verticalScrollBar()->setSingleStep(m_signalHeight);
    m_valueArea->verticalScrollBar()->setSingleStep(m_signalHeight);
    m_waveArea->verticalScrollBar()->setSingleStep(m_signalHeight);

    double timeRange = m_globalMaxTime - m_globalMinTime;
    int contentWidth = timeRange * m_pixelsPerTimeUnit;

    m_waveArea->horizontalScrollBar()->setRange(0, contentWidth - m_waveAreaWidth);
    m_waveArea->horizontalScrollBar()->setPageStep(m_waveAreaWidth);

    double timeOffset = m_minTime - m_globalMinTime;
    int scrollValue = static_cast<int>(timeOffset * m_pixelsPerTimeUnit);
    m_waveArea->horizontalScrollBar()->setValue(scrollValue);
}
void WaveformDisplay::handleVerticalScroll(int value)
{
    m_verticalOffset = value;
    update();
}
void WaveformDisplay::handleHorizontalScroll(int value)
{
    m_horizontalOffset = value;
    double timeRange = m_maxTime - m_minTime;
    double visibleStartTime = m_globalMinTime + value / m_pixelsPerTimeUnit;
    setTimeRange(visibleStartTime, visibleStartTime + timeRange);
    update();
}
void WaveformDisplay::handleSplitterMoved(int pos, int index)
{
    Q_UNUSED(pos);
    updateAreaWidth();
    if (index == 2)
    {
        updateContentWidth();
    }
    else
    {
        updateScrollBars();
    }
    update();
}
void WaveformDisplay::syncVerticalScroll(int value)
{
    if (m_syncingScroll)
        return;
    m_syncingScroll = true;

    QScrollBar *senderBar = qobject_cast<QScrollBar *>(sender());
    if (senderBar != m_nameArea->verticalScrollBar())
    {
        m_nameArea->verticalScrollBar()->setValue(value);
    }
    if (senderBar != m_valueArea->verticalScrollBar())
    {
        m_valueArea->verticalScrollBar()->setValue(value);
    }
    if (senderBar != m_waveArea->verticalScrollBar())
    {
        m_waveArea->verticalScrollBar()->setValue(value);
    }

    m_verticalOffset = value;
    m_syncingScroll = false;
    update();
}
void WaveformDisplay::syncHorizontalScroll(int value)
{
    if (m_syncingScroll)
        return;
    m_syncingScroll = true;

    m_horizontalOffset = value;

    double timeRange = m_maxTime - m_minTime;
    double visibleStartTime = m_globalMinTime + value / m_pixelsPerTimeUnit;
    setTimeRange(visibleStartTime, visibleStartTime + timeRange);
    m_syncingScroll = false;
    update();
}
QString WaveformDisplay::getSignalValueAtTimeOnly(QSharedPointer<Signal> signal, Time time)
{
    auto idx = m_waveform->get_index_by_signal_and_time(signal, time);
    if (idx >= 0)
        return QString::fromStdString(signal->get_signal_values()[idx]);
    else
        return "";
}
QString WaveformDisplay::getSignalValueAtTime(const DisplaySignal &displaySignal, Time time)
{
    QSharedPointer<Signal> signal = displaySignal.signal;
    Time before_time = m_waveform->get_next_time(signal, time, false);
    QString cur_value = getSignalValueAtTimeOnly(signal, time);
    QString before_value = getSignalValueAtTimeOnly(signal, before_time);
    QString trans_cur_value;
    QString trans_before_value;
    if (displaySignal.translator)
    {

        VariableMeta meta = m_waveform->var_to_meta(displaySignal.var_ref);
        TranslatedValue translated = displaySignal.translator->translate(meta, cur_value);
        trans_cur_value = translated.value;
    }
    else
    {
        trans_cur_value = cur_value;
    }
    if (before_value != cur_value && time > 1)
    {
        if (displaySignal.translator)
        {

            VariableMeta meta = m_waveform->var_to_meta(displaySignal.var_ref);
            TranslatedValue translated = displaySignal.translator->translate(meta, before_value);
            trans_before_value = translated.value;
        }
        else
        {
            trans_before_value = before_value;
        }
        return trans_before_value + "->" + trans_cur_value;
    }

    return trans_cur_value;
}
void WaveformDisplay::changeSignalValue()
{
    SignalCrusorTimeAndPosition x = getDistanceFromLastTimeAndNextTime();

    if (m_selectTime < 0)
    {
        return;
    }

    for (const QString &groupName : m_groupOrder)
    {
        if (!m_groups.contains(groupName))
        {
            continue;
        }
        SignalGroup &group = m_groups[groupName];

        for (const QString &signalName : group.v_signals)
        {
            if (!m_signals.contains(signalName))
            {
                continue;
            }
            auto &displaySignal = m_signals[signalName];
            if (displaySignal.signal.isNull())
            {
                continue;
            }

            QString value = getSignalValueAtTime(displaySignal, m_selectTime);

            displaySignal.currentValue = value;
            displaySignal.valueTime = m_selectTime;
        }
    }

    update();
}
void WaveformDisplay::setSelectTimeWithDistance()
{

    QString signalKey = findSignalAtClickPosition();
    if (!signalKey.isEmpty() && m_signals.contains(signalKey))
    {

        QString originalSelectedSignal = m_lastSelectedSignalKey;
        m_lastSelectedSignalKey = signalKey;

        SignalCrusorTimeAndPosition x = getDistanceFromLastTimeAndNextTime();
        if (x.lastTimeDistance >= 0 && x.nextTimeDistance >= 0)
        {

            if (x.lastTimeDistance > x.nextTimeDistance)
            {
                setSelectTime(x.nextTime);
            }
            else
            {
                setSelectTime(x.lastTime);
            }
        }
        else
        {

            setSelectTime(m_clickTime);
        }

        m_lastSelectedSignalKey = originalSelectedSignal;
    }
    else
    {

        setSelectTime(m_clickTime);
    }
}

QString WaveformDisplay::findSignalAtClickPosition()
{
    QPoint clickPos = m_dragStartPosWithOffset;
    if (m_dragStartPosWithoutOffset.y() < getTopPinSignalsAreaBottomY())
    {
        QString signalKey = getSignalKeyInTopArea(m_dragStartPosWithoutOffset.y());
        return signalKey;
    }
    else
    {
        QString signalKey = getSignalKeyInScrollerArea(clickPos.y());
        return signalKey;
    }
    return QString();
}

void WaveformDisplay::setSelectTime(Time time)
{
    if (m_selectTime != time)
    {
        m_selectTime = time;
        changeSignalValue();

        recalculateMaxWidths();

        int valueViewportWidth = m_valueArea->viewport()->width();
        if (m_signalValueLongestWidth > valueViewportWidth)
        {
            m_valueArea->horizontalScrollBar()->setValue(m_signalValueLongestWidth - valueViewportWidth);
        }
        updateContentWidth();
    }
}
void WaveformDisplay::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange)
    {

        recalculateMaxWidths();
        updateContentWidth();
    }
    QWidget::changeEvent(event);
}
QString WaveformDisplay::join_path_with_dot(const QVector<QString> &path)
{
    if (path.empty())
        return QString();
    QString result = path[0];
    for (size_t i = 1; i < path.size(); ++i)
    {
        if (!path[i].isEmpty())
        {
            result += "." + path[i];
        }
    }
    return result;
}
QString WaveformDisplay::join_path_with_slash(const QVector<QString> &path)
{
    if (path.empty())
        return QString();
    QString result = path[0];
    for (size_t i = 1; i < path.size(); ++i)
    {
        if (!path[i].isEmpty())
        {
            result += "/" + path[i];
        }
    }
    return result;
}
void WaveformDisplay::updateRadixMenu()
{

    m_radixMenu->clear();

    if (!m_signals.contains(m_lastSelectedSignalKey))
        return;
    auto &signal = m_signals[m_lastSelectedSignalKey];

    VariableMeta meta = m_waveform->var_to_meta(signal.var_ref);

    QSharedPointer<Translator> currentTranslator = signal.translator;

    QList<QSharedPointer<Translator>> applicableTranslators;
    for (auto &t : m_translatorManager->getAllTranslators())
    {
        if (t->preference(meta) > 0)
        {
            applicableTranslators.append(t);
        }
    }

    for (auto &translator : applicableTranslators)
    {
        QAction *action = m_radixMenu->addAction(translator->name());
        action->setCheckable(true);
        action->setChecked(translator == currentTranslator);
        connect(action, &QAction::triggered, this, [this, translator]()
        {
            for (const QString &signalKey : m_selectedSignalKeys) {
                if (m_signals.contains(signalKey)) {
                    m_signals[signalKey].translator = translator;
                    if(translator->name()=="FSMStatus"){
                        QString fullPath = join_path_with_dot(m_signals[signalKey].scopes) +
                                "." + m_signals[signalKey].signal_name;
                        emit getStatusByFullPath(QString::number(m_signals[signalKey].var_ref),fullPath);
                    }
                    if (m_selectTime >= 0) {
                        m_signals[signalKey].currentValue = getSignalValueAtTime( m_signals[signalKey], m_selectTime);
                        m_signals[signalKey].valueTime = m_selectTime;
                    }
                    update();
                }
            } });
    }
}
QString WaveformDisplay::getSignalValueByVarRef(const VarRef var_ref, const QSharedPointer<Signal> &signal)
{
    DisplaySignal displaySignal;
    bool found = false;
    for (const auto &[key, signal] : m_signals.asKeyValueRange())
    {
        if (signal.var_ref == var_ref)
        {
            displaySignal = signal;
            found = true;
            break;
        }
    }
    if (!found)
    {
        displaySignal.var_ref = var_ref;
        displaySignal.signal = signal;
        auto meta = m_waveform->var_to_meta(var_ref);
        QSharedPointer<Translator> defaultTranslator = m_translatorManager->findTranslator(meta);
        displaySignal.translator = defaultTranslator;
    }

    return getSignalValueAtTime(displaySignal, m_selectTime) + displaySignal.translator->prefixName();
}
void WaveformDisplay::setSearchTime(QString selectTime)
{
    bool ok;
    Time clickTime = selectTime.toULongLong(&ok);
    if (!ok)
    {
        return;
    }
    m_clickTime = clickTime;
    setSelectTime(clickTime);
    ensureCursorVisible();
    m_showClickIndicator = true;
    m_clickPosition = m_dragStartPosWithOffset;
    update();
}
QSet<VarRef> WaveformDisplay::getDisplayVarRefs()
{
    QSet<VarRef> varRefSet;
    for (SignalGroup &signalGroup : m_groups)
    {
        for (QString signalName : signalGroup.v_signals)
        {
            if (m_signals.contains(signalName))
            {
                const DisplaySignal &displaySignal = m_signals[signalName];
                varRefSet.insert(displaySignal.var_ref);
            }
        }
    }
    return varRefSet;
}
int WaveformDisplay::getRemainWidth(std::vector<DimInfo> dimInfoVec, int idx)
{
    int width = 1;
    for (int i = idx + 1; i < dimInfoVec.size(); i++)
    {
        width *= dimInfoVec[i].width();
    }
    return width;
}
QString WaveformDisplay::getRemainDimInfoStr(std::vector<DimInfo> dimInfoVec, int idx)
{
    QString multiarray;
    for (int i = idx + 1; i < dimInfoVec.size(); i++)
    {
        multiarray += dimInfoVec[i].to_string();
    }
    return multiarray;
}
void WaveformDisplay::expandMultiArray(DisplaySignal &signal, QString groupName, int signalIndex)
{
    Var var = m_waveform->get_hierarchy().get_var(signal.var_ref);
    std::vector<DimInfo> dimInfoVec = m_waveform->parse_dim_string(var.multi_array);
    if (!var.multi_array.empty() && var.signal_type.width > 1)
    {
        if (!signal.is_expansion && signal.expansion_id == 0)
        {
            QString multiarray;
            signal.expansion_id = expansion_id++;
            signal.is_expansion = true;
            for (int j = 0; j < dimInfoVec.size(); j++)
            {
                DimInfo dimInfo = dimInfoVec[j];
                if (!dimInfo.is_range)
                {
                    multiarray += dimInfo.to_string();
                }
                else
                {
                    int width = getRemainWidth(dimInfoVec, j);
                    QString remainMultiArray = getRemainDimInfoStr(dimInfoVec, j);
                    for (int i = dimInfo.get_logical_min(); i <= dimInfo.get_logical_max(); i++)
                    {
                        QString old_var_name = m_waveform->get_hierarchy().get_full_scope_path(var.parent_scope) + "." + QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id)) + "[" + QString::number(i) + "]" + remainMultiArray;
                        VarRef var_ref = m_waveform->get_hierarchy().get_varRef_by_hierarchy(old_var_name.toStdString());
                        if (var_ref != INVALID_VAR_REF)
                        {
                            const Var &exist_var = m_waveform->get_hierarchy().get_var(var_ref);
                            QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(exist_var.name_id)) + QString::fromStdString(exist_var.multi_array);
                            SignalRef signal_ref = exist_var.handle;
                            m_reader->load_signal_data(m_waveform, signal_ref);
                            QSharedPointer<Signal> signal2 = m_waveform->getSignalShared(signal_ref);
                            addSignal(var_ref, var_name, signal2, m_currentPositionGroupName, m_currentSignalPositionInGroup + 1, signal.expansion_id, signal.indent_level + 1);
                        }
                        else
                        {
                            var_ref = m_waveform->get_hierarchy().add_var_by_var_multiarray(var, i, multiarray, remainMultiArray, width);
                            Var newVar = m_waveform->get_hierarchy().get_var(var_ref);
                            QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(newVar.name_id)) + QString::fromStdString(newVar.multi_array);
                            QSharedPointer<Signal> signal2 = m_waveform->extract_bit(m_waveform, signal.var_ref, i, dimInfo, width);
                            if (!signal2)
                            {
                                QMessageBox::critical(this, "Error", "Failed to create logical signal!");
                                return;
                            }

                            m_waveform->add_signal(newVar.handle, signal2);
                            addSignal(var_ref, var_name, signal2, m_currentPositionGroupName, m_currentSignalPositionInGroup + 1, signal.expansion_id, signal.indent_level + 1);
                        }
                    }
                    break;
                }
            }
        }
        else if (signal.is_expansion)
        {
            int old_expansion_id = signal.expansion_id;
            signal.is_expansion = false;
            signal.expansion_id = expansion_id++;
            QSet<int> expansion_set;
            if (m_groups.contains(groupName))
            {
                const SignalGroup &group = m_groups[groupName];
                while (signalIndex >= 0 && group.v_signals.size() > ++signalIndex)
                {
                    QString signal_key = group.v_signals[signalIndex];
                    if (!m_signals.contains(signal_key))
                    {
                        break;
                    }
                    DisplaySignal &child_signal = m_signals[signal_key];
                    if (child_signal.parent_expansion_id != old_expansion_id && !expansion_set.contains(child_signal.parent_expansion_id))
                    {
                        break;
                    }
                    child_signal.visible = false;
                    if (child_signal.expansion_id != 0)
                    {
                        expansion_set.insert(child_signal.expansion_id);
                        child_signal.is_expansion = false;
                    }
                    if (child_signal.parent_expansion_id == old_expansion_id)
                    {
                        child_signal.parent_expansion_id = signal.expansion_id;
                    }
                }
                updateContentWidth();
                updateScrollBars();
                update();
            }
        }
        else if (!signal.is_expansion)
        {
            signal.is_expansion = true;
            for (DisplaySignal &child_signal : m_signals)
            {
                if (child_signal.parent_expansion_id == signal.expansion_id && !child_signal.visible)
                {
                    child_signal.visible = true;
                }
            }
            updateContentWidth();
            updateScrollBars();
            update();
        }
    }
}
void WaveformDisplay::initializeShortcuts()
{
    ShortcutsManager *manager = ShortcutsManager::instance();
    manager->registerShortcut(Shortcuts::Waveform::GROUPADD,
                              "Alt+Q",
                              tr("Group Add"),
                              tr("Waveform"));
    manager->registerShortcut(Shortcuts::Waveform::HIERARCHY_NAME,
                              "Ctrl+H",
                              tr("Hierarchy Name"),
                              tr("Waveform"));

    manager->bindToAction(Shortcuts::Waveform::GROUPADD, m_addGroupAction);
    manager->bindToAction(Shortcuts::Waveform::HIERARCHY_NAME, m_hierarchicalNameAction);
}
void WaveformDisplay::addExpandSignals(QString signalName)
{
    if(signalName.isEmpty()) return ;
    VarRef old_var_ref = m_waveform->get_hierarchy().get_varRef_by_hierarchy(signalName.toStdString());
    if(old_var_ref > 0) {
        Var var = m_waveform->get_hierarchy().get_var(old_var_ref);
        std::vector<DimInfo> dimInfoVec =  m_waveform->parse_dim_string(var.multi_array);
        if(!var.multi_array.empty() && var.signal_type.width > 1) {
            QString multiarray;
            for(int j = 0; j< dimInfoVec.size(); j++)
            {
                DimInfo dimInfo = dimInfoVec[j];
                if(!dimInfo.is_range) {
                    multiarray+=dimInfo.to_string();
                }else{
                    int width = getRemainWidth(dimInfoVec,j);
                    QString remainMultiArray = getRemainDimInfoStr(dimInfoVec,j);
                    for(int i = dimInfo.get_logical_min(); i <= dimInfo.get_logical_max(); i++ ){
                        QString old_var_name = m_waveform->get_hierarchy().get_full_scope_path(var.parent_scope) +"."
                                + QString::fromStdString(m_waveform->get_hierarchy().get_string(var.name_id))
                                + "[" + QString::number(i)  + "]"+remainMultiArray;
                        VarRef var_ref = m_waveform->get_hierarchy().get_varRef_by_hierarchy(old_var_name.toStdString());
                        if(var_ref == INVALID_VAR_REF){
                            var_ref = m_waveform->get_hierarchy().add_var_by_var_multiarray(var,i,multiarray,remainMultiArray,width);
                            Var newVar = m_waveform->get_hierarchy().get_var(var_ref);
                            QString var_name = QString::fromStdString(m_waveform->get_hierarchy().get_string(newVar.name_id))+QString::fromStdString(newVar.multi_array);
                            SignalRef signal_ref = var.handle;
                            qDebug()<< "var_name "<< var_name <<" signal_ref " << signal_ref;
                            QSharedPointer<Signal> signal = m_waveform->extract_bit(m_waveform,old_var_ref,i,dimInfo,width);
                            if (!signal) {
                                QMessageBox::critical(this, "Error", "Failed to create logical signal!");
                                return;
                            }

                            m_waveform->add_signal(newVar.handle, signal);

                        }
                    }
                    break;
                }
            }
        }
    }
}
