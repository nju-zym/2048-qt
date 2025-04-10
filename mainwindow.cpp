#include "mainwindow.h"

#include "ui_mainwindow.h"
#include "utils/StyleManager.h"

#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), gameBoard(4), animationInProgress(false) {
    ui->setupUi(this);
    setFocusPolicy(Qt::StrongFocus);  // 确保窗口能响应键盘事件

    initializeTiles();  // 初始化棋盘上对应的标签
    startNewGame();     // 开始一个新的游戏
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::initializeTiles() {
    // 首先清除任何现有的标签
    for (auto& row : tileLabels) {
        for (auto& label : row) {
            if (label) {
                delete label;
            }
        }
    }

    tileLabels.clear();
    tileLabels.resize(4);
    for (int i = 0; i < 4; ++i) {
        tileLabels[i].resize(4, nullptr);
    }

    // 清空棋盘所使用的布局中的所有项
    QLayoutItem* item;
    while ((item = ui->boardLayout->takeAt(0)) != nullptr) {
        delete item;
    }

    // 为每个棋盘位置创建新的QLabel
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            QLabel* label = new QLabel(ui->gameBoard);
            label->setAlignment(Qt::AlignCenter);
            label->setFixedSize(80, 80);
            label->setStyleSheet(StyleManager::getTileStyleSheet(0));
            label->setText("");

            ui->boardLayout->addWidget(label, row, col);
            tileLabels[row][col] = label;

            // 更新标签显示
            updateTileAppearance(row, col);
        }
    }
}

void MainWindow::startNewGame() {
    gameBoard.reset();
    gameState.setWinAlertShown(false);
    gameState.clearHistory();

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            updateTileAppearance(i, j);
        }
    }

    updateScore(0);
    updateStatus("Join the tiles, get to 2048!");

    // 随机生成两个初始方块
    bool animated = false;
    gameBoard.generateNewTile(&animated);
    gameBoard.generateNewTile(&animated);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            updateTileAppearance(i, j);
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (animationInProgress) {
        return;  // 如果动画正在进行，忽略键盘输入
    }

    // 根据按键的方向值，调用对应的移动函数
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
            QMainWindow::keyPressEvent(event);  // 对于其他键，调用默认处理
    }
}

void MainWindow::handleMove(int direction) {
    // 保存当前状态以便撤销
    gameState.saveState(gameBoard.getBoardState(), gameBoard.getScore());

    // 执行移动操作
    bool moved = gameBoard.moveTiles(direction);

    if (moved) {
        // 更新UI
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                updateTileAppearance(i, j);
            }
        }

        updateScore(gameBoard.getScore());

        // 播放移动动画
        QVector<GameBoard::TileMove> const& moves = gameBoard.getLastMoves();
        if (!moves.isEmpty()) {
            animationInProgress = true;

            for (GameBoard::TileMove const& move : moves) {
                if (move.fromRow != move.toRow || move.fromCol != move.toCol) {
                    QPoint from = tileLabels[move.fromRow][move.fromCol]->pos();
                    QPoint to   = tileLabels[move.toRow][move.fromCol]->pos();

                    // 创建临时标签用于动画
                    QLabel* tempLabel = new QLabel(ui->gameBoard);
                    tempLabel->setGeometry(from.x(),
                                           from.y(),
                                           tileLabels[move.fromRow][move.fromCol]->width(),
                                           tileLabels[move.fromRow][move.fromCol]->height());
                    tempLabel->setAlignment(Qt::AlignCenter);
                    tempLabel->setText(QString::number(move.value));
                    tempLabel->setStyleSheet(StyleManager::getTileStyleSheet(move.value));
                    tempLabel->show();

                    // 使用动画管理器执行动画
                    animator.animateTileMovement(tempLabel, from, to, 100, [this, tempLabel, move]() {
                        tempLabel->deleteLater();

                        // 如果是合并，则添加合并动画
                        if (move.merged) {
                            animator.animateTileMerge(tileLabels[move.toRow][move.toCol]);
                        }
                    });
                }
            }

            // 使用定时器在动画完成后继续游戏逻辑
            QTimer::singleShot(200, this, [this]() {
                // 生成新的方块
                bool animated = true;
                gameBoard.generateNewTile(&animated);
                updateTileAppearance(0, 0);  // 更新会刷新所有方块

                // 检查游戏状态
                if (gameBoard.isGameWon()) {
                    handleGameWon();
                } else if (gameBoard.isGameOver()) {
                    handleGameOver();
                }

                animationInProgress = false;
            });
        }
    }
}

void MainWindow::handleGameWon() {
    if (!gameState.isWinAlertShown()) {
        showWinMessage();
    }
}

void MainWindow::handleGameOver() {
    showGameOverMessage();
}

void MainWindow::updateTileAppearance(int row, int col) {
    // 更新所有方块的外观
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int value     = gameBoard.getTileValue(i, j);
            QLabel* label = tileLabels[i][j];

            label->setStyleSheet(StyleManager::getTileStyleSheet(value));
            label->setText(value == 0 ? "" : QString::number(value));
        }
    }
}

void MainWindow::updateScore(int newScore) {
    // 如果分数增加，添加动画效果
    int oldScore      = gameBoard.getScore();
    int scoreIncrease = newScore - oldScore;

    if (scoreIncrease > 0) {
        QPoint scorePos = ui->scoreValue->mapToParent(QPoint(0, 0));
        animator.animateScoreIncrease(this, scorePos, scoreIncrease);
    }

    gameBoard.setScore(newScore);
    ui->scoreValue->setText(QString::number(newScore));

    // 更新最高分
    gameState.updateBestScore(newScore);
    ui->bestValue->setText(QString::number(gameState.getBestScore()));
}

void MainWindow::updateStatus(QString const& message) {
    ui->statusLabel->setText(message);
}

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

void MainWindow::showWinMessage() {
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

void MainWindow::on_newGameButton_clicked() {
    startNewGame();
}

void MainWindow::on_undoButton_clicked() {
    if (gameState.canUndo()) {
        QPair<QVector<QVector<int>>, int> lastState = gameState.undo();
        gameBoard.setBoardState(lastState.first);
        gameBoard.setScore(lastState.second);

        updateScore(lastState.second);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                updateTileAppearance(i, j);
            }
        }
    }
}

void MainWindow::on_settingsButton_clicked() {
    QMessageBox::information(this, "Settings", "Settings dialog will be implemented here.");
}