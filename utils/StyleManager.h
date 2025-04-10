#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>

class StyleManager {
   public:
    StyleManager()  = default;
    ~StyleManager() = default;

    // 获取方块样式表
    static QString getTileStyleSheet(int value);
};

#endif  // STYLEMANAGER_H