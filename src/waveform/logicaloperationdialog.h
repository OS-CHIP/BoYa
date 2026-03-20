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

#ifndef LOGICALOPERATIONDIALOG_H
#define LOGICALOPERATIONDIALOG_H
#include <QDialog>
#include <QSharedPointer>
#include <QVector>
#include <QMap>
#include <QStack>
#include <QString>
#include <QListWidgetItem>
#include "GlobalCounter.h"
#include "waveform_types.h"
class Waveform;
class Signal;
class QTextEdit;
class QComboBox;
class QListWidget;
class QPushButton;
class QCheckBox;
class QLabel;
struct OperatorInfo {
    QString symbol;
    int precedence;
    bool isUnary;
    bool isBinary;
    bool isLogical;
    bool isBitwise;
    bool isArithmetic;
    bool isShift;
    bool isComparison;
    bool rightAssociative;
    bool isPower;
};
struct SignalInfo {
    QString name;
    QString fullName;
    int width;
    int msb;
    int lsb;
    bool isBus;
    SignalInfo() : width(1), msb(0), lsb(0), isBus(false) {}
};
struct LogicalExpression {
    QString name;
    QString expression;
    int width;
    int msb;
    int lsb;
    QString resultBusFormat;
    VarType type;
    SignalInfo signalType;
    SignalRef signalRef;
    
    QVector<QString> tokens;
    QVector<QString> postfixTokens;
    LogicalExpression() : width(1), msb(0), lsb(0), type(VarType::Wire) {}
};
struct Token {
    enum Type {
        SIGNAL,      
        CONSTANT,    
        OPERATOR,    
        BRACKET,      
        CONCAT     
    };
    Type type;
    QString value;
    int width;  
    Token() : type(SIGNAL), width(1) {}
    Token(Type t, const QString &v, int w = 1) : type(t), value(v), width(w) {}
};
class LogicalOperationDialog : public QDialog
{
    Q_OBJECT
public:
    LogicalOperationDialog(const QString &initialSignal,
                           QSharedPointer<Waveform> waveform,
                           QWidget *parent = nullptr);
    ~LogicalOperationDialog();
    QString getCurrentExpression() const;
    QString getCurrentResult() ;
    
signals:
    void expressionCreated(const LogicalExpression &expr);
    void expressionModified(const LogicalExpression &expr);
    void expressionDeleted(const QString &name);
    void addToWaveform(const QSet<VarRef>& varRefs);
    void loadSignalData(SignalRef signal_ref);
protected slots:
    void onClearClicked();
    void onAddSignalClicked();
    void onCreateModifyClicked();
    void onEvaluateClicked();
    void onAddToWaveClicked();
    void onDeleteClicked();
    void onDeleteAllClicked();
    void onCloseClicked();
    void onOperatorClicked();
    void onSignalListItemDoubleClicked(QListWidgetItem *item);
private:
    void setupUI();
    void initializeOperators();
    void updateComboBoxName();
    void createOperatorButton(const QString &text, const QString &toolTip);
    void updateSignalList();
    
    int calculateExpressionWidth(const QString &expression) const;
    int inferUnaryOpWidth(const QString &op, int operandWidth) const;
    int inferBinaryOpWidth(const QString &op, int leftWidth, int rightWidth) const;
    QString evaluateExpressionAtTime(const QString &expression, Time time);
    QString bitwiseOperation(const QString &op, const QString &left,
                             const QString &right, int resultWidth) const;
    void onAddToWaveByExpr(LogicalExpression &expr);
    
    LogicalExpression* findExpression(const QString &name);
    LogicalExpression* findExpressionBySignalRef(SignalRef ref);
    SignalInfo getSignalInfo(const QString &signalName) const;
    int extractBitSelect(const QString& signalName) const;
    QPair<int, int> extractBusRange(const QString& signalName) const;
    QSharedPointer<Signal> createLogicalSignal(const LogicalExpression &expr);
    
    QString decToBin(qint64 value, int width) const;
    QString hexToBin(const QString &hexStr, int width) const;
    QString binToBin(const QString &binStr, int width) const;
    qint64 binToDec(const QString &binStr) const;
    QString addBinary(const QString &a, const QString &b) const;
    QString subBinary(const QString &a, const QString &b) const;
    QString mulBinary(const QString &a, const QString &b) const;
    QString shiftBinary(const QString &value, int shiftAmount,
                        bool left, bool arithmetic) const;
    QString extendWidth(const QString &value, int currentWidth,
                        int targetWidth, bool isSigned) const;
    
    QVector<Token> tokenizeExpression(const QString &expression);
    QVector<Token> infixToPostfix(const QVector<Token> &tokens);
    QString evaluatePostfixAtTime(const QVector<Token> &postfix, Time time);
    QString getSignalValueAtTime(const QString &signalName, Time time, bool *ok = nullptr);
    QString applyConcatOperation(const QVector<QString>& operands, const QVector<int>& widths);
    QString convertConstantToBinary(const QString &constant, int width);
    QString applyUnaryOperator(const QString &op, const QString &operand, int width);
    QString applyBinaryOperator(const QString &op, const QString &left,
                                const QString &right, int width);
    int calculateConstantWidth(const QString &constant) const;
    bool validateExpression(const QString &expression, QString &error) const;
    QVector<QString> extractSignalNames(const QString &expression) const;
    int getSignalBitWidth(const QString &signalName) const;
    SignalRef findSignalInWaveform(const QString &signalPath);
    bool parseExplicitWidthConstant(const QString &constant, int &width, QString &base, QString &valueStr) const;
    QString convertExplicitWidthConstant(const QString &constant, int targetWidth);
    QString convertSimpleConstantToBinary(const QString &constant, int width);
    QString adjustToWidth(const QString &binaryStr, int targetWidth, QChar fillChar = '0') const;
    QString applyFourValueOperation(const QString &op, QChar left, QChar right) const;
    bool containsXorZ(const QString &str);
private:
    QSharedPointer<Waveform> waveform;
    
    QTextEdit *expressionEdit;
    QComboBox *nameComboBox;
    QListWidget *signalList;
    QPushButton *clearBtn;
    QPushButton *addSignalBtn;
    QPushButton *createModifyBtn;
    QPushButton *addToWaveBtn;
    QPushButton *deleteBtn;
    QPushButton *deleteAllBtn;
    QPushButton *closeBtn;
    
    QVector<QPushButton*> operatorButtons;
    
    QMap<QString, OperatorInfo> operators;
    QMap<QString, int> signalValues;
    QMap<QString, int> signalWidths;
    
    QMap<QString, QSharedPointer<Signal>> signalCache;
    mutable std::vector<Time> cachedTimeTable;
    QString initialSignal;
};
extern QVector<QString> logicalNames;
extern QVector<LogicalExpression> logicalExpressions;
#endif 