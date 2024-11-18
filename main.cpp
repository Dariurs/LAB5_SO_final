#include <iostream>
#include <windows.h>
#include <cstdlib>
#include <ctime>

using namespace std;

const int MAX_COUNT = 1000;

wchar_t* convertToWideChar(const char* charArray) {
    size_t len = strlen(charArray) + 1;
    wchar_t* wCharArray = new wchar_t[len];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, wCharArray, len, charArray, _TRUNCATE);
    return wCharArray;
}

void startProcess(const char* processName, const char* argument) {
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    char commandLine[256];
    sprintf_s(commandLine, sizeof(commandLine), "%s %s", processName, argument);

    wchar_t* wCommandLine = convertToWideChar(commandLine);

    if (!CreateProcess(
        NULL,
        wCommandLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        cerr << "Eroare la crearea procesului. Cod eroare: " << GetLastError() << endl;
    }
    else {
        cout << "Proces creat cu PID: " << pi.dwProcessId << " cu argumentul: " << argument << endl;
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    delete[] wCommandLine;
}

void childProcessLogic() {
    srand(static_cast<unsigned int>(time(0)) + GetCurrentProcessId());

    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(int),
        L"Global\\SharedMemory"
    );

    if (hMapFile == NULL) {
        cerr << "Nu s-a putut crea/deschide obiectul de memorie partajata. Cod de eroare: " << GetLastError() << endl;
        exit(1);
    }

    int* sharedMemory = static_cast<int*>(MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(int)
    ));

    if (sharedMemory == NULL) {
        cerr << "Nu s-a putut mappa obiectul de memorie. Cod de eroare: " << GetLastError() << endl;
        CloseHandle(hMapFile);
        exit(1);
    }

    if (*sharedMemory == 0) {
        *sharedMemory = 0;
    }

    HANDLE hSemaphore = CreateSemaphore(
        NULL,
        1,
        1,
        L"Global\\MemorySemaphore"
    );

    if (hSemaphore == NULL) {
        cerr << "Nu s-a putut crea/deschide semaforul. Cod de eroare: " << GetLastError() << endl;
        UnmapViewOfFile(sharedMemory);
        CloseHandle(hMapFile);
        exit(1);
    }
    while (*sharedMemory < MAX_COUNT) {
        WaitForSingleObject(hSemaphore, INFINITE);

        int currentValue = *sharedMemory;
        int coinFlip = rand() % 2 + 1;

        cout << "Procesul " << GetCurrentProcessId() << " este in executie." << endl;
        if (coinFlip == 2) {
            *sharedMemory = currentValue + 1;
            cout << "Procesul " << GetCurrentProcessId() << " a incrementat la valoarea: " << *sharedMemory
                << " (banul a picat pe " << coinFlip << ")" << endl;
        }
        else {
            cout << "Procesul " << GetCurrentProcessId() << " nu a incrementat (banul a picat pe " << coinFlip << ")" << endl;
        }

        ReleaseSemaphore(hSemaphore, 1, NULL);
        Sleep(50);
    }
    UnmapViewOfFile(sharedMemory);
    CloseHandle(hMapFile);
    CloseHandle(hSemaphore);
    cout << "Procesul " << GetCurrentProcessId() << " a terminat numaratoarea." << endl;
    exit(0);
}
int main(int argc, char* argv[]) {
    if (argc > 1 && string(argv[1]) == "child") {
        childProcessLogic();
    }

    startProcess(argv[0], "child");
    startProcess(argv[0], "child");

    cout << "Procesul principal a lansat procesele copil." << endl;
    system("pause");
    return 0;
}
