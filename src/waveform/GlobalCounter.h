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

#ifndef GLOBALCOUNTER_H
#define GLOBALCOUNTER_H
#include <QMutex>
class GlobalCounter {
private:
    static GlobalCounter& instance() {
        static GlobalCounter counter;
        return counter;
    }
    int expressionCounter = 1;
    QMutex mutex;
public:
    static int getNextExpressionId() {
        QMutexLocker locker(&instance().mutex);
        return instance().expressionCounter++;
    }
    static int getCurrentExpressionId() {
        QMutexLocker locker(&instance().mutex);
        return instance().expressionCounter;
    }
    static void resetExpressionCounter(int value = 1) {
        QMutexLocker locker(&instance().mutex);
        instance().expressionCounter = value;
    }
};
#endif 