#include <iostream>
#include <thread>
#include <windows.h>
#include <vector>
#include <fstream>
#include <chrono> 
#include <random> 
#include <algorithm>

using namespace std;

struct InputParam {
    int index;
    int localBestResult;
};
struct Task {
    int duration;
    vector<int> workers;
};

//? globals
int taskNumber;
string filePath = "widgets.txt";
int workerNumber;
int processorNumber;
int expectedTime;
vector<int> availability;
vector<int> tasksTimes;

void readDataFromFile() {
    std::cout << "3: start reading data function\n";
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    processorNumber = sysInfo.dwNumberOfProcessors;

    ifstream dataFile(filePath);
    dataFile >> taskNumber >> workerNumber;

    tasksTimes.resize(taskNumber);
    for (int i = 0; i < taskNumber; i++) {
        dataFile >> tasksTimes[i];
    }

    availability.resize(taskNumber * workerNumber);
    for (int i = 0; i < taskNumber * workerNumber; i++) {
        dataFile >> availability[i];
    }

    dataFile.close();
    std::cout << "4: enter expected Time to search :\n";
    cin >> expectedTime;
}

int calculateBestAnswer(const vector<Task>& tasks, int numTasks, int numWorkers) {
    int bestDifference = 3232131;
    auto start = chrono::steady_clock::now();
    while (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count() < expectedTime) {
        vector<int> workerTimes(numWorkers, 0);
        random_device rd;
        mt19937 gen(rd());
        for (int i = 0; i < numTasks; ++i) {
            vector<int> availableWorkers;
            for (int j = 0; j < numWorkers; ++j) {
                if (tasks[i].workers[j] == 1) {
                    availableWorkers.push_back(j);
                }
            }
            if (!availableWorkers.empty()) {
                uniform_int_distribution<> dis(0, availableWorkers.size() - 1);
                int chosenWorker = availableWorkers[dis(gen)];
                workerTimes[chosenWorker] += tasks[i].duration;
            }
        }
        int maxTime = *max_element(workerTimes.begin(), workerTimes.end());
        int minTime = *min_element(workerTimes.begin(), workerTimes.end());
        int difference = maxTime - minTime;
        if (difference < bestDifference) {
            bestDifference = difference;
        }
    }
    return bestDifference;
}

DWORD WINAPI child(void* param) {
    InputParam* inp = static_cast<InputParam*>(param);
    int index = inp->index;

    vector<Task> tasks(taskNumber);
    for (int i = 0; i < taskNumber; i++) {
        tasks[i].duration = tasksTimes[i];
    }
    int taskindex = 0;
    vector<int> tmp;
    for (int i = 1; i <= taskNumber * workerNumber; i++) {
        if (i % workerNumber == 0) {
            tmp.push_back(availability[i - 1]);
            for (int p = 0; p < workerNumber; p++) {
                tasks[taskindex].workers.push_back(tmp[p]);
            }
            tmp.clear();
            taskindex++;
        }
        else {
            tmp.push_back(availability[i - 1]);
        }
    }

    inp->localBestResult = calculateBestAnswer(tasks, taskNumber, workerNumber);
    //cout << "Thread No." << index << " : " << inp->localBestResult<<endl;
    return 0;
}

int main() {
    std::cout << "1: start main function\n";
    readDataFromFile();


    vector<InputParam> threadParameters(processorNumber);
    vector<HANDLE> threadHandles(processorNumber);

    std::cout << "2:creating threads\n" << "______________________\n";
    for (int i = 0; i < processorNumber; ++i) {
        threadParameters[i].index = i;
        threadHandles[i] = CreateThread(NULL, 0, child, &threadParameters[i], 0, NULL);
    }
    WaitForMultipleObjects(threadHandles.size(), threadHandles.data(), TRUE, INFINITE);
    vector<int>results(processorNumber);
    for (int i = 0; i < processorNumber; i++) {
        results[i] = threadParameters[i].localBestResult;
        cout << "thread No." << i << " best result : " << results[i]<<endl;
    }

    std::cout << "\n\n\n4: progress completed \n" << "______________________\n";

    auto bestResult = std::min_element(results.begin(), results.end());
    int minelement = std::distance(results.begin(), bestResult);
    std::cout << "Best cost: " << results[minelement] << endl;
    std::cout << "calculated by Thread No. " << minelement << endl;


    return 0;
}
