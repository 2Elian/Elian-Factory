#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <map>
#include <random>
#include <memory>
#include <ctime>
#include <regex>
#include <locale>
#include <codecvt>
#include <iomanip>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#endif
std::wstring s2ws(const std::string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}
#if defined(_WIN32) || defined(__linux__)
#define TRY_USE_CUDA
#endif

#ifdef TRY_USE_CUDA
struct NvmlFunctions {
    void* handle;
    int (*nvmlInit)();
    int (*nvmlShutdown)();
    int (*nvmlDeviceGetCount)(unsigned int*);
    int (*nvmlDeviceGetHandleByIndex)(unsigned int, void*);
    int (*nvmlDeviceGetName)(void*, char*, unsigned int);
    int (*nvmlDeviceGetMemoryInfo)(void*, void*);
    int (*nvmlDeviceGetUtilizationRates)(void*, void*);
    bool initialized;

    NvmlFunctions() : handle(nullptr), nvmlInit(nullptr), nvmlShutdown(nullptr),
                     nvmlDeviceGetCount(nullptr), nvmlDeviceGetHandleByIndex(nullptr),
                     nvmlDeviceGetName(nullptr), nvmlDeviceGetMemoryInfo(nullptr),
                     nvmlDeviceGetUtilizationRates(nullptr), initialized(false) {}

    ~NvmlFunctions() {
        if (initialized && nvmlShutdown) {
            nvmlShutdown();
        }
#ifdef _WIN32
        if (handle) FreeLibrary((HMODULE)handle);
#else
        // Linux处理
#endif
    }
};

std::unique_ptr<NvmlFunctions> loadNvml() {
    std::unique_ptr<NvmlFunctions> nvml(new NvmlFunctions());
#ifdef _WIN32
    nvml->handle = LoadLibraryA("nvml.dll");
    if (!nvml->handle) {
        const char* cudaPaths[] = {
            "C:\\Program Files\\NVIDIA Corporation\\NVSMI\\nvml.dll",
            "C:\\Windows\\System32\\nvml.dll",
        };
        for (const char* path : cudaPaths) {
            nvml->handle = LoadLibraryA(path);
            if (nvml->handle) break;
        }
    }

    if (nvml->handle) {
        nvml->nvmlInit = (int(*)())GetProcAddress((HMODULE)nvml->handle, "nvmlInit_v2");
        nvml->nvmlShutdown = (int(*)())GetProcAddress((HMODULE)nvml->handle, "nvmlShutdown");
        nvml->nvmlDeviceGetCount = (int(*)(unsigned int*))GetProcAddress((HMODULE)nvml->handle, "nvmlDeviceGetCount_v2");
        nvml->nvmlDeviceGetHandleByIndex = (int(*)(unsigned int, void*))GetProcAddress((HMODULE)nvml->handle, "nvmlDeviceGetHandleByIndex_v2");
        nvml->nvmlDeviceGetName = (int(*)(void*, char*, unsigned int))GetProcAddress((HMODULE)nvml->handle, "nvmlDeviceGetName");
        nvml->nvmlDeviceGetMemoryInfo = (int(*)(void*, void*))GetProcAddress((HMODULE)nvml->handle, "nvmlDeviceGetMemoryInfo");
        nvml->nvmlDeviceGetUtilizationRates = (int(*)(void*, void*))GetProcAddress((HMODULE)nvml->handle, "nvmlDeviceGetUtilizationRates");
        
        if (nvml->nvmlInit && nvml->nvmlInit() == 0) {
            nvml->initialized = true;
            return nvml;
        }
    }
#else
    // Linux加载实现
    // dlopen, dlsym等
#endif
    return nullptr;
}
#endif

// 服务器配置
const int PORT = 10171;
const std::string WEB_DIR = "./web"; // 前端服务地址 npm run build会自动构建到web目录下
const std::string API_PREFIX = "/api"; // 训练请求api

// GPU信息结构体
struct GPUInfo {
    std::string name;
    int memory_total; // MB
    int memory_free;  // MB
    int utilization;  // %
    std::string status;
};

// 执行命令并返回输出
std::string exec_command(const std::string& cmd) {
    std::string result;
    char buffer[128];
    
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    
    if (!pipe) {
        return "执行命令失败";
    }
    
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    }
    
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    
    return result;
}

// 通过nvidia-smi命令获取GPU信息
std::vector<GPUInfo> detect_gpus() {
    std::vector<GPUInfo> gpus;
    
    // 尝试执行nvidia-smi命令
    std::string cmd;
#ifdef _WIN32
    cmd = "where nvidia-smi >nul 2>&1 && nvidia-smi --query-gpu=name,memory.total,memory.free,utilization.gpu --format=csv,noheader,nounits";
#else
    cmd = "command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi --query-gpu=name,memory.total,memory.free,utilization.gpu --format=csv,noheader,nounits";
#endif

    std::string output = exec_command(cmd);
    
    // 如果nvidia-smi命令执行成功并有输出
    if (!output.empty() && output != "执行命令失败") {
        std::istringstream stream(output);
        std::string line;
        
        // 处理每一行（每个GPU）
        while (std::getline(stream, line)) {
            GPUInfo gpu;
            std::istringstream line_stream(line);
            std::string token;
            
            // 格式: name, memory.total [MiB], memory.free [MiB], utilization.gpu [%]
            if (std::getline(line_stream, token, ',')) {
                gpu.name = token;
                gpu.name.erase(0, gpu.name.find_first_not_of(" \t\r\n"));
                gpu.name.erase(gpu.name.find_last_not_of(" \t\r\n") + 1);
            }
            
            if (std::getline(line_stream, token, ',')) {
                try {
                    gpu.memory_total = std::stoi(token);
                } catch (...) {
                    gpu.memory_total = 0;
                }
            }
            
            if (std::getline(line_stream, token, ',')) {
                try {
                    gpu.memory_free = std::stoi(token);
                } catch (...) {
                    gpu.memory_free = 0;
                }
            }
            
            if (std::getline(line_stream, token, ',')) {
                try {
                    gpu.utilization = std::stoi(token);
                } catch (...) {
                    gpu.utilization = 0;
                }
            }
            
            gpu.status = "ready";
            gpus.push_back(gpu);
        }
    }
    
    // 如果没有检测到GPU，尝试更简单的检测方法
    if (gpus.empty()) {
#ifdef _WIN32
        // 检查NVIDIA驱动是否存在
        HMODULE nvcuda = LoadLibraryA("nvcuda.dll");
        if (nvcuda != NULL) {
            FreeLibrary(nvcuda);
            
            // 尝试使用更简单的nvidia-smi命令输出
            std::string simple_output = exec_command("nvidia-smi -L");
            std::regex gpu_pattern("GPU (\\d+): (.+?)(?:\\(UUID:.+?\\))?$");
            std::smatch matches;
            std::string::const_iterator start = simple_output.cbegin();
            std::string::const_iterator end = simple_output.cend();
            
            while (std::regex_search(start, end, matches, gpu_pattern)) {
                GPUInfo gpu;
                gpu.name = matches[2].str();
                gpu.memory_total = 0; // 未知
                gpu.memory_free = 0;  // 未知
                gpu.utilization = 0;  // 未知
                gpu.status = "ready";
                gpus.push_back(gpu);
                
                start = matches.suffix().first;
            }
        }
#else
        // Linux检测代码 - 简单模拟
        if (system("command -v nvidia-smi >/dev/null 2>&1") == 0) {
            std::string simple_output = exec_command("nvidia-smi -L");
            std::regex gpu_pattern("GPU (\\d+): (.+?)(?:\\(UUID:.+?\\))?$");
            std::smatch matches;
            std::string::const_iterator start = simple_output.cbegin();
            std::string::const_iterator end = simple_output.cend();
            
            while (std::regex_search(start, end, matches, gpu_pattern)) {
                GPUInfo gpu;
                gpu.name = matches[2].str();
                gpu.memory_total = 0; // 未知
                gpu.memory_free = 0;  // 未知
                gpu.utilization = 0;  // 未知
                gpu.status = "ready";
                gpus.push_back(gpu);
                
                start = matches.suffix().first;
            }
        }
#endif
    }
    
    // 如果以上所有方法都没有检测到GPU
    if (gpus.empty()) {
        // 执行WMI查询以获取显卡信息（Windows系统）
#ifdef _WIN32
        std::string wmi_output = exec_command("wmic path win32_VideoController get Name, AdapterRAM /format:csv");
        if (!wmi_output.empty() && wmi_output != "执行命令失败") {
            std::istringstream stream(wmi_output);
            std::string line;
            bool header = true;
            
            while (std::getline(stream, line)) {
                // 跳过头行
                if (header) {
                    header = false;
                    continue;
                }
                
                if (!line.empty()) {
                    GPUInfo gpu;
                    std::size_t comma_pos = line.find(",", 0);
                    if (comma_pos != std::string::npos) {
                        comma_pos = line.find(",", comma_pos + 1);
                        if (comma_pos != std::string::npos) {
                            gpu.name = line.substr(comma_pos + 1);
                            
                            // 尝试提取内存信息（如果有）
                            std::string ram_part = line.substr(0, comma_pos);
                            std::size_t last_comma = ram_part.find_last_of(",");
                            if (last_comma != std::string::npos) {
                                std::string ram_str = ram_part.substr(last_comma + 1);
                                try {
                                    // AdapterRAM单位是字节，转换为MB
                                    unsigned long long ram = std::stoull(ram_str);
                                    gpu.memory_total = static_cast<int>(ram / (1024 * 1024));
                                } catch (...) {
                                    gpu.memory_total = 0;
                                }
                            }
                            
                            gpu.memory_free = 0;  // 未知
                            gpu.utilization = 0;  // 未知
                            gpu.status = "unknown";
                            gpus.push_back(gpu);
                        }
                    }
                }
            }
        }
#endif
    }
    
    return gpus;
}

// 获取系统信息
std::string get_python_version() {
    std::string version = "未知";
    
#ifdef _WIN32
    std::string cmd = "python --version 2>&1";
#else
    std::string cmd = "python --version 2>&1";
#endif

    std::string output = exec_command(cmd);
    if (!output.empty() && output.find("Python") != std::string::npos) {
        // 提取版本号
        std::regex version_pattern("Python\\s+(\\d+\\.\\d+\\.\\d+)");
        std::smatch matches;
        if (std::regex_search(output, matches, version_pattern)) {
            version = matches[1].str();
        }
    }
    
    return version;
}

// 下面三段代码是获取python环境的配置信息 暂时V1.0版本中没在前端展示
std::string get_pytorch_version() {
    std::string cmd = "python -c \"try: import torch; print(torch.__version__); except: print('未安装')\" 2>&1";
    return exec_command(cmd);
}

std::string get_transformers_version() {
    std::string cmd = "python -c \"try: import transformers; print(transformers.__version__); except: print('未安装')\" 2>&1";
    return exec_command(cmd);
}

std::string get_cuda_version() {
    std::string cmd = "python -c \"try: import torch; print(torch.version.cuda); except: print('未检测到')\" 2>&1";
    return exec_command(cmd);
}

// 获取系统内存信息 (以MB为单位返回)
std::pair<int, int> get_system_memory() {
    int total_memory = 0;
    int available_memory = 0;
    
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        total_memory = static_cast<int>(memInfo.ullTotalPhys / (1024 * 1024));
        available_memory = static_cast<int>(memInfo.ullAvailPhys / (1024 * 1024));
    }
#else
    // Linux实现 - 通过/proc/meminfo读取
    std::string output = exec_command("cat /proc/meminfo");
    std::regex mem_total_pattern("MemTotal:\\s+(\\d+) kB");
    std::regex mem_avail_pattern("MemAvailable:\\s+(\\d+) kB");
    std::smatch matches;
    
    if (std::regex_search(output, matches, mem_total_pattern)) {
        try {
            total_memory = std::stoi(matches[1].str()) / 1024;
        } catch (...) {}
    }
    
    if (std::regex_search(output, matches, mem_avail_pattern)) {
        try {
            available_memory = std::stoi(matches[1].str()) / 1024;
        } catch (...) {}
    }
#endif
    
    return {total_memory, available_memory};
}

// 获取磁盘空间 (以MB为单位返回)
int get_disk_space() {
    int disk_space = 0;
    
#ifdef _WIN32
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceExA("C:\\", &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        disk_space = static_cast<int>(totalNumberOfBytes.QuadPart / (1024 * 1024));
    }
#else
    std::string output = exec_command("df -k / | tail -1 | awk '{print $2}'");
    try {
        disk_space = std::stoi(output) / 1024;
    } catch (...) {}
#endif
    
    return disk_space;
}

// 生成JSON响应
std::string json_response(const std::string& json_content) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: application/json; charset=utf-8\r\n";
    response << "Content-Length: " << json_content.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";  // 允许跨域
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";  // 允许的请求方法
    response << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";  // 允许的请求头
    response << "Cache-Control: no-cache, no-store, must-revalidate\r\n";  // 禁止缓存
    response << "Pragma: no-cache\r\n";  // 兼容HTTP/1.0
    response << "Expires: 0\r\n";  // 过期时间
    response << "\r\n";
    response << json_content;
    return response.str();
}

bool ends_with(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return (str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0);
}

bool starts_with(const std::string& str, const std::string& prefix) {
    if (str.length() < prefix.length()) {
        return false;
    }
    return (str.compare(0, prefix.length(), prefix) == 0);
}

std::vector<std::string> list_files_in_directory(const std::string& directory, const std::string& extension) {
    std::vector<std::string> files;
    
    try {
#ifdef _WIN32
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, directory.c_str(), -1, NULL, 0);
        std::vector<wchar_t> wdirectory(size_needed);
        MultiByteToWideChar(CP_UTF8, 0, directory.c_str(), -1, wdirectory.data(), size_needed);
        
        std::wstring wsearch_path = std::wstring(wdirectory.data()) + L"\\*.*";
        
        WIN32_FIND_DATAW find_data;
        HANDLE find_handle = FindFirstFileW(wsearch_path.c_str(), &find_data);
        
        if (find_handle != INVALID_HANDLE_VALUE) {
            do {
                // 跳过"."和".."目录
                if (wcscmp(find_data.cFileName, L".") != 0 && wcscmp(find_data.cFileName, L"..") != 0) {
                    // 只添加文件，不添加目录
                    if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        // 将宽字符文件名转换回UTF-8
                        int name_size = WideCharToMultiByte(CP_UTF8, 0, find_data.cFileName, -1, NULL, 0, NULL, NULL);
                        std::vector<char> utf8_filename(name_size);
                        WideCharToMultiByte(CP_UTF8, 0, find_data.cFileName, -1, utf8_filename.data(), name_size, NULL, NULL);
                        
                        std::string file_name = utf8_filename.data();
                        // 使用我们自己的ends_with函数检查文件后缀
                        if (extension.empty() || ends_with(file_name, extension)) {
                            files.push_back(file_name);
                        }
                    }
                }
            } while (FindNextFileW(find_handle, &find_data));
            
            FindClose(find_handle);
        }
#else
        // Linux实现
        DIR* dir = opendir(directory.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    std::string file_path = directory + "/" + entry->d_name;
                    struct stat statbuf;
                    if (stat(file_path.c_str(), &statbuf) != -1) {
                        if (S_ISREG(statbuf.st_mode)) {
                            std::string file_name = entry->d_name;
                            if (extension.empty() || ends_with(file_name, extension)) {
                                files.push_back(file_name);
                            }
                        }
                    }
                }
            }
            closedir(dir);
        }
#endif
    } catch (const std::exception& e) {
        std::cerr << "Error listing files: " << e.what() << std::endl;
    }
    
    return files;
}

// 检查文件或目录是否存在
bool file_exists(const std::string& path) {
#ifdef _WIN32
    // 将UTF-8路径转换为宽字符
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
    std::vector<wchar_t> wpath(size_needed);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath.data(), size_needed);
    
    // 使用宽字符API检查文件是否存在
    DWORD dwAttrib = GetFileAttributesW(wpath.data());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES);
#else
    struct stat buffer;   
    return (stat(path.c_str(), &buffer) == 0);
#endif
}

// 读取文件前N行作为预览 N=10
std::string read_file_preview(const std::string& file_path, int max_lines = 10) {
#ifdef _WIN32
    // 将UTF-8路径转换为宽字符
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, file_path.c_str(), -1, NULL, 0);
    std::vector<wchar_t> wpath(size_needed);
    MultiByteToWideChar(CP_UTF8, 0, file_path.c_str(), -1, wpath.data(), size_needed);
    
    // 使用C文件API打开文件
    FILE* file = _wfopen(wpath.data(), L"r");
    if (!file) {
        return "";
    }
    
    std::string preview;
    char buffer[4096];
    int line_count = 0;
    
    while (line_count < max_lines && fgets(buffer, sizeof(buffer), file)) {
        preview += buffer;
        line_count++;
    }
    
    fclose(file);
    return preview;
#else
    std::ifstream file(file_path);
    std::string line;
    std::vector<std::string> lines;
    int count = 0;
    
    if (!file.is_open()) {
        return "";
    }
    
    while (std::getline(file, line) && count < max_lines) {
        lines.push_back(line);
        count++;
    }
    
    file.close();
    
    std::string preview;
    for (const auto& l : lines) {
        preview += l + "\n";
    }
    
    return preview;
#endif
}

// 从HTTP请求中提取POST数据体
std::string extract_post_data(const std::string& request) {
    // 查找请求体开始的位置（在空行之后）
    std::string delimiter = "\r\n\r\n";
    size_t pos = request.find(delimiter);
    
    if (pos != std::string::npos) {
        // 返回请求体部分
        return request.substr(pos + delimiter.length());
    }
    
    return "";
}

std::string extract_post_data_config(const std::string& request) {
    // 查找正文开始的位置
    size_t body_start = request.find("\r\n\r\n");
    if (body_start == std::string::npos) {
        return ""; // 没有找到正文
    }

    // 跳过分隔符
    body_start += 4;

    // 提取正文内容
    return request.substr(body_start);
}

std::map<std::string, std::string> parse_config_json(const std::string& json_text) {
    std::map<std::string, std::string> result;
    
    // 提取config_name
    std::regex config_name_regex("\"config_name\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch config_name_match;
    if (std::regex_search(json_text, config_name_match, config_name_regex)) {
        result["config_name"] = config_name_match[1].str();
    }
    
    // 查找config_data的起始位置
    std::string start_marker = "\"config_data\":";
    size_t start_pos = json_text.find(start_marker);
    if (start_pos != std::string::npos) {
        start_pos += start_marker.length();
        
        // 跳过空白字符
        while (start_pos < json_text.length() && std::isspace(json_text[start_pos])) {
            start_pos++;
        }
        
        // 确保找到的是一个JSON对象
        if (start_pos < json_text.length() && json_text[start_pos] == '{') {
            // 查找匹配的结束括号
            int brace_count = 1;
            size_t end_pos = start_pos + 1;
            
            while (end_pos < json_text.length() && brace_count > 0) {
                if (json_text[end_pos] == '{') brace_count++;
                else if (json_text[end_pos] == '}') brace_count--;
                end_pos++;
            }
            
            if (brace_count == 0) {
                // 提取完整的config_data对象
                std::string config_data = json_text.substr(start_pos, end_pos - start_pos);
                result["config_data"] = config_data;
            }
        }
    }
    
    return result;
}

// 简单的JSON解析函数，将JSON文本转换为键值对
std::map<std::string, std::string> parse_json(const std::string& json_text) {
    std::map<std::string, std::string> result;
    
    // 非常简单的解析，仅用于演示
    // 实际应用中应使用成熟的JSON解析库
    std::regex key_value_regex("\"([^\"]+)\"\\s*:\\s*(\"[^\"]*\"|\\d+|true|false|null)");
    std::smatch matches;
    std::string::const_iterator search_start(json_text.cbegin());
    
    while (std::regex_search(search_start, json_text.cend(), matches, key_value_regex)) {
        std::string key = matches[1].str();
        std::string value = matches[2].str();
        
        // 如果值是字符串，去掉引号
        if (value[0] == '"' && value[value.length() - 1] == '"') {
            value = value.substr(1, value.length() - 2);
        }
        // 处理布尔值，统一转为小写形式，便于后续判断
        else if (value == "true" || value == "false") {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c) { return std::tolower(c); });
        }
        
        result[key] = value;
        search_start = matches.suffix().first;
    }
    
    return result;
}

// JSON字符串转义函数，确保动态内容安全地嵌入到JSON中
std::string escape_json(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u"
                  << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(*c);
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}

// 处理API请求
std::string handle_api_request(const std::string& url, const std::string& request, const std::string& method) {
    // 处理OPTIONS请求（CORS预检请求）
    if (method == "OPTIONS") {
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        response << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
        response << "Access-Control-Max-Age: 86400\r\n";  // 预检请求结果缓存24小时
        response << "Content-Length: 0\r\n";
        response << "\r\n";
        return response.str();
    }

    if (url == "/api/gpu/status" || url == "/api/gpus") {
        std::vector<GPUInfo> gpus = detect_gpus();
        
        std::ostringstream json;
        json << "{";
        
        if (gpus.empty()) {
            json << "\"success\": false, \"message\": \"没有检测到GPU\", \"data\": []";
        } else {
            json << "\"success\": true, \"data\": [";
            
            for (size_t i = 0; i < gpus.size(); ++i) {
                const auto& gpu = gpus[i];
                json << "{";
                json << "\"name\": \"" << gpu.name << "\", ";
                json << "\"memory_total\": " << gpu.memory_total << ", ";
                json << "\"memory_free\": " << gpu.memory_free << ", ";
                json << "\"utilization\": " << gpu.utilization << ", ";
                json << "\"status\": \"" << gpu.status << "\"";
                json << "}";
                
                if (i < gpus.size() - 1) {
                    json << ", ";
                }
            }
            
            json << "]";
        }
        
        json << "}";
        return json_response(json.str());
    } 
    else if (url == "/api/system/info") {
        // 获取实际系统信息
        std::string python_version = get_python_version();
        std::string pytorch_version = get_pytorch_version();
        std::string transformers_version = get_transformers_version();
        std::string cuda_version = get_cuda_version();
        
        std::pair<int, int> memory_info = get_system_memory();
        int total_memory = memory_info.first;
        int available_memory = memory_info.second;
        
        int disk_space = get_disk_space();
        
        // 清理版本字符串中的换行符
        pytorch_version.erase(std::remove(pytorch_version.begin(), pytorch_version.end(), '\n'), pytorch_version.end());
        pytorch_version.erase(std::remove(pytorch_version.begin(), pytorch_version.end(), '\r'), pytorch_version.end());
        transformers_version.erase(std::remove(transformers_version.begin(), transformers_version.end(), '\n'), transformers_version.end());
        transformers_version.erase(std::remove(transformers_version.begin(), transformers_version.end(), '\r'), transformers_version.end());
        cuda_version.erase(std::remove(cuda_version.begin(), cuda_version.end(), '\n'), cuda_version.end());
        cuda_version.erase(std::remove(cuda_version.begin(), cuda_version.end(), '\r'), cuda_version.end());
        
        std::ostringstream json;
        json << "{";
        json << "\"success\": true, ";
        json << "\"data\": {";
        
        // 操作系统信息
        #ifdef _WIN32
        json << "\"os\": \"Windows\", ";
        #else
        json << "\"os\": \"Linux/Unix\", ";
        #endif
        
        // 实际检测到的信息
        json << "\"python_version\": \"" << python_version << "\", ";
        json << "\"pytorch_version\": \"" << pytorch_version << "\", ";
        json << "\"transformers_version\": \"" << transformers_version << "\", ";
        json << "\"cuda_version\": \"" << cuda_version << "\", ";
        json << "\"cudnn_version\": \"自动随PyTorch安装\", ";
        
        // 系统资源信息
        json << "\"memory_total\": " << total_memory << ", ";
        json << "\"memory_available\": " << available_memory << ", ";
        json << "\"disk_space\": " << disk_space;
        
        json << "}";
        json << "}";
        return json_response(json.str());
    }
    else if (url == "/api/default-config") {
        // 返回默认配置
        std::ostringstream json;
        json << "{";
        json << "\"model_name_or_path\":\"/ckpt/ds\",";
        json << "\"output_dir\":\"/output/your_task/lora\",";
        json << "\"train_file\":\"/data/sample.jsonl\",";
        json << "\"num_train_epochs\":3,";
        json << "\"per_device_train_batch_size\":1,";
        json << "\"gradient_accumulation_steps\":4,";
        json << "\"learning_rate\":0.0002,";
        json << "\"max_seq_length\":1024,";
        json << "\"logging_steps\":1,";
        json << "\"save_steps\":200,";
        json << "\"save_total_limit\":3,";
        json << "\"lr_scheduler_type\":\"constant_with_warmup\",";
        json << "\"warmup_steps\":30,";
        json << "\"lora_rank\":8,";
        json << "\"lora_alpha\":16,";
        json << "\"lora_dropout\":0.05,";
        json << "\"gradient_checkpointing\":true,";
        json << "\"optim\":\"adamw_torch\",";
        json << "\"train_mode\":\"lora\",";
        json << "\"seed\":42,";
        json << "\"fp16\":false,";
        json << "\"distributed\":false";
        json << "}";
        return json_response(json.str());
    }
    else if (url == "/api/data/files") {
        // 获取当前工作目录
        std::string current_dir;
#ifdef _WIN32
        // 使用宽字符函数处理可能包含中文的路径
        wchar_t wbuffer[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, wbuffer);
        
        // 将宽字符转换为UTF-8编码的字符串
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, NULL, 0, NULL, NULL);
        std::vector<char> utf8_buffer(size_needed);
        WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, utf8_buffer.data(), size_needed, NULL, NULL);
        
        current_dir = std::string(utf8_buffer.data());
        
        // 替换Windows路径中的反斜杠为正斜杠（统一处理）
        for (char &c : current_dir) {
            if (c == '\\') c = '/';
        }
#else
        char buffer[PATH_MAX];
        if (getcwd(buffer, PATH_MAX) != NULL) {
            current_dir = buffer;
        } else {
            current_dir = ".";
        }
#endif
        // 列出data目录下的所有JSONL文件
        std::string data_dir = current_dir + "/llm/data";
        std::vector<std::string> files = list_files_in_directory(data_dir, ".jsonl");
        
        std::ostringstream json;
        json << "{";
        json << "\"success\":true,";
        json << "\"files\":[";
        
        for (size_t i = 0; i < files.size(); ++i) {
            json << "\"" << files[i] << "\"";
            if (i < files.size() - 1) {
                json << ",";
            }
        }
        
        json << "]}";
        return json_response(json.str());
    }
    else if (url.find("/api/data/preview?file=") == 0) {
        // 获取当前工作目录
        std::string current_dir;
#ifdef _WIN32
        // 使用宽字符函数处理可能包含中文的路径
        wchar_t wbuffer[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, wbuffer);
        
        // 将宽字符转换为UTF-8编码的字符串
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, NULL, 0, NULL, NULL);
        std::vector<char> utf8_buffer(size_needed);
        WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, utf8_buffer.data(), size_needed, NULL, NULL);
        
        current_dir = std::string(utf8_buffer.data());
        
        // 替换Windows路径中的反斜杠为正斜杠（统一处理）
        for (char &c : current_dir) {
            if (c == '\\') c = '/';
        }
#else
        char buffer[PATH_MAX];
        if (getcwd(buffer, PATH_MAX) != NULL) {
            current_dir = buffer;
        } else {
            current_dir = ".";
        }
#endif

        // 获取文件内容预览
        std::string file_param = url.substr(url.find("=") + 1);
        // 进行URL解码
        std::string filename;
        for (size_t i = 0; i < file_param.length(); ++i) {
            if (file_param[i] == '%' && i + 2 < file_param.length()) {
                int value = 0;
                if (sscanf(file_param.substr(i + 1, 2).c_str(), "%x", &value) == 1) {
                    filename += static_cast<char>(value);
                    i += 2;
                }
                else {
                    filename += file_param[i];
                }
            }
            else if (file_param[i] == '+') {
                filename += ' ';
            }
            else {
                filename += file_param[i];
            }
        }
        
        // 安全检查，防止路径遍历攻击
        if (filename.find("..") != std::string::npos) {
            std::ostringstream json;
            json << "{\"success\":false,\"message\":\"Invalid filename\"}";
            return json_response(json.str());
        }
        // 预览数据逻辑
        std::string file_path = current_dir + "/llm/data/" + filename;
        std::string preview = read_file_preview(file_path, 20); // 预览前20行
        
        // 将换行符转换为\n转义序列
        std::string escaped_preview;
        for (char c : preview) {
            if (c == '\n') {
                escaped_preview += "\\n";
            } 
            else if (c == '\r') {
                // 忽略回车符
            } 
            else if (c == '"') {
                escaped_preview += "\\\"";
            } 
            else if (c == '\\') {
                escaped_preview += "\\\\";
            } 
            else {
                escaped_preview += c;
            }
        }
        
        std::ostringstream json;
        json << "{";
        json << "\"success\":true,";
        json << "\"content\":\"" << escaped_preview << "\"";
        json << "}";
        return json_response(json.str());
    }
    else if (url.find("/api/training/") == 0) {
        // 训练API
        std::ostringstream json;
        json << "{";
        json << "\"success\":true,";
        json << "\"message\":\"训练请求已接收\",";
        json << "\"data\":{";
        json << "\"task_id\":\"task_" << std::rand() % 1000 << "\",";
        json << "\"status\":\"pending\"";
        json << "}";
        json << "}";
        return json_response(json.str());
    }
    else if (url == "/api/train") {
        // 读取POST数据体
        std::string request_body = extract_post_data(request);
        std::cout << "收到训练请求：" << request_body << std::endl;
        
        // 解析JSON配置
        std::map<std::string, std::string> formData = parse_json(request_body);
        
        // 获取当前工作目录（程序运行的目录）
        std::string current_dir;
#ifdef _WIN32
        // 使用宽字符函数处理可能包含中文的路径
        wchar_t wbuffer[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, wbuffer);
        
        // 将宽字符转换为UTF-8编码的字符串
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, NULL, 0, NULL, NULL);
        std::vector<char> utf8_buffer(size_needed);
        WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, utf8_buffer.data(), size_needed, NULL, NULL);
        
        current_dir = std::string(utf8_buffer.data());
#else
        char buffer[PATH_MAX];
        if (getcwd(buffer, PATH_MAX) != NULL) {
            current_dir = buffer;
        } else {
            current_dir = ".";
        }
#endif

        // 构建Python命令 开启训练脚本
        std::wstring cmdCommand;
        std::string cmd = "";
        std::string error_message = "";
        std::wstring main_py_path = L"./llm/main.py";
        // 检查分布式设置
        bool use_distributed = false;
        if (formData.find("distributed") != formData.end()) {
            use_distributed = (formData["distributed"] == "true");
        }

        // 根据distributed 构建不同的启动main.py的命令
        if (use_distributed) {
            // 获取GPU数量
            std::vector<GPUInfo> gpus = detect_gpus();
            int gpu_count = static_cast<int>(gpus.size());
            if (gpu_count == 0) gpu_count = 1;
            
            // 使用torchrun启动分布式训练
            #ifdef _WIN32
            std::replace(current_dir.begin(), current_dir.end(), '/', '\\'); // 确保Windows下使用反斜杠
            std::string main_path = current_dir + "\\llm\\main.py";
            std::replace(main_path.begin(), main_path.end(), '/', '\\'); // 确保Windows下使用反斜杠
            cmdCommand = L"start /B cmd.exe /C \"conda activate elianfactory && set TORCHRUN_USE_LIBUV=0 && set PYTHONIOENCODING=utf-8 && chcp 65001 && torchrun --nproc_per_node="
             + std::to_wstring(gpu_count) + L" " +  main_py_path + L" " + L" > ./train_log.txt 2>&1\"";
            #else
            cmdCommand = L"start /B cmd.exe /C \"conda activate elianfactory && set TORCHRUN_USE_LIBUV=0 && set PYTHONIOENCODING=utf-8 && chcp 65001 && torchrun --nproc_per_node="
             + std::to_wstring(gpu_count) + L" " +  main_py_path + L"\"";
            #endif
        } else {
            // 使用普通python命令
            #ifdef _WIN32
            std::replace(current_dir.begin(), current_dir.end(), '/', '\\'); // 确保Windows下使用反斜杠
            std::string main_path = current_dir + "\\llm\\main.py";
            std::replace(main_path.begin(), main_path.end(), '/', '\\'); // 确保Windows下使用反斜杠
            cmdCommand = L"start /B cmd.exe /C \""
             L"conda activate elianfactory && "
             L"set PYTHONIOENCODING=utf-8 && "
             L"chcp 65001 > nul && "
             L"python " + main_py_path + L" > \"./train_log.txt\" 2>&1\"";
            #else
            cmdCommand = L"start /B cmd.exe /C \"conda activate elianfactory && set PYTHONIOENCODING=utf-8 && chcp 65001 && python ./llm/main.py \"";
            #endif
        }
        
        // 添加训练参数
        // 基础模型路径
        if (formData.find("model_name_or_path") != formData.end()) {
            std::string model_path = formData["model_name_or_path"];
            if (model_path.size() > 2 && model_path[0] == '"' && model_path[model_path.size() - 1] == '"') {
                model_path = model_path.substr(1, model_path.size() - 2);
            }
            
            // 处理路径：添加基础目录
            if (model_path.size() <= 1 || model_path[1] != ':') { // 非绝对路径
                if (model_path[0] == '/' || model_path[0] == '\\') {
                    model_path = model_path.substr(1);
                }
                #ifdef _WIN32
                std::replace(current_dir.begin(), current_dir.end(), '/', '\\'); // 确保Windows下使用反斜杠
                model_path = current_dir + "\\llm\\" + model_path;
                std::replace(model_path.begin(), model_path.end(), '/', '\\'); // 确保Windows下使用反斜杠
                
                // 创建模型路径目录
                std::string model_dir = model_path.substr(0, model_path.find_last_of('\\'));
                if (!file_exists(model_dir)) {
                    std::cout << "创建模型目录: " << model_dir << std::endl;
                    // 递归创建目录
                    std::string path_so_far;
                    std::istringstream path_stream(model_dir);
                    std::string folder;
                    while (std::getline(path_stream, folder, '\\')) {
                        if (path_so_far.empty()) {
                            path_so_far = folder;
                        } else {
                            path_so_far += "\\" + folder;
                        }
                        if (!path_so_far.empty() && !file_exists(path_so_far)) {
                            if (!CreateDirectoryA(path_so_far.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
                                error_message = "无法创建模型目录: " + path_so_far + ", 错误码: " + std::to_string(GetLastError());
                                break;
                            }
                        }
                    }
                }
                #else
                model_path = current_dir + "/llm/" + model_path;
                #endif
            }
            
            // 检查模型路径是否存在
            if (!file_exists(model_path) && error_message.empty()) {
                error_message = "模型路径不存在: " + model_path;
            }
            
            cmd += " --model_name_or_path \"" + model_path + "\"";
        } else {
            error_message = "缺少必要参数: model_name_or_path";
        }
        
        // 微调输出权重路径
        if (formData.find("output_dir") != formData.end() && error_message.empty()) {
            std::string output_path = formData["output_dir"];
            if (output_path.size() > 2 && output_path[0] == '"' && output_path[output_path.size() - 1] == '"') {
                output_path = output_path.substr(1, output_path.size() - 2);
            }
            
            // 处理路径：添加基础目录
            if (output_path.size() <= 1 || output_path[1] != ':') { // 非绝对路径
                if (output_path[0] == '/' || output_path[0] == '\\') {
                    output_path = output_path.substr(1);
                }
                #ifdef _WIN32
                std::replace(current_dir.begin(), current_dir.end(), '/', '\\'); // 确保Windows下使用反斜杠
                output_path = current_dir + "\\llm\\" + output_path;
                std::replace(output_path.begin(), output_path.end(), '/', '\\'); // 确保Windows下使用反斜杠
                
                // 创建输出目录
                std::string output_dir = output_path;
                #else
                output_path = current_dir + "/llm/" + output_path;
                #endif
            }
            
            cmd += " --output_dir \"" + output_path + "\"";
        } else if (error_message.empty()) {
            error_message = "缺少必要参数: output_dir";
        }
        
        // 训练文件路径
        if (formData.find("train_file") != formData.end() && error_message.empty()) {
            std::string train_file = formData["train_file"];
            if (train_file.size() > 2 && train_file[0] == '"' && train_file[train_file.size() - 1] == '"') {
                train_file = train_file.substr(1, train_file.size() - 2);
            }
            
            // 处理路径：添加基础目录
            if (train_file.size() <= 1 || train_file[1] != ':') { // 非绝对路径
                if (train_file[0] == '/' || train_file[0] == '\\') {
                    train_file = train_file.substr(1);
                }
                #ifdef _WIN32
                std::replace(current_dir.begin(), current_dir.end(), '/', '\\'); // 确保Windows下使用反斜杠
                train_file = current_dir + "\\llm\\" + train_file;
                std::replace(train_file.begin(), train_file.end(), '/', '\\'); // 确保Windows下使用反斜杠
                
                // 创建数据目录
                std::string data_dir = train_file.substr(0, train_file.find_last_of('\\'));
                if (!file_exists(data_dir)) {
                    std::cout << "创建数据目录: " << data_dir << std::endl;
                    // 递归创建目录
                    std::string path_so_far;
                    std::istringstream path_stream(data_dir);
                    std::string folder;
                    while (std::getline(path_stream, folder, '\\')) {
                        if (path_so_far.empty()) {
                            path_so_far = folder;
                        } else {
                            path_so_far += "\\" + folder;
                        }
                        if (!path_so_far.empty() && !file_exists(path_so_far)) {
                            if (!CreateDirectoryA(path_so_far.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
                                error_message = "无法创建数据目录: " + path_so_far + ", 错误码: " + std::to_string(GetLastError());
                                break;
                            }
                        }
                    }
                }
                #else
                train_file = current_dir + "/llm/" + train_file;
                #endif
            }
            
            // 检查文件是否存在
            if (!file_exists(train_file) && error_message.empty()) {
                error_message = "训练数据文件不存在: " + train_file;
            }
            
            cmd += " --train_file \"" + train_file + "\"";
        } else if (error_message.empty()) {
            error_message = "缺少必要参数: train_file";
        }
        
        // 添加其他参数
        if (error_message.empty()) {
            // 非布尔参数直接添加
            if (formData.find("num_train_epochs") != formData.end())
                cmd += " --num_train_epochs " + formData["num_train_epochs"];
            
            if (formData.find("per_device_train_batch_size") != formData.end())
                cmd += " --per_device_train_batch_size " + formData["per_device_train_batch_size"];
            
            if (formData.find("gradient_accumulation_steps") != formData.end())
                cmd += " --gradient_accumulation_steps " + formData["gradient_accumulation_steps"];
            
            if (formData.find("learning_rate") != formData.end())
                cmd += " --learning_rate " + formData["learning_rate"];
            
            if (formData.find("max_seq_length") != formData.end())
                cmd += " --max_seq_length " + formData["max_seq_length"];
            
            if (formData.find("logging_steps") != formData.end())
                cmd += " --logging_steps " + formData["logging_steps"];
            
            if (formData.find("save_steps") != formData.end())
                cmd += " --save_steps " + formData["save_steps"];
            
            if (formData.find("save_total_limit") != formData.end())
                cmd += " --save_total_limit " + formData["save_total_limit"];
            
            if (formData.find("lr_scheduler_type") != formData.end())
                cmd += " --lr_scheduler_type " + formData["lr_scheduler_type"];
            
            if (formData.find("warmup_steps") != formData.end())
                cmd += " --warmup_steps " + formData["warmup_steps"];
            
            if (formData.find("lora_rank") != formData.end())
                cmd += " --lora_rank " + formData["lora_rank"];
            
            if (formData.find("lora_alpha") != formData.end())
                cmd += " --lora_alpha " + formData["lora_alpha"];
            
            if (formData.find("lora_dropout") != formData.end())
                cmd += " --lora_dropout " + formData["lora_dropout"];
            
            // 布尔参数 - 只有为true时才添加参数，为false时不传递
            if (formData.find("gradient_checkpointing") != formData.end()) {
                bool is_enabled = (formData["gradient_checkpointing"] == "true");
                if (is_enabled) {
                    cmd += " --gradient_checkpointing";
                }
            }
            
            if (formData.find("fp16") != formData.end()) {
                bool is_enabled = (formData["fp16"] == "true");
                if (is_enabled) {
                    cmd += " --fp16";
                }
            }
            
            // 其他非布尔参数
            if (formData.find("optim") != formData.end())
                cmd += " --optim " + formData["optim"];
            
            if (formData.find("train_mode") != formData.end())
                cmd += " --train_mode " + formData["train_mode"];
            
            if (formData.find("seed") != formData.end())
                cmd += " --seed " + formData["seed"];
            
            // 分布式参数处理，只有为true时才添加
            if (use_distributed) {
                cmd += " --distributed"; // 分布式模式下添加标志
            }
        }

        // 如果有错误，直接返回错误信息
        if (!error_message.empty()) {
            std::ostringstream json;
            json << "{";
            json << "\"success\":false,";
            json << "\"message\":\"参数验证失败\",";
            json << "\"error\":\"" << escape_json(error_message) << "\"";
            json << "}";
            return json_response(json.str());
        }

        bool success = false;
        std::string cmd_output;
        
#ifdef _WIN32
        // 使用宽字符版本获取路径
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(NULL, buffer, MAX_PATH);
        
        // 转换为宽字符串并提取目录部分
        std::wstring fullPath(buffer);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        std::wstring currentDir = fullPath.substr(0, lastSlash);
        
        // 配置文件路径 - 使用默认配置文件
        std::wstring configFilePathW = L"C:\\Windows\\elianfactory\\default_config.json";

        // 检查配置文件是否存在
        DWORD configFileAttrs = GetFileAttributesW(configFilePathW.c_str());
        if (configFileAttrs == INVALID_FILE_ATTRIBUTES) {
            std::cout << "警告: 默认配置文件不存在，使用命令行参数" << std::endl;
        } else {
            // 输出配置文件路径
            std::wcout << L"使用配置文件: " << configFilePathW << std::endl;
            
            // 修改命令为使用配置文件
            std::wstring configFileArgW = L"--config \"" + configFilePathW + L"\"";
            system("chcp 65001 > nul");
            std::wcout << L"当前目录：" << currentDir << std::endl;
            // cmdCommand += fullPath.substr(0, lastSlash + 1); // 保留末尾分隔符
            // cmdCommand += L"llm\\main.py\"";
            // cmdCommand += L" " + configFileArgW;
            // 添加输出重定向

            
            // 执行命令
            std::wcout << L"执行命令: " << cmdCommand << std::endl;
            int result = _wsystem(cmdCommand.c_str());
            success = (result == 0);
            
            // 构建响应
            std::ostringstream json;
            json << "{";
            if (success) {
                json << "\"success\":true,";
                json << "\"message\":\"训练任务已成功启动，正在后台运行\",";
                json << "\"data\":{";
                json << "\"task_id\":\"task_" << std::rand() % 1000 << "\",";
                json << "\"status\":\"running\"";
                json << "}";
            } else {
                json << "\"success\":false,";
                json << "\"message\":\"启动训练任务失败\",";
                json << "\"error\":\"" << escape_json(cmd_output) << "\"";
            }
            json << "}";
            return json_response(json.str());
        }
#else
        // Linux下使用后台运行
        std::string full_cmd = "conda activate elianfactory && " + cmd + " > \"" + current_dir + "/llm/train_log.txt\" 2>&1 &";
        int result = system(full_cmd.c_str());
        success = (result == 0);
        cmd_output = "命令执行结果: " + std::to_string(result);
#endif
        
        // 构建响应
        std::ostringstream json;
        json << "{";
        if (success) {
            json << "\"success\":true,";
            json << "\"message\":\"训练任务已成功启动，正在后台运行。\",";
            json << "\"data\":{";
            json << "\"task_id\":\"task_" << std::rand() % 1000 << "\",";
            json << "\"status\":\"running\"";
            json << "}";
        } else {
            json << "\"success\":false,";
            json << "\"message\":\"启动训练任务失败，请您重新尝试。\",";
            json << "\"error\":\"" << escape_json(cmd_output) << "\"";
        }
        json << "}";
        return json_response(json.str());
    }
    else if (url == "/api/train/logs") {
    std::string log_content;
    std::string debug_info;  // 用于收集调试信息
    
    try {
        // 获取当前工作目录（改进版本）
        std::string current_dir;
#ifdef _WIN32
        wchar_t wbuffer[MAX_PATH];
        if (GetCurrentDirectoryW(MAX_PATH, wbuffer)) {
            char utf8_buffer[MAX_PATH * 4];
            WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, utf8_buffer, sizeof(utf8_buffer), NULL, NULL);
            current_dir = utf8_buffer;
            std::replace(current_dir.begin(), current_dir.end(), '\\', '/');
            debug_info += "Current dir: " + current_dir + "\n";
        }
#else
        char buffer[PATH_MAX];
        if (getcwd(buffer, sizeof(buffer))) {
            current_dir = buffer;
            debug_info += "Current dir: " + current_dir + "\n";
        }
#endif

        // 构建日志文件路径（统一使用正斜杠）
        std::string log_path = current_dir + "/train_log.txt";
        debug_info += "Log path: " + log_path + "\n";

        // 尝试打开文件（改进的文件打开方式）
#ifdef _WIN32
        // Windows下更可靠的文件打开方式
        std::wstring wlog_path;
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, log_path.c_str(), -1, NULL, 0);
        if (size_needed > 0) {
            wlog_path.resize(size_needed);
            MultiByteToWideChar(CP_UTF8, 0, log_path.c_str(), -1, &wlog_path[0], size_needed);
        }
        
        FILE* file = _wfopen(wlog_path.c_str(), L"rb");
#else
        FILE* file = fopen(log_path.c_str(), "rb");
#endif

        if (file) {
            debug_info += "File opened successfully\n";
            
            // 获取文件大小
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            debug_info += "File size: " + std::to_string(file_size) + " bytes\n";
            
            if (file_size > 0) {
                // 读取整个文件内容
                std::vector<char> buffer(file_size + 1);
                size_t bytes_read = fread(buffer.data(), 1, file_size, file);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    log_content.assign(buffer.data(), bytes_read);
                    debug_info += "Read " + std::to_string(bytes_read) + " bytes\n";
                } else {
                    debug_info += "Failed to read file content\n";
                }
            } else {
                debug_info += "File is empty\n";
            }
            
            fclose(file);
        } else {
            debug_info += "Failed to open file\n";
#ifdef _WIN32
            debug_info += "Error code: " + std::to_string(GetLastError()) + "\n";
#else
            debug_info += "Error: " + std::string(strerror(errno)) + "\n";
#endif
        }
    } catch (const std::exception& e) {
        debug_info += "Exception: " + std::string(e.what()) + "\n";
    }

    // 如果没有读取到内容，返回调试信息（开发阶段）
    if (log_content.empty()) {
        log_content = "暂无训练日志或训练尚未开始...\n\n调试信息:\n" + debug_info;
    }

    // 构建JSON响应
    std::ostringstream json;
    json << "{";
    json << "\"success\":" << (log_content.find("暂无训练日志") == std::string::npos ? "true" : "false") << ",";
    json << "\"timestamp\":" << std::time(nullptr) << ",";
    json << "\"logs\":\"" << escape_json(log_content) << "\"";
    json << "}";
    
    return json_response(json.str());
}
    
    // 配置相关API
    else if (url == "/api/config/save" && method == "POST") {
        // 解析POST数据
        std::string post_data = extract_post_data_config(request);
        std::cout << "收到配置保存请求: " << post_data << std::endl;
        
        auto params = parse_config_json(post_data);
        
        std::string config_name = "default_config";
        std::string config_data = params["config_data"];
        std::cout << "原始数据: " << config_data << std::endl;
        
        // 获取当前工作目录
        std::string current_dir;
#ifdef _WIN32
        // 使用宽字符函数处理可能包含中文的路径
        wchar_t wbuffer[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, wbuffer);
        
        // 将宽字符转换为UTF-8编码的字符串
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, NULL, 0, NULL, NULL);
        std::vector<char> utf8_buffer(size_needed);
        WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, utf8_buffer.data(), size_needed, NULL, NULL);
        
        current_dir = std::string(utf8_buffer.data());
        
        // 替换Windows路径中的反斜杠为正斜杠以适应JSON格式
        std::string json_dir = current_dir;
        std::replace(json_dir.begin(), json_dir.end(), '\\', '/');
        
        // 使用简单的正则表达式查找和替换路径
        // 为模型路径添加前缀 - 匹配以/开头但不是完整绝对路径的路径
        std::regex model_path_regex("\"model_name_or_path\"\\s*:\\s*\"(/[^:\"]+)\"");
        config_data = std::regex_replace(config_data, model_path_regex, "\"model_name_or_path\":\"" + json_dir + "/llm$1\"");
        
        // 为输出目录添加前缀 - 匹配以/开头但不是完整绝对路径的路径
        std::regex output_dir_regex("\"output_dir\"\\s*:\\s*\"(/[^:\"]+)\"");
        config_data = std::regex_replace(config_data, output_dir_regex, "\"output_dir\":\"" + json_dir + "/llm$1\"");
        
        // 为训练数据文件添加前缀 - 原来的逻辑已经可以正确工作
        std::regex train_file_regex("\"train_file\"\\s*:\\s*\"([^/\\\"][^\"]+)\"");
        config_data = std::regex_replace(config_data, train_file_regex, "\"train_file\":\"" + json_dir + "/llm/$1\"");
        
        std::cout << "修改后的数据: " << config_data << std::endl;
#else
        // Linux实现
        char buffer[PATH_MAX];
        if (getcwd(buffer, PATH_MAX) != NULL) {
            current_dir = buffer;
            
            // 修改路径
            std::regex model_path_regex("\"model_name_or_path\"\\s*:\\s*\"([^/][^\"]+)\"");
            config_data = std::regex_replace(config_data, model_path_regex, "\"model_name_or_path\":\"" + current_dir + "/llm/$1\"");
            
            std::regex output_dir_regex("\"output_dir\"\\s*:\\s*\"([^/][^\"]+)\"");
            config_data = std::regex_replace(config_data, output_dir_regex, "\"output_dir\":\"" + current_dir + "/llm/$1\"");
            
            std::regex train_file_regex("\"train_file\"\\s*:\\s*\"([^/][^\"]+)\"");
            config_data = std::regex_replace(config_data, train_file_regex, "\"train_file\":\"" + current_dir + "/llm/$1\"");
        }
#endif
        
        std::cout << "配置名称: " << config_name << std::endl;
        
        // 防止路径遍历攻击
        config_name = std::regex_replace(config_name, std::regex("[^a-zA-Z0-9_\\-]"), "_");

#ifdef _WIN32
        // 使用宽字符版本获取路径
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(NULL, buffer, MAX_PATH);
        
        // 转换为宽字符串并提取目录部分
        std::wstring fullPath(buffer);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        std::wstring current_config_dir = fullPath.substr(0, lastSlash);
        // 转换宽字符串到UTF-8字符串
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string current_config_dir_str = converter.to_bytes(current_config_dir);
        std::cout << "当前目录: " << current_config_dir_str << std::endl;
        
        // 构建配置目录路径 (使用正斜杠以避免转义问题)
        std::wstring configs_dir_w = current_config_dir + L"\\llm\\configs";
        std::string configs_dir = converter.to_bytes(configs_dir_w);
#else
        // Linux实现
        char buffer[PATH_MAX];
        if (getcwd(buffer, PATH_MAX) != NULL) {
            configs_dir = std::string(buffer) + "/llm/configs";
            // 确保目录存在
            mkdir(configs_dir.c_str(), 0755);
        } else {
            configs_dir = "./llm/configs";
            mkdir(configs_dir.c_str(), 0755);
        }
#endif

        // 日志记录，便于调试
        //std::cout << "保存配置: " << config_name << " 到目录: " << configs_dir << std::endl;
        
        // 保存配置文件
        // std::string config_path = configs_dir + "\\" + config_name + ".json";
        // 使用固定的非中文路径
        std::string config_path = "C:\\Windows\\elianfactory\\default_config.json";
        // std::cout << "完整配置路径: " << config_path << std::endl;
        
        bool success = false;
        std::string error_msg = "";
        
        try {
            // 使用独占写入模式打开文件
            std::ofstream config_file(config_path, std::ios::out | std::ios::trunc);
            if (config_file.is_open()) {
                config_file << config_data;
                config_file.flush(); // 确保数据刷新到磁盘
                config_file.close();
                
                // 验证文件是否真的被写入
                std::ifstream verify_file(config_path);
                std::string content;
                if (verify_file.is_open()) {
                    std::stringstream buffer;
                    buffer << verify_file.rdbuf();
                    content = buffer.str();
                    verify_file.close();
                    
                    if (content.find(config_data) != std::string::npos) {
                        success = true;
                        std::cout << "配置保存成功并已验证: " << config_path << std::endl;
                    } else {
                        error_msg = "文件写入验证失败，内容不匹配";
                        std::cerr << error_msg << std::endl;
                    }
                } else {
                    error_msg = "无法打开文件进行验证";
                    std::cerr << error_msg << std::endl;
                }
            } else {
                error_msg = "无法打开文件进行写入: " + config_path;
                std::cerr << error_msg << std::endl;
            }
        } catch (const std::exception& e) {
            error_msg = e.what();
            std::cerr << "保存配置文件异常: " << error_msg << std::endl;
        }
        
        // 返回结果
        std::ostringstream json;
        json << "{";
        json << "\"success\":" << (success ? "true" : "false");
        if (!success) {
            json << ",\"message\":\"保存配置失败\"";
            json << ",\"error\":\"" << escape_json(error_msg) << "\"";
        } else {
            json << ",\"message\":\"配置已成功保存\"";
            json << ",\"path\":\"" << escape_json(config_path) << "\"";
        }
        json << "}";
        
        return json_response(json.str());
    }
    else if (url == "/api/config/list") {
        std::vector<std::string> configs;
        std::string error = "";
        
        // 获取configs目录路径
        std::string configs_dir;
#ifdef _WIN32
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(NULL, buffer, MAX_PATH);
        std::wstring fullPath(buffer);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        std::wstring configsDirW = fullPath.substr(0, lastSlash) + L"\\llm\\configs";
        
        // 确保目录存在
        CreateDirectoryW(configsDirW.c_str(), NULL);
        
        // 列出所有配置文件
        WIN32_FIND_DATAW ffd;
        HANDLE hFind = FindFirstFileW((configsDirW + L"\\*.json").c_str(), &ffd);
        
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    // 将文件名转换为UTF-8
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, ffd.cFileName, -1, NULL, 0, NULL, NULL);
                    std::vector<char> nameBuffer(size_needed);
                    WideCharToMultiByte(CP_UTF8, 0, ffd.cFileName, -1, nameBuffer.data(), size_needed, NULL, NULL);
                    
                    std::string fileName(nameBuffer.data());
                    
                    // 移除.json后缀
                    if (ends_with(fileName, ".json")) {
                        fileName = fileName.substr(0, fileName.length() - 5);
                        configs.push_back(fileName);
                    }
                }
            } while (FindNextFileW(hFind, &ffd) != 0);
            
            FindClose(hFind);
        }
#else
        // Linux实现
        char buffer[PATH_MAX];
        if (getcwd(buffer, PATH_MAX) != NULL) {
            configs_dir = std::string(buffer) + "/llm/configs";
        } else {
            configs_dir = "./llm/configs";
        }
        
        // 确保目录存在
        mkdir(configs_dir.c_str(), 0755);
        
        DIR* dir = opendir(configs_dir.c_str());
        if (dir != NULL) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                std::string fileName = entry->d_name;
                if (ends_with(fileName, ".json")) {
                    fileName = fileName.substr(0, fileName.length() - 5);
                    configs.push_back(fileName);
                }
            }
            closedir(dir);
        }
#endif

        // 构建JSON响应
        std::ostringstream json;
        json << "{\"success\":true,\"configs\":[";
        
        for (size_t i = 0; i < configs.size(); ++i) {
            if (i > 0) json << ",";
            json << "\"" << escape_json(configs[i]) << "\"";
        }
        
        json << "]}";
        return json_response(json.str());
    }
    else if (starts_with(url, "/api/config/load")) {
        // 解析查询参数
        std::string config_name;
        size_t query_start = url.find('?');
        
        if (query_start != std::string::npos) {
            std::string query = url.substr(query_start + 1);
            std::istringstream query_stream(query);
            std::string param;
            
            while (std::getline(query_stream, param, '&')) {
                size_t eq_pos = param.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = param.substr(0, eq_pos);
                    std::string value = param.substr(eq_pos + 1);
                    
                    if (key == "name") {
                        config_name = value;
                        break;
                    }
                }
            }
        }
        
        if (config_name.empty()) {
            std::ostringstream json;
            json << "{\"success\":false,\"message\":\"缺少配置名称参数\"}";
            return json_response(json.str());
        }
        
        // 防止路径遍历攻击
        config_name = std::regex_replace(config_name, std::regex("[^a-zA-Z0-9_\\-]"), "_");
        
        // 构建配置文件路径
        std::string config_path;
        std::string config_content;
        bool success = false;
        
#ifdef _WIN32
        // 构建配置路径
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(NULL, buffer, MAX_PATH);
        std::wstring fullPath(buffer);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        
        // 先检查目录是否存在
        std::wstring configDirW = fullPath.substr(0, lastSlash) + L"\\llm\\configs";
        CreateDirectoryW(configDirW.c_str(), NULL); // 确保目录存在
        
        std::wstring configFileW = configDirW + L"\\" + s2ws(config_name) + L".json";
        
        // 转换为多字节字符串路径
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, configFileW.c_str(), -1, NULL, 0, NULL, NULL);
        std::vector<char> configPathBuffer(size_needed);
        WideCharToMultiByte(CP_UTF8, 0, configFileW.c_str(), -1, configPathBuffer.data(), size_needed, NULL, NULL);
        config_path = std::string(configPathBuffer.data());
#else
        // Linux实现
        char buffer[PATH_MAX];
        if (getcwd(buffer, PATH_MAX) != NULL) {
            std::string current_dir = buffer;
            config_path = current_dir + "/llm/configs/" + config_name + ".json";
        } else {
            config_path = "./llm/configs/" + config_name + ".json";
        }
#endif
        
        // 检查文件是否存在
        if (file_exists(config_path)) {
            try {
                // 读取配置文件
                std::ifstream config_file(config_path);
                if (config_file.is_open()) {
                    std::stringstream buffer;
                    buffer << config_file.rdbuf();
                    config_content = buffer.str();
                    config_file.close();
                    success = true;
                }
            } catch (const std::exception& e) {
                // 读取出错
                success = false;
                config_content = std::string("读取配置文件出错: ") + e.what();
            }
        }

        // 返回结果
        std::ostringstream json;
        json << "{";
        json << "\"success\":" << (success ? "true" : "false");
        
        if (success) {
            json << ",\"config\":" << config_content;
        } else {
            json << ",\"message\":\"配置文件不存在或无法读取\"";
            if (!config_content.empty()) {
                json << ",\"error\":\"" << escape_json(config_content) << "\"";
            }
        }
        
        json << "}";
        return json_response(json.str());
    }
    else if (starts_with(url, "/api/config/delete") && method == "DELETE") {
        // 解析查询参数
        std::string config_name;
        size_t query_start = url.find('?');
        
        if (query_start != std::string::npos) {
            std::string query = url.substr(query_start + 1);
            std::istringstream query_stream(query);
            std::string param;
            
            while (std::getline(query_stream, param, '&')) {
                size_t eq_pos = param.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = param.substr(0, eq_pos);
                    std::string value = param.substr(eq_pos + 1);
                    
                    if (key == "name") {
                        config_name = value;
                        break;
                    }
                }
            }
        }
        
        if (config_name.empty()) {
            std::ostringstream json;
            json << "{\"success\":false,\"message\":\"缺少配置名称参数\"}";
            return json_response(json.str());
        }
        
        // 防止路径遍历攻击
        config_name = std::regex_replace(config_name, std::regex("[^a-zA-Z0-9_\\-]"), "_");
        
        // 构建配置文件路径
        std::string config_path;
        bool success = false;
        std::string error_msg;
        
#ifdef _WIN32
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(NULL, buffer, MAX_PATH);
        std::wstring fullPath(buffer);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        std::wstring configFileW = fullPath.substr(0, lastSlash) + L"\\llm\\configs\\" + s2ws(config_name) + L".json";
        
        // 删除文件
        if (DeleteFileW(configFileW.c_str())) {
            success = true;
        } else {
            DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND) {
                error_msg = "配置文件不存在";
            } else {
                error_msg = "删除文件失败，错误码: " + std::to_string(error);
            }
        }
#else
        // Linux实现
        char buffer[PATH_MAX];
        if (getcwd(buffer, PATH_MAX) != NULL) {
            config_path = std::string(buffer) + "/llm/configs/" + config_name + ".json";
        } else {
            config_path = "./llm/configs/" + config_name + ".json";
        }
        
        if (remove(config_path.c_str()) == 0) {
            success = true;
        } else {
            error_msg = "删除文件失败: " + std::string(strerror(errno));
        }
#endif
        
        // 返回结果
        std::ostringstream json;
        json << "{";
        json << "\"success\":" << (success ? "true" : "false");
        
        if (success) {
            json << ",\"message\":\"配置已成功删除\"";
        } else {
            json << ",\"message\":\"删除配置失败\"";
            json << ",\"error\":\"" << escape_json(error_msg) << "\"";
        }
        
        json << "}";
        return json_response(json.str());
    }
    
    // 其他API返回404
    std::ostringstream json;
    json << "{\"success\": false, \"message\": \"API端点不存在\", \"error\": \"not_found\"}";
    std::ostringstream response;
    response << "HTTP/1.1 404 Not Found\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Content-Length: " << json.str().length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "\r\n";
    response << json.str();
    return response.str();
}

// 简单的HTTP响应处理
std::string handle_request(const std::string& request) {
    // 解析HTTP请求的第一行来获取URL和方法
    std::istringstream req_stream(request);
    std::string method, url, version;
    req_stream >> method >> url >> version;

    
    // 处理API请求
    if (starts_with(url, API_PREFIX)) {
        return handle_api_request(url, request, method);
    }
    
    if (url == "/") {
        url = "/index.html";
        // std::cout << "重定向到: " << url << std::endl;
    }
    
    // 读取请求的文件
    std::string file_path = WEB_DIR + url;
    // std::cout << "尝试读取文件: " << file_path << std::endl;
    
    std::ifstream file(file_path, std::ios::binary);
    
    if (!file) {
        std::cout << "文件不存在: " << file_path << std::endl;
        
        // 检查文件系统，查看web目录下有哪些文件
        std::vector<std::string> files = list_files_in_directory(WEB_DIR, "");
        std::cout << "Web目录内容:" << std::endl;
        for (const auto& f : files) {
            std::cout << " - " << f << std::endl;
        }
        
        // 文件不存在，返回404错误
        return "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p><p>Requested: " + url + "</p></body></html>";
    }
    
    // 读取文件内容
    std::ostringstream content_stream;
    content_stream << file.rdbuf();
    std::string content = content_stream.str();
    // std::cout << "文件读取成功，大小: " << content.length() << " 字节" << std::endl;
    
    // 确定内容类型
    std::string content_type = "text/plain";
    if (ends_with(url, ".html") || ends_with(url, ".htm")) {
        content_type = "text/html";
    } else if (ends_with(url, ".css")) {
        content_type = "text/css";
    } else if (ends_with(url, ".js")) {
        content_type = "application/javascript";
    } else if (ends_with(url, ".json")) {
        content_type = "application/json";
    } else if (ends_with(url, ".png")) {
        content_type = "image/png";
    } else if (ends_with(url, ".jpg") || ends_with(url, ".jpeg")) {
        content_type = "image/jpeg";
    }
    
    // 构建HTTP响应
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "\r\n";
    response << content;
    
    return response.str();
}

// 开启简单的HTTP服务器
void start_server() {
#ifdef _WIN32
    // Windows平台的简单服务器实现
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return;
    }
    
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return;
    }
    
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return;
    }
    
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return;
    }
    
    std::cout << "Server started on port " << PORT << std::endl;
    std::cout << "Serving files from " << WEB_DIR << std::endl;
    std::cout << "请在浏览器中输入：http://127.0.0.1:10171/进行使用 "  << std::endl;
    
    while (true) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        
        char buffer[4096] = {0};
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string response = handle_request(buffer);
            send(client_socket, response.c_str(), response.length(), 0);
        }
        
        closesocket(client_socket);
    }
    
    closesocket(server_socket);
    WSACleanup();
#else
    // UNIX/Linux平台的简单服务器实现
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Failed to set socket options" << std::endl;
        return;
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return;
    }
    
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return;
    }
    
    std::cout << "Server started on port " << PORT << std::endl;
    std::cout << "Serving files from " << WEB_DIR << std::endl;
    
    while (true) {
        int new_socket;
        int addrlen = sizeof(address);
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        
        char buffer[4096] = {0};
        int bytes_received = read(new_socket, buffer, sizeof(buffer) - 1);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string response = handle_request(buffer);
            write(new_socket, response.c_str(), response.length());
        }
        
        close(new_socket);
    }
#endif
}

int main() {
    // 设置控制台输出编码为UTF-8以解决中文乱码问题
#ifdef _WIN32
    // Windows平台设置控制台代码页为UTF-8
    SetConsoleOutputCP(65001);  // 65001是UTF-8的代码页
#endif

    // 打印服务器版本和API信息
    std::cout << "==========================================================================" << std::endl;
    std::cout << "ElianFactory Server v1.0 配置信息" << std::endl;
    std::cout << "端口: " << PORT << std::endl;
    std::cout << "Web目录: " << WEB_DIR << std::endl;
    std::cout << "配置信息:" << std::endl;
    std::cout << " - conda环境            elianfactory" << std::endl;
    std::cout << " - GitHub地址           https://github.com/2Elian/Elian-Factory" << std::endl;
    std::cout << "==========================================================================" << std::endl;

    // 确保web目录存在
#ifdef _WIN32
    // 使用宽字符API创建目录
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, WEB_DIR.c_str(), -1, NULL, 0);
    std::vector<wchar_t> web_dir_w(size_needed);
    MultiByteToWideChar(CP_UTF8, 0, WEB_DIR.c_str(), -1, web_dir_w.data(), size_needed);
    
    // 检查web目录是否存在，如果不存在则创建
    DWORD attrs = GetFileAttributesW(web_dir_w.data());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        std::cout << "Web目录不存在，正在创建..." << std::endl;
        if (!CreateDirectoryW(web_dir_w.data(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            std::cerr << "创建Web目录失败，错误码: " << GetLastError() << std::endl;
        } else {
            std::cout << "" << std::endl;
        }
    } else {
        std::cout << "" << std::endl;
    }
#else
    if (mkdir(WEB_DIR.c_str(), 0755) != 0 && errno != EEXIST) {
        std::cerr << "创建Web目录失败: " << strerror(errno) << std::endl;
    }
#endif

    // 列出web目录下的文件进行调试
    // std::cout << "Web目录文件列表:" << std::endl;
    std::vector<std::string> web_files = list_files_in_directory(WEB_DIR, "");
    if (web_files.empty()) {
        std::cout << "Web目录为空" << std::endl;
    } else {
        for (const auto& file : web_files) {
            std::cout << " - " << file << std::endl;
        }
    }
    
    // 仅当web目录为空时创建默认index.html
    bool web_dir_empty = web_files.empty();
    
    // 仅在web目录为空时创建默认index.html
    if (web_dir_empty) {
        std::string index_path = WEB_DIR + "/index.html";
        std::cout << "创建默认index.html: " << index_path << std::endl;
        std::ofstream create_index(index_path);
        create_index << "<!DOCTYPE html>\n"
                     << "<html lang=\"zh\">\n"
                     << "<head>\n"
                     << "    <meta charset=\"UTF-8\">\n"
                     << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                     << "    <title>LLM Trainer</title>\n"
                     << "    <style>\n"
                     << "        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }\n"
                     << "        h1 { color: #333; }\n"
                     << "    </style>\n"
                     << "</head>\n"
                     << "<body>\n"
                     << "    <h1>LLM Trainer</h1>\n"
                     << "    <p>欢迎使用LLM训练工具。</p>\n"
                     << "    <p>网络连接故障时的静态备份页面。</p>\n"
                     << "</body>\n"
                     << "</html>";
        create_index.close();
        std::cout << "默认index.html创建完成" << std::endl;
    } else {
        std::cout << "" << std::endl;
    }

    // 确保llm目录和其子目录存在
    std::string llm_dir = "./llm";
#ifdef _WIN32
    size_needed = MultiByteToWideChar(CP_UTF8, 0, llm_dir.c_str(), -1, NULL, 0);
    std::vector<wchar_t> llm_dir_w(size_needed);
    MultiByteToWideChar(CP_UTF8, 0, llm_dir.c_str(), -1, llm_dir_w.data(), size_needed);
    
    if (!CreateDirectoryW(llm_dir_w.data(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        std::cerr << "创建llm目录失败，错误码: " << GetLastError() << std::endl;
    }

    // 创建数据目录
    std::string data_dir = "./llm/data";
    size_needed = MultiByteToWideChar(CP_UTF8, 0, data_dir.c_str(), -1, NULL, 0);
    std::vector<wchar_t> data_dir_w(size_needed);
    MultiByteToWideChar(CP_UTF8, 0, data_dir.c_str(), -1, data_dir_w.data(), size_needed);
    
    if (!CreateDirectoryW(data_dir_w.data(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        std::cerr << "创建data目录失败，错误码: " << GetLastError() << std::endl;
    }
#else
    mkdir(llm_dir.c_str(), 0755);
    mkdir((llm_dir + "/data").c_str(), 0755);
#endif

    // 在Windows目录下创建elianfactory文件夹及配置文件
    std::string windows_dir = "C:\\Windows";
    std::string config_dir = windows_dir + "\\elianfactory";
    std::string config_file = config_dir + "\\default_config.json";
    
    // 创建目录
    if (!file_exists(config_dir)) {
#ifdef _WIN32
        // 创建目录并设置权限
        if (!CreateDirectoryA(config_dir.c_str(), NULL)) {
            std::cerr << "无法创建目录: " << config_dir << std::endl;
        }
#else
        // Linux方式创建目录
        mkdir(config_dir.c_str(), 0777);
#endif
    }
    
    // 创建默认配置文件
    if (!file_exists(config_file)) {
        std::string default_config = R"({
  "model_name_or_path": "/ckpt/ds",
  "output_dir": "/output/your_task/lora",
  "train_file": "data/sample.jsonl",
  "num_train_epochs": 3,
  "per_device_train_batch_size": 1,
  "gradient_accumulation_steps": 4,
  "learning_rate": 0.0002,
  "max_seq_length": 1024,
  "logging_steps": 1,
  "save_steps": 200,
  "save_total_limit": 3,
  "lr_scheduler_type": "constant_with_warmup",
  "warmup_steps": 30,
  "lora_rank": 8,
  "lora_alpha": 16,
  "lora_dropout": 0.05,
  "gradient_checkpointing": true,
  "optim": "adamw_torch",
  "train_mode": "lora",
  "seed": 42,
  "fp16": false,
  "distributed": false
})";

        // 创建并写入配置文件
        std::ofstream config_out(config_file);
        if (config_out.is_open()) {
            config_out << default_config;
            config_out.close();
            
#ifdef _WIN32
            // 设置文件权限为可读写
            DWORD attributes = GetFileAttributesA(config_file.c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES) {
                SetFileAttributesA(config_file.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
            }
#else
            // Linux设置权限
            chmod(config_file.c_str(), 0666);
#endif
            std::cout << "已创建默认配置文件: " << config_file << std::endl;
        } else {
            std::cerr << "无法创建配置文件: " << config_file << std::endl;
        }
    }

    std::cout << "服务器准备启动..." << std::endl;
    start_server();
    return 0;
} 

