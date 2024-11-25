#include <iostream>
#include <windows.h>
#include <fstream>
#include <vector>
#include <cstdlib> 
using namespace std;

LPCWSTR childExePath = L"C:\\code\\cs\\child\\x64\\Debug\\child.exe %d";
string filePath = "widgets.txt";

int main() {

	int task_number, worker_number, processorNumber;
	HANDLE _taskHandle,_reportHandle,_basicsHandle,_availabilityHandle, _resultHandle{};
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
	_resultHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int) * processorNumber, L"results");
	_reportHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int) * processorNumber * task_number, L"reports");
	vector<HANDLE> processes(processorNumber);
	for (int i = 0; i < processorNumber; ++i) {
		STARTUPINFO si = { sizeof(STARTUPINFO) };
		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));
		wchar_t cmdLine[100];
		swprintf(cmdLine, 100, childExePath, i);
		if (!CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			cout << "Error creating child process." << endl;
			exit(1);
		}
		processes[i] = pi.hProcess;
		CloseHandle(pi.hThread);
	}

	WaitForMultipleObjects(processorNumber, processes.data(), TRUE, INFINITE);
	cout << "-------------------------------------FINISHED IN PARENT-----------------------------------------\n";
	int bestResult = 1230030;
	int bestproccess = 0;
	vector<int> assignsReport (task_number);
	int* sharedResult = (int*)MapViewOfFile(_resultHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * processorNumber);
	int* sharedReportWorkerAssigns = (int*)MapViewOfFile(_reportHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * processorNumber * task_number);
	
	if (sharedResult != NULL) {
		for (int i = 0; i < processorNumber; i++) {
			if (sharedResult[i] < bestResult) {
				bestResult = sharedResult[i];
				bestproccess = i;
			}
		}
		cout << "best procc :" << bestproccess << endl;
		UnmapViewOfFile(sharedResult);
		int startindex = bestproccess * task_number;
		for (int j = 0; j < task_number; j++) {
			assignsReport[j] = sharedReportWorkerAssigns[startindex];
			startindex++; 
		}
		UnmapViewOfFile(sharedReportWorkerAssigns);
	}
	cout << "bset allocation ;";
	for (int i = 0; i < task_number; i++) {
		cout << assignsReport[i] << "\t";
	}

	cout << "\ncost: " << bestResult << endl;
	cout << "find by proccess No.: " << bestproccess << endl;

	//? closing handles
	CloseHandle(_resultHandle);
	CloseHandle(_reportHandle);
	CloseHandle(_basicsHandle);
	CloseHandle(_taskHandle);
	CloseHandle(_availabilityHandle);

	return 0;
}
