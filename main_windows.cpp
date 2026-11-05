#include <iostream>
#include <windows.h>
#include <string>
#include <cmath>

bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i <= std::sqrt(n); ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

void run_child(HANDLE hRead, HANDLE hWrite) {
    int bounds[2];
    DWORD bytesRead, bytesWritten;

    ReadFile(hRead, bounds, sizeof(bounds), &bytesRead, NULL);
    CloseHandle(hRead);

    int start = bounds[0];
    int end = bounds[1];

    for (int num = start; num <= end; ++num) {
        if (is_prime(num)) {
            WriteFile(hWrite, &num, sizeof(num), &bytesWritten, NULL);
        }
    }

    int sentinel = -1;
    WriteFile(hWrite, &sentinel, sizeof(sentinel), &bytesWritten, NULL);
    CloseHandle(hWrite);
}

int main(int argc, char* argv[]) {
    // În modul copil, procesul își citește argumentele primite prin linia de comandă
    if (argc == 4 && std::string(argv[1]) == "child") {
        HANDLE hRead = (HANDLE)(uintptr_t)std::stoull(argv[2]);
        HANDLE hWrite = (HANDLE)(uintptr_t)std::stoull(argv[3]);
        run_child(hRead, hWrite);
        return 0;
    }

    // === PROCES PĂRINȚI ===
    const int NUM_PROCESSES = 10;
    const int INTERVAL_SIZE = 1000;

    HANDLE hParentWrite[NUM_PROCESSES];
    HANDLE hChildRead[NUM_PROCESSES];
    HANDLE hProcessHandles[NUM_PROCESSES];

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE; // Permite moștenirea handle-urilor
    saAttr.lpSecurityDescriptor = NULL;

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    // Creează pipe-urile și cele 10 procese copil cu CreateProcess
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        HANDLE hReadP2C, hWriteP2C;
        HANDLE hReadC2P, hWriteC2P;

        CreatePipe(&hReadP2C, &hWriteP2C, &saAttr, 0);
        CreatePipe(&hReadC2P, &hWriteC2P, &saAttr, 0);

        hParentWrite[i] = hWriteP2C;
        hChildRead[i] = hReadC2P;

        std::string cmd = "\"" + std::string(exePath) + "\" child " +
                          std::to_string((uintptr_t)hReadP2C) + " " +
                          std::to_string((uintptr_t)hWriteC2P);

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        CreateProcessA(NULL, &cmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

        hProcessHandles[i] = pi.hProcess;
        CloseHandle(pi.hThread);

        CloseHandle(hReadP2C);
        CloseHandle(hWriteC2P);
    }

    // Trimite intervalele către copii
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int bounds[2] = { i * INTERVAL_SIZE + 1, (i + 1) * INTERVAL_SIZE };
        DWORD bytesWritten;
        WriteFile(hParentWrite[i], bounds, sizeof(bounds), &bytesWritten, NULL);
        CloseHandle(hParentWrite[i]);
    }

    // Citește și afișează numerele prime
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int prime;
        DWORD bytesRead;
        std::cout << "\n============================================\n";
        std::cout << " Procesul Copil " << (i + 1) << " | Interval [" 
                  << (i * INTERVAL_SIZE + 1) << " - " << ((i + 1) * INTERVAL_SIZE) << "]\n";
        std::cout << "============================================\n";

        while (ReadFile(hChildRead[i], &prime, sizeof(prime), &bytesRead, NULL) && bytesRead > 0) {
            if (prime == -1) break;
            std::cout << prime << " ";
        }
        std::cout << "\n";
        CloseHandle(hChildRead[i]);
    }

    WaitForMultipleObjects(NUM_PROCESSES, hProcessHandles, TRUE, INFINITE);
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        CloseHandle(hProcessHandles[i]);
    }

    std::cout << "\n[SUCCESS] Executie finalizata cu succes pe Windows.\n";
    return 0;
}