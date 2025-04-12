#include "GameController.h"

#include "../ai/auto.h"

#include <QDebug>
#include <QMetaObject>
#include <QRandomGenerator>

#include <QtConcurrent/QtConcurrent>

// 构造函数：初始化游戏数据和定时器
GameController::GameController(QObject* parent)
    : QObject(parent),
      board(4, QVector<int>(4, 0)),
      score(0),
      bestScore(0),
      autoPlayActive(false),
      autoPlayer(std::make_unique<Auto>()),        // 使用make_unique创建Auto对象
      autoPlayTimer(std::make_shared<QTimer>()),   // 使用make_shared创建QTimer
      aiTimeoutTimer(std::make_shared<QTimer>()),  // 使用make_shared创建QTimer
      aiCalculating(false),
      aiCalculatedMove(-1),
      winAlertShown(false) {
    // 设置定时器
    autoPlayTimer->setInterval(300);  // 设置定时器间隔为300毫秒
    // 使用新式的函数指针语法替代SIGNAL/SLOT宏
    connect(autoPlayTimer.get(), &QTimer::timeout, this, &GameController::autoPlayStep);

    // 设置AI超时定时器
    aiTimeoutTimer->setInterval(2000);    // 2秒超时
    aiTimeoutTimer->setSingleShot(true);  // 单次触发
    // 使用新式的函数指针语法替代SIGNAL/SLOT宏
    connect(aiTimeoutTimer.get(), &QTimer::timeout, this, &GameController::onAiCalculationTimeout);
}

// 析构函数：不再需要手动释放内存，智能指针会自动管理
GameController::~GameController() {
    // 停止所有定时器
    if (autoPlayTimer) {
        autoPlayTimer->stop();
    }
    if (aiTimeoutTimer) {
        aiTimeoutTimer->stop();
    }

    // 智能指针会自动管理资源释放，不再需要手动delete
}

// setupBoard: 初始化棋盘数据，确保创建了一个4x4的矩阵，并将所有值设为0
void GameController::setupBoard() {
    // 确保游戏板是4x4的，并且所有值初始化为0
    board.clear();    // 清除旧棋盘数据
    board.resize(4);  // 设置棋盘行数为4
    for (int i = 0; i < 4; ++i) {
        board[i].resize(4, 0);  // 每一行设置4个元素，并初始为0
    }
}

// startNewGame: 重置游戏状态，清除旧数据，并初始化新的游戏开始状态
void GameController::startNewGame() {
    setupBoard();           // 重新初始化棋盘数据
    winAlertShown = false;  // 重置胜利提示标识

    // 清除AI缓存，提高性能
    autoPlayer->clearExpectimaxCache();

    // 在重置当前分数之前，更新最高分记录
    if (score > bestScore) {
        bestScore = score;
    }

    score = 0;             // 重置当前分数为0
    emit scoreUpdated(0);  // 更新分数显示

    history.clear();  // 清除保存的历史状态

    // 随机生成两个初始方块
    generateNewTile(false);
    generateNewTile(false);

    // 更新状态消息显示，让玩家知道游戏开始了
    emit statusUpdated("Join the tiles, get to 2048!");
    emit boardUpdated();
}

// moveTiles: 根据方向对所有方块进行移动合并操作，并更新分数及UI显示
bool GameController::moveTiles(int direction) {
    QVector<QVector<int>> previousBoard = board;  // 保存移动前的棋盘状态
    int scoreGained                     = 0;      // 本次移动获得的分数

    // 根据传入的方向执行不同的移动和合并逻辑
    if (direction == 0) {  // 向上移动
        for (int col = 0; col < 4; col++) {
            int writePos = 0;
            // 第一步：将非零方块上移，消除中间空隙
            for (int row = 0; row < 4; row++) {
                if (board[row][col] != 0) {
                    if (row != writePos) {
                        board[writePos][col] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos++;
                }
            }
            // 第二步：合并相邻且相同的方块
            for (int row = 0; row < 3; row++) {
                if (board[row][col] != 0 && board[row][col] == board[row + 1][col]) {
                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];  // 更新合并后获得的分数
                    board[row + 1][col]  = 0;
                }
            }
            // 第三步：再次上移以消除合并后产生的空隙
            writePos = 0;
            for (int row = 0; row < 4; row++) {
                if (board[row][col] != 0) {
                    if (row != writePos) {
                        board[writePos][col] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos++;
                }
            }
        }
    } else if (direction == 1) {  // 向右移动
        for (int row = 0; row < 4; row++) {
            int writePos = 3;
            // 将非零方块集中到右侧
            for (int col = 3; col >= 0; col--) {
                if (board[row][col] != 0) {
                    if (col != writePos) {
                        board[row][writePos] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos--;
                }
            }
            // 合并右侧相邻相同的方块
            for (int col = 3; col > 0; col--) {
                if (board[row][col] != 0 && board[row][col] == board[row][col - 1]) {
                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row][col - 1]  = 0;
                }
            }
            // 将合并后产生的空隙再次集中到右侧
            writePos = 3;
            for (int col = 3; col >= 0; col--) {
                if (board[row][col] != 0) {
                    if (col != writePos) {
                        board[row][writePos] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos--;
                }
            }
        }
    } else if (direction == 2) {  // 向下移动
        for (int col = 0; col < 4; col++) {
            int writePos = 3;
            // 将非零方块下移，填充底部空缺
            for (int row = 3; row >= 0; row--) {
                if (board[row][col] != 0) {
                    if (row != writePos) {
                        board[writePos][col] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos--;
                }
            }
            // 合并下侧相邻相同的方块
            for (int row = 3; row > 0; row--) {
                if (board[row][col] != 0 && board[row][col] == board[row - 1][col]) {
                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row - 1][col]  = 0;
                }
            }
            // 再次下移以消除中间空隙
            writePos = 3;
            for (int row = 3; row >= 0; row--) {
                if (board[row][col] != 0) {
                    if (row != writePos) {
                        board[writePos][col] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos--;
                }
            }
        }
    } else if (direction == 3) {  // 向左移动
        for (int row = 0; row < 4; row++) {
            int writePos = 0;
            // 将非零方块左移
            for (int col = 0; col < 4; col++) {
                if (board[row][col] != 0) {
                    if (col != writePos) {
                        board[row][writePos] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos++;
                }
            }
            // 合并左侧相邻且相同的方块
            for (int col = 0; col < 3; col++) {
                if (board[row][col] != 0 && board[row][col] == board[row][col + 1]) {
                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row][col + 1]  = 0;
                }
            }
            // 再次左移以填充合并后产生的空隙
            writePos = 0;
            for (int col = 0; col < 4; col++) {
                if (board[row][col] != 0) {
                    if (col != writePos) {
                        board[row][writePos] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos++;
                }
            }
        }
    }

    // 判断棋盘是否发生了改变
    bool changed = false;
    for (int i = 0; i < 4 && !changed; ++i) {
        for (int j = 0; j < 4 && !changed; ++j) {
            if (board[i][j] != previousBoard[i][j]) {
                changed = true;
            }
        }
    }

    // 如果棋盘发生了变化，处理方块移动的动画和历史记录
    if (changed) {
        // 保存移动前的状态用于撤销 - 使用智能指针
        auto historyItem = std::make_shared<QPair<QVector<QVector<int>>, int>>(previousBoard, score);
        history.append(historyItem);

        // 更新分数
        score += scoreGained;
        emit scoreUpdated(score);

        // 检查游戏胜利状态，如果达到2048并且还没提示过，则发送胜利信号
        if (isGameWon() && !winAlertShown) {
            winAlertShown = true;  // 设置标志，确保只提示一次
            emit gameWon();

            // 如果处于自动操作状态，停止自动操作
            if (autoPlayActive) {
                autoPlayActive = false;
                autoPlayTimer->stop();
                emit statusUpdated("Auto play stopped - You won!");
            }

            return true;  // 立即返回，不处理后续操作
        }

        // 构造用于存储方块移动信息的结构体
        struct TileMove {
            int fromRow;
            int fromCol;
            int toRow;
            int toCol;
            bool merged;
        };

        // 跟踪方块移动
        QVector<TileMove> moves;
        QVector<QVector<bool>> processed(4, QVector<bool>(4, false));

        // 根据移动方向分析方块移动情况
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                // 如果当前位置有值且与移动前不同
                if (board[i][j] != 0 && board[i][j] != previousBoard[i][j]) {
                    if (direction == 0) {  // 向上移动
                        // 向下寻找可能的源方块
                        for (int srcRow = i; srcRow < 4; ++srcRow) {
                            // 检查是否是合并的结果
                            if (srcRow < 3 && previousBoard[srcRow + 1][j] != 0 && previousBoard[srcRow][j] != 0
                                && previousBoard[srcRow][j] == previousBoard[srcRow + 1][j]
                                && board[i][j] == previousBoard[srcRow][j] * 2 && !processed[srcRow][j]
                                && !processed[srcRow + 1][j]) {
                                // 记录第一个方块移动
                                TileMove move1;
                                move1.fromRow = srcRow;
                                move1.fromCol = j;
                                move1.toRow   = i;
                                move1.toCol   = j;
                                move1.merged  = false;
                                moves.append(move1);
                                processed[srcRow][j] = true;

                                // 记录第二个方块移动（被合并的方块）
                                TileMove move2;
                                move2.fromRow = srcRow + 1;
                                move2.fromCol = j;
                                move2.toRow   = i;
                                move2.toCol   = j;
                                move2.merged  = true;
                                moves.append(move2);
                                processed[srcRow + 1][j] = true;
                                break;
                            }
                            // 检查简单移动
                            else if (previousBoard[srcRow][j] == board[i][j] && !processed[srcRow][j]) {
                                TileMove move;
                                move.fromRow = srcRow;
                                move.fromCol = j;
                                move.toRow   = i;
                                move.toCol   = j;
                                move.merged  = false;
                                moves.append(move);
                                processed[srcRow][j] = true;
                                break;
                            }
                        }
                    } else if (direction == 1) {  // 向右移动
                        // 向左寻找可能的源方块
                        for (int srcCol = j; srcCol >= 0; --srcCol) {
                            // 检查是否是合并的结果
                            if (srcCol > 0 && previousBoard[i][srcCol - 1] != 0 && previousBoard[i][srcCol] != 0
                                && previousBoard[i][srcCol] == previousBoard[i][srcCol - 1]
                                && board[i][j] == previousBoard[i][srcCol] * 2 && !processed[i][srcCol]
                                && !processed[i][srcCol - 1]) {
                                // 记录第一个方块移动
                                TileMove move1;
                                move1.fromRow = i;
                                move1.fromCol = srcCol;
                                move1.toRow   = i;
                                move1.toCol   = j;
                                move1.merged  = false;
                                moves.append(move1);
                                processed[i][srcCol] = true;

                                // 记录第二个方块移动（被合并的方块）
                                TileMove move2;
                                move2.fromRow = i;
                                move2.fromCol = srcCol - 1;
                                move2.toRow   = i;
                                move2.toCol   = j;
                                move2.merged  = true;
                                moves.append(move2);
                                processed[i][srcCol - 1] = true;
                                break;
                            }
                            // 检查简单移动
                            else if (previousBoard[i][srcCol] == board[i][j] && !processed[i][srcCol]) {
                                TileMove move;
                                move.fromRow = i;
                                move.fromCol = srcCol;
                                move.toRow   = i;
                                move.toCol   = j;
                                move.merged  = false;
                                moves.append(move);
                                processed[i][srcCol] = true;
                                break;
                            }
                        }
                    } else if (direction == 2) {  // 向下移动
                        // 向上寻找可能的源方块
                        for (int srcRow = i; srcRow >= 0; --srcRow) {
                            // 检查是否是合并的结果
                            if (srcRow > 0 && previousBoard[srcRow - 1][j] != 0 && previousBoard[srcRow][j] != 0
                                && previousBoard[srcRow][j] == previousBoard[srcRow - 1][j]
                                && board[i][j] == previousBoard[srcRow][j] * 2 && !processed[srcRow][j]
                                && !processed[srcRow - 1][j]) {
                                // 记录第一个方块移动
                                TileMove move1;
                                move1.fromRow = srcRow;
                                move1.fromCol = j;
                                move1.toRow   = i;
                                move1.toCol   = j;
                                move1.merged  = false;
                                moves.append(move1);
                                processed[srcRow][j] = true;

                                // 记录第二个方块移动（被合并的方块）
                                TileMove move2;
                                move2.fromRow = srcRow - 1;
                                move2.fromCol = j;
                                move2.toRow   = i;
                                move2.toCol   = j;
                                move2.merged  = true;
                                moves.append(move2);
                                processed[srcRow - 1][j] = true;
                                break;
                            }
                            // 检查简单移动
                            else if (previousBoard[srcRow][j] == board[i][j] && !processed[srcRow][j]) {
                                TileMove move;
                                move.fromRow = srcRow;
                                move.fromCol = j;
                                move.toRow   = i;
                                move.toCol   = j;
                                move.merged  = false;
                                moves.append(move);
                                processed[srcRow][j] = true;
                                break;
                            }
                        }
                    } else if (direction == 3) {  // 向左移动
                        // 向右寻找可能的源方块
                        for (int srcCol = j; srcCol < 4; ++srcCol) {
                            // 检查是否是合并的结果
                            if (srcCol < 3 && previousBoard[i][srcCol + 1] != 0 && previousBoard[i][srcCol] != 0
                                && previousBoard[i][srcCol] == previousBoard[i][srcCol + 1]
                                && board[i][j] == previousBoard[i][srcCol] * 2 && !processed[i][srcCol]
                                && !processed[i][srcCol + 1]) {
                                // 记录第一个方块移动
                                TileMove move1;
                                move1.fromRow = i;
                                move1.fromCol = srcCol;
                                move1.toRow   = i;
                                move1.toCol   = j;
                                move1.merged  = false;
                                moves.append(move1);
                                processed[i][srcCol] = true;

                                // 记录第二个方块移动（被合并的方块）
                                TileMove move2;
                                move2.fromRow = i;
                                move2.fromCol = srcCol + 1;
                                move2.toRow   = i;
                                move2.toCol   = j;
                                move2.merged  = true;
                                moves.append(move2);
                                processed[i][srcCol + 1] = true;
                                break;
                            }
                            // 检查简单移动
                            else if (previousBoard[i][srcCol] == board[i][j] && !processed[i][srcCol]) {
                                TileMove move;
                                move.fromRow = i;
                                move.fromCol = srcCol;
                                move.toRow   = i;
                                move.toCol   = j;
                                move.merged  = false;
                                moves.append(move);
                                processed[i][srcCol] = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // 通知UI更新
        emit boardUpdated();

        // 发送方块移动的信号
        for (int i = 0; i < moves.size(); ++i) {
            TileMove& move = moves[i];
            emit tileMoved(move.fromRow,
                           move.fromCol,
                           move.toRow,
                           move.toCol,
                           previousBoard[move.fromRow][move.fromCol],
                           move.merged);
        }
    }

    return changed;
}

// generateNewTile: 随机选择一个空位置，在该位置生成2或4的新方块
void GameController::generateNewTile(bool animate) {
    QVector<QPair<int, int>> emptyTiles = getEmptyTiles();  // 获取所有空位置

    if (emptyTiles.isEmpty()) {
        return;  // 无空格则不生成新方块
    }

    // 修复类型转换问题，使用static_cast明确转换到int类型
    int randomIndex = static_cast<int>(QRandomGenerator::global()->bounded(static_cast<qint64>(emptyTiles.size())));
    int row         = emptyTiles[randomIndex].first;
    int col         = emptyTiles[randomIndex].second;

    // 90%的概率生成数字2，10%生成数字4
    int newValue = (QRandomGenerator::global()->bounded(10) < 9) ? 2 : 4;

    board[row][col] = newValue;                   // 更新棋盘数据
    emit tileAdded(row, col, newValue, animate);  // 发送信号通知UI更新
}

// isGameOver: 检查游戏是否结束（没有空格且相邻数字均不同）
bool GameController::isGameOver() const {
    // 首先检查是否存在空格
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (board[i][j] == 0) {
                return false;
            }
        }
    }
    // 检查水平方向相邻数字是否相同
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] == board[i][j + 1]) {
                return false;
            }
        }
    }
    // 检查垂直方向相邻数字是否相同
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (board[i][j] == board[i + 1][j]) {
                return false;
            }
        }
    }
    return true;  // 没有空格且无法合并，游戏结束
}

// isGameWon: 检查是否有方块达到2048，即玩家是否获胜
bool GameController::isGameWon() const {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (board[i][j] == 2048) {
                return true;
            }
        }
    }
    return false;  // 没有达到2048则继续游戏
}

// isTileEmpty: 检查指定位置是否为空（即存储数字为0）
bool GameController::isTileEmpty(int row, int col) const {
    // 检查索引是否有效
    if (row < 0 || row >= board.size() || col < 0 || col >= board[0].size()) {
        return false;  // 越界则返回false
    }
    return board[row][col] == 0;
}

// getEmptyTiles: 遍历棋盘，返回所有空位置的行列坐标
QVector<QPair<int, int>> GameController::getEmptyTiles() const {
    QVector<QPair<int, int>> emptyTiles;

    for (int row = 0; row < board.size(); ++row) {
        for (int col = 0; col < board[row].size(); ++col) {
            if (board[row][col] == 0) {
                emptyTiles.append(qMakePair(row, col));
            }
        }
    }

    return emptyTiles;
}

// isMoveAvailable: 检查是否有可用的移动
bool GameController::isMoveAvailable() const {
    // 如果有空格，则可以移动
    if (!getEmptyTiles().isEmpty()) {
        return true;
    }

    // 如果没有空格，检查相邻的方块是否有相同的值
    // 检查水平方向
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {  // 修复j的范围，应该是0到2
            if (board[i][j] == board[i][j + 1]) {
                return true;
            }
        }
    }

    // 检查垂直方向
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (board[i][j] == board[i + 1][j]) {
                return true;
            }
        }
    }

    // 没有可用的移动
    return false;
}

// 撤销最后一步操作
void GameController::undo() {
    if (history.isEmpty()) {
        return;  // 没有历史记录则不做处理
    }

    // 从历史记录中恢复上一步的棋盘状态和分数，使用智能指针
    auto lastState = history.last();
    board          = lastState->first;   // 恢复棋盘状态
    score          = lastState->second;  // 恢复分数
    history.pop_back();                  // 移除最后一步记录

    emit scoreUpdated(score);  // 恢复显示分数
    emit boardUpdated();       // 通知UI更新棋盘
}

// toggleAutoPlay: 切换自动操作状态
void GameController::toggleAutoPlay() {
    autoPlayActive = !autoPlayActive;

    if (autoPlayActive) {
        // 开始异步计算最佳移动
        startAiCalculation();

        // 启动定时器
        autoPlayTimer->start();

        emit statusUpdated("Auto play started");
    } else {
        // 停止定时器
        autoPlayTimer->stop();

        // 停止超时定时器
        aiTimeoutTimer->stop();

        // 重置计算状态
        QMutexLocker locker(&aiMutex);
        aiCalculating = false;

        emit statusUpdated("Auto play stopped");
    }
}

// autoPlayStep: 执行一步自动操作
void GameController::autoPlayStep() {
    // 检查是否有可用的移动
    if (!isMoveAvailable()) {
        qDebug() << "No moves available, stopping auto play";
        autoPlayActive = false;
        autoPlayTimer->stop();
        emit gameOver();
        return;
    }

    // 如果游戏结束，不执行操作
    if (isGameOver()) {
        qDebug() << "Game over, skipping auto play step";
        return;
    }

    // 检查是否正在计算中
    QMutexLocker locker(&aiMutex);
    if (aiCalculating) {
        // 如果正在计算，不重复启动新的计算
        qDebug() << "AI calculation in progress, skipping auto play step";
        return;
    }
    locker.unlock();

    // 输出当前棋盘状态信息
    int emptyCount = static_cast<int>(getEmptyTiles().size());
    qDebug() << "Auto play step - Empty tiles:" << emptyCount;

    // 得到最佳移动
    int bestDirection = findBestMove();
    qDebug() << "Best direction:" << bestDirection;

    if (bestDirection != -1) {
        bool moved = moveTiles(bestDirection);
        qDebug() << "Move successful:" << moved;

        // 如果移动成功，生成新的数字块
        if (moved) {
            // 生成新方块
            generateNewTile(true);
            qDebug() << "New tile generated";

            // 检查游戏状态
            if (isGameWon()) {
                // 达到2048后只弹出一次提示
                if (!winAlertShown) {
                    // 暂停自动操作并保存状态
                    bool wasAutoPlaying = autoPlayActive;
                    if (autoPlayActive) {
                        autoPlayTimer->stop();
                    }

                    emit gameWon();  // 发送游戏胜利信号

                    // 如果用户选择继续游戏且之前处于自动操作状态，恢复自动操作
                    if (wasAutoPlaying && autoPlayActive) {
                        qDebug() << "Resuming auto play after win";
                        startAiCalculation();
                        autoPlayTimer->start();
                    }
                } else {
                    // 已经显示过胜利提示，继续计算下一步
                    qDebug() << "Continuing after 2048 - Starting next AI calculation";
                    startAiCalculation();
                }
            } else if (isGameOver()) {
                qDebug() << "Game over detected";
                // 游戏结束时停止自动操作
                autoPlayActive = false;
                autoPlayTimer->stop();
                emit gameOver();
            } else {
                // 如果游戏未结束，开始计算下一步的最佳移动
                qDebug() << "Starting next AI calculation";
                startAiCalculation();
            }
        } else {
            // 如果移动不成功，开始计算下一步的最佳移动
            qDebug() << "Move was not successful, trying another direction";
            startAiCalculation();
        }
    } else {
        // 如果没有有效移动，开始计算下一步的最佳移动
        qDebug() << "No valid direction found, trying again";
        startAiCalculation();
    }
}

// findBestMove: 找出最佳移动方向
int GameController::findBestMove() {
    // 如果已经在计算中，返回上次计算的结果
    QMutexLocker locker(&aiMutex);
    if (aiCalculating) {
        // 如果还没有计算结果，返回随机方向
        if (aiCalculatedMove == -1) {
            return QRandomGenerator::global()->bounded(4);
        }
        return aiCalculatedMove;
    }

    // 将当前棋盘状态传递给AI进行计算
    return autoPlayer->findBestMove(board);
}

// startAiCalculation: 开始异步AI计算
void GameController::startAiCalculation() {
    QMutexLocker locker(&aiMutex);

    // 如果已经在计算中，不重复启动
    if (aiCalculating) {
        return;
    }

    // 标记正在计算
    aiCalculating    = true;
    aiCalculatedMove = -1;

    // 清除expectimax缓存，提高性能
    autoPlayer->clearExpectimaxCache();

    // 复制当前棋盘状态供异步计算使用
    QVector<QVector<int>> boardCopy = board;

    // 启动超时定时器
    aiTimeoutTimer->start();

    // 使用QtConcurrent在单独线程中计算最佳移动
    aiFuture = QtConcurrent::run([this, boardCopy]() -> int {
        // 计算最佳移动
        int bestMove = autoPlayer->findBestMove(boardCopy);

        // 存储计算结果
        QMutexLocker locker(&aiMutex);
        aiCalculatedMove = bestMove;

        // 使用Qt5兼容的方式触发计算完成信号
        QMetaObject::invokeMethod(this, "onAiCalculationFinished", Qt::QueuedConnection);

        return bestMove;
    });
}

// onAiCalculationFinished: 处理AI计算完成
void GameController::onAiCalculationFinished() {
    // 停止超时定时器
    aiTimeoutTimer->stop();

    QMutexLocker locker(&aiMutex);
    aiCalculating = false;

    // 如果自动操作仍然活跃，继续下一步
    if (autoPlayActive) {
        QTimer::singleShot(50, this, SLOT(autoPlayStep()));
    }
}

// onAiCalculationTimeout: 处理AI计算超时
void GameController::onAiCalculationTimeout() {
    QMutexLocker locker(&aiMutex);

    // 如果计算仍然在进行，但超时了
    if (aiCalculating) {
        // 使用随机方向作为备选
        aiCalculatedMove = QRandomGenerator::global()->bounded(4);

        // 不等待计算完成，直接继续
        aiCalculating = false;

        // 如果自动操作仍然活跃，继续下一步
        if (autoPlayActive) {
            QTimer::singleShot(50, this, SLOT(autoPlayStep()));
        }
    }
}