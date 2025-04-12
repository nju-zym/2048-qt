#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <QGridLayout>
#include <QLabel>
#include <QObject>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QVector>
#include <cmath>

class UIManager : public QObject {
    Q_OBJECT

   public:
    explicit UIManager(QObject* parent = nullptr);

    // 初始化UI元素
    void initializeTiles(QGridLayout* boardLayout, QWidget* gameBoard);

    // 更新棋盘UI
    void updateTileAppearance(int row, int col, int value);

    // 动画相关
    void animateTileMovement(int fromRow, int fromCol, int toRow, int toCol, int value, bool merged);
    void animateTileMerge(QLabel* label);
    void animateNewTile(int row, int col, int value);
    void animateScoreChange(QLabel* scoreLabel, int addedScore);

    // 获取棋盘样式
    QString getTileStyleSheet(int value) const;

   public slots:
    void setAnimationInProgress(bool inProgress);
    bool isAnimationInProgress() const;
    int getPendingAnimations() const;

   signals:
    void animationsCompleted();

   private:
    // UI元素
    QVector<QVector<QLabel*>> tileLabels;
    QWidget* gameBoard;
    QGridLayout* boardLayout;

    // 动画状态
    bool animationInProgress;
    int pendingAnimations;
};

#endif  // UIMANAGER_H