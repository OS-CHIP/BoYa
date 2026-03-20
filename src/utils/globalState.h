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

#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H
#include <QMap>
#include <QString>
#include <QMutex>
#include <QMutexLocker>

class GlobalState {
public:
    static GlobalState& instance() {
        static GlobalState instance;
        return instance;
    }

    void set(const QString& category, const QString& key, const QString& value) {
        QMutexLocker locker(&mutex_);
        data_[category][key] = value;
    }

    QString get(const QString& category, const QString& key,
                const QString& defaultVal = QString()) {
        QMutexLocker locker(&mutex_);
        if (data_.contains(category) && data_[category].contains(key)) {
            return data_[category][key];
        }
        return defaultVal;
    }

    void remove(const QString& category, const QString& key) {
        QMutexLocker locker(&mutex_);
        if (data_.contains(category)) {
            data_[category].remove(key);
        }
    }

    void clear() {
        QMutexLocker locker(&mutex_);
        data_.clear();
    }

    void setMap(const QString& category, const QMap<QString, QString>& map) {
        QMutexLocker locker(&mutex_);
        data_[category] = map;
    }

    QMap<QString, QString> getMap(const QString& category) {
        QMutexLocker locker(&mutex_);
        return data_.value(category);
    }

    QMap<QString, QString>& getMapRef(const QString& category) {
        QMutexLocker locker(&mutex_);
        return data_[category];
    }

    const QMap<QString, QString>& getMapConstRef(const QString& category) {
        QMutexLocker locker(&mutex_);
        static QMap<QString, QString> emptyMap;
        auto it = data_.find(category);
        if (it != data_.end()) {
            return *it;
        }
        return emptyMap;
    }

    bool containsMap(const QString& category) {
        QMutexLocker locker(&mutex_);
        return data_.contains(category);
    }

    int mapSize(const QString& category) {
        QMutexLocker locker(&mutex_);
        auto it = data_.find(category);
        if (it != data_.end()) {
            return it->size();
        }
        return 0;
    }

    void mergeMap(const QString& category, const QMap<QString, QString>& map,
                  bool overwrite = true) {
        QMutexLocker locker(&mutex_);
        if (overwrite) {
            for (auto it = map.begin(); it != map.end(); ++it) {
                data_[category][it.key()] = it.value();
            }
        } else {
            for (auto it = map.begin(); it != map.end(); ++it) {
                if (!data_[category].contains(it.key())) {
                    data_[category][it.key()] = it.value();
                }
            }
        }
    }

    QList<QString> categories() {
        QMutexLocker locker(&mutex_);
        return data_.keys();
    }

    QList<QString> mapKeys(const QString& category) {
        QMutexLocker locker(&mutex_);
        auto it = data_.find(category);
        if (it != data_.end()) {
            return it->keys();
        }
        return QList<QString>();
    }

    QList<QString> mapValues(const QString& category) {
        QMutexLocker locker(&mutex_);
        auto it = data_.find(category);
        if (it != data_.end()) {
            return it->values();
        }
        return QList<QString>();
    }

    void clearCategory(const QString& category) {
        QMutexLocker locker(&mutex_);
        data_.remove(category);
    }

    bool isCategoryEmpty(const QString& category) {
        QMutexLocker locker(&mutex_);
        auto it = data_.find(category);
        if (it != data_.end()) {
            return it->isEmpty();
        }
        return true;
    }

private:
    GlobalState() = default;
    ~GlobalState() = default;

    mutable QMutex mutex_;
    QMap<QString, QMap<QString, QString>> data_;
};
#endif // GLOBALSTATE_H
