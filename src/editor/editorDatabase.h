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

#ifndef EDITORDATABASE_H
#define EDITORDATABASE_H
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QThreadStorage>
#include <QMutex>
#include <QElapsedTimer>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QThread>
#include <QAtomicInteger>
#include <QWaitCondition>
class EditorDatabase : public QObject
{
    Q_OBJECT
private:
    struct VarInfo {
        QString name;
        int line;
        int column;
        VarInfo() : line(0), column(0) {}
        VarInfo(const QString &addr, int l, int c) : name(addr), line(l), column(c) {}
    };
    
    struct ThreadLocalData {
        QSqlDatabase database;
        QSqlQuery insertModuleQuery;
        QSqlQuery insertMemberQuery;
        QSqlQuery insertRelationQuery;
        
        QMap<QString, int> instanceAddrToId;
        QMap<QString, int> memberAddrToId;
        QMap<QString, QString> symbolToAddr;
        QMap<QString, QString> definitionAddrToName;
        
        QList<QList<QVariant>> moduleBatch;
        QList<QList<QVariant>> memberBatch;
        QList<QList<QVariant>> relationBatch;
        
        QVector<VarInfo> varInfoPool;
        int varInfoPoolIndex = 0;
        
        int totalModulesProcessed = 0;
        int totalInstancesProcessed = 0;
        int totalMembersProcessed = 0;
        int totalRelationsProcessed = 0;
        ThreadLocalData() = default;
        ~ThreadLocalData() {
            if (database.isOpen()) {
                database.close();
            }
        }
    };
    
    QThreadStorage<ThreadLocalData*> m_threadLocalData;
    
    QString m_globalDbPath;
    QAtomicInteger<int> m_globalBatchSize;
    static bool s_driversLoaded;
    static QMutex s_driversMutex;
    
    class PerformanceTimer {
    private:
        QElapsedTimer m_timer;
        QString m_name;
    public:
        PerformanceTimer(const QString &name) : m_name(name) { m_timer.start(); }
        ~PerformanceTimer() { qDebug() << m_name << "took" << m_timer.elapsed() << "ms"; }
    };
    
    ThreadLocalData* getThreadLocalData();
    bool initializeThreadLocalDatabase(ThreadLocalData* data);
    bool createTables(ThreadLocalData* data);
    bool prepareQueries(ThreadLocalData* data);
    void clearThreadCaches(ThreadLocalData* data);
    void clearThreadBatches(ThreadLocalData* data);
    void resetThreadVarInfoPool(ThreadLocalData* data);
    VarInfo* getVarInfoFromPool(ThreadLocalData* data, const QString &addr, int line, int column);
    bool flushModuleBatch(ThreadLocalData* data);
    bool flushMemberBatch(ThreadLocalData* data);
    bool flushRelationBatch(ThreadLocalData* data);
public:
    explicit EditorDatabase(QObject *parent = nullptr);
    ~EditorDatabase();
    
    bool initializeDatabase(const QString &dbPath = "");
    void releaseDatabase();
    
    bool importSlangAST(const QJsonObject &astData);
    bool importSlangASTWithConnection(const QJsonObject &astData, QSqlDatabase &connection);
    
    QList<QSqlRecord> getModuleDefinitions();
    QSqlRecord getInstancesById(int id);
    QSqlRecord getInstanceByAddr(const QString &addr);
    QSqlRecord getInstanceByModuleAddr(const QString &moduleAddr);
    QSqlRecord getInstanceByModuleName(const QString &moduleName);
    QList<QSqlRecord> getInstancesByParent(const QString &parentAddr = "");
    QList<QSqlRecord> getMembersByInstance(const QString &instanceAddr);
    QList<QSqlRecord> getDriverLoadRelations(const QString &memberAddr = "");
    QList<QSqlRecord> getDriverLoadRelationsByInstance(const QString &instanceAddr);
    QSqlRecord getModuleByAddr(const QString &addr);
    QSqlRecord getModuleByFilePathAndLineNumber(const QString &filePath, int lineNumber);
    QSqlRecord findInstanceByHierarchyCTE(const QString &hierarchyPath);
    QString getHierarchyPathByInstanceId(int instanceId);
    QList<QSqlRecord> getMemberByInstanceIdAndName(int instanceId, const QString &name);
    QList<QSqlRecord> getDriversByAddr(const QString &addr);
    QList<QSqlRecord> getDriverByAddr(const QStringList &addrs);
    QList<QSqlRecord> getLoadByAddr(const QStringList &addrs);
    QSqlRecord getInstanceByMemberAddr(const QString &addr);
    QSqlRecord getMemberByMemberAddr(const QString &addr);
    QList<QSqlRecord> loadMarkersForFile(int instanceId,QString fileName);
    QList<QSqlRecord> loadModuleMarkersForFile(QString fileName);
    QList<QSqlRecord> loadInstanceMarkersForFile(QString fileName);
    QList<QSqlRecord> getTypeNamefromTypeAddr(QString addr);
    QSqlRecord getMemberByIntanceIdAndName(const QString &name ,int instanceId);
    QList<QSqlRecord> getFSMValueAndName(const QStringList &addrs,const QStringList &enumAddrs);
    bool copyInstanceTree(ThreadLocalData* data, int sourceInstanceId, int targetParentId);
    bool copyInstanceStructure(ThreadLocalData* data, int sourceInstanceId, int targetParentId);
    bool copyInstanceMembers(ThreadLocalData* data);
    bool checkIsStruct(QJsonObject member);
    bool checkIsPackage(QJsonObject member) ;
    bool checkMemberByInstanceIdAndName(const QString &name, int instanceId);
    QList<QSqlRecord> getSignalsByInstanceId(int instanceId);
    
    void setBatchSize(int size) {
        m_globalBatchSize.storeRelease(size);  
    }
    int batchSize() const {
        return m_globalBatchSize.loadAcquire();  
    }
private:
    int m_instanceId;
    QMap<int, QSet<int>> instanceMappings;
    
    void buildSymbolMapping(ThreadLocalData* data, const QJsonObject &astData);
    bool insertModuleDefinition(ThreadLocalData* data, const QJsonObject &definition);
    bool insertInstance(ThreadLocalData* data, const QJsonObject &instance, const QString &parentAddr = "");
    bool insertMember(ThreadLocalData* data, const QJsonObject &member, const QString &instanceAddr);
    bool insertConnections(ThreadLocalData* data, const QJsonArray &connections, const QString &instanceAddr);
    bool insertDriverLoadRelation(ThreadLocalData* data, const VarInfo &driver, const VarInfo &load,
                                  const QString &relationType, const QString &sourceFile,
                                  int sourceLineStart, int sourceLineEnd);
    
    bool cacheInstanceId(ThreadLocalData* data, const QString &instanceAddr, int instanceId);
    int getCachedInstanceId(ThreadLocalData* data, const QString &instanceAddr);
    
    bool analyzeVariableInitializer(ThreadLocalData* data, const QJsonObject &initializer,
                                    const QJsonObject &parentExpr, const VarInfo &variable,
                                    const QString &sourceFile, int sourceLineStart, int sourceLineEnd);
    QList<VarInfo> extractMembersFromExpression(ThreadLocalData* data, const QJsonObject &expr,
                                                const QJsonObject &parentExpr);
    QString buildMemberAccessPath(ThreadLocalData* data, const QJsonObject &memberAccessExpr);
    QString buildSelectorExpression(const QJsonObject &selectorExpr);
    QString buildElementSelectPath(ThreadLocalData* data, const QJsonObject &elementSelectExpr);
    bool analyzeAssignment(ThreadLocalData* data, const QJsonObject &assignment,
                           const QString &sourceFile, int sourceLineStart, int sourceLineEnd);
    void processLoadsToRelation(ThreadLocalData* data, const QList<VarInfo> loads, QString relationType,
                                const QString &sourceFile, int sourceLineStart, int sourceLineEnd);
    bool analyzeStatement(ThreadLocalData* data, const QJsonObject &stmt, const QString &sourceFile,
                          int sourceLineStart, int sourceLineEnd);
    bool analyzeProceduralBlock(ThreadLocalData* data, const QJsonObject &block,
                                const QString &sourceFile, int sourceLine, int sourceColumn);
    bool analyzeExpressionToMember(ThreadLocalData* data, const QJsonObject &expr,
                                   const QJsonObject &parentExpr, const VarInfo &member,
                                   const QString &relationType, const QString &sourceFile,
                                   int sourceLineStart, int sourceLineEnd);
    bool analyzeMemberToExpression(ThreadLocalData* data, const VarInfo &member,
                                   const QJsonObject &expr, const QJsonObject &parentExpr,
                                   const QString &relationType, const QString &sourceFile,
                                   int sourceLineStart, int sourceLineEnd);
    QString getMemberAddrFromSymbol(ThreadLocalData* data, const QString &symbol);
    QString getMemberNameFromSymbol(const QString &symbol);
    bool insertInstanceGenerateBlockArray(ThreadLocalData* data, const QJsonObject &member, const QString &parentAddr);
    bool insertTypeMembers(ThreadLocalData* data, const QJsonObject &member, const QString &parentAddr);
    bool insertMemberAccess(ThreadLocalData* data, QString name, int source_line, int source_column, QString addr, int instance_id );
    int findNonStatementBlockInstance(ThreadLocalData* data, int startInstanceId);
    QString m_emun_addr;
};
#endif 
