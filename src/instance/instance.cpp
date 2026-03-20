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

#include "instance.h"
#include "ui_instance.h"
#include <QBuffer>
#include <QDebug>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>
#include <QtConcurrent/QtConcurrentRun>
#include "myutils.h"
#include "pathutils.h"
Instance::Instance(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Instance)
{
    ui->setupUi(this);
    
    ui->treeWidget->setProperty("Instance", true);
    setupConnections();
}
void Instance::setupConnections()
{

    ui->treeWidget->setExpandsOnDoubleClick(false);
    
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &Instance::onTreeItemDoubleClicked);
}
void Instance::notifyTheModuleInstanceAndWaveformAfterGettingTheFilePath(const QString &filePath)
{
    auto reader = create_reader_by_extension(filePath.toUtf8().constData());
    if (!reader) {
        qDebug() << "Unsupported file format";
        
        return;
    }
    
    if (!reader->open(filePath.toUtf8().constData())) {
        QMessageBox::information(this, "BoYa",
                                 "Please enter a valid FST or VCD file.");
        
        return;
    }
    
    auto waveform = reader->read();
    if (!waveform) {
        QMessageBox::information(this, "BoYa",
                                 "Please enter a valid FST or VCD file.");
        return;
    }
    emit signalFileOpened(reader, waveform);
    
    hierarchy = waveform->get_hierarchy();
    
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();  
    
    QString suffix = fileInfo.suffix().toLower();
    QString fullSuffix = "." + suffix;  
    
    if (fullSuffix == ".fst" || fullSuffix == ".vcd") {
        
        waveformName = fileInfo.completeBaseName();  

    }
    emit setWaveformTitleSignal(filePath);
    qDebug() << "cout populateTree";
    
    populateTree();
}
void Instance::openFile(const QString &filePath)
{
    qDebug() << "Instance openFile";
    QString actualFilePath = filePath;
    if (!actualFilePath.isEmpty()) {
        notifyTheModuleInstanceAndWaveformAfterGettingTheFilePath(actualFilePath);
    }
    else
    {
        QFileDialog::getOpenFileContent(
            
            "Simulation Files (*.fst *.vcd)", [this](const QString &fileName, const QByteArray &fileContent) {
                if (fileName.isEmpty()) {
                    return;
                }
                qDebug() << "Selected file:" << fileName;
                qDebug() << "File size:" << fileContent.size() << "bytes";
                QtConcurrent::run([this, fileName, fileContent]() {
                    processFileInBackground(fileName, fileContent);
                });
            });
    }
}
void Instance::processFileInBackground(const QString &fileName, const QByteArray &fileContent) {

    QMetaObject::invokeMethod(this, [this, fileName]() {
        notifyTheModuleInstanceAndWaveformAfterGettingTheFilePath(fileName);
    }, Qt::QueuedConnection);
}
void Instance::onTreeItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    
    if (!item->isExpanded()) {
        item->setExpanded(true);
    }
    
    QVariant data = item->data(0, Qt::UserRole);
    if (!data.isValid()) {
        return;
    }
    ScopeRef scope_ref = data.toULongLong();
    if (scope_ref == INVALID_SCOPE_REF) {
        return;
    }
    QString scope_path = hierarchy.get_full_scope_path(scope_ref);
    emit scopeDoubleClicked(scope_path);
}
void Instance::about()
{

}
const char* ScopeTypeToString(ScopeType type) {
    switch (type) {
    case ScopeType::Module: return "Module";
    case ScopeType::Task: return "Task";
    case ScopeType::Function: return "Function";
    case ScopeType::Begin: return "Begin";
    case ScopeType::Fork: return "Fork";
    case ScopeType::Generate: return "Generate";
    case ScopeType::Struct: return "Struct";
    case ScopeType::Union: return "Union";
    case ScopeType::Class: return "Class";
    case ScopeType::Interface: return "Interface";
    case ScopeType::Package: return "Package";
    case ScopeType::Program: return "Program";
    case ScopeType::VhdlArchitecture: return "VhdlArchitecture";
    case ScopeType::VhdlProcedure: return "VhdlProcedure";
    case ScopeType::VhdlFunction: return "VhdlFunction";
    case ScopeType::VhdlRecord: return "VhdlRecord";
    case ScopeType::VhdlProcess: return "VhdlProcess";
    case ScopeType::VhdlBlock: return "VhdlBlock";
    case ScopeType::VhdlForGenerate: return "VhdlForGenerate";
    case ScopeType::VhdlIfGenerate: return "VhdlIfGenerate";
    case ScopeType::VhdlGenerate: return "VhdlGenerate";
    case ScopeType::VhdlPackage: return "VhdlPackage";
    default: return "Unknown"; 
    }
}
void Instance::populateTree()
{
    ui->treeWidget->clear();
    m_scopeToItemMap.clear(); 
    GlobalPaths::clear();
    
    ui->treeWidget->setIconSize(QSize(18, 18));
    
    ScopeRef root_scope = hierarchy.get_root_scope();
    if (root_scope == INVALID_SCOPE_REF) {
        return;
    }
    QTreeWidgetItem *topLevelItem = new QTreeWidgetItem(ui->treeWidget);
    topLevelItem->setText(0, waveformName);
    topLevelItem->setData(0, Qt::UserRole, QVariant::fromValue(root_scope));
    
    m_scopeToItemMap.insert(root_scope, topLevelItem);
    QString iconPath = MyUtils::getScopeIconPath(ScopeType::Root);
    QIcon icon(iconPath);
    if (!icon.isNull()) {
        
        QColor color = QColor("#6100ff");
        QIcon coloredIcon = MyUtils::changeIconColor(icon, color);
        topLevelItem->setIcon(0, coloredIcon);
    }
    
    loadChildren(topLevelItem, root_scope, 0);
    expandTreeToDepth(topLevelItem, 2);
    emit setGlobalPathTotextEditor();
}
void Instance::loadChildren(QTreeWidgetItem *parentItem, ScopeRef parent_scope_ref, int depth)
{
    const Scope &parent_scope = hierarchy.get_scope(parent_scope_ref);
    
    QVector<QColor> colorList = {QColor("#8d38ff"), QColor("#b65dff"), QColor("#de81ff"), QColor("#ffa4ff")};
    ScopeRef child_scope = parent_scope.first_child;
    while (child_scope != INVALID_SCOPE_REF) {
        const Scope &child = hierarchy.get_scope(child_scope);
        if (child.type == ScopeType::Struct || child.type == ScopeType::Union) {
            child_scope = child.next_sibling;  
            continue;  
        }
        QString name = QString::fromStdString(hierarchy.get_string(child.name_id));
        QString full_name = hierarchy.get_full_scope_path(child_scope);
        
        bool shouldDisplay = true;
        emit requestDisplayCheck(full_name, name, shouldDisplay);
        if (!shouldDisplay) {
            child_scope = child.next_sibling;
            continue;  
        }
        if (!shouldDisplay) {
            child_scope = child.next_sibling;
            continue;  
        }
        GlobalPaths::addPath(full_name);
        QTreeWidgetItem *childItem = new QTreeWidgetItem(parentItem);
        childItem->setText(0, name);
        childItem->setData(0, Qt::UserRole, QVariant::fromValue(child_scope));
        m_scopeToItemMap.insert(child_scope, childItem);
        QString iconPath = MyUtils::getScopeIconPath(child.type);
        
        QPixmap originalPixmap(iconPath);
        QPixmap scaledPixmap = originalPixmap.scaled(QSize(12, 12), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QIcon icon(scaledPixmap);
        
        if (!icon.isNull()) {
            
            QColor color = colorList[depth % colorList.size()];
            QIcon coloredIcon = MyUtils::changeIconColor(icon, color);
            childItem->setIcon(0, coloredIcon);
        }
        bool has_children = (child.first_child != INVALID_SCOPE_REF) || (child.first_var != INVALID_VAR_REF);
        childItem->setChildIndicatorPolicy(has_children ? QTreeWidgetItem::ShowIndicator
                                                        : QTreeWidgetItem::DontShowIndicator);
        if (has_children) {
            
            loadChildren(childItem, child_scope, depth + 1);
        }
        child_scope = child.next_sibling;
    }
}
void Instance::selectScopeInTree(ScopeRef scope_ref)
{
    if (scope_ref == INVALID_SCOPE_REF) {
        ui->treeWidget->clearSelection();
        return;
    }
    
    if (m_scopeToItemMap.contains(scope_ref)) {
        QTreeWidgetItem* item = m_scopeToItemMap.value(scope_ref);
        if (item) {
            ui->treeWidget->setCurrentItem(item);
            //ui->treeWidget->setFocus();

            return;
        }
    }
}
Instance::~Instance()
{

}
void Instance::expandTreeToDepth(QTreeWidgetItem *item, int depth)
{
    if (depth <= 0 || !item) return;
    item->setExpanded(true);
    
    for (int i = 0; i < item->childCount(); ++i) {
        expandTreeToDepth(item->child(i), depth - 1);
    }
}
