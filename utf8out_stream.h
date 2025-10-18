#ifndef UTF8OUT_STREAM_H
#define UTF8OUT_STREAM_H

#include <iostream>
#include <string>
#include <windows.h>

// Windows 平台的 UTF-8 输出流包装器
class utf8out_stream : public std::ostream {
private:
    class utf8_streambuf : public std::streambuf {
    private:
        std::ostream& m_os;
        std::string m_buffer;
        
        std::string toUTF8(const std::string& str) {
            if (str.empty()) return str;
            
            // 检测是否为合法 UTF-8
            int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);
            if (length > 0) {
                return str;
            }
            
            // 尝试 GB18030
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
        
    protected:
        virtual int overflow(int c) override {
            if (c != EOF) {
                m_buffer += static_cast<char>(c);
                if (c == '\n' || m_buffer.size() >= 1024) {
                    // 缓冲区满或遇到换行，进行编码转换并输出
                    m_os << toUTF8(m_buffer);
                    m_buffer.clear();
                }
            }
            return c;
        }
        
        virtual int sync() override {
            if (!m_buffer.empty()) {
                m_os << toUTF8(m_buffer);
                m_buffer.clear();
            }
            return m_os.rdbuf()->pubsync();
        }
        
    public:
        utf8_streambuf(std::ostream& os) : m_os(os) {}
    };
    
    utf8_streambuf m_buf;

public:
    utf8out_stream(std::ostream& os) : std::ostream(&m_buf), m_buf(os) {}
    
    // 重载 operator<< 以确保字符串参数经过编码转换
    utf8out_stream& operator<<(const char* str) {
        if (str) {
            std::ostream::operator<<(str);
        }
        return *this;
    }
    
    utf8out_stream& operator<<(const std::string& str) {
        std::ostream::operator<<(str.c_str());
        return *this;
    }
};

#endif // UTF8OUT_STREAM_H