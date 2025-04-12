#include "UIManager.h"

#include <QDebug>
#include <QPoint>
#include <QTimer>
#include <cmath>  // 添加数学函数头文件

UIManager::UIManager(QObject* parent)
    : QObject(parent), gameBoard(nullptr), boardLayout(nullptr), animationInProgress(false), pendingAnimations(0) {}

// 初始化棋盘上的图块标签
void UIManager::initializeTiles(QGridLayout* layout, QWidget* parent) {
    boardLayout = layout;
    gameBoard   = parent;

    // 首先清除任何现有的标签
    for (auto& row : tileLabels) {
        for (auto& label : row) {
            if (label) {
                delete label;
                label = nullptr;  // 删除后置空
            }
        }
    }

    tileLabels.clear();    // 清除之前存放标签的容器
    tileLabels.resize(4);  // 为4行预留空间
    for (int i = 0; i < 4; ++i) {
        tileLabels[i].resize(4, nullptr);  // 每一行预留4个空指针(未来存放QLabel)
    }

    // 清空棋盘所使用的布局中的所有项，防止重复添加控件
    QLayoutItem* item = nullptr;  // 初始化为nullptr
    while ((item = boardLayout->takeAt(0)) != nullptr) {
        delete item;  // 删除布局中的旧项
    }

    // 为每个棋盘位置创建新的QLabel，设置初始样式和大小，并添加到布局中
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            QLabel* label = new QLabel(gameBoard);       // 使用明确的类型声明
            label->setAlignment(Qt::AlignCenter);        // 标签内容居中显示
            label->setFixedSize(80, 80);                 // 固定标签大小为80x80像素
            label->setStyleSheet(getTileStyleSheet(0));  // 使用初始值0的样式
            label->setText("");                          // 设置为空字符串

            boardLayout->addWidget(label, row, col);  // 将标签添加到网格布局中的(row, col)位置
            tileLabels[row][col] = label;             // 保存标签引用

            // 初始化为0表示空白方块
            updateTileAppearance(row, col, 0);
        }
    }
}

// 更新指定位置方块的外观
void UIManager::updateTileAppearance(int row, int col, int value) {
    // 检查索引是否有效
    if (row < 0 || row >= tileLabels.size() || col < 0 || col >= tileLabels[0].size()) {
        return;
    }

    QLabel* label = tileLabels[row][col];
    if (label == nullptr) {
        return;
    }

    label->setStyleSheet(getTileStyleSheet(value));  // 根据数字获取对应样式
    if (value == 0) {
        label->setText("");  // 数字为0时显示为空
    } else {
        label->setText(QString::number(value));  // 将数字转为字符串显示
    }
}

// 根据方块的值返回对应的CSS样式
QString UIManager::getTileStyleSheet(int value) const {
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
    int digits
        = (value == 0) ? 0 : static_cast<int>(log10(static_cast<double>(value))) + 1;  // 使用static_cast替代C风格转换
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

// 动画：方块移动
void UIManager::animateTileMovement(int fromRow, int fromCol, int toRow, int toCol, int value, bool merged) {
    if (gameBoard == nullptr || tileLabels[fromRow][fromCol] == nullptr || tileLabels[toRow][toCol] == nullptr) {
        return;
    }

    QPoint from = tileLabels[fromRow][fromCol]->pos();
    QPoint to   = tileLabels[toRow][toCol]->pos();

    // 创建一个临时标签来执行动画
    QLabel* tempLabel = new QLabel(gameBoard);
    tempLabel->setGeometry(
        from.x(), from.y(), tileLabels[fromRow][fromCol]->width(), tileLabels[fromRow][fromCol]->height());
    tempLabel->setAlignment(Qt::AlignCenter);
    tempLabel->setText(QString::number(value));
    tempLabel->setStyleSheet(getTileStyleSheet(value));
    tempLabel->show();

    // 创建移动动画
    QPropertyAnimation* animation = new QPropertyAnimation(tempLabel, "geometry");
    animation->setDuration(100);
    animation->setStartValue(QRect(from.x(), from.y(), tempLabel->width(), tempLabel->height()));
    animation->setEndValue(QRect(to.x(), to.y(), tempLabel->width(), tempLabel->height()));
    animation->setEasingCurve(QEasingCurve::OutQuad);

    pendingAnimations++;
    setAnimationInProgress(true);

    // 动画结束后删除临时标签
    connect(animation, &QPropertyAnimation::finished, [this, tempLabel, toRow, toCol, merged]() {
        tempLabel->deleteLater();
        pendingAnimations--;

        // 如果发生了合并，添加合并动画
        if (merged) {
            animateTileMerge(tileLabels[toRow][toCol]);
        }

        if (pendingAnimations <= 0) {
            setAnimationInProgress(false);
            emit animationsCompleted();
        }
    });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

// 动画：方块合并效果
void UIManager::animateTileMerge(QLabel* label) {
    if (label == nullptr) {
        return;
    }

    pendingAnimations++;
    setAnimationInProgress(true);

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry");
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
    connect(animation, &QPropertyAnimation::finished, [this]() {
        pendingAnimations--;
        if (pendingAnimations <= 0) {
            setAnimationInProgress(false);
            emit animationsCompleted();
        }
    });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

// 动画：新方块生成效果
void UIManager::animateNewTile(int row, int col, int value) {
    if (row < 0 || row >= tileLabels.size() || col < 0 || col >= tileLabels[0].size()
        || tileLabels[row][col] == nullptr) {
        return;
    }

    QLabel* label = tileLabels[row][col];

    // 更新方块外观
    updateTileAppearance(row, col, value);

    pendingAnimations++;
    setAnimationInProgress(true);

    // 创建缩放动画
    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(200);  // 设置动画持续时间为200毫秒
    QRect rect = label->geometry();

    // 从小到大的动画效果
    animation->setKeyValueAt(0, QRect(rect.center().x() - 5, rect.center().y() - 5, 10, 10));
    animation->setKeyValueAt(1, rect);
    animation->setEasingCurve(QEasingCurve::OutBack);  // 使用弹性效果

    // 动画结束时减少计数
    connect(animation, &QPropertyAnimation::finished, [this]() {
        pendingAnimations--;
        if (pendingAnimations <= 0) {
            setAnimationInProgress(false);
            emit animationsCompleted();
        }
    });

    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

// 动画：分数变化效果
void UIManager::animateScoreChange(QLabel* scoreLabel, int addedScore) {
    if (scoreLabel == nullptr || addedScore <= 0) {
        return;
    }

    // 创建一个临时标签显示分数增加值
    QLabel* scoreAddLabel = new QLabel(QString("+%1").arg(addedScore), scoreLabel->parentWidget());
    scoreAddLabel->setStyleSheet("color: #776e65; font-weight: bold; font-size: 18px; background-color: transparent;");

    // 将标签定位在分数显示区域附近
    QPoint scorePos = scoreLabel->mapToParent(QPoint(0, 0));
    scoreAddLabel->move(scorePos.x() + (scoreLabel->width() / 2), scorePos.y());
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

// 设置动画状态
void UIManager::setAnimationInProgress(bool inProgress) {
    animationInProgress = inProgress;
}

// 检查动画是否正在进行
bool UIManager::isAnimationInProgress() const {
    return animationInProgress;
}

// 获取正在进行的动画数量
int UIManager::getPendingAnimations() const {
    return pendingAnimations;
}