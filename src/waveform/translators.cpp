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

#include "translators.h"
#include <QStringList>
#include <cmath>
#include <cstring>
#include "globalState.h"
QStringList groupRightToLeft(const QString& value, int groupSize) {
    QStringList groups;
    int totalBits = value.length();
    int pos = totalBits; 
    while (pos > 0) {
        int start = qMax(0, pos - groupSize);
        int length = pos - start;
        groups.prepend(value.mid(start, length)); 
        pos -= length;
    }
    return groups;
}
TranslatorManager::TranslatorManager() {
    initializeBuiltinTranslators();
    
    for (auto& t : translators) {
        if (t->name() == "Hexadecimal") {
            setDefaultTranslator(t);
            break;
        }
    }
}
TranslatorManager::~TranslatorManager() {
    
    translators.clear();
}
void TranslatorManager::initializeBuiltinTranslators() {
    
    registerTranslator(QSharedPointer<Translator>(new HexTranslator()));
    registerTranslator(QSharedPointer<Translator>(new OctalTranslator()));
    registerTranslator(QSharedPointer<Translator>(new BinaryTranslator()));
    registerTranslator(QSharedPointer<Translator>(new GroupingBinaryTranslator()));
    registerTranslator(QSharedPointer<Translator>(new FSMStatusTranslator()));
    registerTranslator(QSharedPointer<Translator>(new StringTranslator()));
    registerTranslator(QSharedPointer<Translator>(new SignedTranslator()));
    registerTranslator(QSharedPointer<Translator>(new UnsignedTranslator()));
    registerTranslator(QSharedPointer<Translator>(new FloatTranslator()));
    registerTranslator(QSharedPointer<Translator>(new ASCIITranslator()));
    registerTranslator(QSharedPointer<Translator>(new DecimalTranslator())); 
}
void TranslatorManager::registerTranslator(QSharedPointer<Translator> translator) {
    if (translator && !translators.contains(translator)) {
        translators.append(translator);
    }
}
QSharedPointer<Translator> TranslatorManager::findTranslator(const VariableMeta& meta) const {
    QSharedPointer<Translator> preferred;
    int maxPreference = 0;
    for (auto& t : translators) {
        int pref = t->preference(meta);
        if (pref > maxPreference) {
            maxPreference = pref;
            preferred = t;
        }
    }
    return preferred ? preferred : defaultTranslator;
}
void TranslatorManager::setDefaultTranslator(QSharedPointer<Translator> translator) {
    if (translator && translators.contains(translator)) {
        defaultTranslator = translator;
    }
}
TranslatedValue handleSpecialValue(const QString& value) {
    
    return {value, ValueKind::Normal};

}
QString getGroupSpecialChar(const QString& group) {
    
    if (group.contains('x')) return "x";
    if (group.contains('z')) return "z";
    if (group.contains('u')) return "u";
    if (group.contains('w')) return "w";
    if (group.contains('h') || group.contains('l')) return "?";
    if (group.contains('-')) return "-";
    return ""; 
}
ValueKind determineOverallKind(const QString& binStr) {
    
    if (binStr.contains('x')) return ValueKind::Undef;
    if (binStr.contains('z')) return ValueKind::HighImp;
    if (binStr.contains('u')) return ValueKind::Undef;
    if (binStr.contains('w')) return ValueKind::Undef;
    if (binStr.contains('h') || binStr.contains('l')) return ValueKind::Weak;
    if (binStr.contains('-')) return ValueKind::DontCare;
    return ValueKind::Normal;
}
QString HexTranslator::name() const { return "Hexadecimal"; }
QString HexTranslator::prefixName() const { return "(hex)"; }
TranslatedValue HexTranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    TranslatedValue special = handleSpecialValue(value);
    if (special.kind != ValueKind::Normal) {
        return special;
    }
    
    QStringList groups = groupRightToLeft(value, 4);
    QString hexStr;
    for (const QString& group : groups) {
        
        QString specialChar = getGroupSpecialChar(group);
        if (!specialChar.isEmpty()) {
            hexStr += specialChar; 
        } else {
            
            bool ok;
            ulong num = group.toULong(&ok, 2);
            if (ok) {
                
                int hexDigits = (group.length() + 3) / 4;
                QString hex = QString::number(num, 16).toUpper();
                
                if (hex.length() < hexDigits) {
                    hex = hex.rightJustified(hexDigits, '0');
                }
                hexStr += hex;
            } else {
                hexStr += "?";
            }
        }
    }
    
    QStringList hexGroups = groupRightToLeft(hexStr, 4);
    return {hexGroups.join("_"), ValueKind::Normal};
}
int HexTranslator::preference(const VariableMeta& meta) const {
    
    if (meta.type == VarType::String || meta.numBits <= 0) {
        return 0; 
    }
    
    return (meta.type == VarType::BitVector ||
            meta.type == VarType::StdLogicVector ||
            meta.type == VarType::StdULogicVector) ? 2 : 1;
}
QString OctalTranslator::name() const { return "Octal"; }
QString OctalTranslator::prefixName() const { return "(oct)"; }
TranslatedValue OctalTranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    TranslatedValue special = handleSpecialValue(value);
    if (special.kind != ValueKind::Normal) {
        return special;
    }
    
    QStringList groups = groupRightToLeft(value, 3);
    QString octalStr;
    for (const QString& group : groups) {
        
        QString specialChar = getGroupSpecialChar(group);
        if (!specialChar.isEmpty()) {
            octalStr += specialChar;
        } else {
            
            bool ok;
            ulong num = group.toULong(&ok, 2);
            if (ok) {
                
                int octalDigits = (group.length() + 2) / 3;
                QString octal = QString::number(num, 8);
                
                if (octal.length() < octalDigits) {
                    octal = octal.rightJustified(octalDigits, '0');
                }
                octalStr += octal;
            } else {
                octalStr += "?";
            }
        }
    }
    
    QStringList octalGroups = groupRightToLeft(octalStr, 4);
    return {octalGroups.join("_"), ValueKind::Normal};
}
int OctalTranslator::preference(const VariableMeta& meta) const {
    
    if (meta.type == VarType::String || meta.numBits <= 0) {
        return 0; 
    }
    return 1; 
}
QString BinaryTranslator::name() const { return "Binary"; }
QString BinaryTranslator::prefixName() const { return "(bin)"; }
TranslatedValue BinaryTranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    TranslatedValue special = handleSpecialValue(value);
    if (special.kind != ValueKind::Normal) {
        return special;
    }
    
    return {value, ValueKind::Normal};
}
int BinaryTranslator::preference(const VariableMeta& meta) const {
    
    return meta.numBits > 0 ? 1 : 0;
}
QString GroupingBinaryTranslator::name() const { return "Grouped Binary"; }
QString GroupingBinaryTranslator::prefixName() const { return "(bin)"; }
TranslatedValue GroupingBinaryTranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    TranslatedValue special = handleSpecialValue(value);
    if (special.kind != ValueKind::Normal) {
        
        QStringList groups = groupRightToLeft(value, 4);
        return {groups.join("_"), special.kind};
    }
    
    QStringList groups = groupRightToLeft(value, 4);
    return {groups.join("_"), ValueKind::Normal};
}
int GroupingBinaryTranslator::preference(const VariableMeta& meta) const {
    
    return meta.numBits > 0 ? 1 : 0;
}
QString FSMStatusTranslator::name() const { return "FSMStatus"; }
QString FSMStatusTranslator::prefixName() const { return "(fsm)"; }
TranslatedValue FSMStatusTranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    TranslatedValue special = handleSpecialValue(value);
    if (special.kind != ValueKind::Normal) {
        return {"UNKNOWN", ValueKind::Undef};
    }
    QMap<QString, QString> stateMap = GlobalState::instance().getMap(QString::number(meta.var_ref));
    bool ok;
    ulong num = value.toULong(&ok, 2);
    return {stateMap.value(QString::number(num), "UNKNOWN_STATE"), ValueKind::Normal};
}
int FSMStatusTranslator::preference(const VariableMeta& meta) const {
    
    return meta.type == VarType::Enum ? 2 : 1;
}
QString StringTranslator::name() const { return "String"; }
QString StringTranslator::prefixName() const { return "(str)"; }
TranslatedValue StringTranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    return {value, ValueKind::Normal};
}
int StringTranslator::preference(const VariableMeta& meta) const {
    
    return meta.type == VarType::String ? 2 : 0;
}
QString SignedTranslator::name() const { return "Signed"; }
QString SignedTranslator::prefixName() const { return "(signed)"; }
TranslatedValue SignedTranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    TranslatedValue special = handleSpecialValue(value);
    if (special.kind != ValueKind::Normal) {
        return {"x", ValueKind::Undef};
    }
    bool ok;
    ulong num = value.toULong(&ok, 2);
    if (ok && meta.numBits > 0) {
        
        ulong signBit = 1UL << (meta.numBits - 1);
        if (num & signBit) {
            
            ulong magnitude = (~num + 1) & ((1UL << meta.numBits) - 1);
            return {"-" + QString::number(magnitude), ValueKind::Normal};
        }
        return {QString::number(num), ValueKind::Normal};
    }
    return {value, ValueKind::Warn};
}
int SignedTranslator::preference(const VariableMeta& meta) const {
    
    return (meta.type == VarType::Integer ||
            meta.type == VarType::Int ||
            meta.type == VarType::ShortInt ||
            meta.type == VarType::LongInt ||
            meta.type == VarType::Byte) ? 2 : 1;
}
QString UnsignedTranslator::name() const { return "Unsigned"; }
QString UnsignedTranslator::prefixName() const { return "(unsigned)"; }
TranslatedValue UnsignedTranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    TranslatedValue special = handleSpecialValue(value);
    if (special.kind != ValueKind::Normal) {
        return {"x", ValueKind::Undef};
    }
    bool ok;
    ulong num = value.toULong(&ok, 2);
    if (ok) {
        return {QString::number(num), ValueKind::Normal};
    }
    return {value, ValueKind::Warn};
}
int UnsignedTranslator::preference(const VariableMeta& meta) const {
    
    return (meta.type == VarType::Integer ||
            meta.type == VarType::Int ||
            meta.type == VarType::ShortInt ||
            meta.type == VarType::LongInt ||
            meta.type == VarType::Byte) ? 1 : 0;
}
QString FloatTranslator::name() const { return "Floating Point"; }
QString FloatTranslator::prefixName() const { return "(float)"; }
TranslatedValue FloatTranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    TranslatedValue special = handleSpecialValue(value);
    if (special.kind != ValueKind::Normal) {
        return {"x", ValueKind::Undef};
    }
    bool ok;
    ulong num = value.toULong(&ok, 2);
    if (ok) {
        if (meta.numBits == 32) {
            
            float f;
            memcpy(&f, &num, sizeof(float));
            return {QString::number(f), ValueKind::Normal};
        } else if (meta.numBits == 64) {
            
            double d;
            if (sizeof(num) >= sizeof(double)) {
                
                d = static_cast<double>(num);
            } else {
                
                d = static_cast<double>(num); 
            }
            return {QString::number(d), ValueKind::Normal};
        }
    }
    return {value, ValueKind::Warn};
}
int FloatTranslator::preference(const VariableMeta& meta) const {
    
    return (meta.type == VarType::Real ||
            meta.type == VarType::ShortReal) ? 2 : 1;
}
QString ASCIITranslator::name() const { return "ASCII"; }
QString ASCIITranslator::prefixName() const { return "(ascii)"; }
TranslatedValue ASCIITranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    TranslatedValue special = handleSpecialValue(value);
    if (special.kind != ValueKind::Normal) {
        return special;
    }
    
    QString ascii;
    for (int i = 0; i < value.length(); i += 8) {
        QString byteStr = value.mid(i, 8);
        if (byteStr.length() < 8) break;
        bool ok;
        char c = static_cast<char>(byteStr.toULong(&ok, 2));
        if (ok) {
            ascii.append(c);
        }
    }
    return {ascii, ValueKind::Normal};
}
int ASCIITranslator::preference(const VariableMeta& meta) const {
    
    return (meta.type == VarType::Byte ||
            meta.type == VarType::StdLogicVector ||
            meta.type == VarType::StdULogicVector) ? 2 : 1;
}
QString DecimalTranslator::name() const { return "Decimal"; }
QString DecimalTranslator::prefixName() const { return "(dec)"; }
TranslatedValue DecimalTranslator::translate(const VariableMeta& meta, const QString& value) const {
    
    TranslatedValue special = handleSpecialValue(value);
    if (special.kind != ValueKind::Normal) {
        return {"x", ValueKind::Undef};
    }
    bool ok;
    ulong num = value.toULong(&ok, 2);
    if (ok) {
        
        return {QString::number(num), ValueKind::Normal};
    }
    return {value, ValueKind::Warn};
}
int DecimalTranslator::preference(const VariableMeta& meta) const {
    
    return (meta.type == VarType::Integer ||
            meta.type == VarType::Int ||
            meta.type == VarType::ShortInt ||
            meta.type == VarType::LongInt ||
            meta.type == VarType::Byte) ? 2 : 1;
}
