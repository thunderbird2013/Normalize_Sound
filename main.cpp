/*
 * MP3 Normalizer – Multithreaded Audio Normalization Tool
 *
 * Autor         : Matthias Stoltze
 * Datum         : 19.06.2025
 * Uhrzeit       : 18:00 Uhr
 * Version       : 1.0
 * Beschreibung  : Dieses Programm normalisiert die Lautstärke von MP3-Dateien mit ffmpeg,
 *                unterstützt Multithreading und visuelles Monitoring in der Konsole.
 *
 * Lizenz        : Dieses Projekt ist frei verwendbar (Free License).
 *                Nutzung auf eigene Verantwortung. Keine Garantie oder Haftung.
 *
 * Abhängigkeiten: ffmpeg, Windows API, C++17
 * Compiler      : Clang++ 64bit hat man die Besten Ergebnisse erzielt.
 */

#include <windows.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <cstdlib>
#include <windows.h>
#include <iomanip>
#include <map>
#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <io.h>

namespace fs = std::filesystem;

// Globale Einstellungen
std::wstring ffmpegPath;
const std::wstring configFileName = L"config.ini";
const std::wstring errorLogFile = L"failed_jobs.txt";
fs::path inputDir; 
int maxThreads = 4;
bool forceReencode = false;

// Thread-Synchronisierung
std::mutex queueMutex;
std::condition_variable cv;
std::condition_variable monitorCv;
bool done = false;
std::queue<std::pair<fs::path, fs::path>> jobQueue;
std::mutex logMutex;

// Statusüberwachung
enum JobStatus { Pending, Working, Done, Error, Skipped };
std::map<std::wstring, JobStatus> jobStatusMap;
std::mutex statusMutex;

// Lautstärke-Einstellungen
double targetLoudness = -14.0; // Default

// UTF-8 für Konsole aktivieren
void enableUtf8Console() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
}

// Einfache INI-Datei lesen
std::wstring readFfmpegPathFromConfig(const std::wstring& filename) {
    std::wifstream config(filename.c_str());
    std::wstring line;
    while (std::getline(config, line)) {
        if (line.rfind(L"ffmpeg_path=", 0) == 0) {
            return line.substr(12);
        }
    }
    return L"";
}

std::wstring formatSize(std::uintmax_t bytes) {
    double mb = bytes / (1024.0 * 1024.0);
    std::wostringstream oss;
    oss << std::fixed << std::setprecision(2) << mb << L" MB";
    return oss.str();
}

bool normalizeFile(const fs::path& input, const fs::path& output) {
    std::wstringstream cmd;
    cmd << ffmpegPath << L" -hide_banner -loglevel error -y -i \"" << input.wstring()
        << L"\" -af loudnorm=I=" << targetLoudness << L":TP=-1.5:LRA=11 \"" << output.wstring() << L"\"";
    return _wsystem(cmd.str().c_str()) == 0;
}

void workerThread(int id) {
    while (true) {
        std::pair<fs::path, fs::path> job;

        // Job aus der Queue holen
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [] { return !jobQueue.empty() || done; });
            if (jobQueue.empty()) {
                if (done) return; // keine Jobs mehr, fertig
                continue;
            }
            job = jobQueue.front();
            jobQueue.pop();
        }

        // Status auf Working setzen, Monitor benachrichtigen
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            jobStatusMap[job.first.filename().wstring()] = JobStatus::Working;
        }
        monitorCv.notify_one();

        // Job bearbeiten
        bool success = normalizeFile(job.first, job.second);

        // Status updaten und Monitor benachrichtigen
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            jobStatusMap[job.first.filename().wstring()] = success ? JobStatus::Done : JobStatus::Error;
        }
        monitorCv.notify_one();

        // Loggen bei Fehlern, falls nötig
        if (!success) {
            std::lock_guard<std::mutex> logLock(logMutex);
            std::wofstream log(errorLogFile.c_str(), std::ios::app);
            log << job.first.wstring() << std::endl;
        }
    }
}


// Monitor-Thread wartet auf Statusänderungen (monitorCv) und aktualisiert Konsole
void monitorThread() {

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        std::wcerr << L"Konsole nicht verfügbar. Verwende normale Ausgabe.\n";
        return;
    }

    DWORD written;
    DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;

    // Konsole vollständig leeren
    FillConsoleOutputCharacterW(hConsole, L' ', cells, {0, 0}, &written);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cells, {0, 0}, &written);

    // Cursor nach oben links setzen
    SetConsoleCursorPosition(hConsole, {0, 0});

    //std::wstring separator(width - 2, L'=');

     // Konsole: Breite ermitteln und Separator-Zeile ausgeben
    int width = csbi.dwSize.X;
    std::wstring separator(width - 2, L'=');
    std::wcout << L" " << separator << L" \n";

    // Innen-Breite und Startkoordinaten
    const int cellWidth = width - 2;
    const SHORT startX = 0, startY = 1;

    // Cursor verbergen
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(hConsole, &ci);
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &ci);

    // Lambda zum Abschneiden führender Leerzeichen
    auto trimLeading = [&](const std::wstring& s) {
        size_t pos = s.find_first_not_of(L" \t");
        return (pos == std::wstring::npos) ? std::wstring() : s.substr(pos);
    };

    while (true) {
        // Bildschirm löschen
        //SetConsoleCursorPosition(hConsole, {0,0});

        {
            std::lock_guard<std::mutex> lock(statusMutex);
            //std::lock_guard g(statusMutex);        
            int pending=0, working=0, done=0, error=0, skipped=0;
            std::vector<std::wstring> inArbeit;
            std::wstringstream ss;

            for (auto& [f,s] : jobStatusMap) {
                if (s==JobStatus::Pending) ++pending;
                if (s==JobStatus::Working) {++working; inArbeit.push_back(f);}
                if (s==JobStatus::Done) ++done;
                if (s==JobStatus::Error) ++error;
                if (s==JobStatus::Skipped) ++skipped;
            }
      
  // Summary-Zeile
        {
            std::wstringstream ss;
            ss << L" Pending: "  << pending
               << L" - Working: " << working
               << L" - Done: "    << done
               << L" - Error: "   << error
               << L" - Skipped: " << skipped;
            
            auto text = ss.str();        
            if ((int)text.size() > cellWidth) text.resize(cellWidth);

            COORD pos{ startX, startY };
            SetConsoleCursorPosition(hConsole, pos);
            std::wcout << trimLeading(text)
                       << std::wstring(cellWidth - text.size(), L' ');                        
        }

         // 2) Separator direkt darunter
        {
            SetConsoleCursorPosition(hConsole, {startX, SHORT(startY + 1)});
            std::wcout << separator;
        }

        // Fünf Detail-Zeilen
        for (int row = 0; row < maxThreads; ++row) {
            std::wstring detail;
            {
               // if (row < (int)inArbeit.size()) detail = trimLeading(L"[Thread " + std::to_wstring(row + 1) + L"] " + inArbeit[row]);
                if (row < (int)inArbeit.size()) {
                    std::wstring filename = inArbeit[row];
                    fs::path filePath = inputDir / filename;
                    std::wstring sizeStr;

                    try {
                        if (fs::exists(filePath)) {
                            sizeStr = L" (" + formatSize(fs::file_size(filePath)) + L")";
                        }
                    } catch (...) {
                        sizeStr = L"";
                    }

                    detail = trimLeading(L"[Thread " + std::to_wstring(row + 1) + L"] " + filename + sizeStr);
                }
            }
            if ((int)detail.size() > cellWidth) detail.resize(cellWidth);

            COORD pos{ startX, SHORT(startY + 2 + row) };
            SetConsoleCursorPosition(hConsole, pos);
            std::wcout << detail
                       << std::wstring(cellWidth - detail.size(), L' ');
        }
        std::wcout.flush();
          

            // erst am Schluss abbrechen
            if (pending==0 && working==0) break;             
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
     // Cursor am Ende unter den Rahmen setzen
    COORD endPos = {0, SHORT(startY + 7)};
    SetConsoleCursorPosition(hConsole, endPos);
    std::wcout << L"\n";

    // ganz am Ende: Cursor wieder anzeigen
    ci.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &ci);
}




int wmain(int argc, wchar_t* argv[]) {

    enableUtf8Console();
    _setmode(_fileno(stdout), _O_U16TEXT); // zusätzlich nötig für UTF-16-Ausgabe

  std::wcout << L"\n╔════════════════════════════════════════════╗\n"
               << L"║   MP3 Normalizer v1.0 – by Matthias Stoltze║\n"
               << L"║        (c) 2025                            ║\n"
               << L"╚════════════════════════════════════════════╝\n\n";

    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"--help" || arg == L"/?") {
            std::wcout << L"Verwendung:\n"
                       << L"normalize.exe <Input-Ordner> <Output-Ordner> [Threads] [--force] [--target=-14]\n"
                       << L"--force        : Neuverarbeitung auch wenn Datei existiert\n"
                       << L"--target=-14   : Ziel-Lautstärke in dB LUFS (Standard: -14)\n";
            return 0;
        }
    }

    if (argc < 3) {
        std::wcerr << L"Verwendung: normalize.exe <Input-Ordner> <Output-Ordner> [Threads] [--force] [--target=-14]" << std::endl;
        return 1;
    }


    ::inputDir = argv[1];
    fs::path outputDir = argv[2];

    if (argc >= 4) {
        maxThreads = std::stoi(argv[3]);
        if (maxThreads <= 0) {
            maxThreads = std::thread::hardware_concurrency();
            if (maxThreads <= 0) maxThreads = 4;
        }
    }

    for (int i = 4; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"--force") {
            forceReencode = true;
        } else if (arg.rfind(L"--target=", 0) == 0) {
            targetLoudness = std::stod(arg.substr(9));
        }
    }

    if (!fs::exists(inputDir) || !fs::is_directory(inputDir)) {
        std::wcerr << L"Eingabeordner existiert nicht oder ist kein Verzeichnis." << std::endl;
        return 1;
    }

    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }

    ffmpegPath = readFfmpegPathFromConfig(configFileName);
    if (ffmpegPath.empty()) {
        std::wcerr << L"Fehler: ffmpeg_path nicht gefunden in " << configFileName << std::endl;
        return 1;
    }

    std::wofstream clearLog(errorLogFile.c_str(), std::ios::trunc);
    clearLog.close();

    for (const auto& file : fs::directory_iterator(inputDir)) {
        cv.notify_one();
        monitorCv.notify_one();
        if (file.path().extension() == L".mp3") {
            fs::path outFile = outputDir / file.path().filename();
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                jobQueue.emplace(file.path(), outFile);
            }
            {
                std::lock_guard<std::mutex> statusLock(statusMutex);
                jobStatusMap[file.path().filename().wstring()] = JobStatus::Pending;
            }
        }
    }


    std::vector<std::thread> threads;
    for (int i = 0; i < maxThreads; ++i) {
        threads.emplace_back(workerThread, i + 1);
    }

        std::thread monitor(monitorThread);

    {
        std::unique_lock<std::mutex> lock(queueMutex);
        done = true;
    }
    cv.notify_all();

    for (auto& t : threads) {
        t.join();
    }

    monitor.join();

    std::wcout << L"\n✅ Fertig. Alle MP3s wurden verarbeitet.";
    if (fs::exists(errorLogFile)) {
        std::wcout << L"\nEinige Dateien konnten nicht verarbeitet werden. Siehe: " << errorLogFile;
    }
    std::wcout << L"\n";
    return 0;
}
