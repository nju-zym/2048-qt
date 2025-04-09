#ifndef OPENCL_MANAGER_H
#define OPENCL_MANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QFile>
#include <QDebug>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <vector>
#include <string>
#include <memory>

// Forward declaration
using BitBoard = uint64_t;

class OpenCLManager : public QObject {
    Q_OBJECT

public:
    explicit OpenCLManager(QObject* parent = nullptr);
    ~OpenCLManager();

    // Initialize OpenCL
    bool initialize();

    // Check if OpenCL is available and initialized
    bool isInitialized() const { return initialized; }

    // Board evaluation functions
    QVector<double> evaluateBoards(const QVector<BitBoard>& boards, const QVector<double>& weights);

    // Board move functions
    QVector<BitBoard> makeMoves(const QVector<BitBoard>& boards, int direction, QVector<bool>& moveResults);

    // Simulation functions
    struct SimulationResult {
        int score;
        int maxTile;
        int moves;
    };
    QVector<SimulationResult> simulateGames(const QVector<double>& weights, int simulationCount, int maxMoves = 1000);

private:
    // Helper functions
    bool buildProgram(const QString& sourceFile, cl_program& program);
    QString readFile(const QString& fileName);
    void checkError(cl_int error, const QString& message);

    // OpenCL objects
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue commandQueue;

    // Programs
    cl_program evaluationProgram;
    cl_program movesProgram;
    cl_program simulationProgram;

    // Kernels
    cl_kernel evaluateBoardKernel;
    cl_kernel makeMoveKernel;
    cl_kernel simulateGameKernel;

    // State
    bool initialized;
};

#endif // OPENCL_MANAGER_H
