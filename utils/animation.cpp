#include "animation.h"

#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

namespace Animation {

QPropertyAnimation* createAppearAnimation(QLabel* label) {
    if (!label) {
        return nullptr;
    }

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(200);  // 200毫秒的动画

    QRect rect = label->geometry();

    // 从小到大的动画
    animation->setKeyValueAt(0, QRect(rect.center().x() - 5, rect.center().y() - 5, 10, 10));
    animation->setKeyValueAt(1, rect);
    animation->setEasingCurve(QEasingCurve::OutBack);  // 弹性效果

    return animation;
}

QPropertyAnimation* createMoveAnimation(QLabel* label, QPoint const& from, QPoint const& to) {
    if (!label) {
        return nullptr;
    }

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(100);  // 100毫秒的动画

    animation->setStartValue(QRect(from.x(), from.y(), label->width(), label->height()));
    animation->setEndValue(QRect(to.x(), to.y(), label->width(), label->height()));
    animation->setEasingCurve(QEasingCurve::OutQuad);  // 平滑退出效果

    return animation;
}

QPropertyAnimation* createMergeAnimation(QLabel* label) {
    if (!label) {
        return nullptr;
    }

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(150);  // 150毫秒的动画

    QRect rect = label->geometry();

    animation->setKeyValueAt(0, rect);  // 初始状态
    // 中间状态：缩小后居中
    animation->setKeyValueAt(0.5,
                             QRect(rect.x() + (rect.width() / 8),
                                   rect.y() + (rect.height() / 8),
                                   rect.width() * 3 / 4,
                                   rect.height() * 3 / 4));
    animation->setKeyValueAt(1, rect);  // 恢复原尺寸

    animation->setEasingCurve(QEasingCurve::OutBounce);  // 弹跳效果

    return animation;
}

QParallelAnimationGroup* createScoreAnimation(QLabel* scoreLabel, int scoreIncrease) {
    if (!scoreLabel || scoreIncrease <= 0) {
        return nullptr;
    }

    // 创建一个临时标签显示分数增加值
    QLabel* scoreAddLabel = new QLabel(QString("+%1").arg(scoreIncrease));
    scoreAddLabel->setStyleSheet("color: #776e65; font-weight: bold; font-size: 18px; background-color: transparent;");
    scoreAddLabel->setParent(scoreLabel->parentWidget());

    // 将标签定位在分数显示区域附近
    QPoint scorePos = scoreLabel->mapToParent(QPoint(0, 0));
    scoreAddLabel->move(scorePos.x() + scoreLabel->width() / 2, scorePos.y());
    scoreAddLabel->show();

    // 创建透明度动画
    QPropertyAnimation* fadeOut = new QPropertyAnimation(scoreAddLabel, "windowOpacity");
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setDuration(1000);  // 1秒渐隐

    // 创建位置动画，向上浮动
    QPropertyAnimation* moveUp = new QPropertyAnimation(scoreAddLabel, "pos");
    moveUp->setStartValue(scoreAddLabel->pos());
    moveUp->setEndValue(QPoint(scoreAddLabel->pos().x(), scoreAddLabel->pos().y() - 30));
    moveUp->setDuration(1000);  // 1秒向上浮动

    // 创建动画组并行执行两个动画
    QParallelAnimationGroup* animGroup = new QParallelAnimationGroup();
    animGroup->addAnimation(fadeOut);
    animGroup->addAnimation(moveUp);

    // 动画结束后删除标签
    QObject::connect(
        animGroup, &QParallelAnimationGroup::finished, [scoreAddLabel]() { scoreAddLabel->deleteLater(); });

    return animGroup;
}

}  // namespace Animation