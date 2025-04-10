#include "mainwindow.h"

#include "ai/autoplayer.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QMessageBox>
#include <QParallelAnimationGroup>
#include <QProgressDialog>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QRect>
#include <QTime>
#include <QTimer>
#include <cmath>

namespace {
// 新增静态变量，用于记录是否已弹出胜利提示
bool winAlertShown = false;
}  // namespace

// 构造函数：初始化UI、棋盘和标签，并开始新游戏
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      gameBoard(new Board()),
      score(0),
      bestScore(0),
      animationInProgress(false),
      pendingAnimations(0),
      autoPlayer(nullptr),
      autoPlayTimer(nullptr) {
    ui->setupUi(this);  // 初始化UI界面

    setFocusPolicy(Qt::StrongFocus);  // 设置焦点策略，确保窗口能响应键盘事件
    initializeTiles();                // 初始化棋盘上对应的标签，清除旧标签、创建新标签并放到布局中
    setupAutoPlay();                  // 初始化自动游戏功能
    startNewGame();                   // 开始一个新的游戏，重置棋盘状态、分数、历史记录并随机生成两个数字
}

// 析构函数：清理分配的UI资源
MainWindow::~MainWindow() {
    if (autoPlayTimer) {
        autoPlayTimer->stop();
        delete autoPlayTimer;
    }

    if (autoPlayer) {
        delete autoPlayer;
    }

    if (gameBoard) {
        delete gameBoard;
    }

    delete ui;  // 释放UI占用资源
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
    gameBoard->init();      // 重新初始化棋盘数据
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

    gameBoard->setBoard(lastBoard);  // 恢复棋盘状态
    updateScore(lastScore);          // 恢复显示分数

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
    // 保存移动前的棋盘状态
    QVector<QVector<int>> previousBoard = gameBoard->getBoard();

    // 使用Board类的move方法执行移动
    QPair<bool, int> result = gameBoard->move(direction);
    bool changed            = result.first;
    int scoreGained         = result.second;

    // 如果棋盘发生了变化
    if (changed) {
        history.append(qMakePair(previousBoard, score));  // 保存本次移动前的棋盘状态和分数
        updateScore(score + scoreGained);                 // 更新总分

        // 更新所有方块的显示
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                updateTileAppearance(i, j);
            }
        }

        // 生成新的方块
        generateNewTile();

        // 检查游戏是否结束
        if (isGameOver()) {
            QMessageBox::information(this, "Game Over", "Game Over! No more moves available.");
        }
        // 检查是否达到胜利条件
        else if (isGameWon() && !winAlertShown) {
            winAlertShown = true;  // 设置标志，避免重复显示
            QMessageBox::information(
                this, "Congratulations", "You've reached 2048! Continue playing to get a higher score!");
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

    // 使用Board类生成新方块
    QPair<QPair<int, int>, int> newTile = gameBoard->generateNewTile();
    int row                             = newTile.first.first;
    int col                             = newTile.first.second;
    int newValue                        = newTile.second;

    // 如果无法生成新方块，直接返回
    if (row == -1 || col == -1 || newValue == 0) {
        return;
    }

    // 更新对应标签的外观
    updateTileAppearance(row, col);

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
    if (row < 0 || row >= 4 || col < 0 || col >= 4) {
        return;
    }

    // 确保tileLabels已初始化且索引有效
    if (tileLabels.size() <= row || tileLabels[row].size() <= col || !tileLabels[row][col]) {
        return;
    }

    int value     = gameBoard->getTileValue(row, col);
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
    return gameBoard->isGameOver();
}

// isGameWon: 检查是否有方块达到2048，即玩家是否获胜
bool MainWindow::isGameWon() const {
    return gameBoard->isGameWon();
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
    if (row < 0 || row >= 4 || col < 0 || col >= 4) {
        return false;  // 越界则返回false
    }
    return gameBoard->getTileValue(row, col) == 0;
}

// getEmptyTiles: 遍历棋盘，返回所有空位置的行列坐标
QVector<QPair<int, int>> MainWindow::getEmptyTiles() const {
    return gameBoard->getEmptyTiles();
}

// setupAutoPlay: 初始化自动游戏功能
void MainWindow::setupAutoPlay() {
    // 创建自动游戏对象
    autoPlayer = new AutoPlayer(this);

    // 创建定时器，用于定时执行自动移动
    autoPlayTimer = new QTimer(this);
    autoPlayTimer->setInterval(200);  // 设置间隔为200毫秒

    // 连接定时器信号到自动移动函数
    connect(autoPlayTimer, &QTimer::timeout, this, &MainWindow::performAutoMove);

    // 连接自动游戏对象的信号
    connect(autoPlayer, &AutoPlayer::moveDecided, this, &MainWindow::onMoveDecided);
    connect(autoPlayer, &AutoPlayer::trainingProgress, this, &MainWindow::onTrainingProgress);
    connect(autoPlayer, &AutoPlayer::trainingComplete, this, &MainWindow::onTrainingComplete);
}

// performAutoMove: 执行自动移动
void MainWindow::performAutoMove() {
    // 如果游戏结束，停止自动游戏
    if (isGameOver()) {
        autoPlayTimer->stop();
        autoPlayer->stopAutoPlay();
        updateStatus("Auto play stopped.");
        ui->autoPlayButton->setText("Auto Play");
        return;
    }

    // 如果动画正在进行，等待下一次定时器触发
    if (animationInProgress) {
        return;
    }

    // 请求自动玩家计算下一步移动
    // 移动将通过 onMoveDecided 信号处理
    autoPlayer->getBestMove(gameBoard->getBoard());
}

// on_autoPlayButton_clicked: 自动游戏按钮点击事件
void MainWindow::on_autoPlayButton_clicked() {
    if (autoPlayTimer->isActive()) {
        // 如果定时器正在运行，停止自动游戏
        autoPlayTimer->stop();
        autoPlayer->stopAutoPlay();
        updateStatus("Auto play stopped.");
        ui->autoPlayButton->setText("Auto Play");
    } else {
        // 否则开始自动游戏
        autoPlayer->startAutoPlay();
        autoPlayTimer->start();
        updateStatus("Auto play started.");
        ui->autoPlayButton->setText("Stop Auto");
    }
}

// on_trainButton_clicked: 训练按钮点击事件
void MainWindow::on_trainButton_clicked() {
    if (autoPlayer->isTraining()) {
        // 如果正在训练，停止训练
        autoPlayer->stopTraining();
        updateStatus("Training stopped.");
        ui->trainButton->setText("Train");
    } else {
        // 否则开始训练
        // 创建进度对话框
        QProgressDialog* progressDialog = new QProgressDialog("Training in progress...", "Cancel", 0, 100, this);
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setValue(0);
        progressDialog->show();

        // 连接进度信号
        connect(autoPlayer, &AutoPlayer::trainingProgress, progressDialog, &QProgressDialog::setValue);
        connect(progressDialog, &QProgressDialog::canceled, autoPlayer, &AutoPlayer::stopTraining);

        // 开始训练，使用异步方式
        updateStatus("Training started...");
        ui->trainButton->setText("Stop Training");

        // 在单独的线程中运行训练
        QTimer::singleShot(0, [this, progressDialog]() {
            autoPlayer->startTraining(1000);  // 训练1000次
            progressDialog->deleteLater();
        });
    }
}

// onMoveDecided: 处理自动游戏决定的移动
void MainWindow::onMoveDecided(int direction) {
    // 如果没有有效移动或游戏结束，停止自动游戏
    if (direction == -1 || isGameOver()) {
        autoPlayTimer->stop();
        autoPlayer->stopAutoPlay();
        updateStatus("Auto play stopped.");
        ui->autoPlayButton->setText("Auto Play");
        return;
    }

    // 如果动画正在进行，等待下一次定时器触发
    if (animationInProgress) {
        return;
    }

    // 模拟按键事件
    QKeyEvent* event = nullptr;
    switch (direction) {
        case 0:  // 上
            event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
            break;
        case 1:  // 右
            event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
            break;
        case 2:  // 下
            event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
            break;
        case 3:  // 左
            event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
            break;
        default:
            return;  // 无效方向
    }

    if (event != nullptr) {
        // 处理按键事件
        keyPressEvent(event);
        delete event;
    }
}

// onTrainingProgress: 处理训练进度更新
void MainWindow::onTrainingProgress(int current, int total) {
    // 更新状态栏显示训练进度
    updateStatus(QString("Training progress: %1/%2").arg(current).arg(total));
}

// onTrainingComplete: 处理训练完成
void MainWindow::onTrainingComplete() {
    // 更新UI和状态
    updateStatus("Training complete!");
    ui->trainButton->setText("Train");

    // 显示训练结果消息框
    QMessageBox::information(
        this,
        "Training Complete",
        "Training has been completed successfully. The AI has learned new parameters for better gameplay.");
}
