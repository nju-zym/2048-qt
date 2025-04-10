#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H

#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPoint>
#include <QPropertyAnimation>
#include <functional>

class AnimationManager {
   public:
    AnimationManager();
    ~AnimationManager();

    // 动画完成的回调函数类型
    using AnimationCallback = std::function<void()>;

    // 为移动的方块添加动画
    void animateTileMovement(
        QLabel* label, QPoint const& from, QPoint const& to, int duration = 100, AnimationCallback callback = nullptr);

    // 为合并方块添加动画
    void animateTileMerge(QLabel* label, int duration = 150, AnimationCallback callback = nullptr);

    // 为新生成的方块添加动画
    void animateNewTile(QLabel* label, int duration = 200, AnimationCallback callback = nullptr);

    // 为分数增加添加动画效果
    void animateScoreIncrease(QWidget* parent,
                              QPoint const& position,
                              int scoreIncrease,
                              int duration               = 1000,
                              AnimationCallback callback = nullptr);

    // 获取当前正在进行的动画数量
    int getPendingAnimationsCount() const;

   private:
    int pendingAnimationsCount;  // 记录正在进行的动画数量
};

#endif  // ANIMATIONMANAGER_H