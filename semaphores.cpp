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
    
};
struct Task {
    int duration;
    vector<int> workers;
};


struct resultFormat {
    int bestResult;
    vector<int> bestAssigns;
    int threadIndex;
};


//? globals
int taskNumber;
string filePath = "widgets.txt";
int workerNumber;
int processorNumber;
int expectedTime;
vector<int> availability;
vector<int> tasksTimes;
vector<Task>tasks;
resultFormat globalResult;
int read_count = 0;
int threadCounter;
bool flag = false;
//semaphores
HANDLE resReadSemaphore; 
HANDLE resWriteSemaphore; 
HANDLE threadCounterSemaphore;
HANDLE flagSemaphore;



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

     
    tasks.resize(taskNumber);
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
    globalResult.bestResult = INT_MAX;
    globalResult.bestAssigns.resize(taskNumber, -1);
    globalResult.threadIndex = -1;
    threadCounter = processorNumber;
    std::cout << "4: enter expected Time to search :\n";
    cin >> expectedTime;
}

int calculateBestAnswer(int index) {
    int bestDifference = 3232131;
    auto start = chrono::steady_clock::now();

    while (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count() < expectedTime) {
        vector<int> workerTimes(workerNumber, 0);
        random_device rd;
        mt19937 gen(rd());
        for (int i = 0; i < taskNumber; ++i) {
            vector<int> availableWorkers;
            for (int j = 0; j < workerNumber; ++j) {
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

        WaitForSingleObject(resReadSemaphore, INFINITE);
        if (difference < globalResult.bestResult) {
            globalResult.bestResult = difference;
            globalResult.threadIndex = index;
            WaitForSingleObject(flagSemaphore, INFINITE);
            flag = true;
            ReleaseSemaphore(flagSemaphore, 1, NULL);
        }
        ReleaseSemaphore(resReadSemaphore, 1, NULL);





    }
    return 0;
}

DWORD WINAPI child(void* param){

    InputParam* inp = static_cast<InputParam*>(param);
    int index = inp->index;


   calculateBestAnswer(index);

    WaitForSingleObject(threadCounterSemaphore, INFINITE);
    threadCounter--;
    ReleaseSemaphore(threadCounterSemaphore, 1, NULL);
    return 0;
}

int main() {
    std::cout << "1: start main function\n";
    readDataFromFile();
    
    flagSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
    resReadSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
    threadCounterSemaphore = CreateSemaphore(NULL, 1, 1, NULL);

    vector<InputParam> threadParameters(processorNumber);
    vector<HANDLE> threadHandles(processorNumber);

    std::cout << "2:creating threads\n" << "______________________\n";
    for (int i = 0; i < processorNumber; ++i) {
        threadParameters[i].index = i;
        threadHandles[i] = CreateThread(NULL, 0, child, &threadParameters[i], 0, NULL);
    }
    while (true)
    {
        if (flag == true) {
            WaitForSingleObject(resReadSemaphore, INFINITE);
            cout << "find a number : " << globalResult.bestResult<< endl;
            WaitForSingleObject(flagSemaphore, INFINITE);
            flag = false;
            ReleaseSemaphore(flagSemaphore, 1, NULL);
            ReleaseSemaphore(resReadSemaphore, 1, NULL);
        }
        else if (threadCounter == 0) {
            break;
        }
    }




    WaitForMultipleObjects(threadHandles.size(), threadHandles.data(), TRUE, INFINITE);


    return 0;
}
