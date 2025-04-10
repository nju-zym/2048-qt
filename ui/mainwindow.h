#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../ai/autoplayer.h"
#include "../core/board.h"
#include "../utils/animation.h"

#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QPair>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QTimer>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}  // namespace Ui
QT_END_NAMESPACE

/**
 * @brief 游戏主窗口类，负责UI界面和用户交互
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    MainWindow(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow();

   protected:
    /**
     * @brief 处理键盘按下事件
     * @param event 键盘事件对象
     */
    void keyPressEvent(QKeyEvent* event) override;

   private slots:
    /**
     * @brief 新游戏按钮点击事件
     */
    void on_newGameButton_clicked();

    /**
     * @brief 撤销按钮点击事件
     */
    void on_undoButton_clicked();

    /**
     * @brief 设置按钮点击事件
     */
    void on_settingsButton_clicked();

    /**
     * @brief 自动游戏按钮点击事件
     */
    void on_autoPlayButton_clicked();

    /**
     * @brief 训练按钮点击事件
     */
    void on_trainButton_clicked();

    /**
     * @brief 处理AI决定的移动方向
     * @param direction 移动方向
     */
    void onMoveDecided(int direction);

    /**
     * @brief 处理训练进度更新
     * @param current 当前进度
     * @param total 总进度
     */
    void onTrainingProgress(int current, int total);

    /**
     * @brief 处理训练完成事件
     */
    void onTrainingComplete();

   private:
    Ui::MainWindow* ui;  // UI界面

    // 游戏数据
    Board* gameBoard;                                    // 游戏棋盘核心逻辑
    QVector<QVector<QLabel*>> tileLabels;                // 棋盘上的方块标签
    int score;                                           // 当前得分
    int bestScore;                                       // 最高分数
    QVector<QPair<QVector<QVector<int>>, int>> history;  // 用于撤销操作，存储棋盘状态和分数
    bool animationInProgress;                            // 标记动画是否正在进行
    int pendingAnimations;                               // 跟踪当前正在进行的动画数量

    // 自动游戏
    AutoPlayer* autoPlayer;  // 自动游戏AI
    QTimer* autoPlayTimer;   // 自动游戏定时器

    // 初始化函数

    /**
     * @brief 初始化方块标签
     */
    void initializeTiles();

    /**
     * @brief 开始新游戏
     */
    void startNewGame();

    /**
     * @brief 获取空位置
     * @return 空位置的坐标列表
     */
    QVector<QPair<int, int>> getEmptyTiles() const;

    /**
     * @brief 检查指定位置是否为空
     * @param row 行索引
     * @param col 列索引
     * @return 如果位置为空返回true，否则返回false
     */
    bool isTileEmpty(int row, int col) const;

    // 游戏逻辑
    /**
     * @brief 移动方块
     * @param direction 移动方向(0:上, 1:右, 2:下, 3:左)
     * @return 是否发生了有效移动
     */
    bool moveTiles(int direction);

    /**
     * @brief 生成新方块
     * @param animate 是否带有动画效果
     */
    void generateNewTile(bool animate = true);

    /**
     * @brief 检查游戏是否结束
     * @return 游戏是否结束
     */
    bool isGameOver() const;

    /**
     * @brief 检查游戏是否获胜
     * @return 是否达成2048
     */
    bool isGameWon() const;

    // UI更新
    /**
     * @brief 更新方块外观
     * @param row 行索引
     * @param col 列索引
     */
    void updateTileAppearance(int row, int col);

    /**
     * @brief 更新分数显示
     * @param newScore 新分数
     */
    void updateScore(int newScore);

    /**
     * @brief 更新状态栏信息
     * @param message 要显示的信息
     */
    void updateStatus(QString const& message);

    /**
     * @brief 获取方块的样式表
     * @param value 方块的值
     * @return 样式表字符串
     */
    QString getTileStyleSheet(int value) const;

    /**
     * @brief 显示游戏结束消息
     */
    void showGameOverMessage();

    /**
     * @brief 显示游戏胜利消息
     */
    void showWinMessage();

    // 自动游戏方法
    /**
     * @brief 设置自动游戏
     */
    void setupAutoPlay();

    /**
     * @brief 执行自动移动
     */
    void performAutoMove();

    // 动画
    /**
     * @brief 执行方块移动动画
     * @param label 要移动的标签
     * @param from 起始位置
     * @param to 目标位置
     */
    void animateTileMovement(QLabel* label, QPoint const& from, QPoint const& to);

    /**
     * @brief 执行方块合并动画
     * @param label 要执行动画的标签
     */
    void animateTileMerge(QLabel* label);
};

#endif  // MAINWINDOW_H