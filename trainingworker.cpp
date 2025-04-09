#include "trainingworker.h"

#include "auto.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QMutexLocker>
#include <QThreadPool>
#include <random>

void TrainingWorker::doTraining() {
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 10.0);

    // 初始化种群
    QVector<QVector<double>> population;
    QVector<double> bestParams;
    int bestScore = 0;

    // 使用当前的最佳参数作为起点
    if (autoPlayer->useLearnedParams && !autoPlayer->strategyParams.isEmpty()) {
        bestParams = autoPlayer->strategyParams;
        bestScore  = autoPlayer->bestHistoricalScore;
        qDebug() << "Starting training with existing parameters. Historical best score:" << bestScore;
    } else {
        bestParams = QVector<double>(5, 1.0);
        qDebug() << "Starting training with default parameters.";
    }

    // 初始化种群中的每个个体
    for (int i = 0; i < populationSize; ++i) {
        if (i == 0 && autoPlayer->useLearnedParams) {
            // 将当前最佳参数加入种群
            population.append(bestParams);
        } else if (i == 1 && autoPlayer->useLearnedParams) {
            // 将当前最佳参数的微小变异加入种群
            QVector<double> slightlyModified = bestParams;
            for (int j = 0; j < slightlyModified.size(); ++j) {
                // 在原有参数基础上增加小的随机变化
                slightlyModified[j] *= (1.0 + (dis(gen) * 0.1 - 0.05));  // 正负5%的变化
                slightlyModified[j]  = std::max(0.0, std::min(slightlyModified[j], 20.0));
            }
            population.append(slightlyModified);
        } else {
            // 生成随机参数
            QVector<double> params(5);
            for (int j = 0; j < 5; ++j) {
                params[j] = dis(gen);  // 生成 0-10 之间的随机浮点数
            }
            population.append(params);
        }
    }

    // 进化多代
    for (int gen = 0; gen < generations && autoPlayer->trainingActive.load(); ++gen) {
        QVector<int> scores(populationSize, 0);
        QVector<bool> evaluated(populationSize, false);
        int evaluatedCount = 0;

        // 并行评估每个个体
        for (int i = 0; i < populationSize; ++i) {
            // 复制当前索引和其他必要的值，避免捕获引用
            int currentIndex = i;
            int currentGen   = gen;

            auto* task = new TrainingTask(
                population[i],
                simulations,
                // 最终回调 - 使用共享指针来安全地更新数据
                [this,
                 currentIndex,
                 &scores,
                 &evaluated,
                 &evaluatedCount,
                 &bestScore,
                 &bestParams,
                 currentGen,
                 &population](int score) {
                    // 更新分数
                    scores[currentIndex]    = score;
                    evaluated[currentIndex] = true;
                    evaluatedCount++;

                    // 更新最佳参数
                    if (score > bestScore) {
                        QMutexLocker locker(&autoPlayer->mutex);
                        bestScore  = score;
                        bestParams = population[currentIndex];
                    }

                    // 当所有个体都已评估完成时，发送进度更新
                    QMutexLocker locker(&autoPlayer->mutex);
                    if (evaluatedCount == population.size()) {
                        // 发送进度更新信号
                        emit autoPlayer->trainingProgress.progressUpdated(
                            currentGen + 1, generations, bestScore, bestParams);
                    }
                },
                // 进度回调 - 使用当前的最佳分数和参数，而不是引用外部变量
                [this, currentIndex, currentGen](int current, int total, int avgScore) {
                    // 使用互斥锁保护的情况下获取当前的最佳分数和参数
                    {
                        QMutexLocker locker(&autoPlayer->mutex);
                        // 这里可以访问共享数据如果需要
                    }

                    // 计算总体进度 - 使用更稳定的进度计算
                    int totalProgress = 0;
                    {
                        QMutexLocker locker(&autoPlayer->mutex);
                        // 每个个体的模拟进度占总进度的一小部分
                        double genProgress = static_cast<double>(currentGen) / this->generations;
                        double indProgress = static_cast<double>(currentIndex) / this->populationSize;
                        double simProgress = static_cast<double>(current) / total;

                        // 每一代占总进度的比例
                        double genWeight = 1.0 / this->generations;

                        // 计算总进度
                        totalProgress = static_cast<int>(
                            (genProgress + (indProgress + simProgress * 0.8) * genWeight / this->populationSize) * 100);
                        totalProgress = qMin(totalProgress, 99);  // 确保不会显示100%直到真正完成
                    }

                    // 发送模拟进度更新
                    emit autoPlayer->trainingProgress.simulationUpdated(current, total, avgScore, totalProgress);
                });

            // 设置任务为自动删除
            task->setAutoDelete(true);

            // 提交任务到线程池
            QThreadPool::globalInstance()->start(task);
        }

        // 等待所有任务完成
        QThreadPool::globalInstance()->waitForDone();

        // 如果训练已停止，则退出
        if (!autoPlayer->trainingActive.load()) {
            break;
        }

        // 找出本代最佳分数
        int genBestScore = 0;

        for (int i = 0; i < populationSize; ++i) {
            genBestScore = std::max(genBestScore, scores[i]);

            // 更新全局最佳参数
            if (scores[i] > bestScore) {
                bestScore  = scores[i];
                bestParams = population[i];
            }
        }

        // 发送进度更新信号
        emit autoPlayer->trainingProgress.progressUpdated(gen + 1, generations, bestScore, bestParams);

        // 创建新一代
        QVector<QVector<double>> newPopulation;

        // 精英选择 - 保留最佳的五个个体
        QVector<int> indices = autoPlayer->findTopIndices(scores, 5);
        for (int idx : indices) {
            newPopulation.append(population[idx]);
        }

        // 交叉和变异生成新一代
        while (newPopulation.size() < populationSize) {
            try {
                // 选择两个父代进行交叉
                int parent1 = autoPlayer->tournamentSelection(scores);
                int parent2 = autoPlayer->tournamentSelection(scores);

                // 确保索引有效
                if (parent1 < 0 || parent1 >= population.size() || parent2 < 0 || parent2 >= population.size()) {
                    qDebug() << "Invalid parent index:" << parent1 << parent2;
                    continue;
                }

                // 交叉
                QVector<double> child = autoPlayer->crossover(population[parent1], population[parent2]);

                // 变异 - 增加变异率以提高多样性
                autoPlayer->mutate(child, 0.3);  // 增加变异率到 30%

                // 添加到新种群
                newPopulation.append(child);
            } catch (std::exception const& e) {
                qDebug() << "Exception in crossover/mutation:" << e.what();
                // 出错时创建一个随机个体
                QVector<double> randomParams(5);
                // 创建新的随机数生成器
                std::random_device local_rd;
                std::mt19937 local_gen(local_rd());
                std::uniform_real_distribution<> local_dis(0.0, 10.0);

                for (int j = 0; j < 5; ++j) {
                    randomParams[j] = local_dis(local_gen);  // 生成新的随机参数
                }
                newPopulation.append(randomParams);
            }
        }

        // 替换旧种群
        population = newPopulation;
    }

    // 评估最终参数的性能 - 增加模拟次数进行更全面的评估
    int finalScore = autoPlayer->evaluateParameters(bestParams, 50);  // 增加到50次模拟游戏

    // 输出详细的评估结果
    qDebug() << "\nTraining completed:";
    qDebug() << "Best score in training:" << bestScore;
    qDebug() << "Final evaluation score:" << finalScore;
    qDebug() << "Historical best score:" << autoPlayer->bestHistoricalScore;

    // 只有当新参数比历史最佳参数更好时才更新
    if (finalScore > autoPlayer->bestHistoricalScore) {
        // 保存最佳参数
        autoPlayer->strategyParams      = bestParams;
        autoPlayer->bestHistoricalScore = finalScore;

        // 保存参数到文件
        // 确保数据目录存在
        QDir dataDir("data");
        if (!dataDir.exists()) {
            bool dirCreated = QDir().mkdir("data");
            qDebug() << "Creating data directory:" << (dirCreated ? "success" : "failed");

            // 如果创建失败，尝试获取绝对路径并创建
            if (!dirCreated) {
                QString absPath = QDir::currentPath() + "/data";
                dirCreated      = QDir().mkdir(absPath);
                qDebug() << "Creating data directory at absolute path:" << absPath
                         << (dirCreated ? "success" : "failed");
            }
        }

        QString filename = "data/params_" + QString::number(finalScore) + "_"
                           + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".json";
        autoPlayer->saveParameters(filename);

        // 同时保存到持久化文件
        QString persistentFile = "data/2048_ai_persistent_data.json";
        QJsonObject paramsObj;
        QJsonArray paramsArray;

        for (double param : bestParams) {
            paramsArray.append(param);
        }

        paramsObj["parameters"] = paramsArray;
        paramsObj["score"]      = finalScore;
        paramsObj["timestamp"]  = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonDocument doc(paramsObj);

        // 使用 Auto 类的方法保存持久化数据
        if (autoPlayer->savePersistentData(doc)) {
            qDebug() << "Saved persistent training data with score:" << finalScore;
        } else {
            qDebug() << "Failed to save persistent data to:" << persistentFile;

            // 如果使用 Auto 类的方法失败，尝试直接文件操作
            QFile persistentDataFile(persistentFile);
            if (persistentDataFile.open(QIODevice::WriteOnly)) {
                persistentDataFile.write(doc.toJson());
                persistentDataFile.close();
                qDebug() << "Saved persistent training data with direct file operation, score:" << finalScore;
            } else {
                qDebug() << "Failed to save persistent data with direct file operation:"
                         << persistentDataFile.errorString();
            }
        }

        qDebug() << "New best parameters found and saved to:" << filename;
        qDebug() << "Improvement:" << (finalScore - autoPlayer->bestHistoricalScore) << "points";
    } else {
        qDebug() << "Training did not improve parameters.";
        qDebug() << "Gap to historical best:" << (autoPlayer->bestHistoricalScore - finalScore) << "points";

        // 即使没有改进，也保存当前训练的最佳参数供参考
        // 确保数据目录存在
        QDir dataDir("data");
        if (!dataDir.exists()) {
            QDir().mkdir("data");
        }

        QString filename = "data/params_current_" + QString::number(finalScore) + "_"
                           + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".json";

        // 保存当前参数到文件，但不更新历史最佳参数
        QJsonObject paramsObj;
        QJsonArray paramsArray;

        for (double param : bestParams) {
            paramsArray.append(param);
        }

        paramsObj["parameters"] = paramsArray;
        paramsObj["score"]      = finalScore;
        paramsObj["timestamp"]  = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonDocument doc(paramsObj);
        QFile file(filename);

        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
            qDebug() << "Current best parameters saved to:" << filename;
        } else {
            qDebug() << "Failed to save current parameters to:" << filename << ": " << file.errorString();
            // 如果失败，尝试创建目录并重试
            bool pathCreated = QDir().mkpath("data");
            qDebug() << "Creating data directory path:" << (pathCreated ? "success" : "failed");

            // 尝试使用绝对路径
            QString absPath = QDir::currentPath() + "/" + filename;
            QFile absFile(absPath);
            qDebug() << "Trying absolute path:" << absPath;

            if (absFile.open(QIODevice::WriteOnly)) {
                absFile.write(doc.toJson());
                absFile.close();
                qDebug() << "Current best parameters saved to absolute path:" << absPath;
            } else {
                qDebug() << "Still failed to save current parameters using absolute path:" << absFile.errorString();
            }
        }
    }

    // 重置训练状态
    autoPlayer->trainingActive.store(false);

    // 创建一个副本保存最终结果，避免线程间的数据竞争
    QVector<double> finalBestParams = bestParams;
    int finalBestScore              = finalScore;

    // 在主线程中安全地完成所有后续操作
    QMetaObject::invokeMethod(
        QApplication::instance(),
        [finalBestParams, finalBestScore, finalScore, this]() {
            // 发送100%进度更新
            emit autoPlayer->trainingProgress.progressUpdated(
                generations, generations, finalBestScore, finalBestParams);

            try {
                // 运行一次深度模拟来测试最佳参数的效果
                int maxTile   = 0;
                int testScore = 0;

                // 创建一个新的Auto实例来运行测试，避免与原始实例的冲突
                Auto testAuto;
                testAuto.initTables();
                testAuto.strategyParams = finalBestParams;
                testAuto.simulateFullGameDetailed(finalBestParams, testScore, maxTile);

                qDebug() << "Test game with best parameters - Score:" << testScore << "Max tile:" << maxTile;

                // 显示训练完成的消息框
                QMessageBox* msgBox = new QMessageBox();
                msgBox->setWindowTitle("Training Completed");
                msgBox->setText(QString("Training completed successfully!\n\n"
                                        "Best score in training: %1\n"
                                        "Final evaluation score: %2\n"
                                        "Test game score: %3 (Max tile: %4)")
                                    .arg(finalBestScore)
                                    .arg(finalScore)
                                    .arg(testScore)
                                    .arg(maxTile));
                msgBox->setIcon(QMessageBox::Information);
                msgBox->setStandardButtons(QMessageBox::Ok);
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->show();

                // 连接关闭信号以删除消息框
                QObject::connect(msgBox, &QMessageBox::finished, msgBox, &QMessageBox::deleteLater);
            } catch (std::exception const& e) {
                qDebug() << "Exception during final simulation:" << e.what();

                // 即使出现异常也显示训练完成的消息框
                QMessageBox* msgBox = new QMessageBox();
                msgBox->setWindowTitle("Training Completed");
                msgBox->setText(QString("Training completed with errors!\n\n"
                                        "Best score in training: %1\n"
                                        "Final evaluation score: %2\n"
                                        "Error: %3")
                                    .arg(finalBestScore)
                                    .arg(finalScore)
                                    .arg(e.what()));
                msgBox->setIcon(QMessageBox::Warning);
                msgBox->setStandardButtons(QMessageBox::Ok);
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->show();

                // 连接关闭信号以删除消息框
                QObject::connect(msgBox, &QMessageBox::finished, msgBox, &QMessageBox::deleteLater);
            } catch (...) {
                qDebug() << "Unknown exception during final simulation";

                // 即使出现未知异常也显示训练完成的消息框
                QMessageBox* msgBox = new QMessageBox();
                msgBox->setWindowTitle("Training Completed");
                msgBox->setText(QString("Training completed with unknown errors!\n\n"
                                        "Best score in training: %1\n"
                                        "Final evaluation score: %2")
                                    .arg(finalBestScore)
                                    .arg(finalScore));
                msgBox->setIcon(QMessageBox::Warning);
                msgBox->setStandardButtons(QMessageBox::Ok);
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->show();

                // 连接关闭信号以删除消息框
                QObject::connect(msgBox, &QMessageBox::finished, msgBox, &QMessageBox::deleteLater);
            }

            // 发送训练完成信号
            emit autoPlayer->trainingProgress.trainingCompleted(finalBestScore, finalBestParams);

            // 确保所有资源都已释放
            QThreadPool::globalInstance()->waitForDone();

            // 在主线程中发出完成信号，确保安全退出
            QMetaObject::invokeMethod(this, "emitFinished", Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
}

// 安全发出完成信号
void TrainingWorker::emitFinished() {
    // 发出完成信号
    emit finished();
}
