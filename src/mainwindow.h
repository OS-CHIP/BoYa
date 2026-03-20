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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QTreeWidget>
#include <QTextEdit>
#include <QTableWidget>
#include <QScrollArea>
#include <QVector>
#include "reader.h"
#include "texteditor.h"
#include "fstapi.h"
#include "wavewindow.h"
#include "instance/instance.h"
#include "oneSearch.h"
#include "theme/ThemeManager.h"
#include "simpleTextEditor.h"
#include "hierarchyprefixdialog.h"
#include "signallist.h"

namespace Ui {
class MainWindow;
}
class WaveformWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WaveformWidget(QWidget *parent = nullptr);
    void setData(const QVector<int> &data);
private:
    QVector<int> waveformData;
};
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Instance *instance;
    void setApplicationFont();
signals:
    void fileListFileReady(const QStringList allFiles); 
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private slots:
    void about();
    void waitToUpdate();
    void onSignalFileOpened(QSharedPointer<IWaveformReader> reader, QSharedPointer<Waveform> waveform);
    void handleScopeDoubleClick(QString scope_path);
    void goToCalling();
    void goToDefinition();
    void goBack();
    void goForward();
    void updateNavigationState(bool backFlag, bool forwardFlag);
    void updateDriverLoadState(bool flag);
    void setEditorTitleAndScope(QString title,QString Scope);
    void addToWaveform(QString SignalName);
    void goToDriver();
    void goToLoad();
    void onSearchButtonClicked();
    void onHierarchyChangeClicked();
    void setFilelistPath(const QStringList allFiles);
    void openFileAtLine(const QString &filePath, int lineNumber);
    void openFileAtPosition(const QString &filePath, int lineNumber, int columnNumber);
    void toggleTheme(const QString &theme);
    void saveGroup();
    void loadGroup();
    void onThemeChanged(const QString &theme);
    void loadThemeForWaveform(const QString &themeName);
    void refreshDefaultLayout();
    void applyDefaultLayoutSizes();
    void toggleDriverLoadArea();
    void updateDriverLoadInfo(QList<QMap<QString, QVariant>> result);
    void onDriverLoadLinkClicked(const QUrl &url);
    void onDialogPrefixChanged(const QString& original, const QString& newPrefix);
    void onDialogPrefixReset();
    bool onRequestDisplayCheck(QString& full_name, const QString& name, bool& shouldDisplay);
    void populateTree();
    void onFontButtonClicked();
    void mergeformat(QTextCharFormat fmt);
    void setGlobalPathTotextEditor();
    void setWaveformTitle(QString name);
    void showSimpleGoToLineDialog();
    void on_actionShortcuts_triggered();
    
    void onWaveTabChanged(int index);
    void onWaveTabCloseRequested(int index);
    void onWaveWindowTitleChanged(const QString& title);
    
    void createNewWaveTab(const QString& title);
    void updateWaveWindowConnections();
    void disconnectCurrentWaveWindowConnections();
    void rebuildWaveWindowMapping();
    void onCheckWaveformRequested(bool& hasWaveform);
    void reapplyApplicationFont();
    void toggleSignalListDock();
    void onTimeChangeForGetSignals();
    void onSignalListSignalClicked(QStringList signalList);
    void onSignalListSignalDoubleClicked(QStringList signalList);
    void onAddSignalsToWaveformFromSignalList(const QVector<QStringList> &signalList);
    void addAndShowExpandSignals(const QString& signalName);

public slots:
    void onWaveformDisplaySignalDoubleClicked(const QString &fullPath); 
    void fetchTipData(QString signalName);
    void getStatusByFullPath(const QString varRefStr, const QString &fullPath);
private:
    Ui::MainWindow *ui;
    bool textEditorLoaded = false;
    QLineEdit *searchLineEdit; 
    TextEditor *m_textEditor = nullptr; 
    Instance *m_instance = nullptr;

    QDockWidget *m_textEditorDock;
    SignalListDock *m_signalListDock = nullptr;;
    QDockWidget *m_instanceDock; 
    QString doubleClickedSignalFullPath;
    QList<QDockWidget*> m_docks; 
    
    QScrollArea *waveformScrollArea;
    void removeAllDock();
    void addDockToMDocks();
    void showDock(const QList<int>& index = QList<int>());
    void setupConnections();
    void showWindowDock(const QString name);
    QSharedPointer<IWaveformReader> m_reader;
    QSharedPointer<Waveform> m_waveform;
    QStringList fileListPath;
    void setupThemeConnections(); 
    void updateUIForTheme(const QString &themeName);
    void setFontForAllChildren(QWidget *parent, const QFont &font);
    void initDriverLoadArea();
    QDockWidget *m_driverLoadDock;
    QWidget *m_driverLoadWidget;
    QTextBrowser  *m_driverLoadTextEdit; 
    bool m_driverLoadVisible = false;
    QByteArray m_previousLayout;
    QDockWidget *m_secondEditorDock = nullptr;    
    SimpleTextEditor *m_secondEditor = nullptr;   
    HierarchyPrefixDialog* m_dialog = nullptr;
    void initializeShortcuts();
    void setupSignalListDock();
    void getSiganlLists();
    
    QTabWidget *m_waveTabWidget = nullptr;                     
    QMap<WaveWindow*, int> m_waveWindowToTabIndex;            
    QMap<WaveWindow*, int> m_waveWindowToTabNumber;
    WaveWindow *m_currentActiveWaveWindow = nullptr;         
    WaveWindow *m_waveWindow = nullptr;                      
    QDockWidget *m_waveformDock = nullptr;                   
    QFont m_applicationFont; 
    Instance* m_currentInstance = nullptr;  
    QMap<WaveWindow*, Instance*> m_waveToInstanceMap;  
    QMap<int, WaveWindow*> m_tabNumberToWaveWindow;  
    int m_nextTabNumber = 1;
    QString m_signallist_scope;
    QString m_signallist_fileName;
};
#endif 
