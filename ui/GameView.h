#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include "../core/GameBoard.h"
#include "../utils/AnimationManager.h"
#include "TileView.h"

#include <QGridLayout>
#include <QLabel>
#include <QObject>
#include <QPair>
#include <QVector>

class MainWindow;

class GameView : public QObject {
    Q_OBJECT

   public:
    explicit GameView(MainWindow* parent, QGridLayout* boardLayout);
    ~GameView();

    // 初始化界面
    void initializeTiles();

    // 更新界面
    void updateAllTiles(QVector<QVector<int>> const& boardState);
    void updateTile(int row, int col, int value);

    // 播放动画
    void playMoveAnimation(QVector<GameBoard::TileMove> const& moves);
    void playNewTileAnimation(int row, int col);

    // 查询方法
    QPoint getTilePosition(int row, int col) const;
    TileView* getTileView(int row, int col) const;

   signals:
    void animationFinished();  // 动画完成信号

   private:
    MainWindow* mainWindow;
    QGridLayout* boardLayout;
    AnimationManager animator;
    QVector<QVector<TileView*>> tileViews;

    // 动画相关状态
    int pendingAnimations;

    // 清理资源
    void clearTileViews();
};

#endif  // GAMEVIEW_H