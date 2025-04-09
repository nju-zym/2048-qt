#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// 包含自动操作类
#include "auto.h"

#include <QFuture>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMutex>
#include <QPair>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <QWaitCondition>

// 使用QtConcurrent::run运行异步任务
#include <QtConcurrent/QtConcurrent>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}  // namespace Ui
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // 禁用复制和移动操作
    MainWindow(MainWindow const&)            = delete;
    MainWindow& operator=(MainWindow const&) = delete;
    MainWindow(MainWindow&&)                 = delete;
    MainWindow& operator=(MainWindow&&)      = delete;

   protected:
    void keyPressEvent(QKeyEvent* event) override;

   private:
    // 私有槽函数
    Q_SLOT void on_newGameButton_clicked();
    Q_SLOT void on_undoButton_clicked();
    Q_SLOT void on_settingsButton_clicked();
    Q_SLOT void on_autoPlayButton_clicked();
    Q_SLOT void on_learnButton_clicked();
    Q_SLOT void on_resetAIButton_clicked();
    Q_SLOT void autoPlayStep();
    Q_SLOT void onAiCalculationFinished();
    Q_SLOT void onAiCalculationTimeout();
    Ui::MainWindow* ui;

    // 游戏数据
    QVector<QVector<int>> board{4, QVector<int>(4, 0)};  // 游戏棋盘数据，初始化为4x4的0矩阵
    QVector<QVector<QLabel*>> tileLabels;                // 棋盘上的标签
    QVector<QPair<QVector<QVector<int>>, int>> history;  // 历史棋盘状态

    int score;
    int bestScore;
    // 用于撤销操作，存储棋盘状态和分数
    bool animationInProgress;  // 标记动画是否正在进行
    int pendingAnimations;     // 跟踪当前正在进行的动画数量

    // 自动操作相关
    QTimer* autoPlayTimer;  // 自动操作定时器
    bool autoPlayActive;    // 标记自动操作是否激活

    // AI线程相关
    Auto* autoPlayer;        // AI自动操作对象
    bool aiCalculating;      // 标记AI是否正在计算
    QMutex aiMutex;          // 用于保护AI计算状态
    int aiCalculatedMove;    // 存储计算出的最佳移动
    QTimer* aiTimeoutTimer;  // 超时定时器，防止AI计算时间过长
    QFuture<int> aiFuture;   // 异步计算的Future对象

    // 初始化函数
    void setupBoard();
    void initializeTiles();
    void startNewGame();

    // 游戏逻辑
    bool moveTiles(int direction);                                              // 0=up, 1=right, 2=down, 3=left
    static bool canMoveTiles(QVector<QVector<int>>& testBoard, int direction);  // 检查是否可以向指定方向移动
    void generateNewTile(bool animate = true);
    bool isMoveAvailable() const;
    bool isGameOver() const;
    bool isGameWon() const;
    bool isTileEmpty(int row, int col) const;        // 检查格子是否为空
    QVector<QPair<int, int>> getEmptyTiles() const;  // 获取所有空格子

    // 自动操作相关
    int findBestMove();         // 找出最佳移动方向
    void startAiCalculation();  // 开始异步AI计算

    // UI更新
    void updateTileAppearance(int row, int col);
    void updateScore(int newScore);
    void updateStatus(QString const& message);
    static QString getTileStyleSheet(int value);
    void showGameOverMessage();
    void showWinMessage();

    // 动画
    void animateTileMovement(QLabel* label, QPoint const& from, QPoint const& to);
    void animateTileMerge(QLabel* label);
};

#endif  // MAINWINDOW_H