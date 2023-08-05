#include <iostream>
#include <stdlib.h>
#include <filesystem>
#include <Windows.h>
#include <tlhelp32.h>
#include <thread>
#include <fstream>
#include <string>

std::string STEAM_EXE = "";
std::string WORKING_DIRECTORY = "";
std::string PATH_TO_FILES_WORLD_LOCAL = "";
std::string USER = "";
const std::string ARGUMENTS_STEAM = "\"steam://rungameid/892970\"";
const std::wstring VALHEIM_EXE = L"valheim.exe";
const std::chrono::duration<int> EXECUTION_CHECK_INTERVAL(10);
const std::string FILE_FLAG_USE_SERVER = "IsServerUse";
const std::string FILE_WORLD_DB = "SlivSliv.db";
const std::string FILE_WORLD_FWL = "SlivSliv.fwl";
const std::string CONFIG_FILE = "config";

std::string GetEnv(std::string name)
{
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, name.c_str()) == 0 && buf != nullptr)
    {
        std::string result = buf;
        free(buf);
        return result;
    }

    return "";
}

void SetWorkingDirectory(std::string argv_0)
{
    WORKING_DIRECTORY = argv_0.substr(0, argv_0.find_last_of("\\")) + "\\";
}

bool IsServerUse(std::string& user)
{
    std::ifstream file;
    file.open(WORKING_DIRECTORY + FILE_FLAG_USE_SERVER, std::ios::in);

    if (!file.is_open())
    {
        return false;
    }

    std::string strActive = "";
    std::getline(file, strActive);
    std::getline(file, user);
    file.close();
    if (strActive == "1")
    {
        return true;
    }
    
    return false;
}

bool SetServerUseStatus(std::string status)
{
    std::ofstream file;
    file.open(WORKING_DIRECTORY + FILE_FLAG_USE_SERVER, std::ios::out);

    if (!file.is_open())
    {
        return false;
    }

    file << status;
    file.close();

    return true;
}

bool SetServerUse()
{
    return SetServerUseStatus("1\n" + USER);
}

bool SetServerNotUse()
{
    return SetServerUseStatus("0");
}

void CopyFileToCatalogGame()
{
    if (std::filesystem::exists(PATH_TO_FILES_WORLD_LOCAL + FILE_WORLD_DB))
    {
        std::remove(std::string(PATH_TO_FILES_WORLD_LOCAL + FILE_WORLD_DB).c_str());
    }
    std::filesystem::copy_file(WORKING_DIRECTORY + FILE_WORLD_DB, PATH_TO_FILES_WORLD_LOCAL + FILE_WORLD_DB);

    if (std::filesystem::exists(PATH_TO_FILES_WORLD_LOCAL + FILE_WORLD_FWL))
    {
        std::remove(std::string(PATH_TO_FILES_WORLD_LOCAL + FILE_WORLD_FWL).c_str());
    }
    std::filesystem::copy_file(WORKING_DIRECTORY + FILE_WORLD_FWL, PATH_TO_FILES_WORLD_LOCAL + FILE_WORLD_FWL);
}

void CopyFileToRepository()
{
    if (std::filesystem::exists(WORKING_DIRECTORY + FILE_WORLD_DB))
    {
        std::remove(std::string(WORKING_DIRECTORY + FILE_WORLD_DB).c_str());
    }
    std::filesystem::copy_file(PATH_TO_FILES_WORLD_LOCAL + FILE_WORLD_DB, WORKING_DIRECTORY + FILE_WORLD_DB);

    if (std::filesystem::exists(WORKING_DIRECTORY + FILE_WORLD_FWL))
    {
        std::remove(std::string(WORKING_DIRECTORY + FILE_WORLD_FWL).c_str());
    }
    std::filesystem::copy_file(PATH_TO_FILES_WORLD_LOCAL + FILE_WORLD_FWL, WORKING_DIRECTORY + FILE_WORLD_FWL);
}

void DownloadRepository()
{
    std::rename(std::string(WORKING_DIRECTORY + CONFIG_FILE).c_str(), std::string(WORKING_DIRECTORY + CONFIG_FILE + ".tmp").c_str());

    std::string cmd = "";
    cmd = "git -C " + WORKING_DIRECTORY + " checkout .";
    system(cmd.c_str());
    cmd = "git -C " + WORKING_DIRECTORY + " pull";
    system(cmd.c_str());

    std::remove(std::string(WORKING_DIRECTORY + CONFIG_FILE).c_str());
    std::rename(std::string(WORKING_DIRECTORY + CONFIG_FILE + ".tmp").c_str(), std::string(WORKING_DIRECTORY + CONFIG_FILE).c_str());
}

void UploadRepository()
{
    std::string cmd = "";
    cmd = "git -C " + WORKING_DIRECTORY + " add " + FILE_FLAG_USE_SERVER + " " + FILE_WORLD_DB + " " + FILE_WORLD_FWL;
    system(cmd.c_str());
    cmd = "git -C " + WORKING_DIRECTORY + " commit -m \"~\"";
    system(cmd.c_str());
    cmd = "git -C " + WORKING_DIRECTORY + " push";
    system(cmd.c_str());
}

bool IsProcessRunning(const wchar_t* processName)
{
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry))
        while (Process32Next(snapshot, &entry))
            if (!_wcsicmp(entry.szExeFile, processName))
                exists = true;

    CloseHandle(snapshot);
    return exists;
}

bool ReadConfig()
{
    std::ifstream file;
    file.open(WORKING_DIRECTORY + CONFIG_FILE, std::ios::in);

    if (!file.is_open())
    {
        return false;
    }

    std::getline(file, STEAM_EXE);
    STEAM_EXE += "\\steam.exe";
    std::getline(file, USER);
    file.close();

    return true;
}

int GO(std::string argv_0)
{
    SetWorkingDirectory(argv_0);

    if (!ReadConfig())
    {
        std::cout << "ERROR: config is incorrect\n";
        std::getchar();
        return 0;
    }

    if (STEAM_EXE.empty())
    {
        std::cout << "ERROR: exe file steam not found\n";
        std::getchar();
        return 0;
    }

    if (!std::filesystem::exists(STEAM_EXE))
    {
        std::cout << "ERROR: exe file steam not found\n";
        std::getchar();
        return 0;
    }

    if (USER.empty())
    {
        std::cout << "ERROR: user from config not found\n";
        std::getchar();
        return 0;
    }

    std::string appData = GetEnv("APPDATA");
    PATH_TO_FILES_WORLD_LOCAL = appData.substr(0, appData.find("Roaming")) + "LocalLow\\IronGate\\Valheim\\worlds_local\\";

    if (!std::filesystem::exists(PATH_TO_FILES_WORLD_LOCAL))
    {
        std::filesystem::create_directory(PATH_TO_FILES_WORLD_LOCAL);
    }

    DownloadRepository();

    std::string whoStart = "";
    if (IsServerUse(whoStart))
    {
        std::cout << "ERROR: server is already running: " << whoStart << std::endl;
        std::getchar();
        return 0;
    }

    if (!SetServerUse())
    {
        std::cout << "ERROR: server status settings failed\n";
        std::getchar();
        return 0;
    }

    UploadRepository();
    CopyFileToCatalogGame();

    std::string cmd = STEAM_EXE + " " + ARGUMENTS_STEAM;
    WinExec(cmd.c_str(), SW_SHOWNORMAL);

    do
    {
        std::this_thread::sleep_for(EXECUTION_CHECK_INTERVAL);
        std::cout << "INFO: process is running" << std::endl;
    } while (IsProcessRunning(VALHEIM_EXE.c_str()));

    CopyFileToRepository();
    if (!SetServerNotUse())
    {
        std::cout << "ERROR: server status settings failed\n";
        std::getchar();
        return 0;
    }

    UploadRepository();

    std::cout << "End\n";
}

int main(int argc, char* argv[])
{
    try
    {
        return GO(argv[0]);
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        std::getchar();
    }

    return 0;
}

