#ifndef TRAININGWORKER_H
#define TRAININGWORKER_H

#include <QObject>
#include <QVector>

class Auto;

// 训练工作线程类
class TrainingWorker : public QObject {
    Q_OBJECT

   public:
    TrainingWorker(Auto* autoPlayer, int populationSize, int generations, int simulations)
        : autoPlayer(autoPlayer), populationSize(populationSize), generations(generations), simulations(simulations) {}

   public slots:
    void doTraining();
    void emitFinished();

   signals:
    void finished();

   private:
    Auto* autoPlayer;
    int populationSize;
    int generations;
    int simulations;
};

#endif  // TRAININGWORKER_H
