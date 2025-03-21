#include "mainwindow.h"

#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QRandomGenerator>
#include <QTime>
#include <cmath>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), board(4, QVector<int>(4, 0)), score(0), bestScore(0) {
    ui->setupUi(this);
    setupBoard();
    initializeTiles();  // 确保首先初始化标签
    startNewGame();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupBoard() {
    // 确保游戏板是4x4的，并且所有值初始化为0
    board.clear();
    board.resize(4);
    for (int i = 0; i < 4; ++i) {
        board[i].resize(4, 0);  // 使用第二个参数初始化所有元素为0
    }
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

    // 重新初始化标签容器
    tileLabels.clear();
    tileLabels.resize(4);
    for (int i = 0; i < 4; ++i) {
        tileLabels[i].resize(4, nullptr);
    }

    // 清除现有的布局项
    QLayoutItem* item;
    while ((item = ui->boardLayout->takeAt(0)) != nullptr) {
        delete item;
    }

    // 为每个位置创建新的标签并添加到布局中
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            QLabel* label = new QLabel(ui->gameBoard);
            label->setAlignment(Qt::AlignCenter);
            label->setFixedSize(80, 80);
            label->setStyleSheet(getTileStyleSheet(0));
            label->setText("");

            // 确保按正确的顺序添加到布局中
            ui->boardLayout->addWidget(label, row, col);
            tileLabels[row][col] = label;

            // 确保标签显示正确的内容
            updateTileAppearance(row, col);
        }
    }
}

void MainWindow::startNewGame() {
    // 确保棋盘和UI都被正确初始化
    setupBoard();

    // 如果标签尚未初始化，先初始化它们
    if (tileLabels.isEmpty()) {
        initializeTiles();
    }

    // 确保所有标签显示正确的初始状态（空白）
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            updateTileAppearance(i, j);
        }
    }

    // 重置分数
    score = 0;
    updateScore(0);

    // 清空历史记录
    history.clear();

    // 随机生成初始方块
    generateNewTile();
    generateNewTile();

    // 重置状态
    updateStatus("Join the tiles, get to 2048!");
}

void MainWindow::on_newGameButton_clicked() {
    startNewGame();
}

void MainWindow::on_undoButton_clicked() {
    if (history.isEmpty()) {
        return;
    }

    // 恢复到上一个状态
    QVector<QVector<int>> lastBoard = history.last();
    history.pop_back();

    // 如果有多个历史记录，可以继续恢复分数
    // 这里简化处理，只恢复棋盘状态
    board = lastBoard;

    // 更新UI
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            updateTileAppearance(i, j);
        }
    }
}

void MainWindow::on_settingsButton_clicked() {
    // 这里可以实现设置对话框
    QMessageBox::information(this, "Settings", "Settings dialog will be implemented here.");
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    bool moved = false;

    switch (event->key()) {
        case Qt::Key_Up:
            moved = moveTiles(0);  // 上
            break;
        case Qt::Key_Right:
            moved = moveTiles(1);  // 右
            break;
        case Qt::Key_Down:
            moved = moveTiles(2);  // 下
            break;
        case Qt::Key_Left:
            moved = moveTiles(3);  // 左
            break;
    }

    if (moved) {
        generateNewTile();

        if (isGameWon()) {
            showWinMessage();
        } else if (isGameOver()) {
            showGameOverMessage();
        }
    }
}

bool MainWindow::moveTiles(int direction) {
    // 保存移动前的状态
    QVector<QVector<int>> previousBoard = board;
    int scoreGained                     = 0;

    // 根据方向进行移动
    if (direction == 0) {  // 上移
        // 对每一列从上到下处理
        for (int col = 0; col < 4; col++) {
            // 第一步：移动所有方块到顶部（消除空格）
            int writePos = 0;
            for (int row = 0; row < 4; row++) {
                if (board[row][col] != 0) {
                    if (row != writePos) {
                        board[writePos][col] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos++;
                }
            }

            // 第二步：合并相同的方块
            for (int row = 0; row < 3; row++) {
                if (board[row][col] != 0 && board[row][col] == board[row + 1][col]) {
                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row + 1][col]  = 0;
                }
            }

            // 第三步：再次移动以消除合并后产生的空格
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
    } else if (direction == 1) {  // 右移
        // 对每一行从右到左处理
        for (int row = 0; row < 4; row++) {
            // 第一步：移动所有方块到右侧
            int writePos = 3;
            for (int col = 3; col >= 0; col--) {
                if (board[row][col] != 0) {
                    if (col != writePos) {
                        board[row][writePos] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos--;
                }
            }

            // 第二步：合并相同的方块
            for (int col = 3; col > 0; col--) {
                if (board[row][col] != 0 && board[row][col] == board[row][col - 1]) {
                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row][col - 1]  = 0;
                }
            }

            // 第三步：再次移动以消除合并后产生的空格
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
    } else if (direction == 2) {  // 下移
        // 对每一列从下到上处理
        for (int col = 0; col < 4; col++) {
            // 第一步：移动所有方块到底部
            int writePos = 3;
            for (int row = 3; row >= 0; row--) {
                if (board[row][col] != 0) {
                    if (row != writePos) {
                        board[writePos][col] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos--;
                }
            }

            // 第二步：合并相同的方块
            for (int row = 3; row > 0; row--) {
                if (board[row][col] != 0 && board[row][col] == board[row - 1][col]) {
                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row - 1][col]  = 0;
                }
            }

            // 第三步：再次移动以消除合并后产生的空格
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
    } else if (direction == 3) {  // 左移
        // 对每一行从左到右处理
        for (int row = 0; row < 4; row++) {
            // 第一步：移动所有方块到左侧
            int writePos = 0;
            for (int col = 0; col < 4; col++) {
                if (board[row][col] != 0) {
                    if (col != writePos) {
                        board[row][writePos] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos++;
                }
            }

            // 第二步：合并相同的方块
            for (int col = 0; col < 3; col++) {
                if (board[row][col] != 0 && board[row][col] == board[row][col + 1]) {
                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row][col + 1]  = 0;
                }
            }

            // 第三步：再次移动以消除合并后产生的空格
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

    // 检查游戏板是否有变化
    bool changed = false;
    for (int i = 0; i < 4 && !changed; ++i) {
        for (int j = 0; j < 4 && !changed; ++j) {
            if (board[i][j] != previousBoard[i][j]) {
                changed = true;
            }
        }
    }

    // 如果有变化，保存历史记录并更新UI
    if (changed) {
        history.append(previousBoard);

        // 更新分数
        updateScore(score + scoreGained);

        // 更新所有方块的UI
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                updateTileAppearance(i, j);
            }
        }
    }

    return changed;
}

void MainWindow::generateNewTile() {
    // 确保tileLabels已初始化
    if (tileLabels.isEmpty()) {
        initializeTiles();
    }

    // 获取所有空格子位置
    QVector<QPair<int, int>> emptyTiles = getEmptyTiles();

    // 如果没有空位置，则返回
    if (emptyTiles.isEmpty()) {
        return;
    }

    // 随机选择一个空位置
    int randomIndex = QRandomGenerator::global()->bounded(emptyTiles.size());
    int row         = emptyTiles[randomIndex].first;
    int col         = emptyTiles[randomIndex].second;

    // 生成新值（90%的概率为2，10%的概率为4）
    int newValue = (QRandomGenerator::global()->bounded(10) < 9) ? 2 : 4;

    // 更新游戏逻辑板
    board[row][col] = newValue;

    // 更新UI显示
    updateTileAppearance(row, col);
}

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

    label->setStyleSheet(getTileStyleSheet(value));
    if (value == 0) {
        label->setText("");
    } else {
        label->setText(QString::number(value));
    }
}

void MainWindow::updateScore(int newScore) {
    score = newScore;
    ui->scoreValue->setText(QString::number(score));

    if (score > bestScore) {
        bestScore = score;
        ui->bestValue->setText(QString::number(bestScore));
    }
}

void MainWindow::updateStatus(QString const& message) {
    ui->statusLabel->setText(message);
}

QString MainWindow::getTileStyleSheet(int value) const {
    // 根据方块值设置不同的样式
    QString style = "QLabel { ";

    // 背景色
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

    // 字体大小根据数字长度调整
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

    // 添加边框和圆角
    style += "border-radius: 6px; ";
    style += "border: none; ";
    style += "font-weight: bold; ";
    style += "font-family: 'Arial'; ";
    style += "text-align: center; ";
    style += "}";

    return style;
}

void MainWindow::animateTileMovement(QLabel* label, QPoint const& from, QPoint const& to) {
    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry", this);
    animation->setDuration(100);
    animation->setStartValue(QRect(from.x(), from.y(), label->width(), label->height()));
    animation->setEndValue(QRect(to.x(), to.y(), label->width(), label->height()));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void MainWindow::animateTileMerge(QLabel* label) {
    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry", this);
    animation->setDuration(150);
    QRect rect = label->geometry();

    // 先缩小一点再恢复原状
    animation->setKeyValueAt(0, rect);
    animation->setKeyValueAt(
        0.5,
        QRect(rect.x() + rect.width() / 8, rect.y() + rect.height() / 8, rect.width() * 3 / 4, rect.height() * 3 / 4));
    animation->setKeyValueAt(1, rect);

    animation->setEasingCurve(QEasingCurve::OutBounce);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

bool MainWindow::isGameOver() const {
    // 检查是否还有空白格子
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (board[i][j] == 0) {
                return false;
            }
        }
    }

    // 检查是否有相邻的相同数字
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] == board[i][j + 1]) {
                return false;
            }
        }
    }

    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (board[i][j] == board[i + 1][j]) {
                return false;
            }
        }
    }

    return true;
}

bool MainWindow::isGameWon() const {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (board[i][j] == 2048) {
                return true;
            }
        }
    }
    return false;
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
    }
}

bool MainWindow::isTileEmpty(int row, int col) const {
    // 检查索引是否有效
    if (row < 0 || row >= board.size() || col < 0 || col >= board[0].size()) {
        return false;
    }
    return board[row][col] == 0;
}

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