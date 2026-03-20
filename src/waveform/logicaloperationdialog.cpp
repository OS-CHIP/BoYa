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

#include "logicaloperationdialog.h"
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QRegularExpression>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QString>
#include <QStack>
#include <QMap>
#include <cmath>
#include "waveform.h"
#include "signal.h"
QVector<LogicalExpression> logicalExpressions;
QVector<QString> logicalNames;
LogicalOperationDialog::LogicalOperationDialog(const QString &initialSignal,
                                               QSharedPointer<Waveform> waveform,
                                               QWidget *parent)
    : QDialog(parent)
    , waveform(waveform)
{
    initializeOperators();
    setupUI();
    updateSignalList();
    this->initialSignal = initialSignal;
    if (!initialSignal.isEmpty()) {
        expressionEdit->setText(initialSignal);
    }
    connect(signalList, &QListWidget::itemDoubleClicked,
            this, &LogicalOperationDialog::onSignalListItemDoubleClicked);
}
LogicalOperationDialog::~LogicalOperationDialog()
{
    
    signalCache.clear();
}
void LogicalOperationDialog::setupUI()
{
    setWindowTitle("Logical Operation Editor");
    setMinimumSize(800, 600);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QHBoxLayout *topLayout = new QHBoxLayout();
    
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    QHBoxLayout *nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Name:"));
    nameComboBox = new QComboBox();
    nameComboBox->setEditable(true);
    nameComboBox->setMinimumWidth(300);
    updateComboBoxName();
    nameLayout->addWidget(nameComboBox);
    nameLayout->addStretch();
    leftLayout->addLayout(nameLayout);
    
    QVBoxLayout *exprLayout = new QVBoxLayout();
    exprLayout->addWidget(new QLabel("Expression:"));
    expressionEdit = new QTextEdit();
    expressionEdit->setMaximumHeight(200);
    expressionEdit->setPlaceholderText("Enter logical expression (e.g., \"top.a\" & \"top.b\")");
    exprLayout->addWidget(expressionEdit);
    leftLayout->addLayout(exprLayout);
    topLayout->addLayout(leftLayout, 3);
    
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(10, 0, 0, 0);
    clearBtn = new QPushButton("Clear");
    addSignalBtn = new QPushButton("Add Signal");
    
    QHBoxLayout *withValueLayout = new QHBoxLayout();
    withValueLayout->addStretch();
    
    withValueLayout->addStretch();
    createModifyBtn = new QPushButton("Create");
    addToWaveBtn = new QPushButton("Add to Wave");
    deleteBtn = new QPushButton("Delete");
    deleteAllBtn = new QPushButton("Delete All");
    closeBtn = new QPushButton("Close");
    rightLayout->addWidget(clearBtn);
    rightLayout->addWidget(addSignalBtn);
    rightLayout->addLayout(withValueLayout);
    rightLayout->addWidget(createModifyBtn);
    rightLayout->addWidget(addToWaveBtn);
    rightLayout->addWidget(deleteBtn);
    rightLayout->addWidget(deleteAllBtn);
    rightLayout->addStretch();
    rightLayout->addWidget(closeBtn);
    topLayout->addLayout(rightLayout, 1);
    mainLayout->addLayout(topLayout);
    
    QVBoxLayout *operatorLayout = new QVBoxLayout();
    operatorLayout->addWidget(new QLabel("Operators:"));
    
    QHBoxLayout *opRow1 = new QHBoxLayout();
    QStringList operators1 = {"~", "!", "&", "|", "^","~&","~|", "~^"};
    for (const QString &op : operators1) {
        createOperatorButton(op, "Operator: " + op);
        opRow1->addWidget(operatorButtons.last());
    }
    opRow1->addStretch();
    operatorLayout->addLayout(opRow1);
    
    QHBoxLayout *opRow2 = new QHBoxLayout();
    QStringList operators2 = {"&&", "||", "==", "!=", "<", ">", "<=", ">="};
    for (const QString &op : operators2) {
        createOperatorButton(op, "Operator: " + op);
        opRow2->addWidget(operatorButtons.last());
    }
    opRow2->addStretch();
    operatorLayout->addLayout(opRow2);
    
    QHBoxLayout *opRow3 = new QHBoxLayout();
    QStringList operators3 = {"+", "-", "*", "/", "%","**", "<<", ">>"};
    for (const QString &op : operators3) {
        createOperatorButton(op, "Operator: " + op);
        opRow3->addWidget(operatorButtons.last());
    }
    opRow3->addStretch();
    operatorLayout->addLayout(opRow3);
    
    QHBoxLayout *opRow4 = new QHBoxLayout();
    QStringList operators4 = {"(", ")","{}" ,"0", "1", "'h", "'d", "'b"};
    for (const QString &op : operators4) {
        QString toolTip = op;
        if (op == "0" || op == "1") toolTip = "Constant: " + op;
        else if (op == "(" || op == ")") toolTip = "Bracket: " + op;
        else if (op == "{}") toolTip = "Concatenation";
        else if (op == "'h") toolTip = "Hex constant";
        else if (op == "'d") toolTip = "Decimal constant";
        else if (op == "'b") toolTip = "Binary constant";
        createOperatorButton(op, toolTip);
        opRow4->addWidget(operatorButtons.last());
    }
    opRow4->addStretch();
    operatorLayout->addLayout(opRow4);
    mainLayout->addLayout(operatorLayout);
    
    QLabel *signalLabel = new QLabel("Available Signals:");
    mainLayout->addWidget(signalLabel);
    signalList = new QListWidget();
    mainLayout->addWidget(signalList);
    
    connect(clearBtn, &QPushButton::clicked, this, &LogicalOperationDialog::onClearClicked);
    connect(addSignalBtn, &QPushButton::clicked, this, &LogicalOperationDialog::onAddSignalClicked);
    connect(createModifyBtn, &QPushButton::clicked, this, &LogicalOperationDialog::onCreateModifyClicked);
    connect(addToWaveBtn, &QPushButton::clicked, this, &LogicalOperationDialog::onAddToWaveClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &LogicalOperationDialog::onDeleteClicked);
    connect(deleteAllBtn, &QPushButton::clicked, this, &LogicalOperationDialog::onDeleteAllClicked);
    connect(closeBtn, &QPushButton::clicked, this, &LogicalOperationDialog::onCloseClicked);
}
void LogicalOperationDialog::initializeOperators()
{
    operators["**"] = {"**", 1, false, true, false, false, true, false, false, true, true};
    
    operators["~"] = {"~", 1, true, false, false, true, false, false, false, false, false};
    operators["!"] = {"!", 1, true, false, true, false, false, false, false, false, false};
    
    operators["*"] = {"*", 2, false, true, false, false, true, false, false, false, false};
    operators["/"] = {"/", 2, false, true, false, false, true, false, false, false, false};
    operators["%"] = {"%", 2, false, true, false, false, true, false, false, false, false};
    
    operators["+"] = {"+", 3, false, true, false, false, true, false, false, false, false};
    operators["-"] = {"-", 3, false, true, false, false, true, false, false, false, false};
    
    operators["<<"] = {"<<", 4, false, true, false, false, false, true, false, false, false};
    operators[">>"] = {">>", 4, false, true, false, false, false, true, false, false, false};
    
    operators["<"] = {"<", 5, false, true, false, false, false, false, true, false, false};
    operators[">"] = {">", 5, false, true, false, false, false, false, true, false, false};
    operators["<="] = {"<=", 5, false, true, false, false, false, false, true, false, false};
    operators[">="] = {">=", 5, false, true, false, false, false, false, true, false, false};
    
    operators["=="] = {"==", 6, false, true, false, false, false, false, true, false, false};
    operators["!="] = {"!=", 6, false, true, false, false, false, false, true, false, false};
    
    operators["&"] = {"&", 7, false, true, false, true, false, false, false, false, false};
    operators["~&"] = {"~&", 7, false, true, false, true, false, false, false, false, false};
    
    operators["^"] = {"^", 8, false, true, false, true, false, false, false, false, false};
    operators["~^"] = {"~^", 8, false, true, false, true, false, false, false, false, false};
    
    operators["|"] = {"|", 9, false, true, false, true, false, false, false, false, false};
    operators["~|"] = {"~|", 9, false, true, false, true, false, false, false, false, false};
    
    operators["&&"] = {"&&", 10, false, true, true, false, false, false, false, false, false};
    
    operators["||"] = {"||", 11, false, true, true, false, false, false, false, false, false};
}
void LogicalOperationDialog::updateComboBoxName()
{
    nameComboBox->clear();
    QString defaultName = QString("expr_%1").arg(GlobalCounter::getNextExpressionId());
    for(QString name : logicalNames){
        nameComboBox->addItem(name);
    }
    nameComboBox->setCurrentText(defaultName);
}
void LogicalOperationDialog::createOperatorButton(const QString &text, const QString &toolTip)
{
    QPushButton *btn = new QPushButton();
    QString displayText = text;
    displayText.replace("&", "&&");
    btn->setProperty("originalText", text);
    btn->setText(displayText);
    btn->setToolTip(toolTip);
    btn->setMinimumSize(40, 25);
    btn->setMaximumSize(60, 25);
    connect(btn, &QPushButton::clicked, this, &LogicalOperationDialog::onOperatorClicked);
    operatorButtons.append(btn);
}
void LogicalOperationDialog::updateSignalList()
{
    signalList->clear();
    
    for (auto it = signalValues.constBegin(); it != signalValues.constEnd(); ++it) {
        signalList->addItem(it.key());
    }
    
    for (const auto& expr : logicalExpressions) {
        QString displayName = QString("%1%2 = %3").arg(expr.name).arg(expr.resultBusFormat).arg(expr.expression);
        signalList->addItem(displayName);
    }
}
int LogicalOperationDialog::calculateExpressionWidth(const QString& expression) const
{
    QString expr = expression.trimmed();
    if (expr.isEmpty()) return 1;
    if (expr.startsWith("{") && expr.endsWith("}")) {
        QString inner = expr.mid(1, expr.length() - 2).trimmed();
        QStringList subExprs = inner.split(',', Qt::SkipEmptyParts);
        int totalWidth = 0;
        for (const QString& subExpr : subExprs) {
            totalWidth += calculateExpressionWidth(subExpr.trimmed());
        }
        return totalWidth;
    }
    
    if (expr.startsWith("(") && expr.endsWith(")")) {
        QString subExpr = expr.mid(1, expr.length() - 2).trimmed();
        return calculateExpressionWidth(subExpr);
    }
    
    if (expr.startsWith("~") || expr.startsWith("!")) {
        QString op = expr.left(1);
        QString subExpr = expr.mid(1).trimmed();
        int subWidth = calculateExpressionWidth(subExpr);
        return inferUnaryOpWidth(op, subWidth);
    }
    
    for (auto it = operators.constBegin(); it != operators.constEnd(); ++it) {
        const OperatorInfo& opInfo = it.value();
        if (!opInfo.isBinary) continue;
        QString op = it.key();
        int pos = expr.indexOf(op);
        if (pos != -1) {
            QString leftExpr = expr.left(pos).trimmed();
            QString rightExpr = expr.mid(pos + op.length()).trimmed();
            int leftWidth = calculateExpressionWidth(leftExpr);
            int rightWidth = calculateExpressionWidth(rightExpr);
            return inferBinaryOpWidth(op, leftWidth, rightWidth);
        }
    }
    
    int constWidth = calculateConstantWidth(expr);
    if (constWidth > 0) {
        return constWidth;
    }
    
    SignalInfo info = getSignalInfo(expr);
    if (info.width > 0) return info.width;
    return 1;
}
int LogicalOperationDialog::inferUnaryOpWidth(const QString& op, int operandWidth) const
{
    if (op == "~") return operandWidth;
    if (op == "!") return 1;
    return operandWidth;
}
int LogicalOperationDialog::inferBinaryOpWidth(const QString& op, int leftWidth, int rightWidth) const
{
    if (op == "+" || op == "-") return qMax(leftWidth, rightWidth) + 1;
    if (op == "*") return leftWidth + rightWidth;
    if (op == "**") {
        int shiftAmount = qMin(rightWidth, 16);
        int estimatedWidth = leftWidth * (1 << shiftAmount);
        return qMin(estimatedWidth, 1024);
    }
    if (op == "&" || op == "|" || op == "^" || op == "~^" || op == "~&" || op == "~|") return qMax(leftWidth, rightWidth);
    if (op == "<<" || op == ">>") return leftWidth;
    if (op == "&&" || op == "||" || op == "==" || op == "!=" ||
        op == "<" || op == ">" || op == "<=" || op == ">=") return 1;
    if (op == "/" || op == "%") return leftWidth;
    return qMax(leftWidth, rightWidth);
}
QVector<Token> LogicalOperationDialog::tokenizeExpression(const QString &expression)
{
    QVector<Token> tokens;
    QString expr = expression.trimmed();
    int i = 0;
    while (i < expr.length()) {
        QChar ch = expr[i];
        
        if (ch.isSpace()) {
            i++;
            continue;
        }
        
        if (ch == '{') {
            i++;
            QString concatExpr;
            int braceCount = 1;
            
            while (i < expr.length() && braceCount > 0) {
                if (expr[i] == '{') braceCount++;
                else if (expr[i] == '}') braceCount--;
                if (braceCount > 0) {
                    concatExpr += expr[i];
                }
                i++;
            }
            Token token(Token::CONCAT, concatExpr.trimmed());
            tokens.append(token);
            continue;
        }
        
        if (ch == '"') {
            i++;
            QString signalName;
            while (i < expr.length() && expr[i] != '"') {
                signalName += expr[i];
                i++;
            }
            i++; 
            Token token(Token::SIGNAL, signalName.trimmed());
            SignalInfo info = getSignalInfo(signalName);
            token.width = info.width;
            tokens.append(token);
            continue;
        }

        if (ch.isDigit() || ch == '\'' || ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z') {
            QString number;
            
            int start = i;
            bool inNumber = true;
            while (i < expr.length() && inNumber) {
                QChar current = expr[i];
                
                if (current.isDigit() || current.isLetter() ||
                    current == '_' || current == '\'') {
                    number += current;
                    i++;
                } else {
                    
                    if (i > start) {
                        
                        if (i < expr.length() - 1) {
                            QChar next = expr[i+1];
                            if (current == 'b' || current == 'B' ||
                                current == 'h' || current == 'H' ||
                                current == 'o' || current == 'O' ||
                                current == 'd' || current == 'D') {
                                
                                number += current;
                                i++;
                                continue;
                            }
                        }
                    }
                    inNumber = false;
                }
            }
            
            if (!number.isEmpty()) {
                int width = calculateConstantWidth(number);
                
                Token token(Token::CONSTANT, number, width);
                tokens.append(token);
            }
            continue;
        }
        
        bool foundOp = false;
        QStringList multiCharOps = {"**", "~&", "~|", "~^", "<<", ">>", "==", "!=", "<=", ">=", "&&", "||"};
        for (const QString &multiOp : multiCharOps) {
            if (i + multiOp.length() <= expr.length() &&
                expr.mid(i, multiOp.length()) == multiOp) {
                tokens.append(Token(Token::OPERATOR, multiOp));
                i += multiOp.length();
                foundOp = true;
                break;
            }
        }
        if (foundOp) {
            continue;
        }
        
        QString singleCharOp = QString(ch);
        if (operators.contains(singleCharOp) || singleCharOp == "{" || singleCharOp == "}" || singleCharOp == ",") {
            tokens.append(Token(Token::OPERATOR, singleCharOp));
            i++;
            continue;
        }
        
        if (ch == '(' || ch == ')') {
            tokens.append(Token(Token::BRACKET, QString(ch)));
            i++;
            continue;
        }
        i++; 
    }
    return tokens;
}
QVector<Token> LogicalOperationDialog::infixToPostfix(const QVector<Token> &tokens)
{
    QVector<Token> output;
    QStack<Token> operatorStack;
    for (const Token &token : tokens) {
        if (token.type == Token::SIGNAL || token.type == Token::CONSTANT || token.type == Token::CONCAT) {
            output.append(token);
        } else if (token.type == Token::OPERATOR) {
            while (!operatorStack.isEmpty() &&
                   operatorStack.top().type == Token::OPERATOR) {
                const OperatorInfo &op1 = operators[operatorStack.top().value];
                const OperatorInfo &op2 = operators[token.value];
                if ((!op1.rightAssociative && op1.precedence >= op2.precedence) ||
                    (op1.rightAssociative && op1.precedence > op2.precedence)) {
                    output.append(operatorStack.pop());
                } else {
                    break;
                }
            }
            operatorStack.push(token);
        } else if (token.type == Token::BRACKET) {
            if (token.value == "(") {
                operatorStack.push(token);
            } else { 
                while (!operatorStack.isEmpty() &&
                       !(operatorStack.top().type == Token::BRACKET &&
                         operatorStack.top().value == "(")) {
                    output.append(operatorStack.pop());
                }
                if (!operatorStack.isEmpty()) {
                    operatorStack.pop(); 
                }
            }
        }
    }
    while (!operatorStack.isEmpty()) {
        output.append(operatorStack.pop());
    }
    return output;
}
int LogicalOperationDialog::calculateConstantWidth(const QString &constant) const
{
    QString constVal = constant.trimmed();
    
    QRegularExpression explicitWidthRegex("^(\\d+)\\s*[']\\s*([bdhoBDHO])\\s*(.+)$");
    QRegularExpressionMatch match = explicitWidthRegex.match(constVal);
    if (match.hasMatch()) {
        
        QString widthStr = match.captured(1);
        bool ok;
        int width = widthStr.toInt(&ok);
        if (ok && width > 0) {
            return width;
        }
    }
    
    if (constVal.startsWith("'h") || constVal.startsWith("'H")) {
        QString hexStr = constVal.mid(2);
        hexStr.remove('_');
        return hexStr.length() * 4;
    }
    else if (constVal.startsWith("'b") || constVal.startsWith("'B")) {
        QString binStr = constVal.mid(2);
        binStr.remove('_');
        return binStr.length();
    }
    else if (constVal.startsWith("'o") || constVal.startsWith("'O")) {
        QString octStr = constVal.mid(2);
        octStr.remove('_');
        return octStr.length() * 3;
    }
    else if (constVal.startsWith("'d") || constVal.startsWith("'D")) {
        
        QString decStr = constVal.mid(2);
        decStr.remove('_');
        if (decStr.contains('x', Qt::CaseInsensitive) || decStr.contains('z', Qt::CaseInsensitive)) {
            
            return 1;
        }
        bool ok;
        qint64 value = decStr.toLongLong(&ok);
        if (!ok) return 0; 
        
        if (value == 0) return 1;
        int bits = 0;
        while (value > 0) {
            value >>= 1;
            bits++;
        }
        return bits;
    }
    else if (constVal.startsWith("'")) {
        
        return 1;
    }
    else if (constVal.startsWith("0x") || constVal.startsWith("0X")) {
        
        QString hexStr = constVal.mid(2);
        hexStr.remove('_');
        return hexStr.length() * 4;
    }
    else {
        
        QString numStr = constVal;
        numStr.remove('_');
        if (numStr.contains('x', Qt::CaseInsensitive) || numStr.contains('z', Qt::CaseInsensitive)) {
            return 1;
        }
        bool ok;
        qint64 value = numStr.toLongLong(&ok);
        if (!ok) return 0; 
        if (value == 0) return 1;
        int bits = 0;
        while (value > 0) {
            value >>= 1;
            bits++;
        }
        return bits;
    }
    return 0; 
}
QString LogicalOperationDialog::getSignalValueAtTime(const QString &signalName, Time time, bool *ok)
{
    if (ok) *ok = false;
    
    if (!signalCache.contains(signalName)) {
        SignalRef ref = findSignalInWaveform(signalName);
        if (ref == 0) {
            qWarning() << "Signal not found:" << signalName;
            return "0";
        }
        
        Signal* rawSignal = waveform->get_signal(ref);
        if (!rawSignal) {
            qWarning() << "Failed to get signal:" << signalName;
            return "0";
        }

        QSharedPointer<Signal> sharedSignal(rawSignal, [](Signal*){  });
        signalCache[signalName] = sharedSignal;
    }
    QSharedPointer<Signal> signal = signalCache[signalName];
    std::vector<TimeTableIdx> time_indices = signal->get_time_indices();
    std::vector<std::string> values = signal->get_signal_values();
    if (time_indices.empty() || values.empty()) {
        qWarning() << "Signal has no data:" << signalName;
        return "0";
    }
    
    if (cachedTimeTable.empty()) {
        cachedTimeTable = waveform->get_time_table();
    }
    if (cachedTimeTable.empty()) {
        qWarning() << "Time table is empty";
        return "0";
    }
    
    TimeTableIdx time_idx = 0;
    for (size_t i = 0; i < cachedTimeTable.size(); i++) {
        if (cachedTimeTable[i] <= time) {
            time_idx = static_cast<TimeTableIdx>(i);
        } else {
            break;
        }
    }
    
    size_t signal_idx = 0;
    std::string value = "0"; 
    for (size_t i = 0; i < time_indices.size(); i++) {
        if (time_indices[i] <= time_idx) {
            signal_idx = i;
            value = values[i];
        } else {
            break;
        }
    }
    if (ok) *ok = true;
    return QString::fromStdString(value);
}
QString LogicalOperationDialog::evaluateExpressionAtTime(const QString& expression, Time time)
{

    QVector<Token> tokens = tokenizeExpression(expression);
    QVector<Token> postfix = infixToPostfix(tokens);
    QStack<QString> valueStack;
    QStack<int> widthStack;
    for (const Token &token : postfix) {
        if (token.type == Token::SIGNAL) {
            bool ok;
            QString value = getSignalValueAtTime(token.value, time, &ok);
            if (ok) {
                valueStack.push(value);
                widthStack.push(token.width);
            } else {
                
                valueStack.push(QString(token.width, '0'));
                widthStack.push(token.width);
            }
        } else if (token.type == Token::CONSTANT) {
            QString binaryValue = convertConstantToBinary(token.value, token.width);
            
            valueStack.push(binaryValue);
            widthStack.push(token.width);
        } else if (token.type == Token::CONCAT) {
            
            QString inner = token.value.trimmed();
            QVector<QString> values;
            QVector<int> widths;
            if (inner.isEmpty()) {
                
                valueStack.push("");
                widthStack.push(0);
                continue;
            }
            
            int start = 0;
            int braceCount = 0;
            for (int j = 0; j < inner.length(); ++j) {
                QChar ch = inner[j];
                if (ch == '{') braceCount++;
                else if (ch == '}') braceCount--;
                else if (ch == ',' && braceCount == 0) {
                    QString subExpr = inner.mid(start, j - start).trimmed();
                    if (!subExpr.isEmpty()) {
                        QString value = evaluateExpressionAtTime(subExpr, time);
                        int width = calculateExpressionWidth(subExpr);
                        values.append(value);
                        widths.append(width);
                    }
                    start = j + 1;
                }
            }
            
            QString lastSubExpr = inner.mid(start).trimmed();
            if (!lastSubExpr.isEmpty()) {
                QString value = evaluateExpressionAtTime(lastSubExpr, time);
                int width = calculateExpressionWidth(lastSubExpr);
                values.append(value);
                widths.append(width);
            }
            
            QString result = applyConcatOperation(values, widths);
            int totalWidth = 0;
            for (int w : widths) totalWidth += w;
            valueStack.push(result);
            widthStack.push(totalWidth);
        } else if (token.type == Token::OPERATOR) {
            
            if (operators[token.value].isUnary) {
                if (valueStack.isEmpty()) {
                    return "0";
                }
                QString operand = valueStack.pop();
                int opWidth = widthStack.pop();
                QString result = applyUnaryOperator(token.value, operand, opWidth);
                int resultWidth = inferUnaryOpWidth(token.value, opWidth);
                valueStack.push(result);
                widthStack.push(resultWidth);
            } else {
                
                if (valueStack.size() < 2) {
                    return "0";
                }
                QString right = valueStack.pop();
                int rightWidth = widthStack.pop();
                QString left = valueStack.pop();
                int leftWidth = widthStack.pop();
                
                int maxWidth = qMax(leftWidth, rightWidth);
                left = extendWidth(left, leftWidth, maxWidth, false);
                right = extendWidth(right, rightWidth, maxWidth, false);
                QString result = applyBinaryOperator(token.value, left, right, maxWidth);
                int resultWidth = inferBinaryOpWidth(token.value, leftWidth, rightWidth);
                valueStack.push(result);
                widthStack.push(resultWidth);
            }
        }
    }
    if (!valueStack.isEmpty()) {
        return valueStack.top();
    }
    return "0";
}
QString LogicalOperationDialog::applyConcatOperation(const QVector<QString>& operands, const QVector<int>& widths)
{
    if (operands.isEmpty()) return "";
    QString result;
    
    for (int i = operands.size() - 1; i >= 0; i--) {
        
        QString operand = adjustToWidth(operands[i], widths[i]);
        
        result = operand + result;
    }
    return result;
}
QString LogicalOperationDialog::convertConstantToBinary(const QString &constant, int width)
{
    QString constVal = constant.trimmed();
    
    int explicitWidth = 0;
    QString base, valueStr;
    if (parseExplicitWidthConstant(constVal, explicitWidth, base, valueStr)) {
        
        return convertExplicitWidthConstant(constVal, explicitWidth);
    }
    
    return convertSimpleConstantToBinary(constVal, width);
}
QString LogicalOperationDialog::applyUnaryOperator(const QString &op, const QString &operand, int width)
{
    if (op == "~") {
        
        QString result;
        for (int i = 0; i < width; i++) {
            QChar bit = (i < operand.length()) ? operand[operand.length() - 1 - i] : QChar('0');
            if (bit == '0') result = '1' + result;
            else if (bit == '1') result = '0' + result;
            else if (bit == 'x' || bit == 'X') result = 'x' + result;
            else if (bit == 'z' || bit == 'Z') result = 'x' + result;
            else result = 'x' + result;
        }
        return result;
    }
    else if (op == "!") {
        
        if (containsXorZ(operand)) {
            return "x";
        }
        bool isZero = true;
        for (int i = 0; i < operand.length(); i++) {
            if (operand[i] == '1') {
                isZero = false;
                break;
            }
        }
        return isZero ? "1" : "0";
    }
    return operand;
}
QString LogicalOperationDialog::applyBinaryOperator(const QString &op, const QString &left,
                                                    const QString &right, int width)
{
        
    bool hasXorZ = containsXorZ(left) || containsXorZ(right);

    if (op == "&" || op == "|" || op == "^" || op == "~&" || op == "~|" || op == "~^") {
        return bitwiseOperation(op, left, right, width);
    }
    else if (op == "**") {
        
        if (hasXorZ) {
            return QString(width, 'x');
        }
        
        qint64 leftVal = binToDec(left);
        qint64 rightVal = binToDec(right);
        
        if (leftVal == 0 && rightVal == 0) {
            return decToBin(1, width);
        }
        if (rightVal < 0) {
            return QString(width, '0');
        }
        
        qint64 result = 1;
        qint64 maxVal = (1LL << (qMin(width, 63) - 1)) - 1;
        for (qint64 i = 0; i < rightVal; i++) {
            if (result > maxVal / leftVal) {
                result = maxVal;
                break;
            }
            result *= leftVal;
        }
        return decToBin(result, width);
    }
    else if (op == "&&") {
        
        if (hasXorZ) {
            return "x";
        }
        bool leftNonZero = (binToDec(left) != 0);
        bool rightNonZero = (binToDec(right) != 0);
        return (leftNonZero && rightNonZero) ? "1" : "0";
    }
    else if (op == "||") {
        
        if (hasXorZ) {
            return "x";
        }
        bool leftNonZero = (binToDec(left) != 0);
        bool rightNonZero = (binToDec(right) != 0);
        return (leftNonZero || rightNonZero) ? "1" : "0";
    }
    else if (op == "==") {
        
        if (hasXorZ) {
            return "x";
        }
        return (left == right) ? "1" : "0";
    }
    else if (op == "!=") {
        
        if (hasXorZ) {
            return "x";
        }
        return (left != right) ? "1" : "0";
    }
    else if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        
        if (hasXorZ) {
            return "x";
        }
        qint64 leftVal = binToDec(left);
        qint64 rightVal = binToDec(right);
        
        if (op == "<") return (leftVal < rightVal) ? "1" : "0";
        if (op == ">") return (leftVal > rightVal) ? "1" : "0";
        if (op == "<=") return (leftVal <= rightVal) ? "1" : "0";
        if (op == ">=") return (leftVal >= rightVal) ? "1" : "0";
    }
    else if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "<<" || op == ">>") {
        
        if (hasXorZ) {
            return QString(width, 'x');
        }
        
        qint64 leftVal = binToDec(left);
        qint64 rightVal = binToDec(right);
        
        if (op == "+") {
            return addBinary(left, right);
        } else if (op == "-") {
            return subBinary(left, right);
        } else if (op == "*") {
            return mulBinary(left, right);
        } else if (op == "/") {
            if (rightVal == 0) return QString(width, 'x'); 
            return decToBin(leftVal / rightVal, width);
        } else if (op == "%") {
            if (rightVal == 0) return QString(width, 'x'); 
            return decToBin(leftVal % rightVal, width);
        } else if (op == "<<") {
            qint64 shiftAmount = binToDec(right);
            return shiftBinary(left, shiftAmount, true, false);
        } else if (op == ">>") {
            qint64 shiftAmount = binToDec(right);
            return shiftBinary(left, shiftAmount, false, false);
        }
    }
    return QString(width, 'x');
}
QString LogicalOperationDialog::bitwiseOperation(const QString& op, const QString& left,
                                                 const QString& right, int resultWidth) const
{
   
    QString leftPadded = adjustToWidth(left, resultWidth);
    QString rightPadded = adjustToWidth(right, resultWidth);
    
    QString result;
    
    for (int i = 0; i < resultWidth; ++i) {
        QChar l = leftPadded[i];
        QChar r = rightPadded[i];
        
        if (l == '0' || l == '1') {
            if (r == '0' || r == '1') {
                
                if (op == "&") {
                    result.append((l == '1' && r == '1') ? '1' : '0');
                }
                else if (op == "|") {
                    result.append((l == '1' || r == '1') ? '1' : '0');
                }
                else if (op == "^") {
                    result.append((l != r) ? '1' : '0');
                }
                else if (op == "~&") {
                    result.append((l == '1' && r == '1') ? '0' : '1');
                }
                else if (op == "~|") {
                    result.append((l == '1' || r == '1') ? '0' : '1');
                }
                else if (op == "~^") {
                    result.append((l == r) ? '1' : '0');
                }
                else {
                    result.append('x');
                }
            } else {
                
                result.append(applyFourValueOperation(op, l, r));
            }
        } else {
            
            result.append(applyFourValueOperation(op, l, r));
        }
    }
    
    return result;
}
void LogicalOperationDialog::onClearClicked()
{
    expressionEdit->clear();
}
void LogicalOperationDialog::onAddSignalClicked()
{
    QString currentText = expressionEdit->toPlainText();
    currentText += QString(" & \"%1\"").arg(initialSignal);
    expressionEdit->setText(currentText);
}
void LogicalOperationDialog::onCreateModifyClicked()
{
    QString expression = expressionEdit->toPlainText().trimmed();
    QString name = nameComboBox->currentText().trimmed();
    if (expression.isEmpty() || name.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Expression and name cannot be empty!");
        return;
    }
    try {

        int width = calculateExpressionWidth(expression);
        if (width <= 0) {
            QMessageBox::warning(this, "Error", "Failed to calculate expression width!");
            return;
        }
        
        LogicalExpression* existingExpr = findExpression(name);

        if (existingExpr){
            name = QString("expr_%1").arg(GlobalCounter::getNextExpressionId());
        }
        LogicalExpression expr;
        expr.name = name;
        expr.expression = expression;
        expr.width = width;
        expr.msb = width - 1;
        expr.lsb = 0;
        expr.resultBusFormat = width > 1 ? QString("[%1:0]").arg(width - 1) : "";
        expr.type = VarType::Wire;
        expr.signalType.width = width;
        logicalExpressions.append(expr);
        
        nameComboBox->addItem(name+expr.resultBusFormat);
        logicalNames.append(name+expr.resultBusFormat);
        onAddToWaveByExpr(expr);
        updateComboBoxName();

        updateSignalList();
        expressionEdit->clear();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Failed to create expression: %1").arg(e.what()));
    }
}
void LogicalOperationDialog::onEvaluateClicked()
{
    QString expression = expressionEdit->toPlainText().trimmed();
    if (expression.isEmpty()) {
        QMessageBox::information(this, "Information", "Please enter an expression to evaluate.");
        return;
    }
    try {
        int width = calculateExpressionWidth(expression);
        QString result = evaluateExpressionAtTime(expression, 0); 
        QString message = QString("Expression: %1\nWidth: %2 bits\nResult: %3")
                              .arg(expression).arg(width).arg(result);
        QMessageBox::information(this, "Evaluation Result", message);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Evaluation failed: %1").arg(e.what()));
    }
}
void LogicalOperationDialog::onAddToWaveByExpr(LogicalExpression &expr)
{
    QString name = expr.name;
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select an expression to add!");
        return;
    }
    if (!waveform) {
        QMessageBox::warning(this, "Warning", "No waveform data available!");
        return;
    }
    SignalType signal_type(false,false,expr.width);
    VarRef var_ref = waveform->get_hierarchy().add_var_to_root(expr.name.toStdString(), VarType::Wire, signal_type,0,expr.resultBusFormat.toStdString());
    Var var = waveform->get_hierarchy().get_var(var_ref);
    
    QSharedPointer<Signal> signal = createLogicalSignal(expr);
    if (!signal) {
        QMessageBox::critical(this, "Error", "Failed to create logical signal!");
        return;
    }
    
    waveform->add_signal(var.handle, signal);
    QSet<VarRef> varSet;
    varSet.insert(var_ref);
    emit addToWaveform(varSet);
}
void LogicalOperationDialog::onAddToWaveClicked()
{
    QString name = nameComboBox->currentText().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select an expression to add!");
        return;
    }
    LogicalExpression* expr = findExpression(name);
    if (!expr) {
        qDebug() << "name is " << name;
        QMessageBox::warning(this, "Warning", "Expression not found!");
        return;
    }
    if (!waveform) {
        QMessageBox::warning(this, "Warning", "No waveform data available!");
        return;
    }
    SignalType signal_type(false,false,expr->width);
    VarRef var_ref = waveform->get_hierarchy().add_var_to_root(expr->name.toStdString(), VarType::Wire, signal_type,0,expr->resultBusFormat.toStdString());
    Var var = waveform->get_hierarchy().get_var(var_ref);
    
    QSharedPointer<Signal> signal = createLogicalSignal(*expr);
    if (!signal) {
        QMessageBox::critical(this, "Error", "Failed to create logical signal!");
        return;
    }
    
    waveform->add_signal(var.handle, signal);
    QSet<VarRef> varSet;
    varSet.insert(var_ref);
    emit addToWaveform(varSet);
}
void LogicalOperationDialog::onDeleteClicked()
{
    QListWidgetItem* currentItem = signalList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "Warning", "Please select a signal to delete!");
        return;
    }
    QString signalName = currentItem->text();
    
    for (int i = 0; i < logicalExpressions.size(); ++i) {
        QString displayName = QString("%1%2 = %3").arg(logicalExpressions[i].name).arg(logicalExpressions[i].resultBusFormat).arg(logicalExpressions[i].expression);
        if (displayName == signalName) {
            logicalExpressions.removeAt(i);
            break;
        }
    }
    delete currentItem;
    updateSignalList();
}
void LogicalOperationDialog::onDeleteAllClicked()
{
    int ret = QMessageBox::question(this, "Confirm Delete",
                                    "Are you sure you want to delete all expressions?");
    if (ret == QMessageBox::Yes) {
        logicalExpressions.clear();
        updateSignalList();
    }
}
void LogicalOperationDialog::onCloseClicked()
{
    accept();
}
void LogicalOperationDialog::onOperatorClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString op = btn->property("originalText").toString();
    QString currentText = expressionEdit->toPlainText();
    if (!currentText.isEmpty() && !currentText.endsWith(' ') && !currentText.endsWith('(') && !currentText.endsWith("'h" ) && !currentText.endsWith("'d") && !currentText.endsWith("'b")) {
        currentText += " ";
    }
    currentText += op;
    if(!(op=="'h" || op=="'d" || op=="'b")){
        currentText += " ";
    }
    expressionEdit->setText(currentText);
}
QString LogicalOperationDialog::getCurrentExpression() const
{
    return expressionEdit->toPlainText();
}
QString LogicalOperationDialog::getCurrentResult()
{
    try {
        QString expr = expressionEdit->toPlainText();
        int width = calculateExpressionWidth(expr);
        QString value = evaluateExpressionAtTime(expr, 0);
        return QString("Width: %1 bits, Value: %2").arg(width).arg(value);
    } catch (...) {
        return "Evaluation Error";
    }
}
LogicalExpression* LogicalOperationDialog::findExpression(const QString& name)
{
    for (int i = 0; i < logicalExpressions.size(); ++i) {
        QString exprName=logicalExpressions[i].name +logicalExpressions[i].resultBusFormat;
        if (exprName == name) {
            return &logicalExpressions[i];
        }
    }
    return nullptr;
}
LogicalExpression* LogicalOperationDialog::findExpressionBySignalRef(SignalRef ref)
{
    for (int i = 0; i < logicalExpressions.size(); ++i) {
        if (logicalExpressions[i].signalRef == ref) {
            return &logicalExpressions[i];
        }
    }
    return nullptr;
}
QPair<int, int> LogicalOperationDialog::extractBusRange(const QString& signalName) const
{
    QPair<int, int> result(-1, -1);
    int startBracket = signalName.indexOf('[');
    int endBracket = signalName.indexOf(']', startBracket);
    if (startBracket >= 0 && endBracket > startBracket) {
        QString inside = signalName.mid(startBracket + 1, endBracket - startBracket - 1).trimmed();
        
        int colonIndex = inside.indexOf(':');
        if (colonIndex > 0) {
            QString leftStr = inside.left(colonIndex).trimmed();
            QString rightStr = inside.mid(colonIndex + 1).trimmed();
            bool ok1, ok2;
            int leftVal = leftStr.toInt(&ok1);
            int rightVal = rightStr.toInt(&ok2);
            if (ok1 && ok2) {
                
                result.first = qMax(leftVal, rightVal);
                result.second = qMin(leftVal, rightVal);
            }
        }
    }
    return result;
}
int LogicalOperationDialog::extractBitSelect(const QString& signalName) const
{
    int startBracket = signalName.indexOf('[');
    int endBracket = signalName.indexOf(']', startBracket);
    if (startBracket >= 0 && endBracket > startBracket) {
        QString inside = signalName.mid(startBracket + 1, endBracket - startBracket - 1).trimmed();
        
        bool ok;
        int bit = inside.toInt(&ok);
        if (ok) {
            return bit;
        }
    }
    return -1; 
}
SignalInfo LogicalOperationDialog::getSignalInfo(const QString& signalName) const
{
    SignalInfo info;
    info.name = signalName;
    info.fullName = signalName;
    if (signalName.contains('[') && signalName.contains(']')) {
        int bitSelect = extractBitSelect(signalName);
        if (bitSelect >= 0) {
            
            info.msb = bitSelect;
            info.lsb = bitSelect;
            info.width = 1;
            info.isBus = false;
            return info;
        } else {
            QPair<int, int> busRange = extractBusRange(signalName);
            if (busRange.first >= 0 && busRange.second >= 0) {
                int msb = busRange.first;
                int lsb = busRange.second;
                if (msb >= lsb) {
                    info.msb = msb;
                    info.lsb = lsb;
                    info.width = msb - lsb + 1;  
                    info.isBus = info.width > 1;
                    return info;
                }
            }
        }
    }
    
    info.width = 1;
    info.msb = 0;
    info.lsb = 0;
    info.isBus = false;
    return info;
    return info;
}
QSharedPointer<Signal> LogicalOperationDialog::createLogicalSignal(const LogicalExpression& expr)
{
    if (!waveform) return nullptr;
    auto signal = QSharedPointer<Signal>::create();
    
    std::vector<Time> time_table = waveform->get_time_table();
    if (time_table.empty()) {
        qWarning() << "Time table is empty";
        return signal;
    }
    std::string prev_value;
    
    for (size_t i = 0; i < time_table.size(); i++) {
        Time currentTime = time_table[i];
        
        QString value = evaluateExpressionAtTime(expr.expression, currentTime);
        
        const std::string& current_value = value.toStdString();
        if (i == 0 || current_value != prev_value) {
            signal->add_value_change(i, current_value);
            prev_value = current_value;
        }
    }
    return signal;
}
QString LogicalOperationDialog::decToBin(qint64 value, int width) const
{
    if (width <= 0) return "";

    if (value < 0) {
        unsigned long long unsignedValue;
        if (width >= 64) {
            
            unsignedValue = (1ULL << (width - 1)) + value;
        } else {
            
            unsignedValue = (1ULL << width) + value;
        }
        
        QString result;
        for (int i = 0; i < width; ++i) {
            result = ((unsignedValue & (1ULL << i)) ? '1' : '0') + result;
        }
        return result;
    }

    QString result;
    for (int i = 0; i < width; ++i) {
        result = ((value & (1LL << i)) ? '1' : '0') + result;
    }
    return result;
}
QString LogicalOperationDialog::hexToBin(const QString& hexStr, int width) const
{
    bool ok;
    qint64 value = hexStr.toLongLong(&ok, 16);
    if (!ok) return QString(width, '0');
    return decToBin(value, width);
}
QString LogicalOperationDialog::binToBin(const QString& binStr, int width) const
{
    if (binStr.length() >= width) return binStr.right(width);
    return QString(width - binStr.length(), '0') + binStr;
}
qint64 LogicalOperationDialog::binToDec(const QString& binStr) const
{
    qint64 result = 0;
    for (int i = 0; i < binStr.length(); ++i) {
        if (binStr[i] == '1') {
            result |= (1LL << (binStr.length() - 1 - i));
        }
    }
    return result;
}
QString LogicalOperationDialog::addBinary(const QString& a, const QString& b) const
{
    qint64 valA = binToDec(a);
    qint64 valB = binToDec(b);
    int width = qMax(a.length(), b.length()) + 1;
    return decToBin(valA + valB, width);
}
QString LogicalOperationDialog::subBinary(const QString& a, const QString& b) const
{
    qint64 valA = binToDec(a);
    qint64 valB = binToDec(b);
    int width = qMax(a.length(), b.length()) + 1;
    return decToBin(valA - valB, width);
}
QString LogicalOperationDialog::mulBinary(const QString& a, const QString& b) const
{
    qint64 valA = binToDec(a);
    qint64 valB = binToDec(b);
    int width = a.length() + b.length();
    return decToBin(valA * valB, width);
}
QString LogicalOperationDialog::shiftBinary(const QString& value, int shiftAmount,
                                            bool left, bool arithmetic) const
{
    qint64 val = binToDec(value);
    if (left) {
        val <<= shiftAmount;
    } else {
        if (arithmetic) {
            
            bool isNegative = !value.isEmpty() && value[0] == '1';
            val >>= shiftAmount;
            if (isNegative) {
                
                qint64 signExtend = ((1LL << shiftAmount) - 1) << (value.length() - shiftAmount);
                val |= signExtend;
            }
        } else {
            val >>= shiftAmount;
        }
    }
    return decToBin(val, value.length());
}
QString LogicalOperationDialog::extendWidth(const QString& value, int currentWidth,
                                            int targetWidth, bool isSigned) const
{
    if (currentWidth >= targetWidth) return value.left(targetWidth);
    QString result = value;
    if (isSigned && !value.isEmpty() && value[0] == '1') {
        
        return QString(targetWidth - currentWidth, '1') + result;
    } else {
        
        return QString(targetWidth - currentWidth, '0') + result;
    }
}
bool LogicalOperationDialog::validateExpression(const QString& expression, QString& error) const
{
    if (expression.isEmpty()) {
        error = "Expression is empty";
        return false;
    }
    
    int parenCount = 0;
    for (int i = 0; i < expression.length(); i++) {
        if (expression[i] == '(') parenCount++;
        else if (expression[i] == ')') parenCount--;
        if (parenCount < 0) {
            error = "Mismatched parentheses";
            return false;
        }
    }
    if (parenCount != 0) {
        error = "Mismatched parentheses";
        return false;
    }
    return true;
}
QVector<QString> LogicalOperationDialog::extractSignalNames(const QString& expression) const
{
    QVector<QString> result;
    QRegularExpression signalPattern("\"([^\"]+)\"");
    QRegularExpressionMatchIterator it = signalPattern.globalMatch(expression);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        result.append(match.captured(1));
    }
    return result;
}
int LogicalOperationDialog::getSignalBitWidth(const QString& signalName) const
{
    SignalInfo info = getSignalInfo(signalName);
    return info.width;
}
SignalRef LogicalOperationDialog::findSignalInWaveform(const QString& signalPath)
{
    if (!waveform) return 0;
    VarRef var_ref = waveform->get_hierarchy().get_varRef_by_hierarchy(signalPath.toStdString());
    if(var_ref > 0) {
        const Var& var = waveform->get_hierarchy().get_var(var_ref);
        QString var_name = QString::fromStdString(waveform->get_hierarchy().get_string(var.name_id)) + QString::fromStdString(var.multi_array);
        SignalRef signal_ref = var.handle;
        emit loadSignalData(signal_ref);
        return signal_ref;
    }

    for (auto it = signalCache.begin(); it != signalCache.end(); ++it) {
        if (it.key() == signalPath) {
            return 1; 
        }
    }
    
    return 0;
}
void LogicalOperationDialog::onSignalListItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    QString itemText = item->text();
    QString expressionName = itemText.section(" = ", 0, 0);;
    QString expressionText = itemText.section(" = ", 1);;

    int index = nameComboBox->findText(expressionName);
    if (index >= 0) {
        nameComboBox->setCurrentIndex(index);
    } else {
        
        nameComboBox->setCurrentText(expressionName);
    }
    
    expressionEdit->setText(expressionText);
    
    QTextCursor cursor = expressionEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    expressionEdit->setTextCursor(cursor);
    
    expressionEdit->setFocus();
}
bool LogicalOperationDialog::parseExplicitWidthConstant(const QString &constant, int &width, QString &base, QString &valueStr) const
{

    QRegularExpression regex("^(\\d+)\\s*[']\\s*([bdhoBDHO])\\s*(.+)$");
    QRegularExpressionMatch match = regex.match(constant.trimmed());
    
    if (match.hasMatch()) {
        width = match.captured(1).toInt();
        base = match.captured(2).toLower();
        valueStr = match.captured(3);
        valueStr.remove('_'); 
        return true;
    }
    
    return false;
}
QString LogicalOperationDialog::convertExplicitWidthConstant(const QString &constant, int targetWidth)
{
    int explicitWidth = 0;
    QString base, valueStr;
    if (!parseExplicitWidthConstant(constant, explicitWidth, base, valueStr)) {
        return QString(targetWidth, '0');
    }
    
    valueStr.remove('_');
    QString binaryStr;
    
    if (base == "h") {
        
        for (int i = 0; i < valueStr.length(); i++) {
            QChar ch = valueStr[i];
            if (ch == 'x' || ch == 'X') {
                binaryStr += "xxxx";
            } else if (ch == 'z' || ch == 'Z') {
                binaryStr += "zzzz";
            } else {
                bool ok;
                int digit = QString(ch).toInt(&ok, 16);
                if (ok) {
                    binaryStr += QString("%1").arg(digit, 4, 2, QChar('0'));
                } else {
                    binaryStr += "xxxx";
                }
            }
        }
    }
    else if (base == "b") {
        
        binaryStr = valueStr;
        
        binaryStr = binaryStr.toLower();
    }
    else if (base == "o") {
        
        for (int i = 0; i < valueStr.length(); i++) {
            QChar ch = valueStr[i];
            if (ch == 'x' || ch == 'X') {
                binaryStr += "xxx";
            } else if (ch == 'z' || ch == 'Z') {
                binaryStr += "zzz";
            } else {
                bool ok;
                int digit = QString(ch).toInt(&ok, 8);
                if (ok) {
                    binaryStr += QString("%1").arg(digit, 3, 2, QChar('0'));
                } else {
                    binaryStr += "xxx";
                }
            }
        }
    }
    else if (base == "d") {

        if (valueStr.contains('x', Qt::CaseInsensitive) || valueStr.contains('z', Qt::CaseInsensitive)) {
            
            binaryStr = QString(explicitWidth, 'x');
        } else {
            bool ok;
            
            qint64 value = valueStr.toLongLong(&ok);
            if (!ok) {
                binaryStr = QString(explicitWidth, 'x');
            } else {
                binaryStr = decToBin(value, explicitWidth);
            }
        }
    }
    
    return adjustToWidth(binaryStr, explicitWidth);
}
QString LogicalOperationDialog::convertSimpleConstantToBinary(const QString &constant, int width)
{
    QString constVal = constant.trimmed();
    
    if (constVal.startsWith("'h", Qt::CaseInsensitive) || constVal.startsWith("'H")) {
        QString hexStr = constVal.mid(2);
        hexStr.remove('_');
        
        if (hexStr.contains('x', Qt::CaseInsensitive) || hexStr.contains('z', Qt::CaseInsensitive)) {
            
            return QString(width, 'x');
        }
        return hexToBin(hexStr, width);
    } 
    else if (constVal.startsWith("'b", Qt::CaseInsensitive) || constVal.startsWith("'B")) {
        QString binStr = constVal.mid(2);
        binStr.remove('_');
        binStr = binStr.toLower(); 
        return binToBin(binStr, width);
    }
    else if (constVal.startsWith("'o", Qt::CaseInsensitive) || constVal.startsWith("'O")) {
        QString octStr = constVal.mid(2);
        octStr.remove('_');
        
        if (octStr.contains('x', Qt::CaseInsensitive) || octStr.contains('z', Qt::CaseInsensitive)) {
            return QString(width, 'x');
        }
        bool ok;
        qint64 value = octStr.toLongLong(&ok, 8);
        if (!ok) value = 0;
        return decToBin(value, width);
    }
    else if (constVal.startsWith("'d", Qt::CaseInsensitive) || constVal.startsWith("'D")) {
        QString decStr = constVal.mid(2);
        decStr.remove('_');
        
        if (decStr.contains('x', Qt::CaseInsensitive) || decStr.contains('z', Qt::CaseInsensitive)) {
            return QString(width, 'x');
        }
        bool ok;
        qint64 value = decStr.toLongLong(&ok);
        if (!ok) value = 0;
        return decToBin(value, width);
    }
    else if (constVal.startsWith("'")) {
        
        QString value = constVal.mid(1);
        if (value == "1") return "1";
        else if (value == "0") return "0";
        else if (value.toLower() == "x") return "x";
        else if (value.toLower() == "z") return "z";
        else return "0";
    }
    else {
        
        QString decStr = constVal;
        decStr.remove('_');
        
        if (decStr.contains('x', Qt::CaseInsensitive) || decStr.contains('z', Qt::CaseInsensitive)) {
            return QString(width, 'x');
        }
        bool ok;
        qint64 value = decStr.toLongLong(&ok);
        if (!ok) value = 0;
        return decToBin(value, width);
    }
}
QString LogicalOperationDialog::adjustToWidth(const QString &binaryStr, int targetWidth, QChar fillChar) const
{
    int currentWidth = binaryStr.length();
    
    if (currentWidth == targetWidth) {
        return binaryStr;
    }
    else if (currentWidth > targetWidth) {
        
        return binaryStr.right(targetWidth);
    }
    else {
        
        int diff = targetWidth - currentWidth;

        QChar fill = fillChar;
        if (binaryStr.length() > 0) {
            QChar msb = binaryStr[0];
            if (msb == 'x' || msb == 'X') {
                fill = 'x';
            } else if (msb == 'z' || msb == 'Z') {
                fill = 'z';
            }
        }
        
        return QString(diff, fill) + binaryStr;
    }
}
QString LogicalOperationDialog::applyFourValueOperation(const QString &op, QChar left, QChar right) const
{
    
    left = left.toLower();
    right = right.toLower();

    if (op == "&") { 
        if (left == '0' || right == '0') return "0";
        if (left == '1' && right == '1') return "1";
        if (left == 'z' || right == 'z') return "x";
        return "x";
    }
    else if (op == "|") { 
        if (left == '1' || right == '1') return "1";
        if (left == '0' && right == '0') return "0";
        if (left == 'z' || right == 'z') return "x";
        return "x";
    }
    else if (op == "^") { 
        if (left == 'x' || right == 'x' || left == 'z' || right == 'z') return "x";
        if (left == right) return "0";
        return "1";
    }
    else if (op == "~&") { 
        QString andResult = applyFourValueOperation("&", left, right);
        if (andResult == "0") return "1";
        if (andResult == "1") return "0";
        return "x";
    }
    else if (op == "~|") { 
        QString orResult = applyFourValueOperation("|", left, right);
        if (orResult == "0") return "1";
        if (orResult == "1") return "0";
        return "x";
    }
    else if (op == "~^") { 
        QString xorResult = applyFourValueOperation("^", left, right);
        if (xorResult == "0") return "1";
        if (xorResult == "1") return "0";
        return "x";
    }
    
    return "x";
}
bool LogicalOperationDialog::containsXorZ(const QString &str) {
    for (QChar ch : str) {
        if (ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z') {
            return true;
        }
    }
    return false;
}
