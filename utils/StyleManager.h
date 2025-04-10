#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QMutex>
#include <QString>

// 声明全局互斥锁，用于保护样式表操作
extern QMutex g_styleSheetMutex;

class StyleManager {
   public:
    StyleManager()  = default;
    ~StyleManager() = default;

    // 获取方块样式表
    static QString getTileStyleSheet(int value);
};

#endif  // STYLEMANAGER_H