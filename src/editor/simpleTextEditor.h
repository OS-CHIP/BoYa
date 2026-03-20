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

#ifndef SIMPLETEXTEDITOR_H
#define SIMPLETEXTEDITOR_H
#include <QMainWindow>
#include <QStack>
#include <QString>
#include <QMap>
#include <QPlainTextEdit>
namespace Ui {
class SimpleTextEditor;
}
class Highlighter; 
class SimpleTextEditor : public QMainWindow
{
    Q_OBJECT
public:
    explicit SimpleTextEditor(QWidget *parent = nullptr);
    ~SimpleTextEditor();
    bool loadFile(const QString &fileName);
    void goToPosition(int line, int column);
    void openFileAtLineAndColumn(const QString &fileName,int line, int column);
    void mergeformat(const QTextCharFormat &fmt);
    void handleThemeChange(const QString &themeName);
public slots:
    void highlightCurrentLine();
private:
    Ui::SimpleTextEditor *ui; 
    Highlighter *m_highlighter;
    QMap<QString, QString> m_languageFiles;
    bool isUntitled;
    QString curFile;
    QColor m_highLightLineColor = QColor(Qt::yellow); 
    void loadLanguageDefinitions(const QString &theme = "light");
    QString getLanguageFileForExtension(const QString &extension);
};
#endif 