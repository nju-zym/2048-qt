#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include "../ai/AIInterface.h"
#include "../core/GameBoard.h"
#include "../core/GameState.h"
#include "../ui/GameView.h"

#include <QKeyEvent>
#include <QObject>
#include <QPoint>
#include <QTimer>

class MainWindow;

class GameController : public QObject {
    Q_OBJECT

   public:
    explicit GameController(MainWindow* parent);
    ~GameController();

    // 设置游戏视图
    void setGameView(GameView* view);

    // 游戏控制
    void startNewGame();
    void handleKeyPress(QKeyEvent* event);
    void handleMove(int direction);  // 0=up, 1=right, 2=down, 3=left
    void undoMove();

    // 状态检查
    bool isAnimationInProgress() const;

    // 更新分数
    void updateScore(int newScore);
    void updateStatus(QString const& message);

    // AI控制
    void startAI(AIInterface* ai);
    void stopAI();
    bool isAIRunning() const;

   public slots:
    void onAnimationFinished();
    void onAITimerTimeout();
    void onAIMoveDecided(int direction);

   private:
    MainWindow* mainWindow;
    GameBoard gameBoard;
    GameState gameState;
    GameView* gameView;

    bool animationInProgress;

    // AI相关
    AIInterface* ai;
    QTimer* aiTimer;
    bool aiRunning;

    // 游戏状态处理
    void handleGameWon();
    void handleGameOver();

    // 显示消息框
    void showGameOverMessage();
    void showWinMessage();

    // 生成新方块
    void generateNewTileWithAnimation();

    // 寻找新生成的方块位置
    QPair<int, int> findNewTilePosition(QVector<QPair<int, int>> const& before, QVector<QPair<int, int>> const& after);
};

#endif  // GAMECONTROLLER_H