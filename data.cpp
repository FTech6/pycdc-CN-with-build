#include "data.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <windows.h>

// 添加编码转换函数
namespace {

std::string toUTF8(const std::string& str) {
    if (str.empty()) return str;
    
    // 检测是否为合法 UTF-8
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);
    if (length > 0) {
        return str; // 已经是 UTF-8
    }
    
    // 尝试 GB18030 转换
    int wlen = MultiByteToWideChar(54936, 0, str.c_str(), -1, nullptr, 0);
    if (wlen > 0) {
        wchar_t* wbuf = new wchar_t[wlen];
        if (MultiByteToWideChar(54936, 0, str.c_str(), -1, wbuf, wlen) > 0) {
            int utf8len = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, nullptr, 0, nullptr, nullptr);
            if (utf8len > 0) {
                char* utf8buf = new char[utf8len];
                if (WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, utf8buf, utf8len, nullptr, nullptr) > 0) {
                    std::string result(utf8buf);
                    delete[] utf8buf;
                    delete[] wbuf;
                    return result;
                }
                delete[] utf8buf;
            }
        }
        delete[] wbuf;
    }
    
    // 尝试 GBK
    wlen = MultiByteToWideChar(936, 0, str.c_str(), -1, nullptr, 0);
    if (wlen > 0) {
        wchar_t* wbuf = new wchar_t[wlen];
        if (MultiByteToWideChar(936, 0, str.c_str(), -1, wbuf, wlen) > 0) {
            int utf8len = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, nullptr, 0, nullptr, nullptr);
            if (utf8len > 0) {
                char* utf8buf = new char[utf8len];
                if (WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, utf8buf, utf8len, nullptr, nullptr) > 0) {
                    std::string result(utf8buf);
                    delete[] utf8buf;
                    delete[] wbuf;
                    return result;
                }
                delete[] utf8buf;
            }
        }
        delete[] wbuf;
    }
    
    // 回退到 Latin-1 保真转换
    std::string result;
    for (unsigned char c : str) {
        if (c <= 0x7F) {
            result += c;
        } else {
            result += static_cast<char>(0xC0 | (c >> 6));
            result += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return result;
}

} // namespace


/* PycData */
int PycData::get16()
{
    /* Ensure endianness */
    int result = getByte() & 0xFF;
    result |= (getByte() & 0xFF) << 8;
    return result;
}

int PycData::get32()
{
    /* Ensure endianness */
    int result = getByte() & 0xFF;
    result |= (getByte() & 0xFF) <<  8;
    result |= (getByte() & 0xFF) << 16;
    result |= (getByte() & 0xFF) << 24;
    return result;
}

Pyc_INT64 PycData::get64()
{
    /* Ensure endianness */
    Pyc_INT64 result = (Pyc_INT64)(getByte() & 0xFF);
    result |= (Pyc_INT64)(getByte() & 0xFF) <<  8;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 16;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 24;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 32;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 40;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 48;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 56;
    return result;
}


/* PycFile */
PycFile::PycFile(const char* filename)
{
    m_stream = fopen(filename, "rb");
}

bool PycFile::atEof() const
{
    int ch = fgetc(m_stream);
    ungetc(ch, m_stream);
    return (ch == EOF);
}

int PycFile::getByte()
{
    int ch = fgetc(m_stream);
    if (ch == EOF) {
        fputs("PycFile::getByte(): Unexpected end of stream\n", stderr);
        std::exit(1);
    }
    return ch;
}

void PycFile::getBuffer(int bytes, void* buffer)
{
    if (fread(buffer, 1, bytes, m_stream) != (size_t)bytes) {
        fputs("PycFile::getBuffer(): Unexpected end of stream\n", stderr);
        std::exit(1);
    }
}


/* PycBuffer */
int PycBuffer::getByte()
{
    if (atEof()) {
        fputs("PycBuffer::getByte(): Unexpected end of stream\n", stderr);
        std::exit(1);
    }
    int ch = (int)(*(m_buffer + m_pos));
    ++m_pos;
    return ch & 0xFF;   // Make sure it's just a byte!
}

void PycBuffer::getBuffer(int bytes, void* buffer)
{
    if (m_pos + bytes > m_size) {
        fputs("PycBuffer::getBuffer(): Unexpected end of stream\n", stderr);
        std::exit(1);
    }
    if (bytes != 0)
        memcpy(buffer, (m_buffer + m_pos), bytes);
    m_pos += bytes;
}

int formatted_print(std::ostream& stream, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = formatted_printv(stream, format, args);
    va_end(args);
    return result;
}

int formatted_printv(std::ostream& stream, const char* format, va_list args)
{
    char buffer[4096];
    int result = vsnprintf(buffer, sizeof(buffer), format, args);
    
    if (result >= 0) {
        // 对输出进行编码转换
        std::string converted = toUTF8(buffer);
        stream << converted;
    } else {
        // 如果格式化失败，直接输出原始格式字符串（进行编码转换）
        stream << toUTF8(format);
    }
    
    return result;
}