# 2048-Qt

这是一个使用Qt框架实现的2048游戏，具有美观的界面和流畅的动画效果。

## 项目结构

项目采用模块化设计，将代码按照功能和职责进行分类：

```
2048-qt/
├── CMakeLists.txt          # 项目构建文件
├── main.cpp                # 程序入口
├── mainwindow.cpp          # 主窗口实现
├── mainwindow.h            # 主窗口头文件
├── mainwindow.ui           # 主窗口UI设计文件
├── core/                   # 核心游戏逻辑模块
│   ├── GameBoard.cpp       # 游戏棋盘实现
│   ├── GameBoard.h         # 游戏棋盘头文件
│   ├── GameState.cpp       # 游戏状态管理实现
│   └── GameState.h         # 游戏状态管理头文件
├── utils/                  # 工具类模块
│   ├── AnimationManager.cpp # 动画管理实现
│   ├── AnimationManager.h   # 动画管理头文件
│   ├── StyleManager.cpp     # 样式管理实现
│   └── StyleManager.h       # 样式管理头文件
├── ui/                     # 界面相关模块（待扩展）
└── ai/                     # AI模块（待扩展）
    └── evaluation/         # 评估函数目录
```

## 模块功能说明

### 核心模块 (core/)

- **GameBoard**: 管理游戏棋盘状态和基本操作，如方块的移动、合并和生成新方块
- **GameState**: 管理游戏状态、历史记录和最高分数，支持撤销操作

### 工具模块 (utils/)

- **StyleManager**: 处理方块的样式和颜色，根据方块数值提供相应的样式表
- **AnimationManager**: 管理所有动画效果，包括方块移动、合并、新方块生成和分数增加的动画

### 界面模块 (ui/)

- 预留用于存放更多UI相关的组件和设计

### AI模块 (ai/)

- 预留用于实现AI玩家和算法
- evaluation/ 目录用于存放评估函数相关代码

## 主要功能

- 基本的2048游戏玩法
- 流畅的方块移动和合并动画
- 分数显示和最高分记录
- 撤销操作支持
- 游戏状态提示（游戏胜利、游戏结束）

## 编译与运行

### 依赖项

- Qt 5.x 或 Qt 6.x
- CMake 3.16 或更高版本
- C++17 兼容的编译器

### 编译步骤

1. 克隆仓库：
```
git clone [仓库地址]
cd 2048-qt
```

2. 创建构建目录：
```
mkdir build
cd build
```

3. 配置和编译：
```
cmake ..
cmake --build .
```

4. 运行游戏：
```
./2048-qt
```

## AI自动操作方案

2048游戏虽然规则简单，但要获得高分甚至达到2048以上的数值（如4096、8192等）需要精心设计的AI策略。本项目将实现多种AI算法，以实现自动操作获取高分和合成更大的数字块。

### 1. Expectimax算法

期望搜索算法（Expectimax）是目前表现最好的2048 AI算法之一，能够稳定达到非常高的分数（平均超过38万分），并且有较高概率达到8192甚至更高的方块。

#### 核心思想
- Expectimax是一种在博弈论中用于最大化期望效用的算法，特别适用于对手行动具有随机性的游戏
- 通过构建搜索树模拟未来可能的游戏状态，树中节点分为两种：
  - 最大化节点：代表玩家的回合，目标是最大化期望得分
  - 机会节点：代表计算机放置新方块的回合，计算所有可能结果的期望值
- 考虑随机生成的新方块（2和4）的概率（2出现概率为90%，4出现概率为10%）
- 使用启发式评估函数来评估叶子节点（最终游戏状态）的"好坏程度"

#### 实现步骤
1. 对每个可能的移动（上、下、左、右）进行模拟
2. 对每个移动后可能出现的新方块位置和数值进行期望值计算
3. 递归地探索搜索树，直到达到预定的深度限制（通常为4-8层）
4. 使用启发式函数评估叶子节点
5. 选择期望值最高的移动方向

#### 关键启发式函数设计
- **单调性（Monotonicity）**：鼓励将数值较大的方块排列在棋盘的一侧（通常是边缘或角落），并使数值沿某个方向递减或递增。这有助于将相同数值的方块聚集在一起，更容易合并成更大的数值。
- **平滑性（Smoothness）**：奖励相邻方块之间数值差异较小的状态。数值相近的方块更容易在下一步合并，从而提高得分。
- **空位数量（Free Tiles）**：拥有更多空位的棋盘状态通常被认为更有利。更多的空位意味着有更多的机会来放置新的方块，并进行后续的移动和合并。
- **高价值方块的位置（Tile Placement）**：将高价值的方块放置在棋盘的角落或边缘通常被认为是好的策略。这有助于防止它们被低价值的方块包围而难以合并。

#### 优化技术
- **位棋盘（Bitboard）表示**：使用64位整数编码整个4x4棋盘，每个方块用4位表示，可以在单个机器寄存器中传递整个棋盘
- **查找表（Lookup Tables）**：预计算所有可能的行/列移动和评分结果，通过表查找加速计算
- **转置表（Transposition Table）**：缓存已经计算过的棋盘状态，避免重复计算

### 2. 蒙特卡洛树搜索（MCTS）

蒙特卡洛树搜索是一种基于随机抽样的搜索算法，通过重复进行随机模拟来估计动作的价值，从而在游戏中做出决策。这是一种不需要特定领域知识的通用算法，在2048游戏中表现也相当不错。

#### 核心思想
MCTS算法主要包含四个阶段，并在每一步迭代中重复执行：
- **选择（Selection）**：从根节点（当前游戏状态）开始，根据UCT（上限置信区间算法）选择一个子节点进行探索
- **扩展（Expansion）**：如果选择的节点存在未尝试过的移动，则创建一个新的子节点
- **模拟（Simulation/Rollout）**：从新扩展的节点开始，进行随机的模拟对弈，直到游戏结束或达到预定的搜索深度
- **回溯（Backpropagation）**：将获得的奖励沿着搜索路径反向传播回所有经过的节点

#### 实现步骤
1. 从当前状态开始，对每个可能的移动（上、下、左、右）构建搜索树
2. 重复执行选择、扩展、模拟和回溯四个阶段，直到达到预定的迭代次数
3. 选择访问次数最多或平均得分最高的子节点对应的移动

#### 纯蒙特卡洛方法
一种更简单的变体是纯蒙特卡洛方法：
1. 对每个可能的移动（上、下、左、右）进行多次随机游戏模拟
2. 记录每次模拟的最终得分
3. 选择平均得分最高的移动方向

这种方法虽然简单，但在2048游戏中也能取得不错的效果，能够达到70,000+的平均分数，80%的概率达到2048，50%的概率达到4096。

### 3. 强化学习方法

强化学习（Reinforcement Learning, RL）是一种机器学习方法，通过让智能体（Agent）在环境中执行动作并接收奖励或惩罚来学习最优策略。

#### 核心思想
- 将2048游戏建模为马尔可夫决策过程（MDP）
  - 状态：游戏棋盘的当前配置
  - 动作：上下左右四个滑动方向
  - 奖励：合并方块时获得的得分
  - 转移概率：由玩家的动作和环境随机生成新方块的机制决定
- 训练智能体学习在任何给定的棋盘状态下采取哪个动作才能最大化未来的总奖励

#### 实现方法
- **深度Q网络（Deep Q-Networks, DQN）**：
  - 使用神经网络近似Q函数，输入为棋盘状态，输出为每个动作的Q值
  - 使用经验回放（Experience Replay）存储和重用过去的经验
  - 使用ε-贪婪策略（ε-greedy）平衡探索和利用

- **奖励函数设计**：
  - 基础奖励：每次合并方块时获得的得分
  - 额外奖励：当合成出更大数值的方块时给予更高奖励
  - 惩罚：对无法进行有效移动或导致游戏过早结束的状态施加惩罚

### 4. 角落策略

这是一种简单但有效的启发式策略，适合作为基础实现或与其他算法结合使用。

#### 核心思想
- 将最大的方块保持在棋盘的一个角落（通常是左上角或右上角）
- 沿着两条边形成递减的序列，形成"蛇形"排列
- 尽量避免使用一个方向的移动（例如向下）

#### 实现步骤
1. 优先选择能够保持最大方块在角落的移动
2. 其次选择能够保持方块沿边递减排列的移动
3. 尽量避免使用特定方向的移动

### 算法性能对比

| 算法           | 平均分数  | 达到2048概率 | 达到4096概率 | 达到8192概率 | 达到16384概率 | 达到32768概率 | 计算复杂度 |
| -------------- | --------- | ------------ | ------------ | ------------ | ------------- | ------------- | ---------- |
| Expectimax     | 387,000+  | 100%         | 100%         | 100%         | 94%           | 36%           | 高         |
| 蒙特卡洛       | 70,000+   | 80%          | 50%          | <1%          | <0.1%         | <0.01%        | 中         |
| 强化学习(DQN)  | 100,000+  | 90%          | 70%          | 10%          | <1%           | <0.1%         | 高         |
| 角落策略       | 20,000+   | 60%          | <10%         | <1%          | <0.1%         | <0.01%        | 低         |

### 超越2048的关键策略

要达到4096、8192甚至更高的方块数值，需要结合以下关键策略：

1. **增加搜索深度**：对于Expectimax算法，增加搜索深度（通常为8层以上）可以显著提高性能，但需要更多的计算资源

2. **优化启发式函数**：精心设计和调整启发式函数的权重，特别是单调性和平滑性的权重

3. **高效的实现**：使用位棋盘表示、查找表和转置表等优化技术提高算法效率

4. **角落策略与蛇形排列**：始终尝试将数值最大的方块保持在棋盘的一个角落，并围绕这个角落构建其他方块

5. **自适应启发式方法**：根据当前的游戏状态动态调整不同启发式策略的权重

### 实现计划

在我们的AI模块中，我们将实现以上算法：

```
ai/
├── ExpectimaxAI.cpp        # Expectimax算法实现
├── ExpectimaxAI.h          # Expectimax算法头文件
├── MonteCarloAI.cpp        # 蒙特卡洛算法实现
├── MonteCarloAI.h          # 蒙特卡洛算法头文件
├── RLAI.cpp                # 强化学习算法实现
├── RLAI.h                  # 强化学习算法头文件
├── CornerAI.cpp            # 角落策略实现
├── CornerAI.h              # 角落策略头文件
├── AIInterface.h           # AI接口定义
├── BitBoard.cpp            # 位棋盘表示实现
├── BitBoard.h              # 位棋盘表示头文件
└── evaluation/             # 评估函数目录
    ├── MonotonicityEval.cpp # 单调性评估
    ├── SmoothnessEval.cpp   # 平滑性评估
    ├── FreeTilesEval.cpp    # 空位数量评估
    ├── MergeEval.cpp        # 合并可能性评估
    └── TilePlacementEval.cpp # 方块位置评估
```

### 代码示例

以下是Expectimax算法的核心实现示例（伪代码）：

```cpp
// 期望搜索算法的核心函数
float expectimax(BitBoard board, int depth, bool maximizingPlayer) {
    // 达到搜索深度限制或游戏结束
    if (depth == 0 || isGameOver(board)) {
        return evaluateBoard(board);
    }

    if (maximizingPlayer) {
        // 玩家回合，尝试所有可能的移动
        float bestScore = -INFINITY;
        for (Move move : {UP, DOWN, LEFT, RIGHT}) {
            BitBoard newBoard = applyMove(board, move);
            if (newBoard != board) { // 确保移动有效
                float score = expectimax(newBoard, depth - 1, false);
                bestScore = max(bestScore, score);
            }
        }
        return bestScore;
    } else {
        // 计算机回合，随机放置新方块
        float expectedScore = 0;
        vector<Position> emptyPositions = getEmptyPositions(board);

        if (emptyPositions.empty()) {
            return evaluateBoard(board);
        }

        float probability = 1.0f / emptyPositions.size();

        for (Position pos : emptyPositions) {
            // 90%概率放置2
            BitBoard boardWith2 = placeNewTile(board, pos, 2);
            expectedScore += 0.9f * probability * expectimax(boardWith2, depth - 1, true);

            // 10%概率放置4
            BitBoard boardWith4 = placeNewTile(board, pos, 4);
            expectedScore += 0.1f * probability * expectimax(boardWith4, depth - 1, true);
        }

        return expectedScore;
    }
}

// 评估函数
float evaluateBoard(BitBoard board) {
    float score = 0;

    // 单调性评估
    score += MONOTONICITY_WEIGHT * evaluateMonotonicity(board);

    // 平滑性评估
    score += SMOOTHNESS_WEIGHT * evaluateSmoothness(board);

    // 空位数量评估
    score += FREE_TILES_WEIGHT * countEmptyTiles(board);

    // 方块位置评估
    score += TILE_PLACEMENT_WEIGHT * evaluateTilePlacement(board);

    return score;
}
```

## 未来计划

- [ ] 添加设置对话框
- [ ] 实现主题切换功能
- [ ] 添加本地存储功能，保存最高分和游戏设置
- [ ] 实现AI模式
- [ ] 添加更多动画和视觉效果
- [ ] 优化AI算法性能
- [ ] 添加AI策略可视化功能

## 如何贡献

欢迎提交问题报告和合并请求！请确保遵循项目的代码风格和贡献指南。

## 许可证

[添加适当的许可证信息]
