#include <iostream>
#include <windows.h>
#include <fstream>
#include <vector>
#include <cstdlib> 
using namespace std;

childExePath = L"[place your child exe path] %d";
string filePath ="[place your data file path]";

int main() {
	int task_number, worker_number, processorNumber;
//? handles 
	HANDLE _taskHandle;
	HANDLE _basicsHandle;
	HANDLE _availabilityHandle;
	HANDLE _resultHandle{};
	SYSTEM_INFO sysInfo;
	
	ifstream dataFile(filePath);
	GetSystemInfo(&sysInfo);

	processorNumber = sysInfo.dwNumberOfProcessors;
//? read from file
	//  insert basic informations to shared memory 
	_basicsHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int) * 3, L"basics");
	int* SharedMemoryBasics = (int*)MapViewOfFile(_basicsHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * 3);
	dataFile >> task_number >> worker_number;
	SharedMemoryBasics[0] = task_number;
	SharedMemoryBasics[1] = worker_number;
	SharedMemoryBasics[2] = processorNumber;
	UnmapViewOfFile(SharedMemoryBasics);
	//  insert task times to shared memory 
	_taskHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int) * task_number, L"tasks");
	int* SharedMemoryTasks = (int*)MapViewOfFile(_taskHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * task_number);
	vector<int>tasks(task_number);
	for (int i = 0; i < task_number; i++)
	{
		dataFile >> tasks[i];
		SharedMemoryTasks[i] = tasks[i];
	}
	UnmapViewOfFile(SharedMemoryTasks);
	// insert availability to shared memory 
	_availabilityHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int) * task_number * worker_number, L"availebility");
	int* SharedMemoryAvailability = (int*)MapViewOfFile(_availabilityHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * task_number * worker_number);
	vector<int> availebility(task_number * worker_number, 0);
	for (int i = 0; i < task_number * worker_number; i++)
	{
		dataFile >> availebility[i];
		SharedMemoryAvailability[i] = availebility[i];
	}
	UnmapViewOfFile(SharedMemoryAvailability);
	cout << "\n";
	dataFile.close();

//? creating proccess
	_resultHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int)* processorNumber, L"results");
	vector<HANDLE> processes(processorNumber);
	for (int i = 0; i < processorNumber; ++i) {
			STARTUPINFO si = { sizeof(STARTUPINFO) };
		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));
		wchar_t cmdLine[100];
		swprintf(cmdLine, 100,childExePath, i);
		if (!CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			cout << "Error creating child process." << endl;
			exit(1);
		}

		processes[i] = pi.hProcess;
		CloseHandle(pi.hThread);
	}
		
	WaitForMultipleObjects(processorNumber, processes.data(), TRUE, INFINITE);
	cout << "-------------------------------------FINISHED IN PARENT-----------------------------------------";
	int bestResult = 123021302130;
	int* sharedResult = (int*)MapViewOfFile(_resultHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int)*processorNumber);
	if (sharedResult != NULL) {
		bestResult = *sharedResult;
		UnmapViewOfFile(sharedResult);
	}
	cout << "cost: " << bestResult << endl;

//? closing handles
	CloseHandle(_resultHandle);
	CloseHandle(_basicsHandle);
	CloseHandle(_taskHandle);
	CloseHandle(_availabilityHandle);

	return 0;
}
