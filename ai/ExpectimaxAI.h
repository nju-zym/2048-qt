#ifndef EXPECTIMAX_AI_H
#define EXPECTIMAX_AI_H

#include "AIInterface.h"
#include "BitBoard.h"

#include <QMutex>
#include <QObject>
#include <QRandomGenerator>
#include <QThread>
#include <QTime>
#include <QWaitCondition>

// 前向声明
class ExpectimaxWorker;

/**
 * @brief Expectimax算法AI实现
 *
 * 使用Expectimax算法为2048游戏选择最佳移动
 */
class ExpectimaxAI : public AIInterface {
    Q_OBJECT

   public:
    /**
     * @brief 构造函数
     * @param depth 搜索深度
     * @param parent 父对象
     */
    explicit ExpectimaxAI(int depth = 3, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ExpectimaxAI();

    /**
     * @brief 获取下一步最佳移动方向
     * @param board 当前游戏棋盘
     * @return 最佳移动方向（0=上, 1=右, 2=下, 3=左）
     */
    int getBestMove(GameBoard const& board) override;

    /**
     * @brief 设置AI的思考深度
     * @param depth 搜索深度
     */
    void setDepth(int depth) override;

    /**
     * @brief 获取当前AI的思考深度
     * @return 当前搜索深度
     */
    int getDepth() const override;

    /**
     * @brief 获取AI算法名称
     * @return 算法名称
     */
    QString getName() const override;

   private slots:
    /**
     * @brief 处理工作线程计算出的移动
     * @param direction 移动方向
     */
    void onMoveCalculated(int direction);

   private:
    int depth;                 // 搜索深度
    int lastCalculatedMove;    // 最后计算出的移动
    bool moveReady;            // 移动是否已经准备好
    QMutex mutex;              // 互斥锁
    QWaitCondition condition;  // 条件变量

    // 工作线程
    ExpectimaxWorker* worker;
};

#endif  // EXPECTIMAX_AI_H
