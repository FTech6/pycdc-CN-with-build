#include <cstring>
#include <fstream>
#include <iostream>
#include "ASTree.h"

#ifdef WIN32
#  define PATHSEP '\\'  // Windows 路径分隔符
#else
#  define PATHSEP '/'   // Unix/Linux 路径分隔符
#endif

int main(int argc, char* argv[])
{
    const char* infile = nullptr;      // 输入文件路径
    bool marshalled = false;           // 是否加载编译的代码对象
    const char* version = nullptr;     // Python 版本号
    std::ostream* pyc_output = &std::cout;  // 输出流，默认为标准输出
    std::ofstream out_file;            // 文件输出流

    for (int arg = 1; arg < argc; ++arg) {
        if (strcmp(argv[arg], "-o") == 0) {
            if (arg + 1 < argc) {
                const char* filename = argv[++arg];  // 获取输出文件名
                out_file.open(filename, std::ios_base::out);  // 打开文件
                if (out_file.fail()) {
                    fprintf(stderr, "错误：打开文件 '%s' 写入失败\n",
                            filename);
                    return 1;
                }
                pyc_output = &out_file;  // 将输出流指向文件
            } else {
                fputs("选项 '-o' 需要指定文件名\n", stderr);
                return 1;
            }
        } else if (strcmp(argv[arg], "-c") == 0) {
            marshalled = true;  // 标记为加载编译的代码对象
        } else if (strcmp(argv[arg], "-v") == 0) {
            if (arg + 1 < argc) {
                version = argv[++arg];  // 获取 Python 版本号
            } else {
                fputs("选项 '-v' 需要指定版本号\n", stderr);
                return 1;
            }
        } else if (strcmp(argv[arg], "--help") == 0 || strcmp(argv[arg], "-h") == 0) {
            fprintf(stderr, "用法：%s [选项] 输入文件.pyc\n\n", argv[0]);
            fputs("选项：\n", stderr);
            fputs("  -o <文件名>    将输出写入到<文件名> (默认：标准输出)\n", stderr);
            fputs("  -c             指定加载编译的代码对象。需要设置版本号\n", stderr);
            fputs("  -v <x.y>       指定Python版本用于加载编译的代码对象\n", stderr);
            fputs("  --help         显示此帮助信息并退出\n", stderr);
            return 0;
        } else {
            infile = argv[arg];
        }
    }

    if (!infile) {
        fputs("未指定输入文件\n", stderr);
        return 1;
    }

    PycModule mod;
    if (!marshalled) {
        try {
            mod.loadFromFile(infile);
        } catch (std::exception& ex) {
            fprintf(stderr, "加载文件 %s 时出错：%s\n", infile, ex.what());
            return 1;
        }
    } else {
        if (!version) {
            fputs("打开原始代码对象需要指定版本号\n", stderr);
            return 1;
        }
        std::string s(version);
        auto dot = s.find('.');
        if (dot == std::string::npos || dot == s.size()-1) {
            fputs("无法解析版本字符串 (请使用 x.y 格式)\n", stderr);
            return 1;
        }
        int major = std::stoi(s.substr(0, dot));
        int minor = std::stoi(s.substr(dot+1, s.size()));
        mod.loadFromMarshalledFile(infile, major, minor);
    }

    if (!mod.isValid()) {
        fprintf(stderr, "无法加载文件 %s\n", infile);
        return 1;
    }
    const char* dispname = strrchr(infile, PATHSEP);
    dispname = (dispname == NULL) ? infile : dispname + 1;
    *pyc_output << "# 源代码由 Decompyle++ 生成\n";
    formatted_print(*pyc_output, "# 文件：%s (Python %d.%d%s)\n\n", dispname,
                    mod.majorVer(), mod.minorVer(),
                    (mod.majorVer() < 3 && mod.isUnicode()) ? " Unicode" : "");
    try {
        decompyle(mod.code(), &mod, *pyc_output);
    } catch (std::exception& ex) {
        fprintf(stderr, "反编译 %s 时出错：%s\n", infile, ex.what());
        return 1;
    }

    return 0;
}