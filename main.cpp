#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <windows.h>
#include <winbase.h>

#define CONFIG_FILE_PATH "config.ini"
#define RUNNING_MUTEX "Global\\ValorantTrueStretchRunning"

bool find_best(const std::string& name, int aspect_ratio_x, int aspect_ratio_y, DEVMODE& dev_mode);
bool get_screen_name(int index, std::string& name);
inline void error(const char* message);
int show_message_box_with_timeout(int timeout, const std::string& message, const std::string& title);

bool find_best(const std::string& name, const int aspect_ratio_x, const int aspect_ratio_y, DEVMODE& dev_mode)
{
    DEVMODE current;
    bool has_found = false;
    int i = 0;

    while (EnumDisplaySettings(name.c_str(), i++, &current))
    {
        if (current.dmPelsHeight * aspect_ratio_x != current.dmPelsWidth * aspect_ratio_y)
        {
            continue;
        }

        if (has_found && current.dmDisplayFrequency < dev_mode.dmDisplayFrequency)
        {
            continue;
        }

        if (has_found && current.dmDisplayFrequency == dev_mode.dmDisplayFrequency && current.dmPelsWidth * current.dmPelsHeight < dev_mode.dmPelsWidth * dev_mode.dmPelsHeight)
        {
            continue;
        }

        dev_mode = current;
        has_found = true;
    }

    return has_found;
}

bool get_screen_name(const int index, std::string& name)
{
    DISPLAY_DEVICE display_device;
    display_device.cb = sizeof(DISPLAY_DEVICE);

    if (EnumDisplayDevices(nullptr, index, &display_device, 0))
    {
        name = display_device.DeviceName;
        return true;
    }

    return false;
}

inline void error(const char* message)
{
    MessageBoxA(nullptr, message, "Error", MB_OK | MB_ICONWARNING);
}

int show_message_box_with_timeout(const int timeout, const std::string& message, const std::string& title)
{
    const HMODULE user32 = LoadLibrary("user32.dll");
    if (!user32)
    {
        return IDNO;
    }

    typedef int (WINAPI*MessageBoxTimeout_t)(HWND, LPCSTR, LPCSTR, UINT, WORD, DWORD);
    const auto MessageBoxTimeout = reinterpret_cast<MessageBoxTimeout_t>(GetProcAddress(user32, "MessageBoxTimeoutA"));

    if (!MessageBoxTimeout)
    {
        FreeLibrary(user32);
        return IDNO;
    }

    const int result = MessageBoxTimeout(
        nullptr,
        message.c_str(),
        title.c_str(),
        MB_YESNO | MB_ICONQUESTION,
        0,
        timeout
    );

    FreeLibrary(user32);
    return result;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    const auto mutex = CreateMutexA(nullptr, TRUE, RUNNING_MUTEX);

    try
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Another instance of this application is already running.");
            return 1;
        }

        std::ifstream config_file(CONFIG_FILE_PATH);

        if (!config_file.is_open())
        {
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Could not open config file.");
            return 1;
        }

        std::string line;
        if (!std::getline(config_file, line))
        {
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Config header not present.");
            return 1;
        }

        if (strcmp(line.c_str(), "[ValorantTrueStretch]") != 0)
        {
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Wrong config header.");
            return 1;
        }

        std::map<std::string, std::string> config;

        while (std::getline(config_file, line))
        {
            if (line.empty())
            {
                continue;
            }

            if (line.at(0) == '#')
            {
                continue;
            }

            const unsigned long off = line.find('=');
            if (off == -1)
            {
                ReleaseMutex(mutex);
                CloseHandle(mutex);
                error("Invalid config entry.");
                return 1;
            }


            config.emplace(line.substr(0, off), line.substr(off + 1, line.length() - off - 1));
        }

        int screen_index;
        int aspect_ratio_x;
        int aspect_ratio_y;
        { // parse config
            try
            {
                screen_index = std::stoi(config["screen_index"]);
            }
            catch (std::exception& _)
            {
                ReleaseMutex(mutex);
                CloseHandle(mutex);
                error("Configuration value screen_index is not a valid number.");
                return 1;
            }

            try
            {
                const std::string aspect_ratio = config["aspect_ratio"];
                const unsigned long off = aspect_ratio.find(':');
                if (off == -1)
                {
                    ReleaseMutex(mutex);
                    CloseHandle(mutex);
                    error("Invalid aspect ratio string.");
                    return 1;
                }

                aspect_ratio_x = std::stoi(aspect_ratio.substr(0, off));
                aspect_ratio_y = std::stoi(aspect_ratio.substr(off + 1, aspect_ratio.length() - off - 1));
            }
            catch (std::exception& _)
            {
                ReleaseMutex(mutex);
                CloseHandle(mutex);
                error("Configuration value aspect_ratio needs formatting x:y where x and y is a number");
                return 1;
            }
        }

        auto window = FindWindowA("VALORANTUnrealWindow", nullptr);
        if (window)
        {
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Please close VALORANT.");
            return 1;
        }

        do
        {
            window = FindWindowA("VALORANTUnrealWindow", nullptr);
        }
        while (!window);

        DWORD process_id;
        GetWindowThreadProcessId(window, &process_id);

        const auto process = OpenProcess(SYNCHRONIZE, FALSE, process_id);
        if (!process)
        {
            CloseHandle(window);
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Could not open Valorant process.");
            return 1;
        }

        std::string screen_name;
        if (!get_screen_name(screen_index, screen_name))
        {
            CloseHandle(window);
            CloseHandle(process);
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Failed to find screen.");
            return 1;
        }

        DEVMODE original_screen_settings;
        if (!EnumDisplaySettings(screen_name.c_str(), ENUM_CURRENT_SETTINGS, &original_screen_settings))
        {
            CloseHandle(window);
            CloseHandle(process);
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Failed to retrieve display settings.");
            return 1;
        }

        DEVMODE dev_mode;
        if (!find_best(screen_name, aspect_ratio_x, aspect_ratio_y, dev_mode))
        {
            CloseHandle(window);
            CloseHandle(process);
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Failed to find a fitting resolutio for that aspect ratio.");
            return 1;
        }

        DEVMODE fixed_dev_mode;
        ZeroMemory(&fixed_dev_mode, sizeof(fixed_dev_mode));
        fixed_dev_mode.dmSize = sizeof(fixed_dev_mode);
        fixed_dev_mode.dmDisplayFrequency = dev_mode.dmDisplayFrequency;
        fixed_dev_mode.dmPelsHeight = dev_mode.dmPelsHeight;
        fixed_dev_mode.dmPelsWidth = dev_mode.dmPelsWidth;
        fixed_dev_mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

        if (ChangeDisplaySettingsExA(screen_name.c_str(), &fixed_dev_mode, nullptr, CDS_RESET, nullptr) != DISP_CHANGE_SUCCESSFUL)
        {
            CloseHandle(window);
            CloseHandle(process);
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Failed to set screen resolution.");
            return 1;
        }

        if (show_message_box_with_timeout(10000, "Please confirm your new display settings within 10 seconds, otherwise they will be reset.", "Confirmation") != IDYES)
        {
            CloseHandle(window);
            CloseHandle(process);
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            if (ChangeDisplaySettingsExA(screen_name.c_str(), &original_screen_settings, nullptr, 0, nullptr) != DISP_CHANGE_SUCCESSFUL)
            {
                error("Failed to reset display settings.");
            }
            else
            {
                error("Did not confirm - display settings reset.");
            }
            return 1;
        }

        if (!SetWindowLongPtr(window, GWL_STYLE, GetWindowLongPtr(window, GWL_STYLE) & ~(WS_DLGFRAME | WS_BORDER)))
        {
            CloseHandle(window);
            CloseHandle(process);
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Failed to set window style.");
            return 1;
        }

        if (!ShowWindow(window, SW_MAXIMIZE))
        {
            CloseHandle(window);
            CloseHandle(process);
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Failed to maximize window.");
            return 1;
        }

        CloseHandle(window);

        WaitForSingleObject(process, INFINITE);

        CloseHandle(process);

        if (ChangeDisplaySettingsExA(screen_name.c_str(), &original_screen_settings, nullptr, 0, nullptr) != DISP_CHANGE_SUCCESSFUL)
        {
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            error("Failed to reset screen resolution.");
            return 1;
        }
    }
    catch (std::exception& _)
    {
    }

    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return 0;
}