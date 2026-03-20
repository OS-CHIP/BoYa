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

#ifndef INSTANCE_H
#define INSTANCE_H
#include <QMainWindow>
#include <QTableWidget>
#include <QTreeWidget>
#include <QWidget>
#include "reader.h"
namespace Ui {
class Instance;
}
class Instance : public QMainWindow
{
    Q_OBJECT
public:
    explicit Instance(QWidget *parent = nullptr);
    
    QSize sizeHint() const override {
        return QSize(50, 100); 
    }
    ~Instance();
    void selectScopeInTree(ScopeRef scope_ref); 
    void processFileInBackground(const QString &fileName, const QByteArray &fileContent);
    void populateTree();
    
signals:
    
    void signalFileOpened(QSharedPointer<IWaveformReader> reader, QSharedPointer<Waveform> waveform);
    void scopeDoubleClicked(QString scope_path);
    void requestDisplayCheck(QString& full_name, const QString& name, bool& shouldDisplay);
    void setWaveformTitleSignal(QString filePath);
    void setGlobalPathTotextEditor();
protected:
public slots:
    void openFile(const QString &filePath = QString());
    
    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);
    void about();
    void setupConnections();
private:
    Ui::Instance *ui; 
    void loadChildren(QTreeWidgetItem *parentItem, ScopeRef parent_scope_ref, int depth);
    Hierarchy hierarchy;
    QSharedPointer<IWaveformReader> reader;
    QSharedPointer<Waveform> waveform;
    
    void notifyTheModuleInstanceAndWaveformAfterGettingTheFilePath(const QString &filePath);
    void expandTreeToDepth(QTreeWidgetItem *item, int depth);
    QHash<ScopeRef, QTreeWidgetItem*> m_scopeToItemMap; 
    QString waveformName;
};
#endif 