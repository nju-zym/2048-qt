# 2048 Game with AI

A Qt-based implementation of the popular 2048 game with an intelligent AI that can play automatically.

## Project Structure

The project is organized into several directories:

- **core/**: Contains the core game logic
  - `board.h/cpp`: Implements the game board and game rules

- **ui/**: Contains the user interface components
  - `mainwindow.h/cpp/ui`: Implements the main game window and UI interactions

- **ai/**: Contains the AI implementation for auto-play
  - `autoplayer.h/cpp`: Implements the auto-play functionality
  - `bitboard.h/cpp`: Implements a bit-based board representation for efficient AI operations
  - `expectimax.h/cpp`: Implements the Expectimax algorithm for decision making
  - **ai/evaluation/**: Contains various evaluation functions used by the AI
    - `evaluation.h/cpp`: Main evaluation framework
    - `monotonicity.h/cpp`: Evaluates the monotonicity of the board (tiles increasing/decreasing in order)
    - `smoothness.h/cpp`: Evaluates the smoothness of the board (adjacent tiles having similar values)
    - `snake.h/cpp`: Evaluates the board based on a snake-like pattern
    - `tile.h/cpp`: Evaluates the board based on tile values and positions
    - `merge.h/cpp`: Evaluates potential merges on the board

- **utils/**: Contains utility functions
  - `animation.h/cpp`: Implements animations for the game

## Features

- Classic 2048 gameplay
- Smooth animations
- Auto-play functionality with AI
- High-performance AI using Expectimax algorithm
- Multiple evaluation strategies for optimal play

## AI Strategy

The AI uses a combination of strategies to determine the best move:

1. **Expectimax Algorithm**: Looks ahead several moves to find the optimal path
2. **Bitboard Representation**: Uses a compact bit-based representation for efficient operations
3. **Evaluation Functions**:
   - **Monotonicity**: Prefers boards where tiles are arranged in increasing/decreasing order
   - **Smoothness**: Prefers boards where adjacent tiles have similar values
   - **Empty Tiles**: Prefers boards with more empty spaces
   - **Merge Potential**: Evaluates potential merges
   - **Snake Pattern**: Evaluates the board based on a snake-like pattern
   - **Corner Strategy**: Prefers keeping high-value tiles in the corners

## Building the Project

This project uses CMake for building. To build the project:

```bash
mkdir build
cd build
cmake ..
make
```

## Running the Game

After building, run the executable:

```bash
./2048-qt
```

Use the arrow keys to move the tiles, or click the "Auto Play" button to let the AI play the game.
