#include "GameView.h"

#include "../mainwindow.h"
#include "../utils/StyleManager.h"

#include <QMutexLocker>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>

GameView::GameView(MainWindow* parent, QGridLayout* boardLayout)
    : QObject(parent), mainWindow(parent), boardLayout(boardLayout), pendingAnimations(0) {
    // 初始化瓦片视图数组
    tileViews.resize(4);
    for (int i = 0; i < 4; ++i) {
        tileViews[i].resize(4, nullptr);
    }
}

GameView::~GameView() {
    clearTileViews();
}

void GameView::initializeTiles() {
    clearTileViews();

    // 清空布局中的所有项
    QLayoutItem* item;
    while ((item = boardLayout->takeAt(0)) != nullptr) {
        delete item;
    }

    // 创建新的方块视图
    QWidget* gameBoard = qobject_cast<QWidget*>(boardLayout->parent());
    if (!gameBoard) {
        return;
    }

    // 设置游戏板背景色
    gameBoard->setStyleSheet("background-color: #bbada0; border-radius: 6px;");

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            TileView* tileView = new TileView();
            QLabel* label      = new QLabel(gameBoard);
            label->setAlignment(Qt::AlignCenter);
            label->setFixedSize(80, 80);
            label->setMargin(5);  // 添加边距，使方块之间有空隙

            tileView->setLabel(label);
            tileView->setValue(0);  // 显式设置为0，确保应用空白方块样式

            // 添加到布局
            boardLayout->addWidget(label, row, col);
            tileViews[row][col] = tileView;
        }
    }

    // 设置布局的间距
    boardLayout->setSpacing(10);
    boardLayout->setContentsMargins(10, 10, 10, 10);

    // 强制更新布局
    boardLayout->update();
    if (gameBoard) {
        gameBoard->update();
    }
}

void GameView::clearTileViews() {
    for (auto& row : tileViews) {
        for (auto& tileView : row) {
            if (tileView) {
                delete tileView;
            }
        }
    }

    tileViews.clear();
    tileViews.resize(4);
    for (int i = 0; i < 4; ++i) {
        tileViews[i].resize(4, nullptr);
    }
}

void GameView::updateAllTiles(QVector<QVector<int>> const& boardState) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            updateTile(i, j, boardState[i][j]);
        }
    }
}

void GameView::updateTile(int row, int col, int value) {
    if (row < 0 || row >= 4 || col < 0 || col >= 4) {
        return;
    }

    TileView* tileView = tileViews[row][col];
    if (tileView) {
        tileView->setValue(value);
    }
}

QPoint GameView::getTilePosition(int row, int col) const {
    if (row < 0 || row >= 4 || col < 0 || col >= 4) {
        return QPoint();
    }

    TileView* tileView = tileViews[row][col];
    if (tileView && tileView->getLabel()) {
        return tileView->getLabel()->pos();
    }

    return QPoint();
}

TileView* GameView::getTileView(int row, int col) const {
    if (row < 0 || row >= 4 || col < 0 || col >= 4) {
        return nullptr;
    }
    return tileViews[row][col];
}

void GameView::playMoveAnimation(QVector<GameBoard::TileMove> const& moves) {
    QWidget* gameBoard = qobject_cast<QWidget*>(boardLayout->parent());
    if (!gameBoard) {
        return;
    }

    pendingAnimations = 0;

    // 先更新界面，隐藏所有要移动的方块和目标位置的方块
    for (GameBoard::TileMove const& move : moves) {
        if (move.fromRow != move.toRow || move.fromCol != move.toCol) {
            // 安全检查：确保索引有效
            if (move.fromRow < 0 || move.fromRow >= 4 || move.fromCol < 0 || move.fromCol >= 4 || move.toRow < 0
                || move.toRow >= 4 || move.toCol < 0 || move.toCol >= 4) {
                continue;
            }

            // 隐藏起点位置的方块（这样用户只看到移动的临时标签）
            TileView* fromTile = getTileView(move.fromRow, move.fromCol);
            if (fromTile && fromTile->getLabel()) {
                fromTile->setValue(0);  // 设置为0，显示为空白
            }

            // 隐藏目标位置的方块，直到动画完成
            TileView* toTile = getTileView(move.toRow, move.toCol);
            if (toTile && toTile->getLabel()) {
                toTile->setValue(0);  // 设置为0，显示为空白
            }
        }
    }

    // 处理所有移动
    for (GameBoard::TileMove const& move : moves) {
        if (move.fromRow != move.toRow || move.fromCol != move.toCol) {
            // 安全检查：确保索引有效
            if (move.fromRow < 0 || move.fromRow >= 4 || move.fromCol < 0 || move.fromCol >= 4 || move.toRow < 0
                || move.toRow >= 4 || move.toCol < 0 || move.toCol >= 4) {
                continue;
            }

            // 获取源和目标标签
            TileView* fromTile = getTileView(move.fromRow, move.fromCol);
            TileView* toTile   = getTileView(move.toRow, move.toCol);

            if (!fromTile || !toTile || !fromTile->getLabel() || !toTile->getLabel()) {
                continue;
            }

            // 获取位置信息
            QPoint from = fromTile->getLabel()->pos();
            QPoint to   = toTile->getLabel()->pos();

            // 创建临时标签用于动画
            QLabel* tempLabel = new QLabel(gameBoard);
            tempLabel->setFixedSize(fromTile->getLabel()->size());
            tempLabel->setAlignment(Qt::AlignCenter);
            tempLabel->setText(QString::number(move.value));

            // 确保临时标签在最上层显示
            tempLabel->raise();
            tempLabel->setWindowOpacity(1.0);

            // 在设置样式表前确保对象有效
            if (move.value >= 0) {
                QString styleSheet = StyleManager::getTileStyleSheet(move.value);
                QMutexLocker locker(&g_styleSheetMutex);
                tempLabel->setStyleSheet(styleSheet);
            }

            // 将标签移动到起始位置
            tempLabel->move(from);
            tempLabel->show();
            pendingAnimations++;

            // 执行动画 - 对所有移动使用相同的持续时间
            // 对于所有移动（包括非合并移动）都使用动画效果
            animator.animateTileMovement(
                tempLabel,
                from,
                to,
                200,  // 使用适当的动画时间
                [this, tempLabel, row = move.toRow, col = move.toCol, merged = move.merged, value = move.value]() {
                    // 删除临时标签
                    if (tempLabel) {
                        tempLabel->deleteLater();
                    }

                    // 更新目标位置的方块
                    TileView* targetTile = getTileView(row, col);
                    if (targetTile) {
                        // 设置正确的值
                        targetTile->setValue(merged ? value * 2 : value);

                        // 对合并的方块使用合并动画效果
                        if (merged && targetTile->getLabel()) {
                            animator.animateTileMerge(targetTile->getLabel(), 150);
                        }
                    }

                    // 减少待处理的动画计数
                    pendingAnimations--;
                    if (pendingAnimations <= 0) {
                        // 所有动画完成后，发出信号
                        QTimer::singleShot(100, this, [this]() { emit animationFinished(); });
                    }
                });
        }
    }

    // 如果没有动画需要处理，直接发出信号
    if (pendingAnimations == 0) {
        emit animationFinished();
    }
}

void GameView::playNewTileAnimation(int row, int col) {
    if (row < 0 || row >= 4 || col < 0 || col >= 4) {
        return;
    }

    TileView* tileView = getTileView(row, col);
    if (tileView && tileView->getLabel()) {
        animator.animateNewTile(tileView->getLabel());
    }
}