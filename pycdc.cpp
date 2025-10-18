#include <cstring>
#include <fstream>
#include <iostream>
#include "ASTree.h"
#include "utf8out_stream.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef WIN32
#define PATHSEP '\\'
#else
#define PATHSEP '/'
#endif

void print_help(const char* argv0)
{
    std::printf("用法: %s [选项] 输入文件.pyc\n\n", argv0);
    std::printf("描述:\n");
    std::printf("  将 Python 字节码文件(.pyc)反编译为可读的 Python 源代码\n\n");
    std::printf("选项:\n");
    std::printf("  -o <文件名>    将反编译结果输出到指定文件\n");
    std::printf("                 默认输出到标准输出(stdout)\n");
    std::printf("  -c             加载编译的代码对象而不是完整的 pyc 文件\n");
    std::printf("                 使用此选项时必须同时指定 -v 版本号\n");
    std::printf("  -v <x.y>       指定 Python 版本号 (例如: 3.8, 3.9)\n");
    std::printf("                 当使用 -c 选项加载代码对象时必须指定\n");
    std::printf("  -h, --help     显示此帮助信息并退出\n");
    std::printf("\n示例:\n");
    std::printf("  %s script.pyc                    # 反编译单个文件\n", argv0);
    std::printf("  %s -o output.py script.pyc       # 输出到文件\n", argv0);
    std::printf("  %s -c -v 3.9 codeobj.bin        # 加载编译的代码对象\n", argv0);
    std::printf("\n注意:\n");
    std::printf("  - 支持 Python 2.7 和 3.x 版本的字节码文件\n");
    std::printf("  - 对于加密或混淆的字节码文件可能无法正确反编译\n");
}

void print_error_help(const char* argv0)
{
    std::fprintf(stderr, "\n使用 '%s -h' 查看完整的帮助信息\n", argv0);
}

class ConsoleEncodingHelper {
#ifdef WIN32
private:
    UINT originalOutputCP;
    UINT originalInputCP;
    bool encodingChanged;
    
public:
    ConsoleEncodingHelper() : encodingChanged(false) {
        originalOutputCP = GetConsoleOutputCP();
        originalInputCP = GetConsoleCP();
        
        SetConsoleOutputCP(65001);
        SetConsoleCP(65001);
        
        encodingChanged = (originalOutputCP != 65001);
        if (encodingChanged) {
            std::cerr << "注意：已将控制台编码从 " << originalOutputCP << " 切换到 UTF-8 (65001)" << std::endl;
            std::cerr << "程序结束后将恢复原编码" << std::endl;
        }
    }
    
    ~ConsoleEncodingHelper() {
        if (encodingChanged) {
            SetConsoleOutputCP(originalOutputCP);
            SetConsoleCP(originalInputCP);
            std::cerr << "Console encoding restored to " << originalOutputCP << std::endl;
        }
    }
    
    void restoreEarly() {
        if (encodingChanged) {
            SetConsoleOutputCP(originalOutputCP);
            SetConsoleCP(originalInputCP);
            std::cerr << "Console encoding restored to " << originalOutputCP << std::endl;
            encodingChanged = false;
        }
    }
#else
public:
    void restoreEarly() {}
#endif
};

int main(int argc, char* argv[])
{
    ConsoleEncodingHelper encodingHelper;

    const char* infile = nullptr;
    bool marshalled = false;
    const char* version = nullptr;
    std::ostream* raw_output = &std::cout;
    std::ofstream out_file;

    for (int arg = 1; arg < argc; ++arg) {
        if (strcmp(argv[arg], "-o") == 0) {
            if (arg + 1 < argc) {
                const char* filename = argv[++arg];
                out_file.open(filename, std::ios_base::out);
                if (out_file.fail()) {
                    fprintf(stderr, "错误：打开文件 '%s' 写入失败\n", filename);
                    print_error_help(argv[0]);
                    encodingHelper.restoreEarly();
                    return 1;
                }
                raw_output = &out_file;
            } else {
                fputs("错误：选项 '-o' 需要指定文件名\n", stderr);
                print_error_help(argv[0]);
                encodingHelper.restoreEarly();
                return 1;
            }
        } else if (strcmp(argv[arg], "-c") == 0) {
            marshalled = true;
        } else if (strcmp(argv[arg], "-v") == 0) {
            if (arg + 1 < argc) {
                version = argv[++arg];
            } else {
                fputs("错误：选项 '-v' 需要指定版本号\n", stderr);
                print_error_help(argv[0]);
                encodingHelper.restoreEarly();
                return 1;
            }
        } else if (strcmp(argv[arg], "--help") == 0 || strcmp(argv[arg], "-h") == 0) {
            print_help(argv[0]);
            return 0;
        } else {
            infile = argv[arg];
        }
    }

    if (!infile) {
        fputs("错误：未指定输入文件\n", stderr);
        print_error_help(argv[0]);
        encodingHelper.restoreEarly();
        return 1;
    }

    PycModule mod;
    if (!marshalled) {
        try {
            mod.loadFromFile(infile);
        } catch (std::exception& ex) {
            fprintf(stderr, "错误：加载文件 %s 时出错：%s\n", infile, ex.what());
            print_error_help(argv[0]);
            encodingHelper.restoreEarly();
            return 1;
        }
    } else {
        if (!version) {
            fputs("错误：打开原始代码对象需要指定版本号\n", stderr);
            print_error_help(argv[0]);
            encodingHelper.restoreEarly();
            return 1;
        }
        std::string s(version);
        auto dot = s.find('.');
        if (dot == std::string::npos || dot == s.size()-1) {
            fputs("错误：无法解析版本字符串 (请使用 x.y 格式)\n", stderr);
            print_error_help(argv[0]);
            encodingHelper.restoreEarly();
            return 1;
        }
        int major = std::stoi(s.substr(0, dot));
        int minor = std::stoi(s.substr(dot+1, s.size()));
        mod.loadFromMarshalledFile(infile, major, minor);
    }

    if (!mod.isValid()) {
        fprintf(stderr, "错误：无法加载文件 %s\n", infile);
        print_error_help(argv[0]);
        encodingHelper.restoreEarly();
        return 1;
    }
    
    const char* dispname = strrchr(infile, PATHSEP);
    dispname = (dispname == NULL) ? infile : dispname + 1;
    
    utf8out_stream pyc_output(*raw_output);
    
    pyc_output << "# 源代码由 Decompyle++ 生成\n";
    
    formatted_print(pyc_output, "# 文件：%s (Python %d.%d%s)\n\n", dispname,
                    mod.majorVer(), mod.minorVer(),
                    (mod.majorVer() < 3 && mod.isUnicode()) ? " Unicode" : "");
    
    try {
        decompyle(mod.code(), &mod, pyc_output);
    } catch (std::exception& ex) {
        fprintf(stderr, "错误：反编译 %s 时出错：%s\n", infile, ex.what());
        print_error_help(argv[0]);
        encodingHelper.restoreEarly();
        return 1;
    }

    return 0;
}