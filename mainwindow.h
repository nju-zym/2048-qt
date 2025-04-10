#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/GameBoard.h"
#include "core/GameState.h"
#include "utils/AnimationManager.h"

#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QPair>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}  // namespace Ui
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

   protected:
    void keyPressEvent(QKeyEvent* event) override;

   private slots:
    void on_newGameButton_clicked();
    void on_undoButton_clicked();
    void on_settingsButton_clicked();

   private:
    Ui::MainWindow* ui;

    // 游戏组件
    GameBoard gameBoard;        // 游戏棋盘
    GameState gameState;        // 游戏状态
    AnimationManager animator;  // 动画管理器

    // UI 元素
    QVector<QVector<QLabel*>> tileLabels;  // 棋盘上的标签
    bool animationInProgress;              // 标记动画是否正在进行

    // 初始化函数
    void initializeTiles();
    void startNewGame();

    // UI更新相关
    void updateTileAppearance(int row, int col);
    void updateScore(int newScore);
    void updateStatus(QString const& message);
    void showGameOverMessage();
    void showWinMessage();

    // 处理游戏逻辑
    void handleMove(int direction);  // 处理移动操作
    void handleGameOver();           // 处理游戏结束
    void handleGameWon();            // 处理获胜
};

#endif  // MAINWINDOW_H