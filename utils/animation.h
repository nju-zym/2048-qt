#ifndef ANIMATION_H
#define ANIMATION_H

#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

/**
 * @brief 提供游戏动画相关的辅助函数
 */
namespace Animation {

/**
 * @brief 创建方块出现动画
 * @param label 方块标签
 * @return 动画对象，需要手动开始和删除
 */
QPropertyAnimation* createAppearAnimation(QLabel* label);

/**
 * @brief 创建方块移动动画
 * @param label 方块标签
 * @param from 起始位置
 * @param to 目标位置
 * @return 动画对象，需要手动开始和删除
 */
QPropertyAnimation* createMoveAnimation(QLabel* label, QPoint const& from, QPoint const& to);

/**
 * @brief 创建方块合并动画
 * @param label 方块标签
 * @return 动画对象，需要手动开始和删除
 */
QPropertyAnimation* createMergeAnimation(QLabel* label);

/**
 * @brief 创建分数增加动画
 * @param scoreLabel 分数标签
 * @param scoreIncrease 分数增加值
 * @return 动画组对象，需要手动开始和删除
 */
QParallelAnimationGroup* createScoreAnimation(QLabel* scoreLabel, int scoreIncrease);

}  // namespace Animation

#endif  // ANIMATION_H