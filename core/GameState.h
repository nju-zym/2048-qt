#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <QPair>
#include <QVector>

class GameState {
   public:
    GameState();
    ~GameState() = default;

    // 历史记录管理
    void saveState(QVector<QVector<int>> const& boardState, int score);
    bool canUndo() const;
    QPair<QVector<QVector<int>>, int> undo();
    void clearHistory();

    // 游戏状态标志
    void setWinAlertShown(bool shown);
    bool isWinAlertShown() const;

    // 最高分数管理
    void setBestScore(int score);
    int getBestScore() const;
    void updateBestScore(int currentScore);

   private:
    QVector<QPair<QVector<QVector<int>>, int>> history;  // 历史记录
    bool winAlertShown;                                  // 是否已显示胜利提示
    int bestScore;                                       // 最高分数
};

#endif  // GAMESTATE_H