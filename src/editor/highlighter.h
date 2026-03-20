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

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <iostream>
#include "textMaker.h"
class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit Highlighter(QTextDocument *parent = nullptr);
    ~Highlighter() override;
    bool loadLanguageDefinition(const QString &languageFilePath);
    void setLanguageDefinition(const QJsonObject &definition);
    void rehighlight();
    void setDocument(QTextDocument *doc);
    void clearHighlightRules();
    void setMemberHighlights(const QList<TextMarker> &markers);
    void addWordHighlight(const TextMarker &marker,bool flag=true);
    void cancelWordHighlight(const TextMarker &marker);
    void clearWordHighlights();
protected:
    void highlightBlock(const QString &text) override;
private:
    struct MemberHighlight {
        int line;       
        int column;     
        int length;     
        QTextCharFormat format;
    };
    QVector<MemberHighlight> memberHighlights;
    QSet<int> highlightedLines;
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    void setupRules() ;
    QTextCharFormat createFormat(const QJsonObject &formatConfig);
    bool isValid;
    QVector<HighlightingRule> highlightingRules;
    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;
    QJsonObject languageDefinition;
    QMap<QString, QTextCharFormat> formatMap;
    
    QList<MemberHighlight> wordHighlights;
    QSet<int> wordHighlightedLines;
    QColor signalColor;
    QColor signalBackgroundColor;
};
#endif 