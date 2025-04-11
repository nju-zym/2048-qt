#include "GameController.h"

#include "../mainwindow.h"

#include <QMessageBox>
#include <QTimer>

GameController::GameController(MainWindow* parent)
    : QObject(parent),
      mainWindow(parent),
      gameBoard(4),
      animationInProgress(false),
      ai(nullptr),
      aiTimer(nullptr),
      aiRunning(false) {
    // GameView 对象将在 MainWindow 中创建并提供给 GameController

    // 创建AI定时器
    aiTimer = new QTimer(this);
    connect(aiTimer, &QTimer::timeout, this, &GameController::onAITimerTimeout);
}

GameController::~GameController() {
    // GameView 的生命周期由 MainWindow 管理

    // 停止AI
    stopAI();

    // 释放AI对象
    if (ai) {
        delete ai;
        ai = nullptr;
    }
}

void GameController::setGameView(GameView* view) {
    gameView = view;

    // 连接动画完成信号
    connect(gameView, &GameView::animationFinished, this, &GameController::onAnimationFinished);
}

void GameController::startNewGame() {
    gameBoard.reset();
    gameState.setWinAlertShown(false);
    gameState.clearHistory();

    // 更新界面 - 这一步会设置所有格子为0
    gameView->updateAllTiles(gameBoard.getBoardState());

    // 使用QEventLoop确保UI更新
    QEventLoop loop;
    QTimer::singleShot(10, &loop, &QEventLoop::quit);
    loop.exec();

    // 更新分数和状态
    updateScore(0);
    updateStatus("Join the tiles, get to 2048!");

    // 随机生成两个初始方块
    bool animated = true;

    // 生成第一个方块
    QVector<QPair<int, int>> emptyTilesBefore = gameBoard.getEmptyTiles();
    gameBoard.generateNewTile(&animated);
    QVector<QPair<int, int>> emptyTilesAfter = gameBoard.getEmptyTiles();

    // 找出第一个新方块的位置
    QPair<int, int> firstNewTile = findNewTilePosition(emptyTilesBefore, emptyTilesAfter);

    // 生成第二个方块
    emptyTilesBefore = emptyTilesAfter;
    gameBoard.generateNewTile(&animated);
    emptyTilesAfter = gameBoard.getEmptyTiles();

    // 找出第二个新方块的位置
    QPair<int, int> secondNewTile = findNewTilePosition(emptyTilesBefore, emptyTilesAfter);

    // 更新界面
    gameView->updateAllTiles(gameBoard.getBoardState());

    // 使用定时器确保UI更新完成后再播放动画
    QTimer::singleShot(50, this, [this, firstNewTile, secondNewTile]() {
        // 播放新方块的出现动画
        if (firstNewTile.first >= 0 && firstNewTile.first < 4 && firstNewTile.second >= 0 && firstNewTile.second < 4) {
            gameView->playNewTileAnimation(firstNewTile.first, firstNewTile.second);
        }

        if (secondNewTile.first >= 0 && secondNewTile.first < 4 && secondNewTile.second >= 0
            && secondNewTile.second < 4) {
            gameView->playNewTileAnimation(secondNewTile.first, secondNewTile.second);
        }
    });
}

QPair<int, int> GameController::findNewTilePosition(QVector<QPair<int, int>> const& before,
                                                    QVector<QPair<int, int>> const& after) {
    // 通过比较生成前后的空白位置找出新方块位置
    for (auto const& beforePos : before) {
        bool found = false;
        for (auto const& afterPos : after) {
            if (beforePos.first == afterPos.first && beforePos.second == afterPos.second) {
                found = true;
                break;
            }
        }
        if (!found) {
            return beforePos;  // 这个位置从空变成了有值，就是新方块位置
        }
    }

    return QPair<int, int>(-1, -1);  // 未找到新方块位置
}

void GameController::handleKeyPress(QKeyEvent* event) {
    if (animationInProgress) {
        return;  // 如果动画正在进行，忽略键盘输入
    }

    // 根据按键值调用相应的移动函数
    switch (event->key()) {
        case Qt::Key_Up:
            handleMove(0);  // 0代表向上移动
            break;
        case Qt::Key_Right:
            handleMove(1);  // 1代表向右移动
            break;
        case Qt::Key_Down:
            handleMove(2);  // 2代表向下移动
            break;
        case Qt::Key_Left:
            handleMove(3);  // 3代表向左移动
            break;
        default:
            break;  // 忽略其他按键
    }
}

void GameController::handleMove(int direction) {
    if (animationInProgress) {
        return;  // 如果动画正在进行，忽略移动操作
    }

    // 保存当前状态以便撤销
    gameState.saveState(gameBoard.getBoardState(), gameBoard.getScore());

    // 执行移动操作
    bool moved = gameBoard.moveTiles(direction);

    if (moved) {
        // 更新UI
        gameView->updateAllTiles(gameBoard.getBoardState());
        updateScore(gameBoard.getScore());

        // 播放移动动画
        QVector<GameBoard::TileMove> const& moves = gameBoard.getLastMoves();
        if (!moves.isEmpty()) {
            animationInProgress = true;
            gameView->playMoveAnimation(moves);
        } else {
            // 如果没有移动动画，直接生成新方块
            generateNewTileWithAnimation();
        }
    }
}

void GameController::onAnimationFinished() {
    // 生成新的方块
    generateNewTileWithAnimation();

    // 检查游戏状态
    if (gameBoard.isGameWon()) {
        handleGameWon();
    } else if (gameBoard.isGameOver()) {
        handleGameOver();
    }

    animationInProgress = false;
}

void GameController::generateNewTileWithAnimation() {
    // 记录生成前的空白位置
    QVector<QPair<int, int>> beforeEmptyTiles = gameBoard.getEmptyTiles();

    // 生成新方块
    bool animated = true;
    gameBoard.generateNewTile(&animated);

    // 记录生成后的空白位置
    QVector<QPair<int, int>> afterEmptyTiles = gameBoard.getEmptyTiles();

    // 找出新生成的方块位置
    QPair<int, int> newTilePos = findNewTilePosition(beforeEmptyTiles, afterEmptyTiles);

    // 更新界面
    gameView->updateAllTiles(gameBoard.getBoardState());

    // 为新方块添加动画效果
    if (newTilePos.first >= 0 && newTilePos.first < 4 && newTilePos.second >= 0 && newTilePos.second < 4) {
        gameView->playNewTileAnimation(newTilePos.first, newTilePos.second);
    }
}

void GameController::undoMove() {
    if (gameState.canUndo() && !animationInProgress) {
        QPair<QVector<QVector<int>>, int> lastState = gameState.undo();
        gameBoard.setBoardState(lastState.first);
        gameBoard.setScore(lastState.second);

        updateScore(lastState.second);
        gameView->updateAllTiles(gameBoard.getBoardState());
    }
}

void GameController::handleGameWon() {
    if (!gameState.isWinAlertShown()) {
        showWinMessage();
    }
}

void GameController::handleGameOver() {
    showGameOverMessage();
}

void GameController::showGameOverMessage() {
    QMessageBox msgBox;
    msgBox.setText("Game Over!");
    msgBox.setInformativeText("Do you want to start a new game?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::Yes) {
        startNewGame();
    }
}

void GameController::showWinMessage() {
    QMessageBox msgBox;
    msgBox.setText("You Win!");
    msgBox.setInformativeText("Do you want to continue playing?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::No) {
        startNewGame();
    } else {
        gameState.setWinAlertShown(true);
        updateStatus("Keep going to get a higher score!");
    }
}

bool GameController::isAnimationInProgress() const {
    return animationInProgress;
}

// AI控制相关方法
void GameController::startAI(AIInterface* aiInstance) {
    // 如果已经有AI在运行，先停止
    stopAI();

    // 释放旧的AI对象
    if (ai) {
        delete ai;
        ai = nullptr;
    }

    // 设置新的AI对象
    ai = aiInstance;

    // 连接AI的移动决策信号
    connect(ai, &AIInterface::moveDecided, this, &GameController::onAIMoveDecided, Qt::DirectConnection);

    qDebug() << "GameController: AI started:" << ai->getName();

    // 启动AI定时器，每2000毫秒执行一次
    // 对于MCTS算法，需要足够的时间
    // 但也不能太长，否则用户体验差
    aiTimer->start(2000);
    aiRunning = true;

    // 更新状态
    updateStatus(QString("AI running: %1").arg(ai->getName()));
}

void GameController::stopAI() {
    if (aiRunning) {
        // 停止定时器
        aiTimer->stop();
        aiRunning = false;

        // 断开信号连接
        if (ai) {
            disconnect(ai, &AIInterface::moveDecided, this, &GameController::onAIMoveDecided);
        }

        // 更新状态
        updateStatus("AI stopped");
    }
}

bool GameController::isAIRunning() const {
    return aiRunning;
}

void GameController::onAITimerTimeout() {
    // 如果动画正在进行或没有AI，则跳过
    if (animationInProgress || !ai) {
        return;
    }

    // 添加静态变量跟踪是否正在计算
    static bool isCalculating = false;

    // 如果已经在计算中，则跳过
    if (isCalculating) {
        qDebug() << "GameController: AI is already calculating, skipping";
        return;
    }

    // 设置计算标志
    isCalculating = true;

    qDebug() << "GameController: AI timer timeout, calculating next move";

    // 让AI开始计算下一步移动
    // 计算结果将通过moveDecided信号发送
    ai->getBestMove(gameBoard);

    // 在下一个事件循环中重置计算标志
    QTimer::singleShot(0, []() { isCalculating = false; });
}

void GameController::onAIMoveDecided(int direction) {
    qDebug() << "GameController: AI move decided:" << direction;

    // 如果动画正在进行或没有AI，则跳过
    if (animationInProgress || !ai || !aiRunning) {
        qDebug() << "GameController: Skipping AI move, animation in progress or AI not running";
        return;
    }

    // 使用QTimer::singleShot延迟执行移动，避免阻塞事件循环
    QTimer::singleShot(100, this, [this, direction]() {
        // 检查移动是否有效
        GameBoard testBoard = gameBoard;
        bool validMove      = testBoard.moveTiles(direction);

        qDebug() << "GameController: AI move valid:" << validMove;

        // 只有当移动有效时才执行
        if (validMove) {
            // 执行移动
            handleMove(direction);
        } else {
            // 如果移动无效，检查是否有任何有效移动
            for (int dir = 0; dir < 4; dir++) {
                GameBoard tempBoard = gameBoard;
                if (tempBoard.moveTiles(dir)) {
                    qDebug() << "GameController: Found valid move:" << dir;
                    // 执行这个有效移动
                    handleMove(dir);
                    break;
                }
            }
        }
    });

    // 游戏结束检查移到lambda中
    QTimer::singleShot(200, this, [this]() {
        // 检查游戏是否结束
        if (gameBoard.isGameOver()) {
            qDebug() << "GameController: Game is over, stopping AI";
            // 停止AI
            stopAI();
            // 显示游戏结束消息
            handleGameOver();
        }
    });
}

// 实现更新分数和状态的函数
void GameController::updateScore(int newScore) {
    if (mainWindow) {
        mainWindow->updateScore(newScore);
    }
}

void GameController::updateStatus(QString const& message) {
    if (mainWindow) {
        mainWindow->updateStatus(message);
    }
}