#define UNICODE
#define _UNICODE

#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <cstdio>
#include <ctime>

#pragma comment(lib, "winhttp.lib")
int httpTimeoutMs = 60000;
std::unordered_map<std::string, std::string> commandMap;

struct ParsedCommand {
    std::string base;
    std::unordered_map<std::string, std::string> params;
};

std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}
#include <iomanip>
#include <sstream>

std::string UriDecode(const std::string& encoded) {
    std::ostringstream decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hex = encoded.substr(i + 1, 2);
            char c = static_cast<char>(std::stoi(hex, nullptr, 16));
            decoded << c;
            i += 2;
        } else if (encoded[i] == '+') {
            decoded << ' ';
        } else {
            decoded << encoded[i];
        }
    }
    return decoded.str();
}

ParsedCommand ParseCommandEntry(const std::string& entry) {
    ParsedCommand result;
    size_t eq = entry.find('=');
    if (eq == std::string::npos) {
        result.base = entry;
        return result;
    }

    result.base = entry.substr(0, eq);
    std::string rest = entry.substr(eq + 1);
    size_t next_eq = rest.find('=');
    if (next_eq != std::string::npos) {
        std::string key = rest.substr(0, next_eq);
        std::string val = rest.substr(next_eq + 1);
        result.params[key] = val;
    } else {
        result.params["MODEL"] = rest;
    }

    return result;
}

bool LoadCommandMap(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        size_t sep = line.find(':');
        if (sep != std::string::npos && line.length() >= 3) {
            std::string key = Trim(line.substr(0, sep));
            std::string value = Trim(line.substr(sep + 1));
            commandMap[key] = value;
        }
    }
    return true;
}

void LogResponseToFile(const std::string& response) {
    std::ofstream log("log.txt", std::ios::app);
    if (log.is_open()) {
        std::time_t now = std::time(nullptr);
        log << std::ctime(&now) << response << "\n\n";
        log.close();
    }
}

void SendBufferToSerial(const std::string& buffer, HANDLE hSerial, int delayMs, bool appendCRLF = true, bool crOnly = false) {
    std::string fullBuffer = buffer;
    if (appendCRLF)
        fullBuffer += crOnly ? "\r" : "\r\n";

    for (char c : fullBuffer) {
        DWORD written;
        WriteFile(hSerial, &c, 1, &written, NULL);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
}

std::string ReadThreeCharCommand(HANDLE hSerial) {
    std::string command;
    DWORD bytesRead;
    while (command.length() < 3) {
        char c;
        if (ReadFile(hSerial, &c, 1, &bytesRead, NULL) && bytesRead == 1) {
            command += c;
            std::cout << c << std::flush;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    std::cout << std::endl;
    return command;
}

std::string ReadLineFromSerial(HANDLE hSerial) {
    std::string line;
    DWORD bytesRead;
    char c;
    while (true) {
        if (ReadFile(hSerial, &c, 1, &bytesRead, NULL) && bytesRead == 1) {
            if (c == '\r' || c == '\n') break;
            line += c;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    return line;
}

extern int httpTimeoutMs;
std::string SendToOllama(const std::string& prompt, const std::string& model) {
    HINTERNET hSession = WinHttpOpen(L"AskClient/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    if (!hSession) return "Failed to open WinHTTP session.";

    WinHttpSetTimeouts(hSession, 5000, 5000, 5000, httpTimeoutMs);

    HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 11434, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "Failed to connect to Ollama.";
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"POST", L"/api/generate",
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "Failed to open request.";
    }

    std::ostringstream oss;
    oss << "{ \"model\": \"" << model << "\", \"prompt\": \"" << prompt << "\", \"stream\": false }";
    std::string payload = oss.str();

    BOOL result = WinHttpSendRequest(
        hRequest,
        L"Content-Type: application/json\r\n",
        -1L,
        (LPVOID)payload.c_str(),
        (DWORD)payload.length(),
        (DWORD)payload.length(),
        0
    );

    if (!result || !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "Failed to send or receive HTTP request.";
    }

    std::string response;
    DWORD size = 0;
    do {
        DWORD downloaded = 0;
        WinHttpQueryDataAvailable(hRequest, &size);
        if (size == 0) break;

        char* buffer = new char[size + 1];
        ZeroMemory(buffer, size + 1);

        if (WinHttpReadData(hRequest, buffer, size, &downloaded))
            response.append(buffer, downloaded);

        delete[] buffer;
    } while (size > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    LogResponseToFile(response);

    size_t pos = response.find("\"response\":\"");
    if (pos != std::string::npos) {
        pos += 12;
        size_t end = response.find("\"", pos);
        std::string raw = response.substr(pos, end - pos);
        return UriDecode(raw);
    }


    return "Ollama response missing.";
}

void HandleAskCommand(HANDLE hSerial, int delayMs, const std::string& model) {
    SendBufferToSerial("\rEnter your question:", hSerial, delayMs);
    SendBufferToSerial("\r>", hSerial, delayMs, false);
    std::string question = ReadLineFromSerial(hSerial);
    SendBufferToSerial("\rThinking...", hSerial, delayMs);
    std::string reply = SendToOllama(question, model);
    SendBufferToSerial(reply, hSerial, delayMs);
}

void HandleHelpCommand(HANDLE hSerial, int delayMs) {
    std::ifstream helpFile("help.txt");
    if (!helpFile.is_open()) {
        SendBufferToSerial("\rHelp file not found.", hSerial, delayMs);
        return;
    }
    std::string line;
    while (std::getline(helpFile, line)) {
        SendBufferToSerial(line, hSerial, delayMs);
    }
    helpFile.close();
}

void ExecuteCommandAndSendOutput(const std::string& command, HANDLE hSerial, int delayMs) {
    auto it = commandMap.find(command);
    if (it == commandMap.end()) {
        SendBufferToSerial("\rUnknown command: " + command, hSerial, delayMs);
        SendBufferToSerial("8->", hSerial, delayMs, false);
        return;
    }

    ParsedCommand parsed = ParseCommandEntry(it->second);

    if (parsed.base == "ASK") {
        std::string model = "llama3";
        if (parsed.params.count("MODEL")) {
            model = parsed.params["MODEL"];
        }
        HandleAskCommand(hSerial, delayMs, model);
        SendBufferToSerial("8->", hSerial, delayMs, false);
        return;
    }

    if (parsed.base == "HLP") {
        HandleHelpCommand(hSerial, delayMs);
        SendBufferToSerial("8->", hSerial, delayMs, false);
        return;
    }

    FILE* pipe = _popen(parsed.base.c_str(), "r");
    if (!pipe) {
        SendBufferToSerial("\rFailed to run command", hSerial, delayMs);
        SendBufferToSerial("8->", hSerial, delayMs, false);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), pipe)) {
        std::string lineStr(line);
        lineStr.erase(lineStr.find_last_not_of("\r\n") + 1);
        SendBufferToSerial(lineStr, hSerial, delayMs);
    }
    _pclose(pipe);
    SendBufferToSerial("8->", hSerial, delayMs, false);
}

#include <nlohmann/json.hpp>
using json = nlohmann::json;

int main(int argc, char* argv[]) {
    std::string comPort, initialMessage;
    int sendDelayMs = 0;

    if (argc == 4) {
        comPort = argv[1];
        sendDelayMs = atoi(argv[2]);
        initialMessage = argv[3];
    } else {
        std::ifstream configFile("config.json");
        if (!configFile.is_open()) {
            std::cerr << "Usage: " << argv[0] << " COM_PORT DELAY_MS \"INITIAL_MESSAGE\" or provide config.json\n";
            return 1;
        }

        json config;
        configFile >> config;
        comPort = config["com_port"].get<std::string>();
        sendDelayMs = config["send_delay_ms"].get<int>();
        initialMessage = config["initial_message"].get<std::string>();
        if (config.contains("http_timeout_ms")) httpTimeoutMs = config["http_timeout_ms"].get<int>();
    }

    if (!LoadCommandMap("cmds.csv")) {
        std::cerr << "Failed to load cmds.csv\n";
        return 1;
    }

    std::wstring portArgW(comPort.begin(), comPort.end());
    std::wstring fullPortName;
 if (portArgW.rfind(L"\\\\.\\", 0) == 0)
    fullPortName = portArgW;
else
    fullPortName = L"\\\\.\\\\" + portArgW;


    HANDLE hSerial = CreateFile(fullPortName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Failed to open " << fullPortName << L". Error: " << GetLastError() << std::endl;
        return 1;
    }

    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(dcb);
    GetCommState(hSerial, &dcb);
    dcb.BaudRate = CBR_2400;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(hSerial, &dcb);
    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

    SendBufferToSerial(std::string(1, 0x0C), hSerial, sendDelayMs, false);
    SendBufferToSerial(initialMessage, hSerial, sendDelayMs, true, true);
    SendBufferToSerial("8->", hSerial, sendDelayMs, false);

    while (true) {
        std::string cmd = ReadThreeCharCommand(hSerial);
        ExecuteCommandAndSendOutput(cmd, hSerial, sendDelayMs);
    }

    CloseHandle(hSerial);
    return 0;
}
