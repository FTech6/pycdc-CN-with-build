#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include "pyc_module.h"
#include "pyc_numeric.h"
#include "bytecode.h"

#ifdef WIN32
#  define PATHSEP '\\'  // Windows路径分隔符
#else
#  define PATHSEP '/'   // Unix/Linux路径分隔符
#endif

// 代码标志位名称
static const char* flag_names[] = {
    "CO_OPTIMIZED", "CO_NEWLOCALS", "CO_VARARGS", "CO_VARKEYWORDS",
    "CO_NESTED", "CO_GENERATOR", "CO_NOFREE", "CO_COROUTINE",
    "CO_ITERABLE_COROUTINE", "CO_ASYNC_GENERATOR", "<0x400>", "<0x800>",
    "CO_GENERATOR_ALLOWED", "<0x2000>", "<0x4000>", "<0x8000>",
    "<0x10000>", "CO_FUTURE_DIVISION", "CO_FUTURE_ABSOLUTE_IMPORT", "CO_FUTURE_WITH_STATEMENT",
    "CO_FUTURE_PRINT_FUNCTION", "CO_FUTURE_UNICODE_LITERALS", "CO_FUTURE_BARRY_AS_BDFL",
            "CO_FUTURE_GENERATOR_STOP",
    "CO_FUTURE_ANNOTATIONS", "CO_NO_MONITORING_EVENTS", "<0x4000000>", "<0x8000000>",
    "<0x10000000>", "<0x20000000>", "<0x40000000>", "<0x80000000>"
};

// 打印代码标志位
static void print_coflags(unsigned long flags, std::ostream& pyc_output)
{
    if (flags == 0) {
        pyc_output << "\n";
        return;
    }

    pyc_output << " (";
    unsigned long f = 1;
    int k = 0;
    while (k < 32) {
        if ((flags & f) != 0) {
            flags &= ~f;
            if (flags == 0)
                pyc_output << flag_names[k];
            else
                pyc_output << flag_names[k] << " | ";
        }
        ++k;
        f <<= 1;
    }
    pyc_output << ")\n";
}

// 缩进输出文本
static void iputs(std::ostream& pyc_output, int indent, const char* text)
{
    for (int i=0; i<indent; i++)
        pyc_output << "    ";
    pyc_output << text;
}

// 缩进格式化输出(可变参数版本)
static void ivprintf(std::ostream& pyc_output, int indent, const char* fmt,
                     va_list varargs)
{
    for (int i=0; i<indent; i++)
        pyc_output << "    ";
    formatted_printv(pyc_output, fmt, varargs);
}

// 缩进格式化输出
static void iprintf(std::ostream& pyc_output, int indent, const char* fmt, ...)
{
    va_list varargs;
    va_start(varargs, fmt);
    ivprintf(pyc_output, indent, fmt, varargs);
    va_end(varargs);
}

// 已输出对象集合(用于检测循环引用)
static std::unordered_set<PycObject *> out_seen;

// 输出Python对象
void output_object(PycRef<PycObject> obj, PycModule* mod, int indent,
                   unsigned flags, std::ostream& pyc_output)
{
    if (obj == NULL) {
        iputs(pyc_output, indent, "<NULL>");
        return;
    }

    // 检测循环引用
    if (out_seen.find((PycObject *)obj) != out_seen.end()) {
        fputs("警告: 检测到循环引用\n", stderr);
        return;
    }
    out_seen.insert((PycObject *)obj);

    switch (obj->type()) {
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        {
            PycRef<PycCode> codeObj = obj.cast<PycCode>();
            iputs(pyc_output, indent, "[代码对象]\n");
            iprintf(pyc_output, indent + 1, "文件名: %s\n", codeObj->fileName()->value());
            iprintf(pyc_output, indent + 1, "对象名: %s\n", codeObj->name()->value());
            if (mod->verCompare(3, 11) >= 0)
                iprintf(pyc_output, indent + 1, "限定名: %s\n", codeObj->qualName()->value());
            iprintf(pyc_output, indent + 1, "参数数量: %d\n", codeObj->argCount());
            if (mod->verCompare(3, 8) >= 0)
                iprintf(pyc_output, indent + 1, "仅位置参数数量: %d\n", codeObj->posOnlyArgCount());
            if (mod->majorVer() >= 3)
                iprintf(pyc_output, indent + 1, "仅关键字参数数量: %d\n", codeObj->kwOnlyArgCount());
            if (mod->verCompare(3, 11) < 0)
                iprintf(pyc_output, indent + 1, "局部变量数量: %d\n", codeObj->numLocals());
            if (mod->verCompare(1, 5) >= 0)
                iprintf(pyc_output, indent + 1, "栈大小: %d\n", codeObj->stackSize());
            if (mod->verCompare(1, 3) >= 0) {
                unsigned int orig_flags = codeObj->flags();
                if (mod->verCompare(3, 8) < 0) {
                    // 将标志位映射回PyCode对象中存储的值
                    orig_flags = (orig_flags & 0xFFFF) | ((orig_flags & 0xFFF00000) >> 4);
                }
                iprintf(pyc_output, indent + 1, "标志位: 0x%08X", orig_flags);
                print_coflags(codeObj->flags(), pyc_output);
            }

            iputs(pyc_output, indent + 1, "[名称表]\n");
            for (int i=0; i<codeObj->names()->size(); i++)
                output_object(codeObj->names()->get(i), mod, indent + 2, flags, pyc_output);

            if (mod->verCompare(1, 3) >= 0) {
                if (mod->verCompare(3, 11) >= 0)
                    iputs(pyc_output, indent + 1, "[局部变量+名称]\n");
                else
                    iputs(pyc_output, indent + 1, "[变量名]\n");
                for (int i=0; i<codeObj->localNames()->size(); i++)
                    output_object(codeObj->localNames()->get(i), mod, indent + 2, flags, pyc_output);
            }

            if (mod->verCompare(3, 11) >= 0 && (flags & Pyc::DISASM_PYCODE_VERBOSE) != 0) {
                iputs(pyc_output, indent + 1, "[局部变量+种类]\n");
                output_object(codeObj->localKinds().cast<PycObject>(), mod, indent + 2, flags, pyc_output);
            }

            if (mod->verCompare(2, 1) >= 0 && mod->verCompare(3, 11) < 0) {
                iputs(pyc_output, indent + 1, "[自由变量]\n");
                for (int i=0; i<codeObj->freeVars()->size(); i++)
                    output_object(codeObj->freeVars()->get(i), mod, indent + 2, flags, pyc_output);

                iputs(pyc_output, indent + 1, "[单元格变量]\n");
                for (int i=0; i<codeObj->cellVars()->size(); i++)
                    output_object(codeObj->cellVars()->get(i), mod, indent + 2, flags, pyc_output);
            }

            iputs(pyc_output, indent + 1, "[常量]\n");
            for (int i=0; i<codeObj->consts()->size(); i++)
                output_object(codeObj->consts()->get(i), mod, indent + 2, flags, pyc_output);

            iputs(pyc_output, indent + 1, "[反汇编]\n");
            bc_disasm(pyc_output, codeObj, mod, indent + 2, flags);

            if (mod->verCompare(3, 11) >= 0) {
                iputs(pyc_output, indent + 1, "[异常表]\n");
                bc_exceptiontable(pyc_output, codeObj, indent+2);
            }

            if (mod->verCompare(1, 5) >= 0 && (flags & Pyc::DISASM_PYCODE_VERBOSE) != 0) {
                iprintf(pyc_output, indent + 1, "首行号: %d\n", codeObj->firstLine());
                iputs(pyc_output, indent + 1, "[行号表]\n");
                output_object(codeObj->lnTable().cast<PycObject>(), mod, indent + 2, flags, pyc_output);
            }
        }
        break;
    case PycObject::TYPE_STRING:
    case PycObject::TYPE_UNICODE:
    case PycObject::TYPE_INTERNED:
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        iputs(pyc_output, indent, "");
        obj.cast<PycString>()->print(pyc_output, mod);
        pyc_output << "\n";
        break;
    case PycObject::TYPE_TUPLE:
    case PycObject::TYPE_SMALL_TUPLE:
        {
            iputs(pyc_output, indent, "(\n");
            for (const auto& val : obj.cast<PycTuple>()->values())
                output_object(val, mod, indent + 1, flags, pyc_output);
            iputs(pyc_output, indent, ")\n");
        }
        break;
    case PycObject::TYPE_LIST:
        {
            iputs(pyc_output, indent, "[\n");
            for (const auto& val : obj.cast<PycList>()->values())
                output_object(val, mod, indent + 1, flags, pyc_output);
            iputs(pyc_output, indent, "]\n");
        }
        break;
    case PycObject::TYPE_DICT:
        {
            iputs(pyc_output, indent, "{\n");
            for (const auto& val : obj.cast<PycDict>()->values()) {
                output_object(std::get<0>(val), mod, indent + 1, flags, pyc_output);
                output_object(std::get<1>(val), mod, indent + 2, flags, pyc_output);
            }
            iputs(pyc_output, indent, "}\n");
        }
        break;
    case PycObject::TYPE_SET:
        {
            iputs(pyc_output, indent, "{\n");
            for (const auto& val : obj.cast<PycSet>()->values())
                output_object(val, mod, indent + 1, flags, pyc_output);
            iputs(pyc_output, indent, "}\n");
        }
        break;
    case PycObject::TYPE_FROZENSET:
        {
            iputs(pyc_output, indent, "frozenset({\n");
            for (const auto& val : obj.cast<PycSet>()->values())
                output_object(val, mod, indent + 1, flags, pyc_output);
            iputs(pyc_output, indent, "})\n");
        }
        break;
    case PycObject::TYPE_NONE:
        iputs(pyc_output, indent, "None\n");
        break;
    case PycObject::TYPE_FALSE:
        iputs(pyc_output, indent, "False\n");
        break;
    case PycObject::TYPE_TRUE:
        iputs(pyc_output, indent, "True\n");
        break;
    case PycObject::TYPE_ELLIPSIS:
        iputs(pyc_output, indent, "...\n");
        break;
    case PycObject::TYPE_INT:
        iprintf(pyc_output, indent, "%d\n", obj.cast<PycInt>()->value());
        break;
    case PycObject::TYPE_LONG:
        iprintf(pyc_output, indent, "%s\n", obj.cast<PycLong>()->repr(mod).c_str());
        break;
    case PycObject::TYPE_FLOAT:
        iprintf(pyc_output, indent, "%s\n", obj.cast<PycFloat>()->value());
        break;
    case PycObject::TYPE_COMPLEX:
        iprintf(pyc_output, indent, "(%s+%sj)\n", obj.cast<PycComplex>()->value(),
                                      obj.cast<PycComplex>()->imag());
        break;
    case PycObject::TYPE_BINARY_FLOAT:
        iprintf(pyc_output, indent, "%g\n", obj.cast<PycCFloat>()->value());
        break;
    case PycObject::TYPE_BINARY_COMPLEX:
        iprintf(pyc_output, indent, "(%g+%gj)\n", obj.cast<PycCComplex>()->value(),
                                      obj.cast<PycCComplex>()->imag());
        break;
    default:
        iprintf(pyc_output, indent, "<类型: %d>\n", obj->type());
    }

    out_seen.erase((PycObject *)obj);
}

int main(int argc, char* argv[])
{
    const char* infile = nullptr;      // 输入文件名
    bool marshalled = false;           // 是否处理已序列化的代码对象
    const char* version = nullptr;     // Python版本号
    unsigned disasm_flags = 0;         // 反汇编标志位
    std::ostream* pyc_output = &std::cout;  // 输出流，默认为标准输出
    std::ofstream out_file;            // 文件输出流

    // 解析命令行参数
    for (int arg = 1; arg < argc; ++arg) {
        if (strcmp(argv[arg], "-o") == 0) {
            // 指定输出文件
            if (arg + 1 < argc) {
                const char* filename = argv[++arg];
                out_file.open(filename, std::ios_base::out);
                if (out_file.fail()) {
                    fprintf(stderr, "错误: 无法打开文件 '%s' 进行写入\n",
                            filename);
                    return 1;
                }
                pyc_output = &out_file;
            } else {
                fputs("错误: 选项 '-o' 需要指定文件名\n", stderr);
                return 1;
            }
        } else if (strcmp(argv[arg], "-c") == 0) {
            // 指定处理已序列化的代码对象
            marshalled = true;
        } else if (strcmp(argv[arg], "-v") == 0) {
            // 指定Python版本
            if (arg + 1 < argc) {
                version = argv[++arg];
            } else {
                fputs("错误: 选项 '-v' 需要指定版本号\n", stderr);
                return 1;
            }
        } else if (strcmp(argv[arg], "--pycode-extra") == 0) {
            // 显示PyCode对象的额外字段
            disasm_flags |= Pyc::DISASM_PYCODE_VERBOSE;
        } else if (strcmp(argv[arg], "--show-caches") == 0) {
            // 在Python 3.11+反汇编中不隐藏CACHE指令
            disasm_flags |= Pyc::DISASM_SHOW_CACHES;
        } else if (strcmp(argv[arg], "--help") == 0 || strcmp(argv[arg], "-h") == 0) {
            // 显示帮助信息
            fprintf(stderr, "用法: %s [选项] 输入文件.pyc\n\n", argv[0]);
            fputs("选项:\n", stderr);
            fputs("  -o <文件名>       将输出写入到<文件名> (默认: 标准输出)\n", stderr);
            fputs("  -c                指定加载已编译的代码对象。需要设置版本号\n", stderr);
            fputs("  -v <x.y>          指定Python版本号用于加载已编译的代码对象\n", stderr);
            fputs("  --pycode-extra    在PyCode对象转储中显示额外字段\n", stderr);
            fputs("  --show-caches     在Python 3.11+反汇编中不隐藏CACHE指令\n", stderr);
            fputs("  --help            显示此帮助信息并退出\n", stderr);
            return 0;
        } else if (argv[arg][0] == '-') {
            fprintf(stderr, "错误: 无法识别的参数 %s\n", argv[arg]);
            return 1;
        } else {
            // 输入文件
            infile = argv[arg];
        }
    }

    // 检查是否指定了输入文件
    if (!infile) {
        fputs("错误: 未指定输入文件\n", stderr);
        return 1;
    }

    PycModule mod;
    if (!marshalled) {
        // 加载普通.pyc文件
        try {
            mod.loadFromFile(infile);
        } catch (std::exception &ex) {
            fprintf(stderr, "反汇编 %s 时出错: %s\n", infile, ex.what());
            return 1;
        }
    } else {
        // 加载已序列化的代码对象
        if (!version) {
            fputs("错误: 打开原始代码对象需要指定Python版本号\n", stderr);
            return 1;
        }
        std::string s(version);
        auto dot = s.find('.');
        if (dot == std::string::npos || dot == s.size()-1) {
            fputs("错误: 无法解析版本号字符串 (请使用 x.y 格式)\n", stderr);
            return 1;
        }
        int major = std::stoi(s.substr(0, dot));  // 主版本号
        int minor = std::stoi(s.substr(dot+1, s.size()));  // 次版本号
        mod.loadFromMarshalledFile(infile, major, minor);
    }
    
    // 提取文件名(不含路径)
    const char* dispname = strrchr(infile, PATHSEP);
    dispname = (dispname == NULL) ? infile : dispname + 1;
    
    // 输出文件头信息
    formatted_print(*pyc_output, "%s (Python %d.%d%s)\n", dispname,
                    mod.majorVer(), mod.minorVer(),
                    (mod.majorVer() < 3 && mod.isUnicode()) ? " -U" : "");
    
    // 执行反汇编
    try {
        output_object(mod.code().try_cast<PycObject>(), &mod, 0, disasm_flags,
                      *pyc_output);
    } catch (std::exception& ex) {
        fprintf(stderr, "反汇编 %s 时出错: %s\n", infile, ex.what());
        return 1;
    }

    return 0;
}