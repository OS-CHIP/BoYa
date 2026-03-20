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

#ifndef TRANSLATORS_H
#define TRANSLATORS_H
#include <QString>
#include <QMap>
#include <QList>
#include <QSharedPointer>
#include <cmath>
#include <QDebug>
#include "waveform.h"
enum class ValueKind {
    Normal,
    Undef,
    HighImp,
    Custom,
    Warn,
    DontCare,
    Weak
};
struct TranslatedValue {
    QString value;
    ValueKind kind;
};
class Translator {
public:
    virtual ~Translator() = default;
    virtual QString name() const = 0;
    virtual QString prefixName() const = 0;
    virtual TranslatedValue translate(const VariableMeta& meta, const QString& value) const = 0;
    virtual int preference(const VariableMeta& meta) const = 0; 
};
class TranslatorManager {
public:
    TranslatorManager();
    ~TranslatorManager();
    void registerTranslator(QSharedPointer<Translator> translator);
    QSharedPointer<Translator> findTranslator(const VariableMeta& meta) const;
    void setDefaultTranslator(QSharedPointer<Translator> translator);
    const QList<QSharedPointer<Translator>>& getAllTranslators() const { return translators; }
private:
    void initializeBuiltinTranslators(); 
    QList<QSharedPointer<Translator>> translators;
    QSharedPointer<Translator> defaultTranslator;
};
class HexTranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
class OctalTranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
class BinaryTranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
class GroupingBinaryTranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
class FSMStatusTranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
class StringTranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
class SignedTranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
class UnsignedTranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
class FloatTranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
class ASCIITranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
class DecimalTranslator : public Translator {
public:
    QString name() const override;
    QString prefixName() const override;
    TranslatedValue translate(const VariableMeta& meta, const QString& value) const override;
    int preference(const VariableMeta& meta) const override;
};
TranslatedValue handleSpecialValue(const QString& value);
QString getGroupSpecialChar(const QString& group);
ValueKind determineOverallKind(const QString& binStr);
#endif 