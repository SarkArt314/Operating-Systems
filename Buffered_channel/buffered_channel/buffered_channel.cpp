#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <mutex>
#include <fstream>
#include <queue>
#include <condition_variable>

template<class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size) : capacity(size) {}

    void Send(T value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this]() { return closed_ || queue_.size() < capacity; });
        if (closed_) {
            throw std::runtime_error("Channel is closed");
        }
        queue_.push(std::move(value));
        not_empty_.notify_one();
    }

    std::pair<T, bool> Recv() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        not_empty_.wait(lock, [this]() { 
            return (closed_ && queue_.empty()) || !queue_.empty(); 
            });
        
        if (queue_.empty() && closed_) {
            return std::make_pair(T(), false);
        }
        
        T value = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return std::make_pair(std::move(value), true);
    }

    void Close() {
        std::unique_lock<std::mutex> lock(mutex_);
        closed_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    size_t capacity;
    bool closed_ = false;
};

std::mutex resultMutex;

struct MatrixBlockTask {
    int blockRowA;
    int blockColA;
    int blockRowB;
    int blockColB;
    int blockSize;
};

void fillRandom(std::vector<std::vector<int>>& matrix, int minValue = 1, int maxValue = 10) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(minValue, maxValue);
    for (auto& row : matrix) {
        for (auto& element : row) {
            element = dist(gen);
        }
    }
}

long long multiplyNaive(const std::vector<std::vector<int>>& matrixA,
    const std::vector<std::vector<int>>& matrixB,
    std::vector<std::vector<int>>& resultMatrix) {
    int size = matrixA.size();
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            int sum = 0;
            for (int k = 0; k < size; ++k) {
                sum += matrixA[i][k] * matrixB[k][j];
            }
            resultMatrix[i][j] = sum;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
}

void processBlockTask(const std::vector<std::vector<int>>& matrixA,
    const std::vector<std::vector<int>>& matrixB,
    std::vector<std::vector<int>>& resultMatrix,
    const MatrixBlockTask& task) {
    int size = matrixA.size();
    int startRowA = task.blockRowA * task.blockSize;
    int endRowA = std::min((task.blockRowA + 1) * task.blockSize, size);
    int startColA = task.blockColA * task.blockSize;
    int endColA = std::min((task.blockColA + 1) * task.blockSize, size);
    int startRowB = task.blockRowB * task.blockSize;
    int endRowB = std::min((task.blockRowB + 1) * task.blockSize, size);
    int startColB = task.blockColB * task.blockSize;
    int endColB = std::min((task.blockColB + 1) * task.blockSize, size);

    for (int i = startRowA; i < endRowA; ++i) {
        for (int j = startColB; j < endColB; ++j) {
            int sum = 0;
            for (int t = 0; t < task.blockSize; ++t) {
                int colA = startColA + t;
                int rowB = startRowB + t;
                if (colA < endColA && rowB < endRowB) {
                    sum += matrixA[i][colA] * matrixB[rowB][j];
                }
            }
            std::lock_guard<std::mutex> lock(resultMutex);
            resultMatrix[i][j] += sum;
        }
    }
}

void taskWorker(BufferedChannel<MatrixBlockTask>& channel,
    const std::vector<std::vector<int>>& matrixA,
    const std::vector<std::vector<int>>& matrixB,
    std::vector<std::vector<int>>& resultMatrix) {
    while (true) {
        auto task = channel.Recv();
        if (!task.second) {
            break;
        }
        processBlockTask(matrixA, matrixB, resultMatrix, task.first);
    }
}

int main() {
    const int size = 124;
    const unsigned maxThreads = 64;

    std::vector<std::vector<int>> matrixA(size, std::vector<int>(size));
    std::vector<std::vector<int>> matrixB(size, std::vector<int>(size));
    std::vector<std::vector<int>> referenceResult(size, std::vector<int>(size, 0));
    std::vector<std::vector<int>> parallelResult(size, std::vector<int>(size, 0));

    fillRandom(matrixA, 1, 100);
    fillRandom(matrixB, 1, 100);

    long long naiveTime = multiplyNaive(matrixA, matrixB, referenceResult);
    std::cout << "Naive " << size << "x" << size << " : " << naiveTime << " ms\n";

    for (int blockSize = 1; blockSize <= size; ++blockSize) {
        for (auto& row : parallelResult) {
            std::fill(row.begin(), row.end(), 0);
        }

        int blocksPerDim = (size + blockSize - 1) / blockSize;
        std::vector<std::thread> threadPool;

        BufferedChannel<MatrixBlockTask> taskChannel(blocksPerDim * blocksPerDim * blocksPerDim);

        for (unsigned i = 0; i < maxThreads && i < blocksPerDim * blocksPerDim * blocksPerDim; ++i) {
            threadPool.emplace_back(taskWorker, std::ref(taskChannel),
                std::cref(matrixA), std::cref(matrixB), std::ref(parallelResult));
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        size_t tasksCreated = 0;
        for (int blockI = 0; blockI < blocksPerDim; ++blockI) {
            for (int blockJ = 0; blockJ < blocksPerDim; ++blockJ) {
                for (int blockK = 0; blockK < blocksPerDim; ++blockK) {
                    MatrixBlockTask task{ blockI, blockK, blockK, blockJ, blockSize };
                    taskChannel.Send(std::move(task));
                    ++tasksCreated;
                }
            }
        }

        taskChannel.Close();

        for (auto& thread : threadPool) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        long long parallelTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        bool isCorrect = true;
        for (int i = 0; i < size && isCorrect; ++i) {
            for (int j = 0; j < size && isCorrect; ++j) {
                if (referenceResult[i][j] != parallelResult[i][j]) {
                    isCorrect = false;
                }
            }
        }

        std::cout << "k=" << blockSize
            << " blocksPerDim=" << blocksPerDim
            << " threadsCreated=" << threadPool.size()
            << " tasksCreated=" << tasksCreated
            << " time_ms=" << parallelTime
            << " correct=" << (isCorrect ? "YES" : "NO") << "\n";
    }

    return 0;
}
