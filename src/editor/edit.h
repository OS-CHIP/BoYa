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

#ifndef EDIT_H
#define EDIT_H
#include <QPlainTextEdit>
#include <QWidget>
class LineNumberArea;
class Edit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit Edit(QWidget *parent = nullptr);
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void setDefaultFont();
    QString theme = "";
protected:
    void contextMenuEvent(QContextMenuEvent* e) override;
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
private:
    LineNumberArea *lineNumberArea;
    QColor m_lineNumberAreaBackgroundColor = QColor(240, 240, 240);
    QColor m_lineNumberAreaIndexColor = QColor(Qt::darkGray);
    QColor m_lineNumberAreaLineColor = QColor(200, 200, 200);
};
class LineNumberArea : public QWidget
{
public:
    LineNumberArea(Edit *editor) : QWidget(editor), editor(editor) {}
    QSize sizeHint() const override {
        return QSize(editor->lineNumberAreaWidth(), 0);
    }
protected:
    void paintEvent(QPaintEvent *event) override {
        editor->lineNumberAreaPaintEvent(event);
    }
private:
    Edit *editor;
};
#endif 