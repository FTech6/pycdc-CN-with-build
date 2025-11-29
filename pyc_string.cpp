#include "pyc_string.h"
#include "pyc_module.h"
#include "data.h"
#include <stdexcept>

// 检查字符串是否为ASCII编码
static bool check_ascii(const std::string& data)
{
    auto cp = reinterpret_cast<const unsigned char*>(data.c_str());
    while (*cp) {
        if (*cp & 0x80)  // 检查最高位，如果为1则表示非ASCII字符
            return false;
        ++cp;
    }
    return true;
}

/* PycString - Python字符串对象 */
void PycString::load(PycData* stream, PycModule* mod)
{
    if (type() == TYPE_STRINGREF) {
        // 处理字符串引用：从intern池中获取已缓存的字符串
        PycRef<PycString> str = mod->getIntern(stream->get32());
        m_type = str->m_type;
        m_value = str->m_value;
    } else {
        int length;
        // 根据字符串类型确定长度字段大小
        if (type() == TYPE_SHORT_ASCII || type() == TYPE_SHORT_ASCII_INTERNED)
            length = stream->getByte();  // 短字符串使用1字节长度
        else
            length = stream->get32();    // 普通字符串使用4字节长度

        if (length < 0)
            throw std::bad_alloc();  // 长度不能为负数

        m_value.resize(length);
        if (length) {
            // 从数据流中读取字符串内容
            stream->getBuffer(length, &m_value.front());
            // 检查ASCII字符串类型的有效性
            if (type() == TYPE_ASCII || type() == TYPE_ASCII_INTERNED ||
                    type() == TYPE_SHORT_ASCII || type() == TYPE_SHORT_ASCII_INTERNED) {
                if (!check_ascii(m_value))
                    throw std::runtime_error("ASCII字符串中包含无效字节");
            }
        }

        // 如果是interned字符串，将其添加到模块的intern池中
        if (type() == TYPE_INTERNED || type() == TYPE_ASCII_INTERNED ||
                type() == TYPE_SHORT_ASCII_INTERNED)
            mod->intern(this);
    }
}

// 比较字符串对象是否相等
bool PycString::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj.type())
        return false;

    PycRef<PycString> strObj = obj.cast<PycString>();
    return isEqual(strObj->m_value);
}

// 输出字符串到流中
void PycString::print(std::ostream &pyc_output, PycModule* mod, bool triple,
                      const char* parent_f_string_quote)
{
    char prefix = 0;  // 字符串前缀：'b'表示字节串，'u'表示Unicode字符串
    switch (type()) {
    case TYPE_STRING:
        prefix = mod->strIsUnicode() ? 'b' : 0;  // 根据模块设置决定前缀
        break;
    case PycObject::TYPE_UNICODE:
        prefix = mod->strIsUnicode() ? 0 : 'u';  // Unicode字符串可能需要'u'前缀
        break;
    case PycObject::TYPE_INTERNED:
        prefix = mod->internIsBytes() ? 'b' : 0;  // 根据intern类型决定前缀
        break;
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        // 这些类型在Python 3.4之后才存在，不需要前缀
        prefix = 0;
        break;
    default:
        throw std::runtime_error("无效的字符串类型");
    }

    // 输出字符串前缀
    if (prefix != 0)
        pyc_output << prefix;

    // 处理空字符串
    if (m_value.empty()) {
        pyc_output << "''";
        return;
    }

    // 确定首选的引号风格（模拟Python的方法）
    bool useQuotes = false;  // true使用双引号，false使用单引号
    if (!parent_f_string_quote) {
        // 如果没有父f-string引号，根据内容选择引号风格
        for (char ch : m_value) {
            if (ch == '\'') {
                useQuotes = true;  // 如果包含单引号，优先使用双引号
            } else if (ch == '"') {
                useQuotes = false;  // 如果包含双引号，使用单引号
                break;
            }
        }
    } else {
        // 如果有父f-string引号，使用相反的引号风格
        useQuotes = parent_f_string_quote[0] == '"';
    }

    // 输出字符串开始引号
    if (!parent_f_string_quote) {
        if (triple)
            pyc_output << (useQuotes ? R"(""")" : "'''");  // 三引号字符串
        else
            pyc_output << (useQuotes ? '"' : '\'');        // 单引号字符串
    }
    
    // 输出字符串内容，处理转义字符
    for (char ch : m_value) {
        if (static_cast<unsigned char>(ch) < 0x20 || ch == 0x7F) {
            // 处理控制字符
            if (ch == '\r') {
                pyc_output << "\\r";  // 回车符
            } else if (ch == '\n') {
                if (triple)
                    pyc_output << '\n';  // 三引号字符串中保留换行
                else
                    pyc_output << "\\n";  // 普通字符串中转义换行
            } else if (ch == '\t') {
                pyc_output << "\\t";  // 制表符
            } else {
                // 其他控制字符使用十六进制转义
                formatted_print(pyc_output, "\\x%02x", (ch & 0xFF));
            }
        } else if (static_cast<unsigned char>(ch) >= 0x80) {
            // 处理非ASCII字符
            if (type() == TYPE_UNICODE) {
                // Unicode字符串以UTF-8存储，直接输出字符
                pyc_output << ch;
            } else {
                // 字节字符串中的非ASCII字符使用十六进制转义
                formatted_print(pyc_output, "\\x%02x", (ch & 0xFF));
            }
        } else {
            // 处理普通ASCII字符
            if (!useQuotes && ch == '\'')
                pyc_output << R"(\')";  // 转义单引号
            else if (useQuotes && ch == '"')
                pyc_output << R"(\")";  // 转义双引号
            else if (ch == '\\')
                pyc_output << R"(\\)";  // 转义反斜杠
            else if (parent_f_string_quote && ch == '{')
                pyc_output << "{{";     // f-string中的双大括号转义
            else if (parent_f_string_quote && ch == '}')
                pyc_output << "}}";     // f-string中的双大括号转义
            else
                pyc_output << ch;       // 普通字符直接输出
        }
    }
    
    // 输出字符串结束引号
    if (!parent_f_string_quote) {
        if (triple)
            pyc_output << (useQuotes ? R"(""")" : "'''");  // 三引号结束
        else
            pyc_output << (useQuotes ? '"' : '\'');        // 单引号结束
    }
}