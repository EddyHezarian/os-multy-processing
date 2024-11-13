#include <iostream>
#include <windows.h>
#include <vector>
#include <chrono>
#include <random>
#include <cstdlib> 

using namespace std; 



struct Task {
    int duration;
    vector<int> workers;
};


vector<Task> importDataFromSharedMemory(int& numTasks,int& numWorkers,int& numProccess) {
    //basics
    HANDLE basicsHandle = OpenFileMapping(FILE_MAP_READ, FALSE, L"basics");
    if (basicsHandle == NULL) {
        cerr << "Error opening file mapping." << endl;
        exit(1);
    }
    
    int* SharedMemoryBasics = (int*)MapViewOfFile(basicsHandle, FILE_MAP_READ, 0, 0, sizeof(int) * 3);
    numTasks = SharedMemoryBasics[0];
    numWorkers = SharedMemoryBasics[1];
    numProccess = SharedMemoryBasics[2];

    cout << numTasks << "\t" << numProccess << "\t" << numWorkers << endl;
    vector<Task>tasks(numTasks);
    //
    HANDLE TasksHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, L"tasks");
    if (TasksHandle == NULL) {
        cerr << "Error opening file mapping." << endl;
        exit(1);
    }
    int* SharedMemoryTasks = (int*)MapViewOfFile(TasksHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int)*numTasks);
    for (int i = 0; i < numTasks; i++) {
        tasks[i].duration = SharedMemoryTasks[i];
    }

    
    HANDLE AvailebilityHandle = OpenFileMapping(FILE_MAP_READ, FALSE, L"availebility");
    if (basicsHandle == NULL) {
        cerr << "Error opening file mapping." << endl;
        exit(1);
    }
    int* SharedMemoryAvailebility = (int*)MapViewOfFile(AvailebilityHandle, FILE_MAP_READ, 0, 0, sizeof(int) * numTasks * numWorkers);
    int taskindex = 0;
    vector<int>tmp;
    for (int i = 1; i <= numTasks*numWorkers; i++)
    {

        if(i%numWorkers == 0){
          
            tmp.push_back(SharedMemoryAvailebility[i - 1]);
            for (int p = 0; p < numWorkers; p++) {
                tasks[taskindex].workers.push_back(tmp[p]);

            }
            tmp.clear();
            taskindex++;
        }
        else {
            tmp.push_back(SharedMemoryAvailebility[i - 1]);
        }
    }
    

    return tasks;
}

int calculateBestAnswer(vector<Task>tasks ,int numTasks ,int numWorkers) {
    int bestDifference = 3232131;
    auto start = chrono::steady_clock::now();
    while (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count() < 30) {
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


int main(int argc, char* argv[]) {

    if (argc < 2) {
        cerr << "Index of process is required." << endl;
        return 1;
    }
    int index = atoi(argv[1]);
    int bestDifference; 
    int numTasks ; 
    int numWorkers ;
    int numProccess ;
    vector<Task>tasks;
    tasks =importDataFromSharedMemory(numTasks, numWorkers, numProccess);

    bestDifference = calculateBestAnswer(tasks, numTasks, numWorkers);
    
    HANDLE ResultsHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, L"results");
    int* SharedMemoryResult = (int *)MapViewOfFile(ResultsHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * numProccess);

    SharedMemoryResult[index] = bestDifference;

    UnmapViewOfFile(SharedMemoryResult);
    CloseHandle(ResultsHandle);

    return 0;


}