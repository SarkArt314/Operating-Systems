#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <mutex>
#include <fstream>

std::mutex resultMutex;

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

void multiplyBlock(const std::vector<std::vector<int>>& matrixA,
    const std::vector<std::vector<int>>& matrixB,
    std::vector<std::vector<int>>& resultMatrix,
    int blockRowA, int blockColA, int blockRowB, int blockColB, int blockSize) {
    int size = matrixA.size();
    int startRowA = blockRowA * blockSize;
    int endRowA = std::min((blockRowA + 1) * blockSize, size);
    int startColA = blockColA * blockSize;
    int endColA = std::min((blockColA + 1) * blockSize, size);
    int startRowB = blockRowB * blockSize;
    int endRowB = std::min((blockRowB + 1) * blockSize, size);
    int startColB = blockColB * blockSize;
    int endColB = std::min((blockColB + 1) * blockSize, size);

    for (int i = startRowA; i < endRowA; ++i) {
        for (int j = startColB; j < endColB; ++j) {
            int sum = 0;
            for (int t = 0; t < blockSize; ++t) {
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

int main() {
    const int size = 32;
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
        size_t threadsCreated = 0;

        auto startTime = std::chrono::high_resolution_clock::now();

        for (int blockI = 0; blockI < blocksPerDim; ++blockI) {
            for (int blockJ = 0; blockJ < blocksPerDim; ++blockJ) {
                for (int blockK = 0; blockK < blocksPerDim; ++blockK) {
                    while (threadPool.size() >= maxThreads) {
                        if (threadPool.front().joinable()) {
                            threadPool.front().join();
                        }
                        threadPool.erase(threadPool.begin());
                    }

                    threadPool.emplace_back(multiplyBlock,
                        std::cref(matrixA), std::cref(matrixB), std::ref(parallelResult),
                        blockI, blockK, blockK, blockJ, blockSize);
                    ++threadsCreated;
                }
            }
        }

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
            << " threadsCreated=" << threadsCreated
            << " time_ms=" << parallelTime
            << " correct=" << (isCorrect ? "YES" : "NO") << "\n";
    }

    return 0;
}