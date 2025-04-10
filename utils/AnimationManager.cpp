#include "AnimationManager.h"

#include "StyleManager.h"  // 引入 StyleManager.h 以访问全局互斥锁

#include <QMutexLocker>
#include <QPointer>

AnimationManager::AnimationManager() : pendingAnimationsCount(0) {}

AnimationManager::~AnimationManager() {}

void AnimationManager::animateTileMovement(
    QLabel* label, QPoint const& from, QPoint const& to, int duration, AnimationCallback callback) {
    if (!label || !label->isVisible()) {
        // 如果标签无效或不可见，不执行动画
        if (callback) {
            callback();
        }
        return;
    }

    pendingAnimationsCount++;  // 增加正在进行的动画计数

    // 确保临时标签在最上层显示
    label->raise();
    label->setWindowOpacity(1.0);  // 确保标签完全不透明

    // 使用位置动画而不是几何动画，以确保动画正确显示
    QPropertyAnimation* animation = new QPropertyAnimation(label, "pos");
    animation->setDuration(duration);  // 使用合适的动画时间

    // 设置起点和终点
    animation->setStartValue(from);  // 起始位置
    animation->setEndValue(to);      // 终止位置

    // 确保标签大小正确
    label->resize(label->width(), label->height());
    label->move(from);  // 设置起始位置

    // 使用缓动曲线模拟2048原版的动画效果
    animation->setEasingCurve(QEasingCurve::OutQuad);  // 使用平滑的减速效果

    // 使用QPointer来安全地跟踪对象
    QPointer<QLabel> safeLabel(label);

    // 连接动画结束信号，减少计数并调用回调
    QObject::connect(animation, &QPropertyAnimation::finished, [this, safeLabel, callback]() {
        pendingAnimationsCount--;
        if (callback) {
            // 即使QLabel已被销毁，回调也能安全执行
            callback();
        }
    });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void AnimationManager::animateTileMerge(QLabel* label, int duration, AnimationCallback callback) {
    if (!label || !label->isVisible()) {
        // 如果标签无效或不可见，不执行动画
        if (callback) {
            callback();
        }
        return;
    }

    pendingAnimationsCount++;  // 增加正在进行的动画计数

    auto animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(duration);
    QRect rect = label->geometry();

    // 模拟2048原版的合并动画：先缩小再放大
    animation->setKeyValueAt(0, rect);  // 初始状态

    // 中间状态：先缩小
    animation->setKeyValueAt(0.4,
                             QRect(rect.x() + (rect.width() / 6),
                                   rect.y() + (rect.height() / 6),
                                   rect.width() * 2 / 3,
                                   rect.height() * 2 / 3));

    // 然后放大超过原始大小
    animation->setKeyValueAt(0.7,
                             QRect(rect.x() - (rect.width() / 10),
                                   rect.y() - (rect.height() / 10),
                                   rect.width() * 6 / 5,
                                   rect.height() * 6 / 5));

    animation->setKeyValueAt(1, rect);  // 最终恢复原尺寸

    // 使用弹性效果，模拟弹性物体的感觉
    animation->setEasingCurve(QEasingCurve::OutElastic);

    // 使用QPointer来安全地跟踪对象
    QPointer<QLabel> safeLabel(label);

    // 连接动画结束信号，减少计数并调用回调
    QObject::connect(animation, &QPropertyAnimation::finished, [this, safeLabel, callback]() {
        pendingAnimationsCount--;
        if (callback) {
            callback();
        }
    });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void AnimationManager::animateNewTile(QLabel* label, int duration, AnimationCallback callback) {
    if (!label || !label->isVisible()) {
        // 如果标签无效或不可见，不执行动画
        if (callback) {
            callback();
        }
        return;
    }

    pendingAnimationsCount++;  // 增加正在进行的动画计数

    auto animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(duration);
    QRect rect = label->geometry();

    // 模拟2048原版的新方块出现动画：从小到大并伴随着一些弹性
    // 起始状态：一个小点，位于方块中心
    animation->setKeyValueAt(0, QRect(rect.center().x() - 2, rect.center().y() - 2, 4, 4));

    // 快速放大到接近原始大小
    animation->setKeyValueAt(
        0.5,
        QRect(rect.x() + rect.width() / 10, rect.y() + rect.height() / 10, rect.width() * 0.8, rect.height() * 0.8));

    // 稍微超过原始大小
    animation->setKeyValueAt(
        0.7,
        QRect(rect.x() - rect.width() / 20, rect.y() - rect.height() / 20, rect.width() * 1.1, rect.height() * 1.1));

    // 最终回到原始大小
    animation->setKeyValueAt(1, rect);

    // 使用弹性效果，增强新方块出现的视觉冲击感
    animation->setEasingCurve(QEasingCurve::OutBack);

    // 使用QPointer来安全地跟踪对象
    QPointer<QLabel> safeLabel(label);

    // 连接动画结束信号，减少计数并调用回调
    QObject::connect(animation, &QPropertyAnimation::finished, [this, safeLabel, callback]() {
        pendingAnimationsCount--;
        if (callback) {
            callback();
        }
    });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void AnimationManager::animateScoreIncrease(
    QWidget* parent, QPoint const& position, int scoreIncrease, int duration, AnimationCallback callback) {
    if (!parent || !parent->isVisible()) {
        // 如果父控件无效或不可见，不执行动画
        if (callback) {
            callback();
        }
        return;
    }

    pendingAnimationsCount++;  // 增加正在进行的动画计数

    // 创建一个临时标签显示分数增加值
    auto scoreAddLabel = new QLabel(QString("+%1").arg(scoreIncrease), parent);

    {
        // 在设置样式表前获取全局互斥锁
        QMutexLocker locker(&g_styleSheetMutex);  // 使用全局互斥锁而不是 styleSheetMutex
        scoreAddLabel->setStyleSheet(
            "color: #776e65; font-weight: bold; font-size: 18px; background-color: transparent;");
    }

    scoreAddLabel->move(position);
    scoreAddLabel->show();

    // 创建透明度动画
    auto fadeOut = new QPropertyAnimation(scoreAddLabel, "windowOpacity");
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setDuration(duration);
    fadeOut->setEasingCurve(QEasingCurve::OutCubic);  // 使用平滑的渐变效果

    // 创建位置动画，向上浮动并有轻微的加速效果
    auto moveUp = new QPropertyAnimation(scoreAddLabel, "pos");
    moveUp->setStartValue(position);
    moveUp->setEndValue(QPoint(position.x(), position.y() - 40));  // 增加浮动高度
    moveUp->setDuration(duration);
    moveUp->setEasingCurve(QEasingCurve::OutQuad);  // 使用加速效果

    // 创建动画组并行执行两个动画
    auto animGroup = new QParallelAnimationGroup();
    animGroup->addAnimation(fadeOut);
    animGroup->addAnimation(moveUp);

    // 使用QPointer安全地跟踪标签对象
    QPointer<QLabel> safeLabel(scoreAddLabel);

    // 动画结束后删除临时标签并调用回调
    QObject::connect(animGroup, &QParallelAnimationGroup::finished, [this, safeLabel, callback]() {
        // 安全删除标签
        if (!safeLabel.isNull()) {
            safeLabel->deleteLater();
        }

        pendingAnimationsCount--;
        if (callback) {
            callback();
        }
    });

    animGroup->start(QAbstractAnimation::DeleteWhenStopped);
}

int AnimationManager::getPendingAnimationsCount() const {
    return pendingAnimationsCount;
}