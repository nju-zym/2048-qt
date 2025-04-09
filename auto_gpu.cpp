#include "auto.h"
#include "opencl_manager.h"

// Initialize GPU acceleration
bool Auto::initializeGPU() {
    // Create OpenCL manager if it doesn't exist
    if (openclManager == nullptr) {
        openclManager = new OpenCLManager(this);
    }

    // Initialize OpenCL
    bool success = openclManager->initialize();

    if (success) {
        qDebug() << "GPU acceleration initialized successfully";
        useGPU = true;
    } else {
        qDebug() << "Failed to initialize GPU acceleration";
        useGPU = false;
    }

    return success;
}

// Enable or disable GPU acceleration
void Auto::enableGPU(bool enable) {
    if (enable && openclManager == nullptr) {
        initializeGPU();
    } else {
        useGPU = enable && openclManager != nullptr && openclManager->isInitialized();
    }

    qDebug() << "GPU acceleration" << (useGPU ? "enabled" : "disabled");
}

// Check if GPU acceleration is enabled
bool Auto::isGPUEnabled() const {
    return useGPU && openclManager != nullptr && openclManager->isInitialized();
}

// GPU-accelerated board evaluation
double Auto::evaluateBoardGPU(BitBoard board) const {
    if (!useGPU || openclManager == nullptr) {
        return evaluateBoard(board);
    }

    // Create a vector with just this board
    QVector<BitBoard> boards = {board};

    // Evaluate using OpenCL
    QVector<double> scores = openclManager->evaluateBoards(boards, weights);

    // Return the score if available, otherwise fall back to CPU
    if (!scores.isEmpty()) {
        return scores[0];
    }

    return evaluateBoard(board);
}

// GPU-accelerated batch board evaluation
QVector<double> Auto::evaluateBoardsGPU(QVector<BitBoard> const& boards) const {
    if (!useGPU || openclManager == nullptr || boards.isEmpty()) {
        // Fall back to CPU evaluation
        QVector<double> scores;
        scores.reserve(boards.size());
        for (BitBoard board : boards) {
            scores.append(evaluateBoard(board));
        }
        return scores;
    }

    // Evaluate using OpenCL
    return openclManager->evaluateBoards(boards, weights);
}

// GPU-accelerated move generation
QVector<BitBoard> Auto::makeMovesGPU(QVector<BitBoard> const& boards, int direction, QVector<bool>& moveResults) const {
    if (!useGPU || openclManager == nullptr || boards.isEmpty()) {
        // Fall back to CPU move generation
        QVector<BitBoard> newBoards;
        moveResults.resize(boards.size());
        newBoards.reserve(boards.size());

        for (int i = 0; i < boards.size(); ++i) {
            bool moved        = false;
            BitBoard newBoard = makeMove(boards[i], direction);
            moved             = (newBoard != boards[i]);
            newBoards.append(newBoard);
            moveResults[i] = moved;
        }

        return newBoards;
    }

    // Use OpenCL for move generation
    return openclManager->makeMoves(boards, direction, moveResults);
}

// GPU-accelerated simulation for training
QVector<double> Auto::runSimulationsGPU(QVector<double> const& params, int simulationCount) {
    if (!useGPU || openclManager == nullptr || simulationCount <= 0) {
        // Fall back to CPU simulation
        QVector<double> scores;
        scores.reserve(simulationCount);

        for (int i = 0; i < simulationCount; ++i) {
            scores.append(runSimulation(params));
        }

        return scores;
    }

    // Use OpenCL for simulation
    auto results = openclManager->simulateGames(params, simulationCount);

    // Convert results to scores
    QVector<double> scores;
    scores.reserve(results.size());

    for (auto const& result : results) {
        // Calculate score using the same formula as in runSimulation
        double score = result.score + (std::log2(result.maxTile) * 1000) + (result.moves * 10);
        scores.append(score);
    }

    return scores;
}
