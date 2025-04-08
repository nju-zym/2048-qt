#include "mainwindow.h"

#include "auto.h"
#include "ui_mainwindow.h"

#include <QCheckBox>
#include <QDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QParallelAnimationGroup>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRect>
#include <QSpinBox>
#include <QTextEdit>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>

namespace {
// 新增静态变量，用于记录是否已弹出胜利提示
bool winAlertShown = false;
}  // namespace

// 构造函数：初始化UI、棋盘和标签，并开始新游戏
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      board(4, QVector<int>(4, 0)),
      score(0),
      bestScore(0),
      animationInProgress(false),
      pendingAnimations(0),
      autoPlayTimer(new QTimer(this)),
      autoPlayActive(false),
      autoPlayer(new Auto()) {
    ui->setupUi(this);  // 初始化UI界面

    setFocusPolicy(Qt::StrongFocus);  // 设置焦点策略，确保窗口能响应键盘事件
    setupBoard();                     // 初始化一个4x4的游戏棋盘，每个元素设为0
    initializeTiles();                // 初始化棋盘上对应的标签，清除旧标签、创建新标签并放到布局中
    startNewGame();                   // 开始一个新的游戏，重置棋盘状态、分数、历史记录并随机生成两个数字

    // 连接自动操作定时器的信号和槽
    autoPlayTimer->setInterval(300);  // 设置定时器间隔为300毫秒
    connect(autoPlayTimer, &QTimer::timeout, this, &MainWindow::autoPlayStep);
}

// 析构函数：清理分配的UI资源
MainWindow::~MainWindow() {
    delete ui;          // 释放UI占用资源
    delete autoPlayer;  // 释放自动操作类资源
}

// setupBoard: 初始化棋盘数据，确保创建了一个4x4的矩阵，并将所有值设为0
void MainWindow::setupBoard() {
    // 确保游戏板是4x4的，并且所有值初始化为0
    board.clear();    // 清除旧棋盘数据
    board.resize(4);  // 设置棋盘行数为4
    for (int i = 0; i < 4; ++i) {
        board[i].resize(4, 0);  // 每一行设置4个元素，并初始为0
    }
}

// initializeTiles: 初始化显示棋盘数字的标签
void MainWindow::initializeTiles() {
    // 首先清除任何现有的标签
    for (auto& row : tileLabels) {
        for (auto& label : row) {
            if (label) {
                delete label;
            }
        }
    }

    tileLabels.clear();    // 清除之前存放标签的容器
    tileLabels.resize(4);  // 为4行预留空间
    for (int i = 0; i < 4; ++i) {
        tileLabels[i].resize(4, nullptr);  // 每一行预留4个空指针(未来存放QLabel)
    }

    // 清空棋盘所使用的布局中的所有项，防止重复添加控件
    QLayoutItem* item;
    while ((item = ui->boardLayout->takeAt(0)) != nullptr) {
        delete item;  // 删除布局中的旧项
    }

    // 为每个棋盘位置创建新的QLabel，设置初始样式和大小，并添加到布局中
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            QLabel* label = new QLabel(ui->gameBoard);
            label->setAlignment(Qt::AlignCenter);        // 标签内容居中显示
            label->setFixedSize(80, 80);                 // 固定标签大小为80x80像素
            label->setStyleSheet(getTileStyleSheet(0));  // 使用初始值0的样式
            label->setText("");                          // 设置为空字符串

            ui->boardLayout->addWidget(label, row, col);  // 将标签添加到网格布局中的(row, col)位置
            tileLabels[row][col] = label;                 // 保存标签引用

            // 更新标签显示，使UI和数据保持一致
            updateTileAppearance(row, col);
        }
    }
}

// startNewGame: 重置游戏状态，清除旧数据，并初始化新的游戏开始状态
void MainWindow::startNewGame() {
    setupBoard();           // 重新初始化棋盘数据
    winAlertShown = false;  // 重置胜利提示标识

    // 如果标签未初始化，则先调用initializeTiles()初始化
    if (tileLabels.isEmpty()) {
        initializeTiles();
    }

    // 更新每个标签的外观（初始状态为空白方块）
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            updateTileAppearance(i, j);
        }
    }

    score = 0;       // 重置当前分数为0
    updateScore(0);  // 更新分数显示

    history.clear();  // 清除保存的历史状态

    // 随机生成两个初始方块，不使用动画效果
    generateNewTile(false);
    generateNewTile(false);

    // 更新状态消息显示，让玩家知道游戏开始了
    updateStatus("Join the tiles, get to 2048!");
}

// on_newGameButton_clicked: 新游戏按钮的槽函数，调用startNewGame重置游戏
void MainWindow::on_newGameButton_clicked() {
    startNewGame();
}

// on_undoButton_clicked: 撤销按钮的槽函数，恢复到上一步的棋盘状态及分数
void MainWindow::on_undoButton_clicked() {
    if (history.isEmpty()) {
        return;  // 没有历史记录则不做处理
    }

    // 从历史记录中恢复上一步的棋盘状态和分数
    QPair<QVector<QVector<int>>, int> lastState = history.last();
    QVector<QVector<int>> lastBoard             = lastState.first;
    int lastScore                               = lastState.second;
    history.pop_back();  // 移除最后一步记录

    board = lastBoard;       // 恢复棋盘状态
    updateScore(lastScore);  // 恢复显示分数

    // 更新每个标签的外观，使UI与恢复后的棋盘数据一致
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            updateTileAppearance(i, j);
        }
    }
}

// on_settingsButton_clicked: 设置按钮的槽函数，显示简单的信息对话框
void MainWindow::on_settingsButton_clicked() {
    QMessageBox::information(this, "Settings", "Settings dialog will be implemented here.");
}

// keyPressEvent: 重写键盘按下事件函数，响应上下左右键操作
void MainWindow::keyPressEvent(QKeyEvent* event) {
    // 如果动画正在进行，忽略键盘输入
    if (animationInProgress) {
        return;
    }

    bool moved = false;

    // 根据按键的方向值，调用对应的移动函数
    switch (event->key()) {
        case Qt::Key_Up:
            moved = moveTiles(0);  // 0代表向上移动
            break;
        case Qt::Key_Right:
            moved = moveTiles(1);  // 1代表向右移动
            break;
        case Qt::Key_Down:
            moved = moveTiles(2);  // 2代表向下移动
            break;
        case Qt::Key_Left:
            moved = moveTiles(3);  // 3代表向左移动
            break;
        default:
            // 其他键不处理
            return;
    }

    // 如果棋盘有变化，则生成新的方块并检测游戏结束或胜利条件
    if (moved) {
        // 如果有动画正在进行，等待所有动画完成后再生成新方块
        if (pendingAnimations > 0) {
            animationInProgress = true;
            // 使用单次计时器延迟生成新方块，等待所有动画完成
            QTimer::singleShot(200, this, [this]() {
                generateNewTile(true);  // 随机在空位置生成一个新的数字，使用动画效果

                // 检查游戏状态
                if (isGameWon()) {
                    // 达到2048后只弹出一次提示
                    if (!winAlertShown) {
                        showWinMessage();
                    }
                } else if (isGameOver()) {
                    showGameOverMessage();
                }

                animationInProgress = false;  // 重置动画状态
            });
        } else {
            generateNewTile(true);  // 随机在空位置生成一个新的数字，使用动画效果

            if (isGameWon()) {
                // 达到2048后只弹出一次提示
                if (!winAlertShown) {
                    showWinMessage();
                }
            } else if (isGameOver()) {
                showGameOverMessage();
            }
        }
    }
}

// moveTiles: 根据方向对所有方块进行移动合并操作，并更新分数及UI显示
bool MainWindow::moveTiles(int direction) {
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

    if (changed) {
        history.append(qMakePair(previousBoard, score));  // 保存本次移动前的棋盘状态和分数
        updateScore(score + scoreGained);                 // 更新总分

        // 创建一个数据结构来跟踪每个方块的移动
        struct TileMove {
            int fromRow, fromCol;  // 原始位置
            int toRow, toCol;      // 目标位置
            bool merged;           // 是否发生合并
        };
        QVector<TileMove> tileMoves;

        // 创建一个二维数组来跟踪每个位置的方块是否已经被处理过
        QVector<QVector<bool>> processed(4, QVector<bool>(4, false));

        // 直接比较移动前后的棋盘状态，找出真正移动的方块
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                // 如果当前位置的值在移动前后不同，说明发生了变化
                if (board[i][j] != previousBoard[i][j]) {
                    // 如果当前位置有值，需要找出它是从哪里移动过来的
                    if (board[i][j] != 0) {
                        // 检查是否是合并的结果（值是移动前某个位置值的两倍）
                        bool isMerged = false;

                        // 根据移动方向查找可能的源位置
                        if (direction == 0) {  // 向上移动，检查下方
                            for (int row = i + 1; row < 4 && !isMerged; ++row) {
                                if (previousBoard[row][j] * 2 == board[i][j] && !processed[row][j]) {
                                    // 找到了合并源
                                    TileMove move = {row, j, i, j, true};
                                    tileMoves.append(move);
                                    processed[row][j] = true;
                                    isMerged          = true;
                                }
                            }

                            // 如果不是合并，则是简单的移动
                            if (!isMerged) {
                                for (int row = i + 1; row < 4; ++row) {
                                    if (previousBoard[row][j] == board[i][j] && !processed[row][j]) {
                                        // 找到了移动源
                                        TileMove move = {row, j, i, j, false};
                                        tileMoves.append(move);
                                        processed[row][j] = true;
                                        break;
                                    }
                                }
                            }
                        } else if (direction == 1) {  // 向右移动，检查左方
                            for (int col = j - 1; col >= 0 && !isMerged; --col) {
                                if (previousBoard[i][col] * 2 == board[i][j] && !processed[i][col]) {
                                    // 找到了合并源
                                    TileMove move = {i, col, i, j, true};
                                    tileMoves.append(move);
                                    processed[i][col] = true;
                                    isMerged          = true;
                                }
                            }

                            // 如果不是合并，则是简单的移动
                            if (!isMerged) {
                                for (int col = j - 1; col >= 0; --col) {
                                    if (previousBoard[i][col] == board[i][j] && !processed[i][col]) {
                                        // 找到了移动源
                                        TileMove move = {i, col, i, j, false};
                                        tileMoves.append(move);
                                        processed[i][col] = true;
                                        break;
                                    }
                                }
                            }
                        } else if (direction == 2) {  // 向下移动，检查上方
                            for (int row = i - 1; row >= 0 && !isMerged; --row) {
                                if (previousBoard[row][j] * 2 == board[i][j] && !processed[row][j]) {
                                    // 找到了合并源
                                    TileMove move = {row, j, i, j, true};
                                    tileMoves.append(move);
                                    processed[row][j] = true;
                                    isMerged          = true;
                                }
                            }

                            // 如果不是合并，则是简单的移动
                            if (!isMerged) {
                                for (int row = i - 1; row >= 0; --row) {
                                    if (previousBoard[row][j] == board[i][j] && !processed[row][j]) {
                                        // 找到了移动源
                                        TileMove move = {row, j, i, j, false};
                                        tileMoves.append(move);
                                        processed[row][j] = true;
                                        break;
                                    }
                                }
                            }
                        } else if (direction == 3) {  // 向左移动，检查右方
                            for (int col = j + 1; col < 4 && !isMerged; ++col) {
                                if (previousBoard[i][col] * 2 == board[i][j] && !processed[i][col]) {
                                    // 找到了合并源
                                    TileMove move = {i, col, i, j, true};
                                    tileMoves.append(move);
                                    processed[i][col] = true;
                                    isMerged          = true;
                                }
                            }

                            // 如果不是合并，则是简单的移动
                            if (!isMerged) {
                                for (int col = j + 1; col < 4; ++col) {
                                    if (previousBoard[i][col] == board[i][j] && !processed[i][col]) {
                                        // 找到了移动源
                                        TileMove move = {i, col, i, j, false};
                                        tileMoves.append(move);
                                        processed[i][col] = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 更新所有方块的外观
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                updateTileAppearance(i, j);
            }
        }

        // 执行移动动画
        for (TileMove const& move : tileMoves) {
            if (move.fromRow != move.toRow || move.fromCol != move.toCol) {
                QPoint from = tileLabels[move.fromRow][move.fromCol]->pos();
                QPoint to   = tileLabels[move.toRow][move.toCol]->pos();

                // 使用源位置的标签创建一个临时标签来执行动画
                QLabel* tempLabel = new QLabel(ui->gameBoard);
                tempLabel->setGeometry(from.x(),
                                       from.y(),
                                       tileLabels[move.fromRow][move.fromCol]->width(),
                                       tileLabels[move.fromRow][move.fromCol]->height());
                tempLabel->setAlignment(Qt::AlignCenter);
                tempLabel->setText(QString::number(previousBoard[move.fromRow][move.fromCol]));
                tempLabel->setStyleSheet(getTileStyleSheet(previousBoard[move.fromRow][move.fromCol]));
                tempLabel->show();

                // 创建移动动画
                QPropertyAnimation* animation = new QPropertyAnimation(tempLabel, "geometry");
                animation->setDuration(100);
                animation->setStartValue(QRect(from.x(), from.y(), tempLabel->width(), tempLabel->height()));
                animation->setEndValue(QRect(to.x(), to.y(), tempLabel->width(), tempLabel->height()));
                animation->setEasingCurve(QEasingCurve::OutQuad);

                // 动画结束后删除临时标签
                connect(animation, &QPropertyAnimation::finished, [tempLabel, this, move]() {
                    tempLabel->deleteLater();
                    pendingAnimations--;

                    // 如果发生了合并，添加合并动画
                    if (move.merged) {
                        animateTileMerge(tileLabels[move.toRow][move.toCol]);
                    }
                });

                pendingAnimations++;
                animation->start(QPropertyAnimation::DeleteWhenStopped);
            }
        }
    }

    return changed;
}

// generateNewTile: 随机选择一个空位置，在该位置生成2或4的新方块
void MainWindow::generateNewTile(bool animate) {
    // 确保标签容器已初始化，否则先初始化
    if (tileLabels.isEmpty()) {
        initializeTiles();
    }

    QVector<QPair<int, int>> emptyTiles = getEmptyTiles();  // 获取所有空位置

    if (emptyTiles.isEmpty()) {
        return;  // 无空格则不生成新方块
    }

    int randomIndex = QRandomGenerator::global()->bounded(emptyTiles.size());
    int row         = emptyTiles[randomIndex].first;
    int col         = emptyTiles[randomIndex].second;

    // 90%的概率生成数字2，10%生成数字4
    int newValue = (QRandomGenerator::global()->bounded(10) < 9) ? 2 : 4;

    board[row][col] = newValue;      // 更新棋盘数据
    updateTileAppearance(row, col);  // 更新对应标签的外观

    // 只有在需要动画时才添加动画效果
    if (animate) {
        pendingAnimations++;  // 增加正在进行的动画计数

        QLabel* label                 = tileLabels[row][col];
        QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry", this);
        animation->setDuration(200);  // 设置动画持续时间为200毫秒
        QRect rect = label->geometry();

        // 从小到大的动画效果
        animation->setKeyValueAt(0, QRect(rect.center().x() - 5, rect.center().y() - 5, 10, 10));
        animation->setKeyValueAt(1, rect);
        animation->setEasingCurve(QEasingCurve::OutBack);  // 使用弹性效果

        // 动画结束时减少计数
        connect(animation, &QPropertyAnimation::finished, [this]() { pendingAnimations--; });

        animation->start(QPropertyAnimation::DeleteWhenStopped);
    }
}

// updateTileAppearance: 根据棋盘数据更新指定位置标签的样式和显示数字
void MainWindow::updateTileAppearance(int row, int col) {
    // 检查索引是否有效
    if (row < 0 || row >= board.size() || col < 0 || col >= board[0].size()) {
        return;
    }

    // 确保tileLabels已初始化且索引有效
    if (tileLabels.size() <= row || tileLabels[row].size() <= col || !tileLabels[row][col]) {
        return;
    }

    int value     = board[row][col];
    QLabel* label = tileLabels[row][col];

    label->setStyleSheet(getTileStyleSheet(value));  // 根据数字获取对应样式
    if (value == 0) {
        label->setText("");  // 数字为0时显示为空
    } else {
        label->setText(QString::number(value));  // 将数字转为字符串显示
    }
}

// updateScore: 更新当前分数并刷新UI显示，同时更新最佳分数
void MainWindow::updateScore(int newScore) {
    // 如果分数增加，添加动画效果
    if (newScore > score) {
        // 创建一个临时标签显示分数增加值
        QLabel* scoreAddLabel = new QLabel(QString("+%1").arg(newScore - score), this);
        scoreAddLabel->setStyleSheet(
            "color: #776e65; font-weight: bold; font-size: 18px; background-color: transparent;");

        // 将标签定位在分数显示区域附近
        QPoint scorePos = ui->scoreValue->mapToParent(QPoint(0, 0));
        scoreAddLabel->move(scorePos.x() + ui->scoreValue->width() / 2, scorePos.y());
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
        connect(animGroup, &QParallelAnimationGroup::finished, [scoreAddLabel]() { scoreAddLabel->deleteLater(); });

        animGroup->start(QAbstractAnimation::DeleteWhenStopped);
    }

    score = newScore;
    ui->scoreValue->setText(QString::number(score));  // 显示当前分数

    if (score > bestScore) {
        bestScore = score;
        ui->bestValue->setText(QString::number(bestScore));  // 如果当前分数高于最佳分数则更新最佳分数显示
    }
}

// updateStatus: 更新状态栏显示的文本信息
void MainWindow::updateStatus(QString const& message) {
    ui->statusLabel->setText(message);
}

// getTileStyleSheet: 根据方块的值返回对应的样式字符串（包括背景色、字体颜色、字体大小等）
QString MainWindow::getTileStyleSheet(int value) const {
    QString style = "QLabel { ";
    // 根据value设定不同的背景颜色和文字样式
    switch (value) {
        case 0:
            style += "background-color: #cdc1b4; ";
            break;
        case 2:
            style += "background-color: #eee4da; color: #776e65; ";
            break;
        case 4:
            style += "background-color: #ede0c8; color: #776e65; ";
            break;
        case 8:
            style += "background-color: #f2b179; color: white; ";
            break;
        case 16:
            style += "background-color: #f59563; color: white; ";
            break;
        case 32:
            style += "background-color: #f67c5f; color: white; ";
            break;
        case 64:
            style += "background-color: #f65e3b; color: white; ";
            break;
        case 128:
            style += "background-color: #edcf72; color: white; ";
            break;
        case 256:
            style += "background-color: #edcc61; color: white; ";
            break;
        case 512:
            style += "background-color: #edc850; color: white; ";
            break;
        case 1024:
            style += "background-color: #edc53f; color: white; ";
            break;
        case 2048:
            style += "background-color: #edc22e; color: white; ";
            break;
        default:
            style += "background-color: #3c3a32; color: white; ";
    }

    // 根据数字的位数调整字体大小
    int digits = (value == 0) ? 0 : (int)log10(value) + 1;
    switch (digits) {
        case 0:
        case 1:
            style += "font-size: 32px; ";
            break;
        case 2:
            style += "font-size: 28px; ";
            break;
        case 3:
            style += "font-size: 24px; ";
            break;
        case 4:
            style += "font-size: 22px; ";
            break;
        default:
            style += "font-size: 18px; ";
    }
    // 设置边框圆角和字体风格
    style += "border-radius: 6px; ";
    style += "border: none; ";
    style += "font-weight: bold; ";
    style += "font-family: 'Arial'; ";
    style += "text-align: center; ";
    style += "}";

    return style;
}

// animateTileMovement: 为移动的方块添加动画效果
void MainWindow::animateTileMovement(QLabel* label, QPoint const& from, QPoint const& to) {
    pendingAnimations++;  // 增加正在进行的动画计数

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry", this);
    animation->setDuration(100);  // 设置动画持续时间为100毫秒
    animation->setStartValue(QRect(from.x(), from.y(), label->width(), label->height()));
    animation->setEndValue(QRect(to.x(), to.y(), label->width(), label->height()));
    animation->setEasingCurve(QEasingCurve::OutQuad);  // 使用缓出效果，使动画更自然

    // 动画结束时减少计数
    connect(animation, &QPropertyAnimation::finished, [this]() { pendingAnimations--; });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

// animateTileMerge: 为合并操作添加动画，先缩小再恢复原状
void MainWindow::animateTileMerge(QLabel* label) {
    pendingAnimations++;  // 增加正在进行的动画计数

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry", this);
    animation->setDuration(150);  // 动画持续150毫秒
    QRect rect = label->geometry();
    animation->setKeyValueAt(0, rect);  // 初始状态
    // 中间状态：缩小后居中
    animation->setKeyValueAt(0.5,
                             QRect(rect.x() + (rect.width() / 8),
                                   rect.y() + (rect.height() / 8),
                                   rect.width() * 3 / 4,
                                   rect.height() * 3 / 4));
    animation->setKeyValueAt(1, rect);  // 恢复原尺寸

    animation->setEasingCurve(QEasingCurve::OutBounce);  // 使用弹跳效果结束动画

    // 动画结束时减少计数
    connect(animation, &QPropertyAnimation::finished, [this]() { pendingAnimations--; });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

// isGameOver: 检查游戏是否结束（没有空格且相邻数字均不同）
bool MainWindow::isGameOver() const {
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
bool MainWindow::isGameWon() const {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (board[i][j] == 2048) {
                return true;
            }
        }
    }
    return false;  // 没有达到2048则继续游戏
}

// showGameOverMessage: 弹出游戏结束提示，并提供重新开始游戏的选项
void MainWindow::showGameOverMessage() {
    QMessageBox msgBox;
    msgBox.setText("Game Over!");
    msgBox.setInformativeText("Do you want to start a new game?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::Yes) {
        startNewGame();
    }
}

// showWinMessage: 弹出游戏胜利提示，并询问是否继续游戏
void MainWindow::showWinMessage() {
    QMessageBox msgBox;
    msgBox.setText("You Win!");
    msgBox.setInformativeText("Do you want to continue playing?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::No) {
        startNewGame();
    } else {
        // 如果选择继续游戏，将标识设置为true，后续操作不再提示
        winAlertShown = true;
        updateStatus("Keep going to get a higher score!");
    }
}

// isTileEmpty: 检查指定位置是否为空（即存储数字为0）
bool MainWindow::isTileEmpty(int row, int col) const {
    // 检查索引是否有效
    if (row < 0 || row >= board.size() || col < 0 || col >= board[0].size()) {
        return false;  // 越界则返回false
    }
    return board[row][col] == 0;
}

// getEmptyTiles: 遍历棋盘，返回所有空位置的行列坐标
QVector<QPair<int, int>> MainWindow::getEmptyTiles() const {
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

// on_autoPlayButton_clicked: 处理自动操作按钮的点击事件
void MainWindow::on_autoPlayButton_clicked() {
    // 切换自动操作状态
    autoPlayActive = !autoPlayActive;

    // 获取自动操作按钮
    QPushButton* autoPlayButton = findChild<QPushButton*>("autoPlayButton");

    if (autoPlayActive) {
        // 开始自动操作
        if (autoPlayButton) {
            autoPlayButton->setText("Stop Auto");
        }
        autoPlayTimer->start();
        updateStatus("Auto play started");
    } else {
        // 停止自动操作
        if (autoPlayButton) {
            autoPlayButton->setText("Auto Play");
        }
        autoPlayTimer->stop();
        updateStatus("Auto play stopped");
    }
}

// autoPlayStep: 执行一步自动操作
void MainWindow::autoPlayStep() {
    // 如果游戏结束或动画正在进行，不执行操作
    if (isGameOver() || animationInProgress) {
        return;
    }

    // 找出最佳移动方向并执行
    int bestDirection = findBestMove();
    if (bestDirection != -1) {
        bool moved = moveTiles(bestDirection);

        // 如果移动成功，生成新的数字块
        if (moved) {
            // 如果有动画正在进行，等待所有动画完成后再生成新方块
            if (pendingAnimations > 0) {
                animationInProgress = true;
                // 使用单次计时器延迟生成新方块，等待所有动画完成
                QTimer::singleShot(200, this, [this]() {
                    generateNewTile(true);  // 生成新方块，使用动画效果

                    // 检查游戏状态
                    if (isGameWon()) {
                        // 达到2048后只弹出一次提示
                        if (!winAlertShown) {
                            showWinMessage();
                        }
                    } else if (isGameOver()) {
                        showGameOverMessage();
                        // 游戏结束时停止自动操作
                        autoPlayActive              = false;
                        QPushButton* autoPlayButton = findChild<QPushButton*>("autoPlayButton");
                        if (autoPlayButton) {
                            autoPlayButton->setChecked(false);
                            autoPlayButton->setText("Auto Play");
                        }
                        autoPlayTimer->stop();
                    }

                    animationInProgress = false;  // 重置动画状态
                });
            } else {
                generateNewTile(true);  // 生成新方块，使用动画效果

                // 检查游戏状态
                if (isGameWon()) {
                    // 达到2048后只弹出一次提示
                    if (!winAlertShown) {
                        showWinMessage();
                    }
                } else if (isGameOver()) {
                    showGameOverMessage();
                    // 游戏结束时停止自动操作
                    autoPlayActive              = false;
                    QPushButton* autoPlayButton = findChild<QPushButton*>("autoPlayButton");
                    if (autoPlayButton) {
                        autoPlayButton->setChecked(false);
                        autoPlayButton->setText("Auto Play");
                    }
                    autoPlayTimer->stop();
                }
            }
        }
    }
}

// findBestMove: 找出最佳移动方向
int MainWindow::findBestMove() {
    // 使用Auto类的findBestMove方法
    return autoPlayer->findBestMove(board);
}

// on_learnButton_clicked: 处理学习按钮的点击事件
void MainWindow::on_learnButton_clicked() {
    // 创建训练设置对话框
    QDialog* settingsDialog = new QDialog(this);
    settingsDialog->setWindowTitle("AI Training Settings");
    settingsDialog->setMinimumSize(300, 200);
    settingsDialog->setWindowFlags(settingsDialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 创建设置控件
    QLabel* populationLabel     = new QLabel("Population Size:", settingsDialog);
    QSpinBox* populationSpinBox = new QSpinBox(settingsDialog);
    populationSpinBox->setRange(10, 200);
    populationSpinBox->setValue(50);
    populationSpinBox->setToolTip("Number of parameter sets to evaluate in each generation");

    QLabel* generationsLabel     = new QLabel("Generations:", settingsDialog);
    QSpinBox* generationsSpinBox = new QSpinBox(settingsDialog);
    generationsSpinBox->setRange(5, 100);
    generationsSpinBox->setValue(30);
    generationsSpinBox->setToolTip("Number of evolutionary generations to run");

    QLabel* simulationsLabel     = new QLabel("Simulations per Set:", settingsDialog);
    QSpinBox* simulationsSpinBox = new QSpinBox(settingsDialog);
    simulationsSpinBox->setRange(5, 50);
    simulationsSpinBox->setValue(15);
    simulationsSpinBox->setToolTip("Number of game simulations to run for each parameter set");

    QCheckBox* saveParamsCheckBox = new QCheckBox("Save parameters after training", settingsDialog);
    saveParamsCheckBox->setChecked(true);

    // 创建按钮
    QPushButton* startButton  = new QPushButton("Start Training", settingsDialog);
    QPushButton* cancelButton = new QPushButton("Cancel", settingsDialog);

    // 布局
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->addWidget(populationLabel, 0, 0);
    gridLayout->addWidget(populationSpinBox, 0, 1);
    gridLayout->addWidget(generationsLabel, 1, 0);
    gridLayout->addWidget(generationsSpinBox, 1, 1);
    gridLayout->addWidget(simulationsLabel, 2, 0);
    gridLayout->addWidget(simulationsSpinBox, 2, 1);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(startButton);

    QVBoxLayout* mainLayout = new QVBoxLayout(settingsDialog);
    mainLayout->addLayout(gridLayout);
    mainLayout->addWidget(saveParamsCheckBox);
    mainLayout->addLayout(buttonLayout);

    // 连接按钮信号
    connect(cancelButton, &QPushButton::clicked, settingsDialog, &QDialog::reject);
    connect(startButton, &QPushButton::clicked, [=]() { settingsDialog->accept(); });

    // 显示设置对话框
    if (settingsDialog->exec() != QDialog::Accepted) {
        settingsDialog->deleteLater();
        return;
    }

    // 获取设置值
    int populationSize = populationSpinBox->value();
    int generations    = generationsSpinBox->value();
    int simulations    = simulationsSpinBox->value();
    bool saveParams    = saveParamsCheckBox->isChecked();

    settingsDialog->deleteLater();

    // 创建训练进度对话框
    QDialog* trainingDialog = new QDialog(this);
    trainingDialog->setWindowTitle("AI Training Progress");
    trainingDialog->setMinimumSize(500, 400);
    trainingDialog->setWindowFlags(trainingDialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 创建进度条和标签
    QProgressBar* progressBar = new QProgressBar(trainingDialog);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);

    QLabel* statusLabel     = new QLabel("Initializing training...", trainingDialog);
    QLabel* generationLabel = new QLabel("Generation: 0/0", trainingDialog);
    QLabel* bestScoreLabel  = new QLabel("Best Score: 0", trainingDialog);

    // 创建参数显示区域
    QGroupBox* paramsGroupBox = new QGroupBox("Current Best Parameters", trainingDialog);
    QGridLayout* paramsLayout = new QGridLayout(paramsGroupBox);
    QVector<QLabel*> paramLabels;
    for (int i = 0; i < 5; i++) {
        QLabel* nameLabel  = new QLabel(QString("Parameter %1:").arg(i + 1));
        QLabel* valueLabel = new QLabel("0.00");
        paramLabels.append(valueLabel);
        paramsLayout->addWidget(nameLabel, i, 0);
        paramsLayout->addWidget(valueLabel, i, 1);
    }

    // 创建训练结果显示区域
    QTextEdit* resultsDisplay = new QTextEdit(trainingDialog);
    resultsDisplay->setReadOnly(true);
    resultsDisplay->setPlaceholderText("Training results will appear here...");

    // 创建取消按钮
    QPushButton* stopButton = new QPushButton("Stop Training", trainingDialog);

    // 布局
    QVBoxLayout* layout = new QVBoxLayout(trainingDialog);
    layout->addWidget(statusLabel);
    layout->addWidget(progressBar);
    layout->addWidget(generationLabel);
    layout->addWidget(bestScoreLabel);
    layout->addWidget(paramsGroupBox);
    layout->addWidget(resultsDisplay);
    layout->addWidget(stopButton);

    // 连接取消按钮
    connect(stopButton, &QPushButton::clicked, trainingDialog, &QDialog::reject);

    // 更新状态显示学习开始
    updateStatus("Learning optimal strategies...");

    // 禁用学习按钮，防止重复点击
    QPushButton* learnButton = findChild<QPushButton*>("learnButton");
    if (learnButton) {
        learnButton->setEnabled(false);
    }

    // 显示训练对话框
    trainingDialog->show();

    // 连接训练进度信号
    TrainingProgress* trainingProgress = autoPlayer->getTrainingProgress();

    connect(trainingProgress,
            &TrainingProgress::progressUpdated,
            [=](int generation, int totalGenerations, int bestScore, QVector<double> const& bestParams) {
                // 更新进度条
                int progress = (generation * 100) / totalGenerations;
                progressBar->setValue(progress);

                // 更新标签
                statusLabel->setText(QString("Training generation %1 of %2...").arg(generation).arg(totalGenerations));
                generationLabel->setText(QString("Generation: %1/%2").arg(generation).arg(totalGenerations));
                bestScoreLabel->setText(QString("Best Score: %1").arg(bestScore));

                // 更新参数显示
                for (int i = 0; i < bestParams.size() && i < paramLabels.size(); i++) {
                    paramLabels[i]->setText(QString::number(bestParams[i], 'f', 2));
                }

                // 添加训练结果到显示区域
                resultsDisplay->append(QString("Generation %1: Best Score = %2").arg(generation).arg(bestScore));
            });

    connect(trainingProgress,
            &TrainingProgress::trainingCompleted,
            [this, statusLabel, progressBar, resultsDisplay, stopButton, learnButton, saveParams](
                int finalScore, QVector<double> const& finalParams) {
                // 更新状态
                statusLabel->setText("Training completed!");
                progressBar->setValue(100);

                // 显示最终结果
                resultsDisplay->append("\nTraining completed!");
                resultsDisplay->append(QString("Final Best Score: %1").arg(finalScore));
                resultsDisplay->append("\nFinal Parameters:");
                for (int i = 0; i < finalParams.size(); i++) {
                    resultsDisplay->append(QString("Parameter %1: %2").arg(i + 1).arg(finalParams[i], 0, 'f', 2));
                }

                // 保存参数
                if (saveParams) {
                    if (autoPlayer->saveParameters()) {
                        resultsDisplay->append("\nParameters saved successfully.");
                    } else {
                        resultsDisplay->append("\nFailed to save parameters.");
                    }
                }

                // 更改按钮文本
                stopButton->setText("Close");

                // 更新状态
                updateStatus("Learning completed! Auto play will now use learned strategies.");

                // 重新启用学习按钮
                if (learnButton) {
                    learnButton->setEnabled(true);
                }
            });

    // 连接对话框关闭信号
    connect(trainingDialog, &QDialog::finished, [=]() {
        // 确保学习按钮被重新启用
        if (learnButton) {
            learnButton->setEnabled(true);
        }

        trainingDialog->deleteLater();
    });

    // 使用QTimer延迟执行学习过程，以便 UI 能够更新
    QTimer::singleShot(100, this, [this, populationSize, generations, simulations]() {
        // 调用Auto类的学习方法
        autoPlayer->learnParameters(populationSize, generations, simulations);
    });
}
