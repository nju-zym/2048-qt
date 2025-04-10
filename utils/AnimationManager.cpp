#include "AnimationManager.h"

AnimationManager::AnimationManager() : pendingAnimationsCount(0) {}

AnimationManager::~AnimationManager() {}

void AnimationManager::animateTileMovement(
    QLabel* label, QPoint const& from, QPoint const& to, int duration, AnimationCallback callback) {
    pendingAnimationsCount++;  // 增加正在进行的动画计数

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(duration);
    animation->setStartValue(QRect(from.x(), from.y(), label->width(), label->height()));
    animation->setEndValue(QRect(to.x(), to.y(), label->width(), label->height()));
    animation->setEasingCurve(QEasingCurve::OutQuad);  // 使用缓出效果，使动画更自然

    // 连接动画结束信号，减少计数并调用回调
    QObject::connect(animation, &QPropertyAnimation::finished, [this, callback]() {
        pendingAnimationsCount--;
        if (callback) {
            callback();
        }
    });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void AnimationManager::animateTileMerge(QLabel* label, int duration, AnimationCallback callback) {
    pendingAnimationsCount++;  // 增加正在进行的动画计数

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(duration);
    QRect rect = label->geometry();

    animation->setKeyValueAt(0, rect);  // 初始状态
    // 中间状态：缩小后居中
    animation->setKeyValueAt(0.5,
                             QRect(rect.x() + (rect.width() / 8),
                                   rect.y() + (rect.height() / 8),
                                   rect.width() * 3 / 4,
                                   rect.height() * 3 / 4));
    animation->setKeyValueAt(1, rect);  // 恢复原尺寸

    animation->setEasingCurve(QEasingCurve::OutBounce);  // 使用弹跳效果结束动画

    // 连接动画结束信号，减少计数并调用回调
    QObject::connect(animation, &QPropertyAnimation::finished, [this, callback]() {
        pendingAnimationsCount--;
        if (callback) {
            callback();
        }
    });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void AnimationManager::animateNewTile(QLabel* label, int duration, AnimationCallback callback) {
    pendingAnimationsCount++;  // 增加正在进行的动画计数

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(duration);
    QRect rect = label->geometry();

    // 从小到大的动画效果
    animation->setKeyValueAt(0, QRect(rect.center().x() - 5, rect.center().y() - 5, 10, 10));
    animation->setKeyValueAt(1, rect);
    animation->setEasingCurve(QEasingCurve::OutBack);  // 使用弹性效果

    // 连接动画结束信号，减少计数并调用回调
    QObject::connect(animation, &QPropertyAnimation::finished, [this, callback]() {
        pendingAnimationsCount--;
        if (callback) {
            callback();
        }
    });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void AnimationManager::animateScoreIncrease(
    QWidget* parent, QPoint const& position, int scoreIncrease, int duration, AnimationCallback callback) {
    pendingAnimationsCount++;  // 增加正在进行的动画计数

    // 创建一个临时标签显示分数增加值
    QLabel* scoreAddLabel = new QLabel(QString("+%1").arg(scoreIncrease), parent);
    scoreAddLabel->setStyleSheet("color: #776e65; font-weight: bold; font-size: 18px; background-color: transparent;");
    scoreAddLabel->move(position);
    scoreAddLabel->show();

    // 创建透明度动画
    QPropertyAnimation* fadeOut = new QPropertyAnimation(scoreAddLabel, "windowOpacity");
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setDuration(duration);

    // 创建位置动画，向上浮动
    QPropertyAnimation* moveUp = new QPropertyAnimation(scoreAddLabel, "pos");
    moveUp->setStartValue(position);
    moveUp->setEndValue(QPoint(position.x(), position.y() - 30));
    moveUp->setDuration(duration);

    // 创建动画组并行执行两个动画
    QParallelAnimationGroup* animGroup = new QParallelAnimationGroup();
    animGroup->addAnimation(fadeOut);
    animGroup->addAnimation(moveUp);

    // 动画结束后删除临时标签并调用回调
    QObject::connect(animGroup, &QParallelAnimationGroup::finished, [this, scoreAddLabel, callback]() {
        scoreAddLabel->deleteLater();
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