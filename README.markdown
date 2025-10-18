# Decompyle++ 汉化版(带有适用于 Windows 平台的可执行的已编译版本的程序(*.exe))
**Python 字节码反汇编器/反编译器**

本仓库提供 Decompyle++ (pycdc) 项目的完整汉化版本，包含源代码汉化和预编译的可执行文件。
本仓库同时对字符串的输出逻辑进行了调整，并将整个项目修改为 UTF-8 编码，也对终端输出进行了 UTF-8 的适配与优化

大部分的注释与输出的汉化工作由于该项目代码量巨大，难以快速进行查找和翻译，故使用了AI工具对大部分的注释与输出进行了汉化，若有问题，请提出 issue

**汉化及优化工作者**：@FlameTech6

---

# 项目简介

Decompyle++ 旨在将编译后的 Python 字节码转换回有效且人类可读的 Python 源代码。虽然其他项目在此领域取得了不同程度的成功，但 Decompyle++ 的独特之处在于它致力于支持所有 Python 版本的字节码。

Decompyle++ 包含两个主要工具：
- **pycdas**: Python 字节码反汇编器
- **pycdc**: Python 字节码反编译器

正如其名所示，Decompyle++ 使用 C++ 编写。

## 功能特性
- 支持 Python 2.4 到最新版本的所有字节码格式
- 提供完整的反汇编和反编译功能
- 支持序列化代码对象处理
- 完全中文化的错误信息和输出

---

# 快速开始

## 下载预编译版本
对于 Windows 用户，我们提供了预编译的可执行文件：
1. 前往 [Release 页面](https://github.com/FTech6/pycdc-CN-with-build/releases)
2. 下载最新版本的 `pycdc.exe` 和 `pycdas.exe`

## 从源代码构建

### 系统要求
- CMake 3.12 或更高版本
- 支持 C++17 的编译器
- Python 3.x (仅测试需要)

### 构建步骤
1. 克隆本仓库：
```bash
git clone https://github.com/FTech6/pycdc-CN-with-build.git
cd pycdc-CN-with-build
```

2. 使用 CMake 生成构建文件：
```bash
cmake -B build
```

3. 编译项目：
```bash
cmake --build build
```

### 调试选项
可以通过以下 CMake 选项启用调试功能：

| 选项 | 描述 |
|------|------|
| `-DCMAKE_BUILD_TYPE=Debug` | 生成调试符号 |
| `-DENABLE_BLOCK_DEBUG=ON` | 启用代码块调试输出 |
| `-DENABLE_STACK_DEBUG=ON` | 启用堆栈调试输出 |

### 运行测试
在 Linux 或 MSYS 环境中，可以运行：
```bash
cd build
make check JOBS=4
```
使用 `FILTER=xxxx` 可以只运行特定的测试用例。

---

# 使用方法

## 基本命令

**使用 pycdas（反汇编器）**：
```bash
./pycdas [PYC文件路径]
```
字节码反汇编结果将输出到标准输出。

**使用 pycdc（反编译器）**：
```bash
./pycdc [PYC文件路径]
```
反编译后的 Python 源代码将输出到标准输出，任何错误信息将输出到标准错误。

## 序列化代码对象支持

两个工具都支持 Python 序列化的代码对象（即 `marshal.dumps(compile(...))` 的输出）。

要使用此功能，必须指定 Python 版本：
```bash
./pycdc -c -v <版本号> [文件路径]
```

示例：
```bash
./pycdc -c -v 3.8 codeobj.bin
```

---

# 命令行参数

## 通用参数

| 选项 | 参数 | 适用工具 | 说明 | 示例 |
|------|------|----------|------|------|
| `-h`, `--help` | 无 | pycdas/pycdc | 显示帮助信息 | `./pycdc -h` |
| `-v` | `<x.y>` | pycdas/pycdc | 指定Python版本 | `./pycdc -v 3.8` |
| `-c` | 无 | pycdas/pycdc | 处理序列化代码对象 | `./pycdc -c -v 2.7` |
| `-o` | `<文件路径>` | pycdas/pycdc | 指定输出文件 | `./pycdc -o output.py` |

## pycdc 专用参数

| 选项 | 说明 |
|------|------|
| `--pycode-extra` | 在 PyCode 对象转储中显示额外字段 |
| `--show-caches` | 在 Python 3.11+ 反汇编中不隐藏 CACHE 指令 |

---

# 项目信息

## 开发团队

**核心开发者**：
- Michael Hansen
- Darryl Pogue

**其他贡献者**：
- charlietang98
- Kunal Parmar
- Olivier Iffrig
- Zlodiy

**汉化维护**：
- @FlameTech6

## 许可证

本项目基于 GNU 通用公共许可证第 3 版发布。  
详见 [LICENSE](LICENSE) 文件。

## 问题反馈

如果您在使用过程中遇到任何问题或有改进建议，请通过以下方式反馈：
1. 在 GitHub 仓库中提交 Issue
2. 通过邮件联系汉化及优化工作者

---

**感谢使用 Decompyle++ 汉化版！**
