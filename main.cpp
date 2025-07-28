#include <windows.h>
#include <iostream>
#include <string>

int main() {
    LPCSTR serialPortName = "\\\\.\\COM3"; // Use appropriate COM port
    HANDLE hSerial = CreateFileA(
        serialPortName,
        GENERIC_READ | GENERIC_WRITE,
        0, // no sharing
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening serial port.\n";
        return 1;
    }

    // Configure serial port
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Failed to get serial parameters.\n";
        return 1;
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Failed to set serial parameters.\n";
        return 1;
    }

    // Set up STARTUPINFO to redirect stdin/stdout
    STARTUPINFOA si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = hSerial;
    si.hStdOutput = hSerial;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE); // Optionally redirect stderr

    std::string command = "cmd.exe /c dir";  // Replace with desired command
    char cmdLine[MAX_PATH];
    strncpy_s(cmdLine, command.c_str(), MAX_PATH);

    if (!CreateProcessA(
        NULL,
        cmdLine,
        NULL,
        NULL,
        TRUE,                // Inherit handles
        0,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        std::cerr << "Failed to launch process.\n";
        CloseHandle(hSerial);
        return 1;
    }

    // Wait for the process to finish
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Cleanup
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hSerial);

    std::cout << "Process completed and output redirected to serial.\n";
    return 0;
}
