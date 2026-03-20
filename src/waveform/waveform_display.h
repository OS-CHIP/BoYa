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

#ifndef WAVEFORM_DISPLAY_H
#define WAVEFORM_DISPLAY_H
#include <QWidget>
#include <QVector>
#include <QMap>
#include <QPainter>
#include <QPainterPath>
#include <QSharedPointer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QToolTip>
#include <QMenu>
#include <QScrollBar>
#include <QSplitter>
#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QTimer>
#include <cmath>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QInputDialog>
#include <QClipboard>
#include "signal.h"
#include "waveform.h"
#include "reader.h"
#include "translators.h"
#include "HighlightDialog.h"
class WaveformDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit WaveformDisplay(double begin_time, double end_time, double width, QWidget *parent = nullptr);
    ~WaveformDisplay();

    struct DisplaySignal {
        VarRef var_ref;
        QString name;
        QString signal_name;
        QVector<QString> scopes;
        QSharedPointer<Signal> signal;
        QSharedPointer<Translator> translator;

        QColor color;

        QColor nameColor;
        QColor valueColor;
        QColor lineColor;
        QColor backgroundColor;

        Time valueTime = -1 ;
        QString currentValue = "";
        bool visible;
        int indent_level = 0;
        bool canExpand = false;
        int expansion_id = 0;
        int parent_expansion_id = 0;
        bool is_expansion = false;
    };

    struct SignalStats {
        int totalTransitions = 0;
        int risingEdges = 0;
        int fallingEdges = 0;
        QSet<QString> uniqueValues;
    };

    struct Marker {
        int id;
        QString name;
        double time;
        QColor color;
        Qt::PenStyle lineStyle;
        bool visible;
        bool isDragging;
        double dragStartTime;
        Marker(int id = 0, const QString& name = "", double time = 0,
               const QColor& color = Qt::red,
               Qt::PenStyle lineStyle = Qt::SolidLine,
               bool visible = true,bool isDragging = false, double dragStartTime = 0)
            : id(id), name(name), time(time), color(color),
            lineStyle(lineStyle), visible(visible),isDragging(false), dragStartTime(time) {}
    };
    enum class ContextMenuType {
        GroupHeader,
        TopSignal,
        ScrollableSignal,
        TimeRuler,
        WaveformArea,
        NameValueArea,
        BlankArea,
        General
    };
    enum class FindSearchEdgeType {
        LastUp,
        LastDown,
        NextUp,
        NextDown
    };
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

    int m_currentGroupPosition = -2;
    int m_currentSignalPosition = -2;
    int m_currentSignalPositionInGroup = -2;
    QString m_currentPositionGroupName = "";


    void addSignal(const VarRef var_ref,const QString &name, const QSharedPointer<Signal> &signal, const QString &group = "G1",
                   const int &index = 0,const int parent_expansion_id = 0,const int indent_level = 0);
    void addSignalToPinTopSignals(const VarRef var_ref, const QString &name,  const QSharedPointer<Signal> &signal, const QString &idx, const int &index = 0);

    void removeSignal(const QString &name,const bool &deep = true);

    void clearSignals();

    void drawPinTopSignalNamesAndValues(QPainter &painter);
    void drawPinTopSignalNames(QPainter &painter);
    void drawPinTopSignalValues(QPainter &painter);
    void saveGroup();
    void loadGroup();

    void setTimeRange(double minTime, double maxTime);

    void autoScaleTime();

    void setSignalSpacing(int spacing);

    void setSignalHeight(int height);

    void setGridVisible(bool visible);

    void setTimeRulerVisible(bool visible);

    void setSignalNamesVisible(bool visible);

    QPair<double, double> timeRange() const;

    int signalCount() const;

    void addGroup(const QString &name, const QColor &color = Qt::gray);

    void renameGroup(const QString& oldName, const QString& newName);

    void removeGroup(const QString &name);

    void clearGroups();

    void setGroupCollapsed(const QString &name, bool collapsed);

    bool isGroupCollapsed(const QString &name) const;

    void setGroupColor(const QString &name, const QColor &color);

    QColor groupColor(const QString &name) const;

    QString signalGroup(const QString &signalName) const;

    void moveSignalToGroup(const QString &signalName, const QString &groupName, const int &targetIndex);
    void updateWavefrom(QSharedPointer<Waveform> waveform, QSharedPointer<IWaveformReader> reader);
    QString getSignalValueAtTimeOnly(QSharedPointer<Signal> signal, Time time);
    void setHierarchicalDisplay(bool enabled);
    QString getSignalValueByVarRef(const VarRef var_ref,const QSharedPointer<Signal> &signal);

    void addMarker(double time, const QString& name, const QColor& color, Qt::PenStyle lineStyle);
    void removeMarker(int markerId);
    void clearMarkers();
    QList<int> getMarkerIds() const;
    Marker getMarker(int markerId) const;
    void setMarkerVisible(int markerId, bool visible);
    int getMarkerAtPosition(const QPoint& pos) const;
    void handleSignalSelection(QMouseEvent *event,bool isPinTop=false);
    void setSearchTime(QString selectTime);
    void createMenuActions();
    void resetContextMenuVisibility();
    ContextMenuType determineContextMenuType(const QPoint& adjustedPos, bool isInTopArea);
    void setupBlankAreaMenu(const QPoint& adjustedPos, bool isInTopArea);
    void setupContextMenuForType(ContextMenuType menuType, const QPoint& adjustedPos, bool isInTopArea);
    void setupGroupHeaderMenu(const QPoint& adjustedPos);
    void setupTopSignalMenu(const QPoint& adjustedPos);
    void setupScrollableSignalMenu(const QPoint& adjustedPos);
    void setupSignalSpecificMenuItems();
    void setupTimeRulerMenu(const QPoint& adjustedPos);
    void setupWaveformAreaMenu(const QPoint& adjustedPos);
    void setupNameValueAreaMenu(const QPoint& adjustedPos, bool isInTopArea);
    void setupGeneralMenu(const QPoint& adjustedPos);
    bool shouldShowContextMenu(ContextMenuType menuType) const;
    void addMarkerAtSpecificTime(double time);
    void clearDynamicMenuActions();
    QSet<VarRef> getDisplayVarRefs();
    int getRemainWidth(std::vector<DimInfo> dimInfoVec,int idx);
    void expandMultiArray(DisplaySignal &signal,QString groupName,int signalIndex);
    QString getRemainDimInfoStr(std::vector<DimInfo> dimInfoVec,int idx);
    bool hasSignals() const { return !m_signals.isEmpty(); }
    QStringList getAllSignalsInDisplayOrder() const;
    void addExpandSignals(QString signalName);
private slots:
    void onDeleteKeyPressed();
public slots:
    void zoomIn();
    void zoomOut();
    void zoomAll();
    void panLeft();
    void panRight();
    void setCursor(bool isNext);
    void findSearchEdgeType(FindSearchEdgeType edgeType);
    void showMarkerManagerDialog();
    void clearAllMarkers();
    void onMarkerSelected();
    void applyMarkerChanges();
    void deleteSelectedMarker();
    void updateMarkerList();
    void onDetailActionTriggered();
    void searchSignalValue(const QString& value);
    void searchSignalTransition(const QString& oldValue, const QString& newValue);
    void clearSearchResults();
    void goToNextSearchResult();
    void goToPreviousSearchResult();
    int searchResultCount() const;
    int currentSearchIndex() const;
    void performValueSearch(const QString& value);
    void performTransitionSearch(const QString& oldValue, const QString& newValue);

    void highlightSearchResult(int index);

    void drawSearchHighlights(QPainter& painter);
    QPair<int, int> getSignalYPositionInView(const QString& signalKey) const;
    void ensureSignalVisible(const QString& signalKey);
    int getSignalAbsoluteTop(const QString& signalKey) const;
signals:
    void timeRangeChanged(double minTime, double maxTime);
    void signalClicked(const QString &name, double time, const QString &value);
    void groupAdded(const QString &name);

    void groupRemoved(const QString &name);
    void signalMoved(const QString &signalName, const QString &fromGroup, const QString &toGroup);
    void signalDoubleClicked(const QString &fullPath);
    void addSignalFromEditorToWaveWindow(const QString &singalName, const QString &targetGroup, const int &targetIndex);
    void markerAdded(int markerId);
    void markerRemoved(int markerId);
    void markersCleared();
    void markerUpdated(int markerId);
    void timeValueChanged(Time clickTime);
    void timeChangeForGetSignals();
    void addSignalFromSource(const QString &singalName,const QString &groupName, const int &targetIndex);
    void logicalOperationSignal(const QString &fullPath);
    void searchResultsUpdated(int count);
    void searchResultChanged(int currentIndex, int totalCount);
    void signalsChanged();
    void getStatusByFullPath(const QString varRefStr, const QString fullPath);
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;


    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void zoomOutAtPoint(double mouseX);
    void zoomInAtPoint(double mouseX);
    bool eventFilter(QObject *obj, QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void changeEvent(QEvent *event) override;
private:
    bool m_ctrlPressed = false;
    bool m_shiftPressed = false;
    QString m_lastSelectedSignalKey;

    void updateSelectionMode();

    // QSet<QString> m_selectedSignalKeys;
    QList<QString> m_selectedSignalKeys;
    enum SelectionMode {
        SingleSelection,
        CtrlMultiSelection,
        ShiftRangeSelection
    };
    SelectionMode m_selectionMode = SingleSelection;

    bool m_isSelectingWithMouse;
    QPoint m_selectionStartPos;
    QRect m_selectionRect;

    struct SignalGroup {
        QString name;
        QColor color;
        bool collapsed;
        QVector<QString> v_signals;
    };

    struct SignalCrusorTimeAndPosition {
        Time lastTime;
        Time nextTime;
        int lastTimeDistance;
        int nextTimeDistance;
        int lastTimeXPosition;
        int nextTimeXPosition;
        int clickTime;
        int clickTimeXPosition;
    };
    bool m_firstShow = true;
    QMap<int, Marker> m_markers;
    int m_nextMarkerId = 1;
    QMenu* m_markerMenu = nullptr;
    QAction* m_addMarkerAction = nullptr;
    QAction* m_manageMarkersAction = nullptr;
    QAction* m_clearMarkersAction = nullptr;
    QDialog* m_markerManagerDialog = nullptr;

    QListWidget* m_markerListWidget = nullptr;
    QLineEdit* m_markerNameEdit = nullptr;
    QDoubleSpinBox* m_markerTimeEdit = nullptr;
    QPushButton* m_applyMarkerBtn = nullptr;
    QPushButton* m_deleteMarkerBtn = nullptr;

    QMenu* m_goToMarkerMenu = nullptr;
    QComboBox* m_lineStyleCombo = nullptr;
    QPushButton* m_colorButton = nullptr;
    QColor m_currentMarkerColor = Qt::red;
    int m_draggingMarkerId = -1;
    Time m_markerDragStartTime;
    Time m_markerBeforeDragTime;
    int m_hoveredMarkerId = -1;
    QMap<QString, SignalHighlightConfig> m_signalHighlights;

    enum ZoomMode {
        ZoomNormal,
        ZoomFull
    };
    void createMarkerManagerDialog();
    void drawMarkersLine(QPainter &painter);
    void drawMarkersRect(QPainter &painter);
    void handleSearch();
    void clearCurrentPosition();
    void performDrag(const QString &dragType);
    void sortSelectedSignalsByDisplayOrder(const QStringList& displayOrder);

    void calculateLayout();

    void handleThemeChange(const QString &themeName);

    void drawTimeRuler(QPainter &painter);

    int drawSignalsArr(QPainter &painter, int currentY, QVector<QString> signalsArr,bool isPinTop = false);
    void drawScrollableSignalNamesAndGroups(QPainter &painter);
    void drawFixedPinTopArea(QPainter &painter);

    int drawSignalNamesArr(QPainter &painter, QVector<QString> signalsArr,int currentY,bool isPinTop = false);

    double timeToX(double time) const;

    Time xToTime(double x) const;

    double valueToY(const int currentY, const QString &signalName, const QString &value, bool isMulBit = false) const;

    int getGroupPosition(const QString &groupName) const;

    int getSignalPositionInGroup(const QString &signalName, const QString &groupName) const;

    void updateMouseTracking(const QPoint &pos);

    int getGroupSignalVisibleSize(QString groupName) const;
    int getViewSignalSpacingIndexAndGroupAtY(int y, int &groupIndex,int &m_currentSignalPositionInGroup, QString &m_currentPositionGroupName) const;
    int getViewSignalGroupIndexAtY(int y) const;
    int getViewSignalIndexInGroupAtY(int y);
    int getRealSignalIndexInGroupAtY(int y) const;

    QString getSignalGroupNameAtY(int y) const;
    QString getSignalKeyInScrollerArea(int y) const;
    QString getSignalKeyInTopArea(int y) const;

    int getGroupIndexAtY(int y) const;

    void updateGroupOrder(int fromIndex, int toIndex);
    QString getGroupNameBySignalKey(const QString &signalKey);

    void createContextMenu();

    void onHighlightButtonClicked();

    void onPinTopButtonClicked(DisplaySignal &signal);

    void updateSignalColors(const QMap<QString, SignalHighlightConfig>& highlights);

    void applySignalHighlights(const QMap<QString, SignalHighlightConfig>& highlights);
    QStringList getAllSignalsInOrder() const;



    int getSignalCurrentY(int y) const;

    SignalCrusorTimeAndPosition getDistanceFromLastTimeAndNextTime() const;

    bool currentPositionIsTime() const;

    void setSelectTimeWithDistance();
    QString findSignalAtClickPosition();
    int getGroupYPosition(const QString &groupName) const;
    void drawHorizontalHexagon(QPainter& painter, const QColor& highColor,const QColor& lowColor,
                               double startX, double endX, double centerY,
                               double height,const QString& valueText,double lineWidth);

    static QString join_path_with_dot(const QVector<QString>& path);

    static QString join_path_with_slash(const QVector<QString>& path);

    void updateScrollBars();

    void updateAreaWidth();

    void handleVerticalScroll(int value);

    void handleHorizontalScroll(int value);

    void ensureCursorVisible();

    void ensureCursorAtMiddle();

    void handleSplitterMoved(int pos, int index);
    int getTopPinSignalsAreaBottomY() const;
    int getTopPinSignalsAreaHeight() const;
    void drawTimeRulerArea(QPainter &painter);
    void drawScrollableGroupArea(QPainter &painter);
    void drawScrollableSignalValues(QPainter &painter);
    void drawScrollableSignalsWaveform(QPainter &painter);
    void drawInteractiveElements(QPainter &painter);

    void syncVerticalScroll(int value);
    void drawArrowHead(QPainter &painter, const QPoint &point, bool directionUp, int size);
    void syncHorizontalScroll(int value);
    void updateContentWidth();
    void recalculateMaxWidths();
    void changeSignalValue();
    void setSelectTime(Time time);
    void updateRadixMenu();
    void goToMarker(int markerId);
    void updateGoToMarkerMenu();
    void updateColorButton(QPushButton* button, const QColor& color);
    int calculateSignalTransitions(const DisplaySignal& displaySignal) const;
    void showSignalDetailDialog();
    SignalStats calculateSignalStatistics(const DisplaySignal& displaySignal, double startTime, double endTime) const;
    void initializeShortcuts();

    QMap<QString, DisplaySignal> m_signals;
    QMap<QString, SignalGroup> m_groups;
    QStringList m_groupOrder;

    QVector<QString> m_pinTopSignals;
    QLineEdit *m_searchLineEdit;
    double m_minTime;
    double m_maxTime;
    double m_timeScale;
    double m_timeOffset;
    double m_minZoomRange;
    ZoomMode m_zoomMode;
    int m_signalSpacing;
    int m_signalHeight;
    int m_timeRulerHeight;
    int m_signalNameWidth;
    int m_valueWidth;
    int m_groupHeaderHeight;
    int m_signalNameLongestWidth;
    int m_signalValueLongestWidth;
    bool m_gridVisible;
    bool m_timeRulerVisible;
    bool m_signalNamesVisible;

    QString m_originalTopSignalKey;
    QPoint m_dragStartPosWithOffset;
    QPoint m_dragStartPosWithoutOffset;
    bool m_isPotentialDragSignalLine = false;
    bool m_horizontalDragging = false;
    bool m_verticalDragging = false;
    Time m_dragStartTime;
    Time m_dragEndTime;
    int m_verticalDragDistance;
    QString m_verticalHintText;
    bool m_isPotentialDragSignalInTopArea = false;
    bool m_isPotentialDragSignalName = false;
    bool m_isPressSignalSpacing = false;
    bool m_isPotentialDragSignalValue = false;
    bool m_isPotentialDragGroupName = false;
    bool m_isPotentialDragGroupValue = false;
    bool m_draggingSignalName = false;
    bool m_isDraggingSignalInTopArea = false;
    bool m_draggingSignalValue = false;
    bool m_draggingGroupName = false;
    bool m_draggingGroupValue = false;
    int m_horizontalDraggingLineStartPositionY;
    QPoint m_lastMousePos;
    QPoint m_currentMousePos;
    QString m_draggedSignalName;
    QString m_draggedGroupName;
    int m_dragStartY;
    int m_dragCurrentY;
    int m_dragCurrentX;
    int m_dragCurrentYWithoutOffset;
    int m_dragCurrentXWithoutOffset;
    QVector<QString> m_dragDataTitleInfo;
    int m_originalSignalIndex;
    int m_originalGroupIndex;

    QString m_hoverSignal;
    double m_hoverTime;
    Time m_selectTime;
    QString m_hoverValue;

    QMenu *m_contextMenu;
    QAction *m_addGroupAction;
    QAction *m_renameGroupAction;
    QAction *m_removeGroupAction;
    QAction *m_collapseGroupAction;
    QAction *m_expandGroupAction;
    QString m_contextMenuGroup;
    QAction *m_removeSignalAction;
    QAction *m_unpinnedSignalAction;
    QAction *m_copyFullPathAction;
    QAction *m_copyValueAction;
    QAction *m_highlightAction;
    QAction *m_pinTopAction;
    QMenu *m_radixMenu;
    QAction *m_logicalOperationAction;
    QAction *m_hierarchicalNameAction = nullptr;
    QShortcut *m_hierarchicalNameActionShortcut = nullptr;
    QAction* m_saveGroupAction = nullptr;
    QAction* m_loadGroupAction = nullptr;
    QAction* m_zoomInAction = nullptr;
    QAction* m_zoomOutAction = nullptr;
    QAction* m_zoomAllAction = nullptr;
    QAction* m_toggleGridAction = nullptr;
    QAction* m_toggleRulerAction = nullptr;
    QAction* m_detailAction = nullptr;
    HighlightDialog *m_highlightDialog = nullptr;
    QPoint m_highlightDialogOldPos;
    QShortcut *m_deleteShortcut;

    QScrollBar *m_verticalScrollBar;
    QScrollBar *m_horizontalScrollBar;

    QSplitter *m_horizontalSplitter;

    QScrollArea *m_nameArea;
    QScrollArea *m_valueArea;
    QScrollArea *m_waveArea;

    int m_verticalOffset;
    int m_horizontalOffset;

    int m_contentHeight;

    int m_nameAreaWidth;
    int m_valueAreaWidth;
    int m_waveAreaWidth;

    bool m_resizingAreas;
    int m_resizeStartX;
    int m_nameAreaStartWidth;
    int m_valueAreaStartWidth;

    bool m_syncingScroll;
    double m_globalMinTime;
    double m_globalMaxTime;
    double m_pixelsPerTimeUnit;
    QSharedPointer<IWaveformReader> m_reader;
    QSharedPointer<Waveform> m_waveform;
    bool m_showClickIndicator = false;
    Time m_clickTime = 0.0;
    Time m_middlePressClickTime = 0;
    QPoint m_clickPosition;
    double m_clickValue = 0.0;
    int signal_idx = 0;
    int expansion_id = 1;
    bool m_hierarchicalDisplay = false;
    TranslatorManager *m_translatorManager;
    static constexpr int DRAG_THRESHOLD = 5;

    QList<SearchResult> m_searchResults;
    int m_currentSearchIndex;
    SearchType m_currentSearchType;
    QString m_currentSearchValue;
    QString m_currentSearchOldValue;
    QString m_currentSearchNewValue;

    QPen m_pen = QPen(Qt::black);

    QPen m_mainRulerLinePen = QPen(Qt::black,1);
    QPen m_mainRulerTextPen = QPen(Qt::black,2);
    QPen m_signalPen = QPen(Qt::black);
    QPen m_horizontalDraggingBorderPen = QPen(Qt::blue);
    QPen m_splitPen = QPen(Qt::black,2);
    QPen m_dragPreviewLinePen = QPen(Qt::blue, 2, Qt::DashLine);
    QPen m_verticalLinePen = QPen(QColor("#003e00"), 1, Qt::DashLine);
    QPen m_verticalLineMiddlePressPen = QPen(QColor("blue"), 1, Qt::DashLine);
    QColor m_rulerRectColor = QColor(220, 220, 220);
    QColor m_arrowColor = QColor(255, 100, 0);
    QColor m_groupRectColor = QColor("#c8e6ff");

    QColor m_highLightColor = QColor("#ffffb5");
    QColor m_highLightHighColor = QColor("#00ff00");
    QColor m_highLightHighToLowColor = QColor("#00ff00");
    QColor m_highLightLowColor = QColor("#00ff00");
    QColor m_highHexagonalHighColor = QColor("#00ff00");
    QColor m_highHexagonalLowColor = QColor("#00ff00");
    QColor m_selectionColor = QColor(100, 100, 255, 50);
    QColor m_highLevelColor = QColor("#00ff00");
    QColor m_highToLowColor = QColor("#00ff00");
    QColor m_lowLevelColor = QColor("#00ff00");
    QColor m_segmentXColor = QColor("#df1f25");
    QColor m_segmentZColor = QColor("​#FFA500");

    QColor m_hexagonalHighColor = QColor("#00ff00");
    QColor m_hexagonalLowColor = QColor("#00ff00");
    QColor m_waveformSignalTextColor = QColor("#333");
    bool m_addingSignalFromEditor = false;
public:
    QString getSignalValueAtTime(const DisplaySignal& displaySignal, Time time);
};
#endif
