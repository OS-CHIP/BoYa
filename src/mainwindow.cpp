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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QDebug>
#include <QDockWidget>
#include <QDialog>
#include <QListWidget>
#include <QHeaderView>
#include <QPainter>
#include <QBuffer>
#include "instance/instance.h"
#include <QLabel>
#include <QPushButton>
#include <QApplication>
#include <QMouseEvent>
#include <QStyle>
#include <QUrlQuery>
#include <QFileInfo>
#include <QShortcut>
#include <QFontDialog>
#include <QToolButton>
#include "editor/simpleTextEditor.h"
#include "shortcutsmanager.h"
#include "shortcutdefinitions.h"
#include "shortcutsettingsdialog.h"
#include "pathutils.h"
#include "globalState.h"


WaveformWidget::WaveformWidget(QWidget *parent) : QWidget(parent) {}
void WaveformWidget::setData(const QVector<int> &data)
{
    waveformData = data;
    update();
}
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    setApplicationFont();
    ui->setupUi(this);
    this->setContextMenuPolicy(Qt::NoContextMenu);
    
    setupThemeConnections();
    this->setObjectName("mainWindow"); 
    ThemeManager::instance().loadTheme("gray");
    ui->mainToolBar->setFixedHeight(30);
    QWidget *p = takeCentralWidget();
    if (p)
        delete p;
    setDockNestingEnabled(true);

    ui->dockWidget_2->setProperty("no-style", true);
    ui->dockWidget->setProperty("Instance", true);
    ui->dockWidget_2->setProperty("Waveform", true);
    ui->dockWidget_3->setProperty("Editor", true);
    ui->dockWidget->widget()->setProperty("Instance", true);
    ui->dockWidget_3->widget()->setProperty("Editor", true);

    ui->dockWidget_2->setContentsMargins(0,0,0,0);

    splitDockWidget(ui->dockWidget, ui->dockWidget_2, Qt::Vertical);
    splitDockWidget(ui->dockWidget, ui->dockWidget_3, Qt::Horizontal);
    
    int mainWindowHeight = this->height();
    int mainWindowWidth = this->width();
    
    QList<QDockWidget*> verticalDocks;
    verticalDocks << ui->dockWidget << ui->dockWidget_2;
    QList<int> verticalSizes;
    verticalSizes << ( mainWindowHeight / 5 ) * 2 << (mainWindowHeight / 5) * 3;
    qDebug() << "verticalSizes:" << verticalSizes;
    resizeDocks(verticalDocks, verticalSizes, Qt::Vertical);
    
    QList<QDockWidget*> horizontalDocks;
    horizontalDocks << ui->dockWidget << ui->dockWidget_3;
    QList<int> horizontalDocksSizes;
    horizontalDocksSizes << ( mainWindowWidth / 6 ) * 1 << (mainWindowWidth / 6) * 5;
    qDebug() << "horizontalDocksSizes:" << horizontalDocksSizes;
    resizeDocks(horizontalDocks, horizontalDocksSizes, Qt::Horizontal);
    removeAllDock();
    addDockToMDocks();

    searchLineEdit = new QLineEdit();
    searchLineEdit->setPlaceholderText("search..");
    searchLineEdit->setClearButtonEnabled(true);
    searchLineEdit->setFixedHeight(25);
    
    searchLineEdit->setStyleSheet(R"(
    QLineEdit {
        border: 2px solid #0078d4;
        border-radius: 6px;
        padding: 2px 22px 2px 10px; 
        margin: 0px;
        min-width: 200px;
    }
    QLineEdit:focus {
        border: 2px solid #0078D7;
    }
    QLineEdit QToolButton {
        border: none;
        width: 16px;
        height: 16px;
        margin: 0px;
    }
    QLineEdit QToolButton[buttonType="searchButton"] {
        border: none;
        width: 18px;
        height: 18px;
        margin: 1px;
    }
)");

    QAction *searchAction = new QAction(searchLineEdit);
    searchAction->setIcon(QIcon(":/icons/icons/search.png"));
    searchLineEdit->addAction(searchAction,QLineEdit::TrailingPosition);

    
    QAction *nextAction = searchLineEdit->addAction(QIcon(":/icons/icons/next.png"),
                                                    QLineEdit::TrailingPosition);
    nextAction->setToolTip("Find Next");
    nextAction->setVisible(false); 
    QAction *prevAction = searchLineEdit->addAction(QIcon(":/icons/icons/pre.png"),
                                                    QLineEdit::TrailingPosition);
    prevAction->setToolTip("Find Previous");
    prevAction->setVisible(false); 
    
    nextAction->setProperty("buttonType", "searchButton");
    prevAction->setProperty("buttonType", "searchButton");
    
    connect(searchLineEdit, &QLineEdit::textChanged, this, [=](const QString &text) {
        bool hasText = !text.isEmpty();
        nextAction->setVisible(hasText);
        prevAction->setVisible(hasText);
    });
    connect(searchAction, &QAction::triggered, this, [=]() {
        bool hasText = !searchLineEdit->text().isEmpty();
        nextAction->setVisible(hasText);
        prevAction->setVisible(hasText);
    });
    
    QWidget *cornerContainer = new QWidget();
    cornerContainer->setFixedHeight(28);
    QHBoxLayout *cornerLayout = new QHBoxLayout(cornerContainer);
    cornerLayout->addWidget(searchLineEdit);
    searchLineEdit->setAcceptDrops(true);
    searchLineEdit->installEventFilter(this);
    
    ui->menubar->setCornerWidget(searchLineEdit, Qt::TopRightCorner);
    m_textEditor = new TextEditor();
    m_textEditorDock = ui->dockWidget_3;
    m_textEditorDock->setWidget(m_textEditor);
    connect(prevAction, &QAction::triggered, this, [this]() {
        QString searchText = searchLineEdit->text();
        if (!searchText.isEmpty()) {
            m_textEditor->findSearchText(searchText,false);
        }
    });
    connect(nextAction, &QAction::triggered, this, [this]() {
        QString searchText = searchLineEdit->text();
        if (!searchText.isEmpty()) {
            m_textEditor->findSearchText(searchText,true);
        }
    });
    connect(searchLineEdit, &QLineEdit::returnPressed, this, [this]() {
        QString searchText = searchLineEdit->text();
        if (!searchText.isEmpty()) {
            m_textEditor->findSearchText(searchText,true);
        }
    });

    QWidget *containerWidget = new QWidget(ui->dockWidget_2);
    containerWidget->setProperty("Waveform",true);
    
    m_waveTabWidget = new QTabWidget(containerWidget);
    m_waveTabWidget->setTabsClosable(true);
    m_waveTabWidget->setMovable(true);
    m_waveTabWidget->setTabPosition(QTabWidget::South);
    
    QVBoxLayout *layout = new QVBoxLayout(containerWidget);
    layout->addWidget(m_waveTabWidget);
    
    layout->setContentsMargins(2, 2, 2, 2);
    
    ui->dockWidget_2->setWidget(containerWidget);
    
    connect(m_waveTabWidget, &QTabWidget::currentChanged,
            this, &MainWindow::onWaveTabChanged);
    connect(m_waveTabWidget, &QTabWidget::tabCloseRequested,
            this, &MainWindow::onWaveTabCloseRequested);
    
    createNewWaveTab("<Wave:1>");

    connect(this, &MainWindow::fileListFileReady, m_textEditor, &TextEditor::setFileListFile);
    connect(ui->actionImport_Design, &QAction::triggered, m_textEditor, &TextEditor::openFile);
    connect(ui->actionlight, &QAction::triggered, this, [this]()
            {
                this->toggleTheme("light");
            });
    connect(ui->actiondark, &QAction::triggered, this, [this]()
            {
                this->toggleTheme("dark");
            });
    connect(ui->actiongray, &QAction::triggered, this, [this]()
            {
                this->toggleTheme("gray");
            });
    connect(ui->actionInstance, &QAction::triggered, this, [this]()
            {
                this->showWindowDock("Instance");
            });
    connect(ui->actionEditor, &QAction::triggered, this, [this]()
            {
                this->showWindowDock("Editor");
            });
    connect(ui->actionWaveform, &QAction::triggered, this, [this]()
            {
                this->showWindowDock("Waveform");
            });
    connect(ui->actionSave_Group, &QAction::triggered, this, [this]()
            {
                this->saveGroup();
            });
    connect(ui->actionLoad_Group, &QAction::triggered, this, [this]()
            {
                this->loadGroup();
            });
    connect(ui->actiondefault_layout, &QAction::triggered, this, [this]()
            {
                this->refreshDefaultLayout();
            });
    instance = new Instance();
    m_instance = instance;
    m_instanceDock = ui->dockWidget;
    m_instanceDock->setWidget(instance);
    connect(instance, &Instance::signalFileOpened, this, &MainWindow::onSignalFileOpened);
    connect(ui->actionOpen, &QAction::triggered, this, [this]() {
        instance->openFile(); 
    });
    connect(ui->actionOpenFile, &QAction::triggered, this, [this]() {
        instance->openFile(); 
    });
    connect(instance, &Instance::scopeDoubleClicked, this, &MainWindow::handleScopeDoubleClick);
    connect(instance, &Instance::requestDisplayCheck, this, &MainWindow::onRequestDisplayCheck);
    connect(instance, &Instance::setWaveformTitleSignal, this, &MainWindow::setWaveformTitle);
    connect(instance, &Instance::setGlobalPathTotextEditor, this, &MainWindow::setGlobalPathTotextEditor);
    setupConnections();
    ui->actionBackwardHistory->setEnabled(false);
    ui->actionForwardHistory->setEnabled(false);
    ui->actionDriverIcon->setEnabled(false);
    ui->actionLoadIcon->setEnabled(false);
    setupSignalListDock();
    setAcceptDrops(true);
    reapplyApplicationFont();
    initDriverLoadArea();
    initializeShortcuts();

}
void MainWindow::toggleTheme(const QString &theme)  
{
    ThemeManager::instance().loadTheme(theme);
}
void MainWindow::saveGroup()  
{
    qDebug() << "saveGroup";
    m_waveWindow->saveGroup();
}
void MainWindow::loadGroup()  
{
    m_waveWindow->loadGroup();
}
void MainWindow::loadThemeForWaveform(const QString &themeName)
{
    QString waveformTheme = themeName;
    if(themeName == "gray")
    {
        waveformTheme = "dark";
    }
    QFile file(QString(":/themes/%1.qss").arg(waveformTheme));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        ui->dockWidget_2->setStyleSheet(styleSheet);
    }
}
void MainWindow::refreshDefaultLayout()
{
    
    if (m_secondEditorDock && m_secondEditorDock->isVisible()) {
        m_secondEditorDock->hide();
    }
    if (m_driverLoadDock && m_driverLoadDock->isVisible()) {
        m_driverLoadDock->hide();
    }
    
    if (ui->dockWidget->isFloating()) {
        ui->dockWidget->setFloating(false);
    }
    if (ui->dockWidget_2->isFloating()) {
        ui->dockWidget_2->setFloating(false);
    }
    if (ui->dockWidget_3->isFloating()) {
        ui->dockWidget_3->setFloating(false);
    }
    
    removeDockWidget(ui->dockWidget);
    removeDockWidget(ui->dockWidget_2);
    removeDockWidget(ui->dockWidget_3);
    
    addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget);
    splitDockWidget(ui->dockWidget, ui->dockWidget_2, Qt::Vertical);
    splitDockWidget(ui->dockWidget, ui->dockWidget_3, Qt::Horizontal);
    
    ui->dockWidget->show();
    ui->dockWidget_2->show();
    ui->dockWidget_3->show();
    
    QTimer::singleShot(100, this, [this]() {
        applyDefaultLayoutSizes();
    });
}
void MainWindow::applyDefaultLayoutSizes()
{
    
    int mainWindowHeight = this->height();
    int mainWindowWidth = this->width();
    qDebug() << "Applying layout sizes - Height:" << mainWindowHeight << "Width:" << mainWindowWidth;
    
    QList<QDockWidget*> verticalDocks;
    verticalDocks << ui->dockWidget << ui->dockWidget_2;
    
    int dock1Height = qMax(200, (mainWindowHeight * 2) / 5);  
    int dock2Height = qMax(300, mainWindowHeight - dock1Height - 50); 
    QList<int> verticalSizes;
    verticalSizes << dock1Height << dock2Height;
    qDebug() << "Vertical sizes:" << verticalSizes;
    
    if (verticalSizes[0] > 0 && verticalSizes[1] > 0) {
        resizeDocks(verticalDocks, verticalSizes, Qt::Vertical);
    }
    
    QList<QDockWidget*> horizontalDocks;
    horizontalDocks << ui->dockWidget << ui->dockWidget_3;
    
    int dock1Width = qMax(200, mainWindowWidth / 6);  
    int dock3Width = qMax(400, mainWindowWidth - dock1Width - 50); 
    QList<int> horizontalSizes;
    horizontalSizes << dock1Width << dock3Width;
    qDebug() << "Horizontal sizes:" << horizontalSizes;
    
    if (horizontalSizes[0] > 0 && horizontalSizes[1] > 0) {
        resizeDocks(horizontalDocks, horizontalSizes, Qt::Horizontal);
    }
    
    this->update();
    QApplication::processEvents();
    qDebug() << "Default layout restored with proper sizing";
}
void MainWindow::showWindowDock(const QString name)
{
    if(name == "Instance"){
        ui->dockWidget->show();
        return;
    } else if(name == "Waveform"){
        ui->dockWidget_2->show();
        return;
    } else if(name == "Editor"){
        ui->dockWidget_3->show();
        return;
    }else if(name == "SignalList"){
        m_signalListDock->show();
        return;
    }
}
void MainWindow::setApplicationFont()
{
    
    QStringList fontPriority = {
        "Consolas",           
        "Cascadia Code",      
        "SF Mono",           
        "Monaco",            
        "JetBrains Mono",    
        "Fira Code",         
        "Ubuntu Mono",       
        "DejaVu Sans Mono",  
        "Courier New",       
        "Monospace"          
    };
    QFont font;
    QString selectedFont = "Monospace";
    
    for (const QString &fontName : fontPriority) {
        if (QFontDatabase().hasFamily(fontName)) {
            font.setFamily(fontName);
            selectedFont = fontName;
            break;
        }
    }
    
    font.setPointSize(10);
    font.setFixedPitch(true);
    font.setStyleHint(QFont::TypeWriter, QFont::PreferMatch);
    
    m_applicationFont = font;
    
    QApplication::setFont(font);
    
    this->setFont(font);
}
void MainWindow::setFontForAllChildren(QWidget *parent, const QFont &font)
{
    if (!parent) return;
    parent->setFont(font);
    
    for (QObject *child : parent->children()) {
        if (QWidget *widget = qobject_cast<QWidget*>(child)) {
            widget->setFont(font);
            setFontForAllChildren(widget, font);
        }
    }
}
void MainWindow::setupThemeConnections()
{
    
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
}
void MainWindow::onThemeChanged(const QString &theme)  
{
    qDebug() << "主题已切换至:" << theme;
    updateUIForTheme(theme);
}
void MainWindow::updateUIForTheme(const QString &themeName)
{
    
    if(m_textEditor != nullptr ) {
        m_textEditor->handleThemeChange(themeName);
    }
    if(m_secondEditor != nullptr ) {
        m_secondEditor->handleThemeChange(themeName);
    }
    if(ui->dockWidget_2) {
        loadThemeForWaveform(themeName);
    }

}
void MainWindow::onWaveformDisplaySignalDoubleClicked(const QString &fullPath) {
    qDebug() << "Signal path received in MainWindow:" << fullPath;
    doubleClickedSignalFullPath = fullPath;
    m_textEditor->goToDriverBySignalName(doubleClickedSignalFullPath);
}
void MainWindow::getStatusByFullPath(const QString varRefStr, const QString &fullPath) {
    if(!GlobalState::instance().containsMap(fullPath)) {
        QMap<QString, QString> map =  m_textEditor->getStatusByFullPath(fullPath);
        GlobalState::instance().setMap(varRefStr, map);
    }
}
void MainWindow::onSignalFileOpened(QSharedPointer<IWaveformReader> reader, QSharedPointer<Waveform> waveform)
{
    m_waveWindow->updateWaveformReader(reader);
    m_waveWindow->updateWaveform(waveform);
    m_waveform = waveform;
    m_waveToInstanceMap[m_waveWindow] = instance;
}
void MainWindow::handleScopeDoubleClick(QString scope_path){
    m_textEditor->getModuleAndLoadFile(scope_path);
    if(m_signalListDock->isVisible()){
        getSiganlLists();
    }
}
MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::removeAllDock()
{
    qDebug() << "m_docks.size():" << m_docks.size();
    for (int i = 0; i < m_docks.size(); ++i)
    {
        removeDockWidget(m_docks[i]);
    }
}
void MainWindow::addDockToMDocks()
{
    m_docks.append(ui->dockWidget);
    m_docks.append(ui->dockWidget_2);
    m_docks.append(ui->dockWidget_3);
}
void MainWindow::showDock(const QList<int> &index)
{
    if (index.isEmpty())
    {
        for (int i = 0; i < m_docks.size(); ++i)
        {
            m_docks[i]->show();
        }
    }
    else
    {
        foreach (int i, index)
        {
            if (i >= 0 && i < m_docks.size())
            {
                m_docks[i]->show();
            }
        }
    }
}
void MainWindow::setupConnections()
{
    connect(ui->actionExit, &QAction::triggered, qApp, &QApplication::quit);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionBackwardHistory, &QAction::triggered, this, &MainWindow::goBack);
    connect(ui->actionCalling, &QAction::triggered, this, &MainWindow::goToCalling);
    connect(ui->actionDefinition, &QAction::triggered, this, &MainWindow::goToDefinition);
    connect(ui->actionForwardHistory, &QAction::triggered, this, &MainWindow::goForward);
    connect(ui->actionNewWaveform, &QAction::triggered, this, [this]() {
        createNewWaveTab(tr("<Wave:%1>").arg(m_nextTabNumber));
    });
    connect(ui->actionDriverIcon, &QAction::triggered, this, &MainWindow::goToDriver);
    connect(ui->actionLoadIcon, &QAction::triggered, this, &MainWindow::goToLoad);
    connect(m_textEditor, &TextEditor::setNavigationState, this, &MainWindow::updateNavigationState);
    connect(m_textEditor, &TextEditor::setTitleAndScope, this, &MainWindow::setEditorTitleAndScope);
    connect(m_textEditor, &TextEditor::addToWaveform, this, &MainWindow::addToWaveform);
    connect(m_textEditor, &TextEditor::fetchTipDataByName, this, &MainWindow::fetchTipData);
    connect(m_textEditor, &TextEditor::setDriverLoadAvailable, this, &MainWindow::updateDriverLoadState);
    connect(ui->actionGlobalSearch,&QAction::triggered, this, &MainWindow::onSearchButtonClicked);
    connect(ui->actionHierarchyChange,&QAction::triggered, this, &MainWindow::onHierarchyChangeClicked);
    connect(m_textEditor, &TextEditor::setFilelistPath, this, &MainWindow::setFilelistPath);
    connect(m_textEditor, &TextEditor::openFileAtPosition, this, &MainWindow::openFileAtPosition);
    connect(m_textEditor, &TextEditor::resultsBuilt, this, &MainWindow::updateDriverLoadInfo);
    connect(ui->actionDriverIcon, &QAction::triggered, this, &MainWindow::toggleDriverLoadArea);
    connect(ui->actionLoadIcon, &QAction::triggered, this, &MainWindow::toggleDriverLoadArea);
    connect(m_textEditor, &TextEditor::populateTreeSignal, this, &MainWindow::populateTree);
    connect(ui->actionFont,&QAction::triggered, this, &MainWindow::onFontButtonClicked);
    connect(m_textEditor, &TextEditor::mergeformatSignal, this, &MainWindow::mergeformat);
    connect(ui->actionGoToLine,&QAction::triggered, this, &MainWindow::showSimpleGoToLineDialog);
    connect(m_textEditor, &TextEditor::checkWaveformRequested,this, &MainWindow::onCheckWaveformRequested);
    connect(ui->actionSignalist, &QAction::triggered, this, [this]() {
        this->toggleSignalListDock();
    });
}
void MainWindow::waitToUpdate()
{
    QMessageBox::about(this, "To be launched online",
                       "This function is pending launch\n"
                       "coming soon");
}
void MainWindow::about()
{
    QMessageBox::about(this, "About BoYa",
                        "An advanced waveform viewer and analysis tool for chip/FPGA designers and verification engineers. \n\n"
                        "Created by OSCHIP team.\n\n"
                        "Version V1.0.1");
}
void MainWindow::goToCalling(){
    m_textEditor->goToCalling();
}
void MainWindow::goToDefinition(){
    m_textEditor->goToDefinition();
}
void MainWindow::goBack()
{
    m_textEditor->goBack();
}
void MainWindow::goForward()
{
    m_textEditor->goForward();
}
void MainWindow::updateNavigationState(bool backFlag, bool forwardFlag){
    ui->actionBackwardHistory->setEnabled(backFlag);
    ui->actionForwardHistory->setEnabled(forwardFlag);
}
void MainWindow::updateDriverLoadState(bool flag){
    ui->actionDriverIcon->setEnabled(flag);
    ui->actionLoadIcon->setEnabled(flag);
}
void MainWindow::setEditorTitleAndScope(QString title,QString scope){
    ui->dockWidget_3->setWindowTitle(title);
    if(m_waveform) {
        auto id = m_waveform->get_hierarchy().find_scope_by_fullpath(scope.toStdString());
        if (id > 0){
            instance->selectScopeInTree(id);
        }
    }
}
void MainWindow::addToWaveform(QString signalName) {
    if(m_waveform && m_waveWindow) {
        m_waveWindow->addSignalFromSource(signalName,"",0);
    }
}
void MainWindow::goToDriver(){
    m_textEditor->goToDriver();
}
void MainWindow::goToLoad(){
    m_textEditor->goToLoad();
}
void MainWindow::fetchTipData(QString signalName){
    if(m_waveform && m_waveWindow) {
        QString value = m_waveWindow->getSignalValue(signalName);
        m_textEditor->receiveTooltipContent(value);
    }
}
void MainWindow::setFilelistPath(const QStringList allFiles){
    fileListPath = allFiles;
}
void MainWindow::onSearchButtonClicked() {
    
    for (int i = 0; i < m_waveTabWidget->count(); i++) {
        OneSearchTab* existingSearchTab = qobject_cast<OneSearchTab*>(m_waveTabWidget->widget(i));
        if (existingSearchTab) {
            
            m_waveTabWidget->setCurrentIndex(i);
            return;
        }
    }
    
    OneSearchTab *searchTab = new OneSearchTab();
    searchTab->setFileListPath(fileListPath);
    connect(searchTab, &OneSearchTab::openFileRequested,
            this, &MainWindow::openFileAtLine);
    int tabIndex = m_waveTabWidget->addTab(searchTab, "GlobalSearch");
    m_waveTabWidget->setCurrentIndex(tabIndex);
}
void MainWindow::onHierarchyChangeClicked() {
    if (!m_dialog) {
        m_dialog = new HierarchyPrefixDialog();
        connect(m_dialog, &HierarchyPrefixDialog::prefixChanged,
                this, &MainWindow::onDialogPrefixChanged);
        connect(m_dialog, &HierarchyPrefixDialog::prefixReset,
                this, &MainWindow::onDialogPrefixReset);
    }
    
    m_dialog->exec();
}
void MainWindow::onDialogPrefixChanged(const QString& original, const QString& newPrefix){
    m_textEditor->setHierarchyPrefix(original,newPrefix);
}
void MainWindow::onDialogPrefixReset(){
    m_textEditor->setHierarchyPrefix("","");
}
void MainWindow::openFileAtLine(const QString &filePath, int lineNumber) {
    m_textEditor->openFileAtLine(filePath,lineNumber);
}
void MainWindow::openFileAtPosition(const QString &filePath, int lineNumber, int columnNumber) {
    
    if (!m_secondEditorDock) {
        m_secondEditorDock = new QDockWidget("Src2", this);
        m_secondEditorDock->setObjectName("secondEditorDock");
        m_secondEditorDock->setAllowedAreas(Qt::AllDockWidgetAreas);
        m_secondEditorDock->setFeatures(QDockWidget::DockWidgetMovable |
                                        QDockWidget::DockWidgetClosable |
                                        QDockWidget::DockWidgetFloatable);
        m_secondEditorDock->setProperty("SecondEditor",true);

        m_secondEditor = new SimpleTextEditor();
        m_secondEditor->setProperty("SecondEditor",true);
        m_secondEditorDock->setWidget(m_secondEditor);
        
        connect(m_secondEditorDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
            if (!visible) {
                
            }
        });
        
        splitDockWidget(ui->dockWidget_3, m_secondEditorDock, Qt::Horizontal);
        
        QList<QDockWidget*> horizontalDocks;
        horizontalDocks << ui->dockWidget_3 << m_secondEditorDock;
        QList<int> horizontalSizes;
        horizontalSizes << this->width() / 2 << this->width() / 2;
        resizeDocks(horizontalDocks, horizontalSizes, Qt::Horizontal);
    }
    
    if (!m_secondEditorDock->isVisible()) {
        m_secondEditorDock->show();
    }
    
    if (m_secondEditor) {
        m_secondEditor->openFileAtLineAndColumn(filePath, lineNumber, columnNumber);
        m_secondEditorDock->setWindowTitle("Src2:"+filePath);
    }
}
void MainWindow::initDriverLoadArea()
{
    
    m_driverLoadDock = new QDockWidget(tr("Driver/Load Details"), this);
    m_driverLoadDock->setProperty("DriverLoad",true);
    m_driverLoadDock->setObjectName("driverLoadDock");
    m_driverLoadDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_driverLoadDock->setFeatures(QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetClosable);
    
    m_driverLoadWidget = new QWidget();
    m_driverLoadWidget->setProperty("DriverLoad",true);
    QVBoxLayout *layout = new QVBoxLayout(m_driverLoadWidget);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    
    m_driverLoadTextEdit = new QTextBrowser();
    m_driverLoadTextEdit->setReadOnly(true);
    m_driverLoadTextEdit->setOpenLinks(false);
    layout->addWidget(m_driverLoadTextEdit);
    m_driverLoadWidget->setLayout(layout);
    m_driverLoadDock->setWidget(m_driverLoadWidget);
    
    m_driverLoadDock->setVisible(false);
    
    connect(m_driverLoadDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        m_driverLoadVisible = visible;
    });
    connect(m_driverLoadTextEdit, &QTextBrowser::anchorClicked, this, &MainWindow::onDriverLoadLinkClicked);
}
void MainWindow::toggleDriverLoadArea()
{
    if (!m_driverLoadDock->isVisible()) {
        m_previousLayout = saveState();
        
        splitDockWidget(ui->dockWidget_3, m_driverLoadDock, Qt::Vertical);
        
        m_driverLoadDock->setFixedHeight(150); 
        m_driverLoadDock->setVisible(true);
        m_driverLoadVisible = true;
    }
}
void MainWindow::updateDriverLoadInfo(QList<QMap<QString, QVariant>> results)
{
    m_driverLoadTextEdit->clear();
    if (results.isEmpty()) {
        return;
    }
    QString html = "<html><body style='font-family: Consolas, monospace; font-size: 10pt;'>";
    for (int i = 0; i < results.size(); ++i) {
        const auto &result = results[i];
        QString sourceFile = result.value("source_file").toString();
        QString lineStr = result.value("line").toString();
        int line = result.value("source_line").toInt();
        int column = result.value("source_column").toInt();
        if (!sourceFile.isEmpty() && line > 0) {
            QString link = QString("<a href='?path=%1&line=%2&column=%3%'>%1:%2:%3</a>")
            .arg(sourceFile)
                .arg(line)
                .arg(column);
            
            m_driverLoadTextEdit->append(lineStr.toHtmlEscaped()  + "     " +link );
        }
    }
}
void MainWindow::onDriverLoadLinkClicked(const QUrl &url){
    QUrlQuery query(url);
    
    QString filePath = query.queryItemValue("path");
    int lineNumber = 1;
    if (query.hasQueryItem("line")) {
        lineNumber = query.queryItemValue("line").toInt();
    }
    int columnNumber = QUrlQuery(url).queryItemValue("column").toInt();
    m_textEditor->openFileAtLine(filePath,lineNumber);
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == searchLineEdit) {
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent*>(event);
            if (dragEvent->mimeData()->hasFormat("application/waveform_display-signal-full-path-data") ||
                dragEvent->mimeData()->hasFormat("application/waveform_display-topSignal-move-data") ||
                dragEvent->mimeData()->hasFormat("application/texteditor") ||
                dragEvent->mimeData()->hasFormat("application/plain")) {
                dragEvent->acceptProposedAction();
                return true;
            }
        }
        else if (event->type() == QEvent::Drop) {
            QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
            QString fullpath;
            if (dropEvent->mimeData()->hasFormat("application/texteditor")) {
                fullpath = dropEvent->mimeData()->data("application/texteditor");
                dropEvent->accept();
            }
            if (dropEvent->mimeData()->hasFormat("text/plain")) {
                fullpath = dropEvent->mimeData()->data("text/plain");
            }
            if (dropEvent->mimeData()->hasFormat("application/waveform_display-signal-full-path-data")) {
                fullpath = dropEvent->mimeData()->data("application/waveform_display-signal-full-path-data");
                dropEvent->accept();
            }
            if (dropEvent->mimeData()->hasFormat("application/waveform_display-topSignal-move-data")) {
                fullpath = dropEvent->mimeData()->data("application/waveform_display-topSignal-move-data");
                dropEvent->accept();
            }
            QString displayText = fullpath.section('.', -1).section('[', 0, 0);
            searchLineEdit->setText(displayText);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
bool MainWindow::onRequestDisplayCheck(QString& full_name, const QString& name, bool& shouldDisplay){
    if (!m_textEditor || !textEditorLoaded) {
        shouldDisplay = true;
        return true;
    }
    shouldDisplay = m_textEditor->checkDisplay(full_name, name);
    return true;
}
void MainWindow::populateTree(){
    textEditorLoaded = true;
    if(m_instance){
        m_instance->populateTree();
    }
}
void MainWindow::onFontButtonClicked() {

    bool ok;
    
    QFont font = QFontDialog::getFont(&ok, m_applicationFont, this);
    if (ok) {
        
        font.setFixedPitch(true);
        font.setStyleHint(QFont::TypeWriter, QFont::PreferMatch);
        
        m_applicationFont = font;
        
        reapplyApplicationFont();
    }
}
void MainWindow::reapplyApplicationFont()
{
    
    QApplication::setFont(m_applicationFont);
    
    this->setFont(m_applicationFont);
    
    if (menuBar()) {
        menuBar()->setFont(m_applicationFont);
    }
    
    QString dockStyle = QString(
                            "QDockWidget {"
                            "    font-family: '%1';"
                            "    font-size: %2pt;"
                            "}"
                            "QDockWidget::title {"
                            "    font-family: '%1';"
                            "    font-size: %2pt;"
                            "}"
                            ).arg(m_applicationFont.family()).arg(m_applicationFont.pointSize());
    
    for (QDockWidget* dock : findChildren<QDockWidget*>()) {
        
        QString currentStyle = dock->styleSheet();
        
        QString mergedStyle = currentStyle + dockStyle;
        dock->setStyleSheet(mergedStyle);
    }
    
    for (QToolBar* toolbar : findChildren<QToolBar*>()) {
        toolbar->setFont(m_applicationFont);
    }
    if (searchLineEdit) {
        searchLineEdit->setFont(m_applicationFont);
    }
}
void MainWindow::mergeformat(QTextCharFormat fmt) {
    if (m_secondEditor) {
        m_secondEditor->mergeformat(fmt);
    }
}
void MainWindow::setGlobalPathTotextEditor() {
    if(m_textEditor) {
        m_textEditor->setGlobalPath(GlobalPaths::getAllPaths());
    }
}
void MainWindow::setWaveformTitle(QString name){
    QFileInfo fileInfo(name);
    QString fileName = fileInfo.fileName();

    if (m_currentActiveWaveWindow) {
        
        if (m_waveWindowToTabNumber.contains(m_currentActiveWaveWindow) && m_waveWindowToTabIndex.contains(m_currentActiveWaveWindow)) {
            int tabIndex = m_waveWindowToTabIndex[m_currentActiveWaveWindow];
            int numIndex = m_waveWindowToTabNumber[m_currentActiveWaveWindow];
            m_currentActiveWaveWindow->setWindowTitle("<Wave:" + QString::number(numIndex) + "> " + name);
            m_waveTabWidget->setTabText(tabIndex, "<Wave:" + QString::number(numIndex) + "> " + fileName);
        }
    }
}
void MainWindow::showSimpleGoToLineDialog()
{
    if (m_textEditor) {
        m_textEditor->showSimpleGoToLineDialog();
    }
}
void MainWindow::initializeShortcuts()
{
    ShortcutsManager* manager = ShortcutsManager::instance();
    
    manager->registerShortcut(Shortcuts::File::IMPORT_DESIGN,
                              "Ctrl+I",
                              tr("Import Design"),
                              tr("File"));
    manager->registerShortcut(Shortcuts::File::OPEN,
                              "Ctrl+O",
                              tr("Open file"),
                              tr("File"));
    manager->registerShortcut(Shortcuts::File::SAVE_GROUP,
                              "Ctrl+G",
                              tr("Save Group"),
                              tr("File"));
    manager->registerShortcut(Shortcuts::File::LOAD_GROUP,
                              "Ctrl+L",
                              tr("Load Group"),
                              tr("File"));
    manager->registerShortcut(Shortcuts::File::NEW_WAVEFORM,
                              "Ctrl+T",
                              tr("New Waveform"),
                              tr("File"));
    manager->registerShortcut(Shortcuts::View::INSTANCE,
                              "Alt+I",
                              tr("Show Instance"),
                              tr("View"));
    manager->registerShortcut(Shortcuts::View::EDITOR,
                              "Alt+E",
                              tr("Show Editor"),
                              tr("View"));
    manager->registerShortcut(Shortcuts::View::WAVEFORM,
                              "Alt+W",
                              tr("Show Waveform"),
                              tr("View"));
    manager->registerShortcut(Shortcuts::Tools::GLOBAL_SEARCH,
                              "Ctrl+F",
                              tr("Global Search"),
                              tr("Tools"));
    manager->registerShortcut(Shortcuts::Tools::HIERARCHY_CHANGE,
                              "Alt+H",
                              tr("Hierarchy Change"),
                              tr("Tools"));
    manager->registerShortcut(Shortcuts::Tools::FONT,
                              "Alt+F",
                              tr("Font"),
                              tr("Tools"));
    manager->registerShortcut(Shortcuts::Tools::GO_TO_LINE,
                              "Alt+G",
                              tr("Go to Line"),
                              tr("Tools"));
    manager->registerShortcut(Shortcuts::Tools::SHORTCUTS,
                              "Alt+S",
                              tr("Shortcuts"),
                              tr("Tools"));
    
    manager->bindToAction(Shortcuts::File::OPEN, ui->actionOpen);
    manager->bindToAction(Shortcuts::File::IMPORT_DESIGN, ui->actionImport_Design);
    manager->bindToAction(Shortcuts::File::SAVE_GROUP, ui->actionSave_Group);
    manager->bindToAction(Shortcuts::File::LOAD_GROUP, ui->actionLoad_Group);
    manager->bindToAction(Shortcuts::File::NEW_WAVEFORM, ui->actionNewWaveform);
    manager->bindToAction(Shortcuts::View::INSTANCE, ui->actionInstance );
    manager->bindToAction(Shortcuts::View::EDITOR, ui->actionEditor);
    manager->bindToAction(Shortcuts::View::WAVEFORM, ui->actionWaveform);
    manager->bindToAction(Shortcuts::Tools::GLOBAL_SEARCH, ui->actionGlobalSearch);
    manager->bindToAction(Shortcuts::Tools::HIERARCHY_CHANGE, ui->actionHierarchyChange);
    manager->bindToAction(Shortcuts::Tools::FONT, ui->actionFont);
    manager->bindToAction(Shortcuts::Tools::GO_TO_LINE, ui->actionGoToLine);
    manager->bindToAction(Shortcuts::Tools::SHORTCUTS, ui->actionShortcuts);
    connect(manager, &ShortcutsManager::shortcutConflicted,
            this, [this](const QString& id1, const QString& id2, const QKeySequence& shortcut) {
                QString msg = tr("Shortcut conflict: %1 and %2 both use %3")
                .arg(id1).arg(id2).arg(shortcut.toString());
                QMessageBox::warning(this, tr("Conflict"), msg);
            });
}
void MainWindow::on_actionShortcuts_triggered()
{
    ShortcutSettingsDialog dlg(this);
    dlg.exec();
}
void MainWindow::createNewWaveTab(const QString& title)
{
    WaveWindow* waveWindow = new WaveWindow(m_waveTabWidget);
    waveWindow->setProperty("Waveform",true);
    m_waveWindow = waveWindow;
    int tabIndex = m_waveTabWidget->addTab(waveWindow, title);
    m_waveTabWidget->setCurrentIndex(tabIndex);
    
    m_waveWindowToTabIndex[waveWindow] = tabIndex;
    
    connect(waveWindow, &WaveWindow::windowTitleChanged,
            this, &MainWindow::onWaveWindowTitleChanged);
    connect(waveWindow, &WaveWindow::waveformDisplaySignalDoubleClicked,
            this, &MainWindow::onWaveformDisplaySignalDoubleClicked);
    connect(waveWindow, &WaveWindow::timeChangeForGetSignals,
            this, &MainWindow::onTimeChangeForGetSignals);
    connect(waveWindow, &WaveWindow::getStatusByFullPath,
            this, &MainWindow::getStatusByFullPath);
    connect(waveWindow, &WaveWindow::waveFileOpen,
            this, [this]() {
                instance->openFile(); 
            });
    Instance* waveInstance = new Instance();
    waveInstance->setObjectName("waveInstance_" + QString::number(m_nextTabNumber));
    instance = waveInstance;
    
    m_waveToInstanceMap[waveWindow] = waveInstance;
    connect(waveInstance, &Instance::scopeDoubleClicked, this, &MainWindow::handleScopeDoubleClick);
    connect(waveInstance, &Instance::requestDisplayCheck, this, &MainWindow::onRequestDisplayCheck);
    connect(waveInstance, &Instance::setWaveformTitleSignal, this, &MainWindow::setWaveformTitle);
    connect(waveInstance, &Instance::setGlobalPathTotextEditor, this, &MainWindow::setGlobalPathTotextEditor);
    
    connect(waveInstance, &Instance::signalFileOpened, this,
            [this, waveWindow, waveInstance](QSharedPointer<IWaveformReader> reader, QSharedPointer<Waveform> waveform) {
                
                if (m_waveToInstanceMap.contains(waveWindow)) {
                    m_waveToInstanceMap[waveWindow] = waveInstance;
                    waveWindow->updateWaveformReader(reader);
                    waveWindow->updateWaveform(waveform);
                }
                
                if (waveWindow == m_currentActiveWaveWindow) {
                    m_instance = waveInstance;
                    
                    if (m_instanceDock->widget() != waveInstance) {
                        QWidget* oldWidget = m_instanceDock->widget();
                        if (oldWidget && oldWidget != instance) {
                            oldWidget->setParent(nullptr); 
                        }
                        m_instanceDock->setWidget(waveInstance);
                    }
                }
            });
    
    if (m_waveTabWidget->count() == 1) {
        m_currentActiveWaveWindow = waveWindow;
        updateWaveWindowConnections();
    } else {
        
        m_currentActiveWaveWindow = waveWindow;
        updateWaveWindowConnections();
    }
    m_currentActiveWaveWindow->setWindowTitle(title);
    m_waveWindowToTabNumber[waveWindow] = m_nextTabNumber;
    m_nextTabNumber++;
}
void MainWindow::onWaveTabChanged(int index)
{
    if (index >= 0) {
        QWidget* widget = m_waveTabWidget->widget(index);
        if (WaveWindow* waveWindow = qobject_cast<WaveWindow*>(widget)) {
            
            m_currentActiveWaveWindow = waveWindow;
            m_waveWindow = waveWindow;
            
            updateWaveWindowConnections();
            if (m_waveToInstanceMap.contains(waveWindow)) {
                Instance* waveInstance = m_waveToInstanceMap[waveWindow];
                m_instance = waveInstance;
                
                if (m_instanceDock->widget() != waveInstance) {
                    
                    QWidget* oldWidget = m_instanceDock->widget();
                    
                    if (oldWidget && oldWidget != instance) {
                        
                        oldWidget->setParent(nullptr);
                        oldWidget->hide();
                    }
                    
                    m_instanceDock->setWidget(waveInstance);

                }
            }
            
            ui->dockWidget_2->setWindowTitle(waveWindow->windowTitle());
        } else if (OneSearchTab* searchTab = qobject_cast<OneSearchTab*>(widget)) {
            
            m_currentActiveWaveWindow = nullptr;
            m_waveWindow = nullptr;
            disconnectCurrentWaveWindowConnections();
            
            ui->dockWidget_2->setWindowTitle("GlobalSearch");
        }
    } else {
        m_currentActiveWaveWindow = nullptr;
        m_waveWindow = nullptr;
        disconnectCurrentWaveWindowConnections();
    }
}
void MainWindow::onWaveTabCloseRequested(int index)
{
    QWidget* widget = m_waveTabWidget->widget(index);
    if (!widget) return;
    if (WaveWindow* waveWindow = qobject_cast<WaveWindow*>(widget)) {
        
        if (waveWindow == m_currentActiveWaveWindow) {
            disconnectCurrentWaveWindowConnections();
            m_currentActiveWaveWindow = nullptr;
            m_waveWindow = nullptr;
        }
        
        if (m_waveToInstanceMap.contains(waveWindow)) {
            Instance* waveInstance = m_waveToInstanceMap.take(waveWindow);
            waveInstance = nullptr;
            
            delete waveInstance;
        }
        
        m_waveWindowToTabIndex.remove(waveWindow);
        m_waveWindowToTabNumber.remove(waveWindow);
        
        m_waveTabWidget->removeTab(index);
        delete waveWindow;
        
        rebuildWaveWindowMapping();
        
        if (m_waveTabWidget->count() == 0) {
            createNewWaveTab(tr("<Wave:%1>").arg(m_nextTabNumber));
        }
    } else if (OneSearchTab* searchTab = qobject_cast<OneSearchTab*>(widget)) {

        disconnect(searchTab, &OneSearchTab::openFileRequested,
                   this, &MainWindow::openFileAtLine);
        
        m_waveTabWidget->removeTab(index);
        
        searchTab->deleteLater();
        
        if (m_waveTabWidget->count() == 0) {
            createNewWaveTab(tr("<Wave:%1>").arg(m_nextTabNumber));
        }
    }
}
void MainWindow::onWaveWindowTitleChanged(const QString& title)
{
    WaveWindow* waveWindow = qobject_cast<WaveWindow*>(sender());
    if (waveWindow && m_waveWindowToTabIndex.contains(waveWindow)) {
        int tabIndex = m_waveWindowToTabIndex[waveWindow];
        m_waveTabWidget->setTabText(tabIndex, title);
        
        if (waveWindow == m_currentActiveWaveWindow) {
            ui->dockWidget_2->setWindowTitle(title);
        }
    }
}
void MainWindow::updateWaveWindowConnections()
{
    if (m_currentActiveWaveWindow) {
        
        disconnectCurrentWaveWindowConnections();
        
        connect(m_currentActiveWaveWindow, &WaveWindow::windowTitleChanged,
                this, [this](const QString &title) {
                    this->ui->dockWidget_2->setWindowTitle(title);
                });

        ui->dockWidget_2->setWindowTitle(m_currentActiveWaveWindow->windowTitle());
    }
}
void MainWindow::disconnectCurrentWaveWindowConnections()
{
    if (m_currentActiveWaveWindow) {
        disconnect(m_currentActiveWaveWindow, &WaveWindow::windowTitleChanged,
                   this, nullptr);
        
    }
}
void MainWindow::rebuildWaveWindowMapping()
{
    m_waveWindowToTabIndex.clear();
    for (int i = 0; i < m_waveTabWidget->count(); ++i) {
        WaveWindow* waveWindow = qobject_cast<WaveWindow*>(m_waveTabWidget->widget(i));
        if (waveWindow) {
            m_waveWindowToTabIndex[waveWindow] = i;
        }
    }
}
void MainWindow::onCheckWaveformRequested(bool& hasWaveform)
{
    hasWaveform = false;
    if (m_currentActiveWaveWindow) {
        hasWaveform = m_currentActiveWaveWindow->hasWaveformLoaded();
    }
}

void MainWindow::toggleSignalListDock()
{
    if (!m_signalListDock) {
        return;
    }

    if (m_signalListDock->isVisible()) {
        m_signalListDock->hide();
    } else {
        m_signalListDock->show();
        m_signalListDock->raise();
        getSiganlLists();
    }
    if (ui->actionSignalist) {
        ui->actionSignalist->setChecked(m_signalListDock->isVisible());
    }
}
void MainWindow::setupSignalListDock()
{
    m_signalListDock = new SignalListDock(this);
    m_signalListDock->setWindowTitle(tr("Signal List"));

    int mainWindowWidth = this->width();
    int dockWidgetOriginalWidth = mainWindowWidth / 6;

    Qt::DockWidgetArea editorArea = dockWidgetArea(ui->dockWidget);
    if (editorArea != Qt::NoDockWidgetArea) {
        addDockWidget(editorArea, m_signalListDock);
        splitDockWidget(ui->dockWidget, m_signalListDock, Qt::Horizontal);
    } else {
        addDockWidget(Qt::RightDockWidgetArea, m_signalListDock);
    }

    if (ui->dockWidget && m_signalListDock) {
        int availableWidth = this->width();
        int dockWidgetWidth = availableWidth / 6;
        int signalListWidth = availableWidth / 6;
        int dockWidget3Width = availableWidth - dockWidgetWidth - signalListWidth;
        QList<int> sizes;
        sizes << dockWidgetWidth << signalListWidth << dockWidget3Width;

        QList<QDockWidget*> docks;
        docks << ui->dockWidget << m_signalListDock << ui->dockWidget_3;
        resizeDocks(docks, sizes, Qt::Horizontal);
    }
    m_signalListDock->hide();

    connect(m_signalListDock, &SignalListDock::signalClicked,
            this, &MainWindow::onSignalListSignalClicked);
    connect(m_signalListDock, &SignalListDock::signalDoubleClicked,
            this, &MainWindow::onSignalListSignalDoubleClicked);
    connect(m_signalListDock, &SignalListDock::addSignalsToWaveform,
            this, &MainWindow::onAddSignalsToWaveformFromSignalList);

    connect(m_signalListDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (ui->actionSignalist) {
            ui->actionSignalist->setChecked(visible);
        }
    });
    connect(m_signalListDock, &SignalListDock::addAndShowExpandSignals,
            this, &MainWindow::addAndShowExpandSignals);
}
void MainWindow::getSiganlLists(){
    if(m_textEditor != nullptr && m_waveform && m_waveWindow) {
        QVector<QStringList>signalLists =  m_textEditor->getSignalsByInstanceId();
        if(signalLists.size() >0){
            QVector<QStringList> updatedSignalLists;
            m_signallist_scope = m_textEditor->getCurrentScope();
            m_signallist_fileName = m_textEditor->getCurrentFileName();
            for (const QStringList &signal : signalLists) {
                if (signal.size() >= 3) {
                    QString value = m_waveWindow->getSignalValue(m_signallist_scope+"."+signal[0]);
                    QStringList updatedSignal = signal;
                    updatedSignal[1] = value;
                    updatedSignalLists.append(updatedSignal);
                }
            }
            m_signalListDock->setScope(m_signallist_scope);
            m_signalListDock->loadSignalsFromWaveform(updatedSignalLists);
        }
    }
}

void MainWindow::onTimeChangeForGetSignals(){
    if(m_signalListDock && m_signalListDock->isVisible()){
        QVector<QStringList>signalLists = m_signalListDock->getSignalLists();
        if(signalLists.size() >0){
            QVector<QStringList> updatedSignalLists;
            for (const QStringList &signal : signalLists) {
                if (signal.size() >= 3) {
                    QString value = m_waveWindow->getSignalValue(m_signallist_scope+"."+signal[0]);
                    QStringList updatedSignal = signal;
                    updatedSignal[1] = value;
                    updatedSignalLists.append(updatedSignal);
                }
            }
            m_signalListDock->updateSignalValueFromWaveform(updatedSignalLists);
        }
    }
}
void MainWindow::onSignalListSignalClicked(QStringList signalList){
    if(m_textEditor != nullptr){
        if (signalList.size() >= 3){
            QString row = signalList[4];
            m_textEditor->openFileAtLine(m_signallist_fileName,row.trimmed().toInt());
        }
    }
}

void MainWindow::onSignalListSignalDoubleClicked(QStringList signalList){
    if(m_textEditor != nullptr){
        if (signalList.size() >= 3){
            QString row = signalList[4];
            m_textEditor->openFileAtLine(m_signallist_fileName,row.trimmed().toInt());
        } else {
            return;
        }
    }
    if(m_waveform && m_waveWindow){
        m_waveWindow->addSignalFromSource(m_signallist_scope+"."+signalList[0],"",0);
    }
}
void MainWindow::onAddSignalsToWaveformFromSignalList(const QVector<QStringList> &signalList){
    if(m_waveform && m_waveWindow) {
        for (const QStringList &signal : signalList) {
            if(signal.size() >0){
                m_waveWindow->addSignalFromSource(m_signallist_scope+"."+signal[0],"",0);
            }
        }
    }
}
void MainWindow::addAndShowExpandSignals(const QString& signalName){
    if(m_waveform && m_waveWindow && m_signalListDock && m_signalListDock->isVisible()){
        m_waveWindow->addExpandSignals(m_signallist_scope+"."+signalName);
        QVector<QStringList>signalLists = m_signalListDock->getSignalLists();
        if(signalLists.size() >0){
            QVector<QStringList> updatedSignalLists;
            for (const QStringList &signal : signalLists) {
                if (signal.size() >= 3) {
                    QString value = m_waveWindow->getSignalValue(m_signallist_scope+"."+signal[0]);
                    QStringList updatedSignal = signal;
                    updatedSignal[1] = value;
                    updatedSignalLists.append(updatedSignal);
                }
            }
            m_signalListDock->updateSignalValueFromWaveform(updatedSignalLists);
        }
    }

}
