#include "opencl_manager.h"
#include <QDir>
#include <QRandomGenerator>

OpenCLManager::OpenCLManager(QObject* parent)
    : QObject(parent),
      platform(nullptr),
      device(nullptr),
      context(nullptr),
      commandQueue(nullptr),
      evaluationProgram(nullptr),
      movesProgram(nullptr),
      simulationProgram(nullptr),
      evaluateBoardKernel(nullptr),
      makeMoveKernel(nullptr),
      simulateGameKernel(nullptr),
      initialized(false) {
}

OpenCLManager::~OpenCLManager() {
    // Release OpenCL resources
    if (evaluateBoardKernel) clReleaseKernel(evaluateBoardKernel);
    if (makeMoveKernel) clReleaseKernel(makeMoveKernel);
    if (simulateGameKernel) clReleaseKernel(simulateGameKernel);
    
    if (evaluationProgram) clReleaseProgram(evaluationProgram);
    if (movesProgram) clReleaseProgram(movesProgram);
    if (simulationProgram) clReleaseProgram(simulationProgram);
    
    if (commandQueue) clReleaseCommandQueue(commandQueue);
    if (context) clReleaseContext(context);
}

bool OpenCLManager::initialize() {
    cl_int error;
    
    // Get platform
    cl_uint numPlatforms;
    error = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (error != CL_SUCCESS || numPlatforms == 0) {
        qDebug() << "Failed to find any OpenCL platforms.";
        return false;
    }
    
    std::vector<cl_platform_id> platforms(numPlatforms);
    error = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
    if (error != CL_SUCCESS) {
        qDebug() << "Failed to get OpenCL platform IDs.";
        return false;
    }
    
    // Choose the first platform
    platform = platforms[0];
    
    // Get device
    cl_uint numDevices;
    error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    if (error != CL_SUCCESS || numDevices == 0) {
        qDebug() << "Failed to find any OpenCL devices.";
        return false;
    }
    
    std::vector<cl_device_id> devices(numDevices);
    error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
    if (error != CL_SUCCESS) {
        qDebug() << "Failed to get OpenCL device IDs.";
        return false;
    }
    
    // Choose the first device
    device = devices[0];
    
    // Create context
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
    if (error != CL_SUCCESS) {
        qDebug() << "Failed to create OpenCL context.";
        return false;
    }
    
    // Create command queue
    #ifdef CL_VERSION_2_0
    commandQueue = clCreateCommandQueueWithProperties(context, device, nullptr, &error);
    #else
    commandQueue = clCreateCommandQueue(context, device, 0, &error);
    #endif
    
    if (error != CL_SUCCESS) {
        qDebug() << "Failed to create OpenCL command queue.";
        return false;
    }
    
    // Build programs
    if (!buildProgram("kernels/board_evaluation.cl", evaluationProgram)) {
        qDebug() << "Failed to build board evaluation program.";
        return false;
    }
    
    if (!buildProgram("kernels/board_moves.cl", movesProgram)) {
        qDebug() << "Failed to build board moves program.";
        return false;
    }
    
    if (!buildProgram("kernels/simulation.cl", simulationProgram)) {
        qDebug() << "Failed to build simulation program.";
        return false;
    }
    
    // Create kernels
    evaluateBoardKernel = clCreateKernel(evaluationProgram, "evaluateBoard", &error);
    if (error != CL_SUCCESS) {
        qDebug() << "Failed to create evaluateBoard kernel.";
        return false;
    }
    
    makeMoveKernel = clCreateKernel(movesProgram, "makeMove", &error);
    if (error != CL_SUCCESS) {
        qDebug() << "Failed to create makeMove kernel.";
        return false;
    }
    
    simulateGameKernel = clCreateKernel(simulationProgram, "simulateGame", &error);
    if (error != CL_SUCCESS) {
        qDebug() << "Failed to create simulateGame kernel.";
        return false;
    }
    
    initialized = true;
    return true;
}

bool OpenCLManager::buildProgram(const QString& sourceFile, cl_program& program) {
    QString source = readFile(sourceFile);
    if (source.isEmpty()) {
        qDebug() << "Failed to read OpenCL source file:" << sourceFile;
        return false;
    }
    
    const char* sourceCStr = source.toUtf8().constData();
    size_t sourceSize = source.size();
    
    cl_int error;
    program = clCreateProgramWithSource(context, 1, &sourceCStr, &sourceSize, &error);
    if (error != CL_SUCCESS) {
        qDebug() << "Failed to create program from source:" << sourceFile;
        return false;
    }
    
    error = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if (error != CL_SUCCESS) {
        // Get build log
        size_t logSize;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        
        std::vector<char> log(logSize);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
        
        qDebug() << "OpenCL build error for" << sourceFile << ":" << log.data();
        return false;
    }
    
    return true;
}

QString OpenCLManager::readFile(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << fileName;
        return QString();
    }
    
    return QString::fromUtf8(file.readAll());
}

void OpenCLManager::checkError(cl_int error, const QString& message) {
    if (error != CL_SUCCESS) {
        qDebug() << message << "Error code:" << error;
    }
}

QVector<double> OpenCLManager::evaluateBoards(const QVector<BitBoard>& boards, const QVector<double>& weights) {
    if (!initialized || boards.isEmpty()) {
        return QVector<double>();
    }
    
    cl_int error;
    size_t boardCount = boards.size();
    
    // Create buffers
    cl_mem boardsBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                        sizeof(BitBoard) * boardCount, (void*)boards.constData(), &error);
    checkError(error, "Failed to create boards buffer.");
    
    cl_mem weightsBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                         sizeof(float) * weights.size(), (void*)weights.constData(), &error);
    checkError(error, "Failed to create weights buffer.");
    
    cl_mem scoresBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                        sizeof(float) * boardCount, nullptr, &error);
    checkError(error, "Failed to create scores buffer.");
    
    // Set kernel arguments
    error = clSetKernelArg(evaluateBoardKernel, 0, sizeof(cl_mem), &boardsBuffer);
    checkError(error, "Failed to set kernel argument 0.");
    
    error = clSetKernelArg(evaluateBoardKernel, 1, sizeof(cl_mem), &weightsBuffer);
    checkError(error, "Failed to set kernel argument 1.");
    
    error = clSetKernelArg(evaluateBoardKernel, 2, sizeof(cl_mem), &scoresBuffer);
    checkError(error, "Failed to set kernel argument 2.");
    
    error = clSetKernelArg(evaluateBoardKernel, 3, sizeof(int), &boardCount);
    checkError(error, "Failed to set kernel argument 3.");
    
    // Execute kernel
    size_t globalWorkSize = boardCount;
    error = clEnqueueNDRangeKernel(commandQueue, evaluateBoardKernel, 1, nullptr,
                                  &globalWorkSize, nullptr, 0, nullptr, nullptr);
    checkError(error, "Failed to enqueue kernel.");
    
    // Read results
    QVector<float> scores(boardCount);
    error = clEnqueueReadBuffer(commandQueue, scoresBuffer, CL_TRUE, 0,
                               sizeof(float) * boardCount, scores.data(), 0, nullptr, nullptr);
    checkError(error, "Failed to read scores buffer.");
    
    // Clean up
    clReleaseMemObject(boardsBuffer);
    clReleaseMemObject(weightsBuffer);
    clReleaseMemObject(scoresBuffer);
    
    // Convert to double
    QVector<double> result;
    result.reserve(boardCount);
    for (float score : scores) {
        result.append(static_cast<double>(score));
    }
    
    return result;
}

QVector<BitBoard> OpenCLManager::makeMoves(const QVector<BitBoard>& boards, int direction, QVector<bool>& moveResults) {
    if (!initialized || boards.isEmpty()) {
        return QVector<BitBoard>();
    }
    
    cl_int error;
    size_t boardCount = boards.size();
    
    // Create buffers
    cl_mem inputBoardsBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                             sizeof(BitBoard) * boardCount, (void*)boards.constData(), &error);
    checkError(error, "Failed to create input boards buffer.");
    
    cl_mem outputBoardsBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                              sizeof(BitBoard) * boardCount, nullptr, &error);
    checkError(error, "Failed to create output boards buffer.");
    
    cl_mem moveResultsBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                             sizeof(int) * boardCount, nullptr, &error);
    checkError(error, "Failed to create move results buffer.");
    
    // Set kernel arguments
    error = clSetKernelArg(makeMoveKernel, 0, sizeof(cl_mem), &inputBoardsBuffer);
    checkError(error, "Failed to set kernel argument 0.");
    
    error = clSetKernelArg(makeMoveKernel, 1, sizeof(cl_mem), &outputBoardsBuffer);
    checkError(error, "Failed to set kernel argument 1.");
    
    error = clSetKernelArg(makeMoveKernel, 2, sizeof(cl_mem), &moveResultsBuffer);
    checkError(error, "Failed to set kernel argument 2.");
    
    error = clSetKernelArg(makeMoveKernel, 3, sizeof(int), &direction);
    checkError(error, "Failed to set kernel argument 3.");
    
    error = clSetKernelArg(makeMoveKernel, 4, sizeof(int), &boardCount);
    checkError(error, "Failed to set kernel argument 4.");
    
    // Execute kernel
    size_t globalWorkSize = boardCount;
    error = clEnqueueNDRangeKernel(commandQueue, makeMoveKernel, 1, nullptr,
                                  &globalWorkSize, nullptr, 0, nullptr, nullptr);
    checkError(error, "Failed to enqueue kernel.");
    
    // Read results
    QVector<BitBoard> outputBoards(boardCount);
    error = clEnqueueReadBuffer(commandQueue, outputBoardsBuffer, CL_TRUE, 0,
                               sizeof(BitBoard) * boardCount, outputBoards.data(), 0, nullptr, nullptr);
    checkError(error, "Failed to read output boards buffer.");
    
    QVector<int> moveResultsInt(boardCount);
    error = clEnqueueReadBuffer(commandQueue, moveResultsBuffer, CL_TRUE, 0,
                               sizeof(int) * boardCount, moveResultsInt.data(), 0, nullptr, nullptr);
    checkError(error, "Failed to read move results buffer.");
    
    // Clean up
    clReleaseMemObject(inputBoardsBuffer);
    clReleaseMemObject(outputBoardsBuffer);
    clReleaseMemObject(moveResultsBuffer);
    
    // Convert move results to bool
    moveResults.resize(boardCount);
    for (int i = 0; i < boardCount; ++i) {
        moveResults[i] = (moveResultsInt[i] != 0);
    }
    
    return outputBoards;
}

QVector<OpenCLManager::SimulationResult> OpenCLManager::simulateGames(
    const QVector<double>& weights, int simulationCount, int maxMoves) {
    
    if (!initialized || simulationCount <= 0) {
        return QVector<SimulationResult>();
    }
    
    cl_int error;
    
    // Convert weights to float
    QVector<float> weightsFloat;
    weightsFloat.reserve(weights.size());
    for (double weight : weights) {
        weightsFloat.append(static_cast<float>(weight));
    }
    
    // Create buffers
    cl_mem weightsBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                         sizeof(float) * weightsFloat.size(), (void*)weightsFloat.constData(), &error);
    checkError(error, "Failed to create weights buffer.");
    
    cl_mem scoresBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                        sizeof(int) * simulationCount, nullptr, &error);
    checkError(error, "Failed to create scores buffer.");
    
    cl_mem maxTilesBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                          sizeof(int) * simulationCount, nullptr, &error);
    checkError(error, "Failed to create max tiles buffer.");
    
    cl_mem movesCountsBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                             sizeof(int) * simulationCount, nullptr, &error);
    checkError(error, "Failed to create moves counts buffer.");
    
    // Generate random seed
    uint randomSeed = QRandomGenerator::global()->generate();
    
    // Set kernel arguments
    error = clSetKernelArg(simulateGameKernel, 0, sizeof(cl_mem), &weightsBuffer);
    checkError(error, "Failed to set kernel argument 0.");
    
    error = clSetKernelArg(simulateGameKernel, 1, sizeof(cl_mem), &scoresBuffer);
    checkError(error, "Failed to set kernel argument 1.");
    
    error = clSetKernelArg(simulateGameKernel, 2, sizeof(cl_mem), &maxTilesBuffer);
    checkError(error, "Failed to set kernel argument 2.");
    
    error = clSetKernelArg(simulateGameKernel, 3, sizeof(cl_mem), &movesCountsBuffer);
    checkError(error, "Failed to set kernel argument 3.");
    
    error = clSetKernelArg(simulateGameKernel, 4, sizeof(uint), &randomSeed);
    checkError(error, "Failed to set kernel argument 4.");
    
    error = clSetKernelArg(simulateGameKernel, 5, sizeof(int), &maxMoves);
    checkError(error, "Failed to set kernel argument 5.");
    
    error = clSetKernelArg(simulateGameKernel, 6, sizeof(int), &simulationCount);
    checkError(error, "Failed to set kernel argument 6.");
    
    // Execute kernel
    size_t globalWorkSize = simulationCount;
    error = clEnqueueNDRangeKernel(commandQueue, simulateGameKernel, 1, nullptr,
                                  &globalWorkSize, nullptr, 0, nullptr, nullptr);
    checkError(error, "Failed to enqueue kernel.");
    
    // Read results
    QVector<int> scores(simulationCount);
    error = clEnqueueReadBuffer(commandQueue, scoresBuffer, CL_TRUE, 0,
                               sizeof(int) * simulationCount, scores.data(), 0, nullptr, nullptr);
    checkError(error, "Failed to read scores buffer.");
    
    QVector<int> maxTiles(simulationCount);
    error = clEnqueueReadBuffer(commandQueue, maxTilesBuffer, CL_TRUE, 0,
                               sizeof(int) * simulationCount, maxTiles.data(), 0, nullptr, nullptr);
    checkError(error, "Failed to read max tiles buffer.");
    
    QVector<int> movesCounts(simulationCount);
    error = clEnqueueReadBuffer(commandQueue, movesCountsBuffer, CL_TRUE, 0,
                               sizeof(int) * simulationCount, movesCounts.data(), 0, nullptr, nullptr);
    checkError(error, "Failed to read moves counts buffer.");
    
    // Clean up
    clReleaseMemObject(weightsBuffer);
    clReleaseMemObject(scoresBuffer);
    clReleaseMemObject(maxTilesBuffer);
    clReleaseMemObject(movesCountsBuffer);
    
    // Create result vector
    QVector<SimulationResult> results;
    results.reserve(simulationCount);
    
    for (int i = 0; i < simulationCount; ++i) {
        SimulationResult result;
        result.score = scores[i];
        result.maxTile = maxTiles[i];
        result.moves = movesCounts[i];
        results.append(result);
    }
    
    return results;
}
