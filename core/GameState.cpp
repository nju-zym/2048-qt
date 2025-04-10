#include "GameState.h"

GameState::GameState() : winAlertShown(false), bestScore(0) {}

void GameState::saveState(QVector<QVector<int>> const& boardState, int score) {
    history.append(qMakePair(boardState, score));
}

bool GameState::canUndo() const {
    return !history.isEmpty();
}

QPair<QVector<QVector<int>>, int> GameState::undo() {
    if (history.isEmpty()) {
        // 返回一个空棋盘和0分作为默认值
        return qMakePair(QVector<QVector<int>>(), 0);
    }

    QPair<QVector<QVector<int>>, int> lastState = history.last();
    history.pop_back();
    return lastState;
}

void GameState::clearHistory() {
    history.clear();
}

void GameState::setWinAlertShown(bool shown) {
    winAlertShown = shown;
}

bool GameState::isWinAlertShown() const {
    return winAlertShown;
}

void GameState::setBestScore(int score) {
    bestScore = score;
}

int GameState::getBestScore() const {
    return bestScore;
}

void GameState::updateBestScore(int currentScore) {
    if (currentScore > bestScore) {
        bestScore = currentScore;
    }
}