#ifndef AIINTERFACE_H
#define AIINTERFACE_H

#include <QObject>
#include "../core/GameBoard.h"

/**
 * @brief AI接口类，定义了AI算法的通用接口
 * 
 * 所有AI算法实现都应该继承这个接口类
 */
class AIInterface : public QObject {
    Q_OBJECT

public:
    explicit AIInterface(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~AIInterface() = default;

    /**
     * @brief 获取下一步最佳移动方向
     * @param board 当前游戏棋盘
     * @return 最佳移动方向（0=上, 1=右, 2=下, 3=左）
     */
    virtual int getBestMove(const GameBoard& board) = 0;

    /**
     * @brief 设置AI的思考深度
     * @param depth 搜索深度
     */
    virtual void setDepth(int depth) = 0;

    /**
     * @brief 获取当前AI的思考深度
     * @return 当前搜索深度
     */
    virtual int getDepth() const = 0;

    /**
     * @brief 获取AI算法名称
     * @return 算法名称
     */
    virtual QString getName() const = 0;

signals:
    /**
     * @brief 当AI计算出最佳移动时发出的信号
     * @param direction 移动方向（0=上, 1=右, 2=下, 3=左）
     */
    void moveDecided(int direction);
};

#endif // AIINTERFACE_H
