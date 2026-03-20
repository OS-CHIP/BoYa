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

#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H
#include <QMainWindow>
#include <QTextCharFormat>
#include "edit.h"
#include "highlighter.h"
#include <QLabel>
#include "editorDatabase.h"
#include <QJsonDocument>
#include <QTextCursor>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDataStream>
#include <QPlainTextEdit>
#include <QPainter>
#include <QDrag>
#include <QFuture>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QAction>
namespace Ui {
class TextEditor;
}
class TextEditor : public QMainWindow
{
    Q_OBJECT
public:
    explicit TextEditor(QWidget *parent = nullptr);
    ~TextEditor();
    void newFile();   
    bool maybeSave(); 
    bool save();      
    bool saveAs();    
    bool saveFile(const QString &fileName); 
    bool loadFile(const QString &fileName); 
    int replaceIndex;
    int replaceLength;
    int lastIndex;
    int lastLength;
    void mergeformat(const QTextCharFormat &fmt);
    void textBold();
    void textItalic();
    void textUnderline();
    void textColor();
    void textCurrentFormatChanged(const QTextCharFormat & fmt);
    void textFont();
    void refreshStack();
    void resetStack();
    void textCopy();
    void textCut();
    void textPaste();
    QString pasteBoard;
    
    int importSlangAst();
    void goToPosition(int line, int column, bool recordHistory = true);
    void goToLine(int line);
    void getModuleAndLoadFile(QString scope_path);
    void goToCalling();
    void goToDefinition();
    void goToCalling(const QPoint& clickPos);
    void goToDefinition(const QPoint& clickPos);
    void goBack();
    void goForward();
    void updateNavigationState();
    QString readFileLine(const QString& filePath, int lineNumber);
    void buildResultsByDrivers(QList<QSqlRecord> records);
    void buildResultsByLoads(QList<QSqlRecord> records);
    void goToDriver();
    void goToDriverBySignalName(QString signalName);
    void goToLoadBySignalName(QString signalName);
    void goToLoad();
    void loadMarkersForFile(QString scopePath, QString fileName);
    void buildMarkerIndex();
    void receiveTooltipContent(const QString& content);
    void findSearchText(const QString &searchText,bool isNext);
    void openFileAtLine(const QString &filePath, int lineNumber);
    void parseAndOpenFile(const QString &lineText);
    void setSelectMarker(QString &varName,int source_line,int source_column);
    
    void processFilesInBackground(const QStringList& fileNames);
    QString replacePrefixOptimized(const QString& str,bool flag = true);
    void setHierarchyPrefix(const QString& original, const QString& newHiePrefix);
    bool checkDisplay(QString& full_name, const QString& name);
    void findBestPathMatch(const QString& fullPath,QString &scope, QString &varName);
    void showSimpleGoToLineDialog();
    void handleThemeChange(const QString &themeName);
    void setGlobalPath(QSet<QString> globalPaths);
    QString extract_and_clean_array_dims(const std::string& type_str);
    QMap<QString, QString> getStatusByFullPath(const QString fullPath);
    unsigned long parseNumberString(const QString& str, bool* ok = nullptr) ;
    QVector<QStringList> getSignalsByInstanceId();
    QString getCurrentScope();
    QString getCurrentFileName();
signals:
    void setNavigationState(bool backFlag, bool forwardFlag);
    void setTitleAndScope(QString title,QString scope);
    void addToWaveform(QString signalName);
    void fetchTipDataByName(QString name);
    void setDriverLoadAvailable(bool flag);
    void setFilelistPath(const QStringList allFiles);
    void openFileAtPosition(QString fileName, int lineNumber, int columnNumber);
    void resultsBuilt(QList<QMap<QString, QVariant>> results);
    void populateTreeSignal();
    void mergeformatSignal(QTextCharFormat fmt);
    void checkWaveformRequested(bool& hasWaveform);
public slots:
    void openFile();
    void setFileListFile(const QStringList allFiles); 
    void highlightCurrentLine();
    void onImportAstFinished();
    void on_action_Copy_triggered();
    void on_action_Font_triggered();
private slots:
    void showContextMenu(const QPoint &pos);
    void copyFullPath();
    void copyFileName();
    void addToWaveformAction();
    void onFileProcessingStarted();
    void onFileProcessingFinished();
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    Edit edit;
    Ui::TextEditor *ui;
    EditorDatabase *db = new EditorDatabase();
    QString m_currentLoadFile;
    
    bool isUntitled;
    
    QString curFile;
    
    bool isLoadFile=0;
    bool undoIsUsed=0;
    Highlighter *m_highlighter = nullptr; 
    QMap<QString, QString> m_languageFiles;    
    QPoint m_currentTooltipPos;
    void loadLanguageDefinitions(const QString &theme = "light");
    QString getLanguageFileForExtension(const QString &extension);
    void updateVisibleBlocks();
    
    void setupHoverTooltip();
    void handleHoverTimeout();
    void showTooltip();
    void hideTooltip();
    void positionTooltip();
    void updateTooltipForPosition(const QPoint& pos);
    void handleMarkerClick(const QPoint& pos);
    bool checkHasModuleMarker(const QPoint& pos);
    bool checkHasInstanceMarker(const QPoint& pos);
    void performDrag(const QString &dragType);
    bool isMarkerSelected(TextMarker marker);
    void removeMarkerSelected(TextMarker marker);
    QPoint m_dragStartPosition;
    bool m_isPotentialDragMarker = false;
    bool m_isDraggingMarker = false;
    static constexpr int DRAG_THRESHOLD = 5; 
    QPoint m_dragStartPosWithoutOffset;
    QMouseEvent *m_editorClickEvent;
    QString m_tooltipData;
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    QLabel* m_tooltipLabel;
    
    QTimer* m_hoverTimer;
    QTimer* m_tooltipTimer;
    struct NavigationPoint {
        QString filePath;
        QString title;
        QString scope;
        int instance_id;
        int line;
        int column;
        qint64 timestamp; 
    };
    QList<NavigationPoint> navigationHistory;
    int currentHistoryIndex = -1;
    bool isNavigating = false;
    QString current_title; 
    QString current_scope; 
    int current_instance_id=0;
    QList<TextMarker> m_markers;
    QList<TextMarker> module_markers;
    QList<TextMarker> instance_markers;
    QHash<QString, QString> line_col_type_map;
    QMap<int, QVector<TextMarker>> m_lineMarkers;
    QMap<int, QVector<TextMarker>> module_lineMarkers;
    QMap<int, QVector<TextMarker>> instance_lineMarkers;
    TextMarker m_currentMarker;
    QPoint m_hoverPosition;
    TextMarker m_selectedMarker;
    QList<TextMarker> m_selectedMarkers;
    
    bool m_isMultiSelectMode = false;
    bool is_SelectBefore = false;
    QList<QTextEdit::ExtraSelection> m_extraSelections; 
    QColor m_highLightLineColor = QColor("#ffffcc"); 
    QFutureWatcher<bool> m_importAstWatcher;
    QFutureWatcher<QVector<TextMarker>> m_loadMarkersWatcher;
    
    bool importSlangAstInThread();
    void paintDragIcon(QPainter& painter, int x, int y);
    QList<QMap<QString, QVariant>> results;
    QShortcut *m_addToWaveformShortcut;

    QString oldPrefix="";
    QString newPrefix="";
    QString m_lastDirectory;
    QFutureWatcher<void> *m_fileProcessingWatcher = nullptr;
    QProgressDialog *m_progressDialog = nullptr;
    void startFileProcessing(const QStringList &allFiles);
    bool m_isProcessing = false;
    bool m_shouldKeepSelection = false;
    QAction *actionAddToWaveformAction = nullptr;
    QSet<QString> m_myPaths;
};
#endif 
