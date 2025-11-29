#ifndef _PYC_ASTNODE_H
#define _PYC_ASTNODE_H

#include "pyc_module.h"
#include <list>
#include <deque>

/* 与 PycObject 类似的接口，因此 PycRef 可以在其上工作... *
 * 但这并*不*意味着两者可以互换！ */
class ASTNode {
public:
    // AST节点类型枚举
    enum Type {
        NODE_INVALID,       // 无效节点
        NODE_NODELIST,      // 节点列表
        NODE_OBJECT,        // 对象节点
        NODE_UNARY,         // 一元操作节点
        NODE_BINARY,        // 二元操作节点
        NODE_COMPARE,       // 比较操作节点
        NODE_SLICE,         // 切片操作节点
        NODE_STORE,         // 存储操作节点
        NODE_RETURN,        // 返回语句节点
        NODE_NAME,          // 名称节点
        NODE_DELETE,        // 删除操作节点
        NODE_FUNCTION,      // 函数定义节点
        NODE_CLASS,         // 类定义节点
        NODE_CALL,          // 函数调用节点
        NODE_IMPORT,        // 导入语句节点
        NODE_TUPLE,         // 元组节点
        NODE_LIST,          // 列表节点
        NODE_SET,           // 集合节点
        NODE_MAP,           // 映射节点
        NODE_SUBSCR,        // 下标操作节点
        NODE_PRINT,         // 打印语句节点
        NODE_CONVERT,       // 转换操作节点
        NODE_KEYWORD,       // 关键字节点
        NODE_RAISE,         // 抛出异常节点
        NODE_EXEC,          // 执行语句节点
        NODE_BLOCK,         // 代码块节点
        NODE_COMPREHENSION, // 推导式节点
        NODE_LOADBUILDCLASS,// 加载构建类节点
        NODE_AWAITABLE,     // 可等待对象节点
        NODE_FORMATTEDVALUE,// 格式化值节点（f-string）
        NODE_JOINEDSTR,     // 连接字符串节点
        NODE_CONST_MAP,     // 常量映射节点
        NODE_ANNOTATED_VAR, // 注解变量节点
        NODE_CHAINSTORE,    // 链式存储节点
        NODE_TERNARY,       // 三元操作节点
        NODE_KW_NAMES_MAP,  // 关键字名称映射节点

        // 空节点类型
        NODE_LOCALS,        // 本地变量节点
    };

    ASTNode(int type = NODE_INVALID) : m_refs(), m_type(type), m_processed() { }
    virtual ~ASTNode() { }

    int type() const { return internalGetType(this); }

    bool processed() const { return m_processed; }    // 是否已处理
    void setProcessed() { m_processed = true; }       // 标记为已处理

private:
    int m_refs;         // 引用计数
    int m_type;         // 节点类型
    bool m_processed;   // 处理标志

    // 内部获取类型的方法（用于兼容clang）
    static int internalGetType(const ASTNode *node)
    {
        return node ? node->m_type : NODE_INVALID;
    }

    // 内部增加引用计数
    static void internalAddRef(ASTNode *node)
    {
        if (node)
            ++node->m_refs;
    }

    // 内部减少引用计数
    static void internalDelRef(ASTNode *node)
    {
        if (node && --node->m_refs == 0)
            delete node;
    }

public:
    void addRef() { internalAddRef(this); }    // 增加引用计数
    void delRef() { internalDelRef(this); }    // 减少引用计数
};


// AST节点列表类
class ASTNodeList : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> list_t;

    ASTNodeList(list_t nodes)
        : ASTNode(NODE_NODELIST), m_nodes(std::move(nodes)) { }

    const list_t& nodes() const { return m_nodes; }
    void removeFirst();     // 移除第一个节点
    void removeLast();      // 移除最后一个节点
    void append(PycRef<ASTNode> node) { m_nodes.emplace_back(std::move(node)); }  // 添加节点

protected:
    ASTNodeList(list_t nodes, ASTNode::Type type)
        : ASTNode(type), m_nodes(std::move(nodes)) { }

private:
    list_t m_nodes;  // 节点列表
};


// 链式存储节点类（用于连续赋值）
class ASTChainStore : public ASTNodeList {
public:
    ASTChainStore(list_t nodes, PycRef<ASTNode> src)
        : ASTNodeList(nodes, NODE_CHAINSTORE), m_src(std::move(src)) { }
    
    PycRef<ASTNode> src() const { return m_src; }  // 获取源节点

private:
    PycRef<ASTNode> m_src;  // 源节点
};


// Python对象包装节点类
class ASTObject : public ASTNode {
public:
    ASTObject(PycRef<PycObject> obj)
        : ASTNode(NODE_OBJECT), m_obj(std::move(obj)) { }

    PycRef<PycObject> object() const { return m_obj; }  // 获取Python对象

private:
    PycRef<PycObject> m_obj;  // Python对象引用
};


// 一元操作节点类
class ASTUnary : public ASTNode {
public:
    // 一元操作符枚举
    enum UnOp {
        UN_POSITIVE,   // 正号 +
        UN_NEGATIVE,   // 负号 -
        UN_INVERT,     // 按位取反 ~
        UN_NOT         // 逻辑非 not
    };

    ASTUnary(PycRef<ASTNode> operand, int op)
        : ASTNode(NODE_UNARY), m_op(op), m_operand(std::move(operand)) { }

    PycRef<ASTNode> operand() const { return m_operand; }  // 获取操作数
    int op() const { return m_op; }                        // 获取操作符
    virtual const char* op_str() const;                    // 获取操作符字符串表示

protected:
    int m_op;  // 操作符

private:
    PycRef<ASTNode> m_operand;  // 操作数节点
};


// 二元操作节点类
class ASTBinary : public ASTNode {
public:
    // 二元操作符枚举
    enum BinOp {
        BIN_ATTR,           // 属性访问 .
        BIN_POWER,          // 幂运算 **
        BIN_MULTIPLY,       // 乘法 *
        BIN_DIVIDE,         // 除法 /
        BIN_FLOOR_DIVIDE,   // 整除 //
        BIN_MODULO,         // 取模 %
        BIN_ADD,            // 加法 +
        BIN_SUBTRACT,       // 减法 -
        BIN_LSHIFT,         // 左移位 <<
        BIN_RSHIFT,         // 右移位 >>
        BIN_AND,            // 按位与 &
        BIN_XOR,            // 按位异或 ^
        BIN_OR,             // 按位或 |
        BIN_LOG_AND,        // 逻辑与 and
        BIN_LOG_OR,         // 逻辑或 or
        BIN_MAT_MULTIPLY,   // 矩阵乘法 @
        
        /* 原地操作 */
        BIN_IP_ADD,              // 原地加法 +=
        BIN_IP_SUBTRACT,         // 原地减法 -=
        BIN_IP_MULTIPLY,         // 原地乘法 *=
        BIN_IP_DIVIDE,           // 原地除法 /=
        BIN_IP_MODULO,           // 原地取模 %=
        BIN_IP_POWER,            // 原地幂运算 **=
        BIN_IP_LSHIFT,           // 原地左移位 <<=
        BIN_IP_RSHIFT,           // 原地右移位 >>=
        BIN_IP_AND,              // 原地按位与 &=
        BIN_IP_XOR,              // 原地按位异或 ^=
        BIN_IP_OR,               // 原地按位或 |=
        BIN_IP_FLOOR_DIVIDE,     // 原地整除 //=
        BIN_IP_MAT_MULTIPLY,     // 原地矩阵乘法 @=
        
        /* 错误情况 */
        BIN_INVALID         // 无效操作
    };

    ASTBinary(PycRef<ASTNode> left, PycRef<ASTNode> right, int op,
              int type = NODE_BINARY)
        : ASTNode(type), m_op(op), m_left(std::move(left)), m_right(std::move(right)) { }

    PycRef<ASTNode> left() const { return m_left; }        // 获取左操作数
    PycRef<ASTNode> right() const { return m_right; }      // 获取右操作数
    int op() const { return m_op; }                        // 获取操作符
    bool is_inplace() const { return m_op >= BIN_IP_ADD; } // 是否为原地操作
    virtual const char* op_str() const;                    // 获取操作符字符串表示

    static BinOp from_opcode(int opcode);                  // 从操作码转换
    static BinOp from_binary_op(int operand);              // 从操作数转换

protected:
    int m_op;  // 操作符

private:
    PycRef<ASTNode> m_left;   // 左操作数
    PycRef<ASTNode> m_right;  // 右操作数
};


// 比较操作节点类
class ASTCompare : public ASTBinary {
public:
    // 比较操作符枚举
    enum CompareOp {
        CMP_LESS,           // 小于 <
        CMP_LESS_EQUAL,     // 小于等于 <=
        CMP_EQUAL,          // 等于 ==
        CMP_NOT_EQUAL,      // 不等于 !=
        CMP_GREATER,        // 大于 >
        CMP_GREATER_EQUAL,  // 大于等于 >=
        CMP_IN,             // 包含 in
        CMP_NOT_IN,         // 不包含 not in
        CMP_IS,             // 是 is
        CMP_IS_NOT,         // 不是 is not
        CMP_EXCEPTION,      // 异常匹配
        CMP_BAD             // 错误的比较
    };

    ASTCompare(PycRef<ASTNode> left, PycRef<ASTNode> right, int op)
        : ASTBinary(std::move(left), std::move(right), op, NODE_COMPARE) { }

    const char* op_str() const override;  // 重写操作符字符串表示
};


// 切片操作节点类
class ASTSlice : public ASTBinary {
public:
    // 切片操作类型枚举
    enum SliceOp {
        SLICE0,  // 简单切片 [:]
        SLICE1,  // 起始切片 [start:]
        SLICE2,  // 结束切片 [:end]
        SLICE3   // 范围切片 [start:end]
    };

    ASTSlice(int op, PycRef<ASTNode> left = {}, PycRef<ASTNode> right = {})
        : ASTBinary(std::move(left), std::move(right), op, NODE_SLICE) { }
};


// 存储操作节点类
class ASTStore : public ASTNode {
public:
    ASTStore(PycRef<ASTNode> src, PycRef<ASTNode> dest)
        : ASTNode(NODE_STORE), m_src(std::move(src)), m_dest(std::move(dest)) { }

    PycRef<ASTNode> src() const { return m_src; }   // 获取源节点
    PycRef<ASTNode> dest() const { return m_dest; } // 获取目标节点

private:
    PycRef<ASTNode> m_src;   // 源节点
    PycRef<ASTNode> m_dest;  // 目标节点
};


// 返回语句节点类
class ASTReturn : public ASTNode {
public:
    // 返回类型枚举
    enum RetType {
        RETURN,      // 普通返回
        YIELD,       // 生成器 yield
        YIELD_FROM   // 生成器 yield from
    };

    ASTReturn(PycRef<ASTNode> value, RetType rettype = RETURN)
        : ASTNode(NODE_RETURN), m_value(std::move(value)), m_rettype(rettype) { }

    PycRef<ASTNode> value() const { return m_value; }    // 获取返回值
    RetType rettype() const { return m_rettype; }        // 获取返回类型

private:
    PycRef<ASTNode> m_value;    // 返回值节点
    RetType m_rettype;          // 返回类型
};


// 名称节点类
class ASTName : public ASTNode {
public:
    ASTName(PycRef<PycString> name)
        : ASTNode(NODE_NAME), m_name(std::move(name)) { }

    PycRef<PycString> name() const { return m_name; }  // 获取名称

private:
    PycRef<PycString> m_name;  // 名称字符串
};


// 删除操作节点类
class ASTDelete : public ASTNode {
public:
    ASTDelete(PycRef<ASTNode> value)
        : ASTNode(NODE_DELETE), m_value(std::move(value)) { }

    PycRef<ASTNode> value() const { return m_value; }  // 获取要删除的值

private:
    PycRef<ASTNode> m_value;  // 要删除的值节点
};


// 函数定义节点类
class ASTFunction : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> defarg_t;

    ASTFunction(PycRef<ASTNode> code, defarg_t defArgs, defarg_t kwDefArgs)
        : ASTNode(NODE_FUNCTION), m_code(std::move(code)),
          m_defargs(std::move(defArgs)), m_kwdefargs(std::move(kwDefArgs)) { }

    PycRef<ASTNode> code() const { return m_code; }                // 获取函数代码
    const defarg_t& defargs() const { return m_defargs; }          // 获取默认参数
    const defarg_t& kwdefargs() const { return m_kwdefargs; }      // 获取关键字默认参数

private:
    PycRef<ASTNode> m_code;        // 函数代码节点
    defarg_t m_defargs;            // 默认参数列表
    defarg_t m_kwdefargs;          // 关键字默认参数列表
};


// 类定义节点类
class ASTClass : public ASTNode {
public:
    ASTClass(PycRef<ASTNode> code, PycRef<ASTNode> bases, PycRef<ASTNode> name)
        : ASTNode(NODE_CLASS), m_code(std::move(code)), m_bases(std::move(bases)),
          m_name(std::move(name)) { }

    PycRef<ASTNode> code() const { return m_code; }   // 获取类代码
    PycRef<ASTNode> bases() const { return m_bases; } // 获取基类列表
    PycRef<ASTNode> name() const { return m_name; }   // 获取类名

private:
    PycRef<ASTNode> m_code;   // 类代码节点
    PycRef<ASTNode> m_bases;  // 基类列表节点
    PycRef<ASTNode> m_name;   // 类名节点
};


// 函数调用节点类
class ASTCall : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> pparam_t;  // 位置参数类型
    typedef std::list<std::pair<PycRef<ASTNode>, PycRef<ASTNode>>> kwparam_t;  // 关键字参数类型

    ASTCall(PycRef<ASTNode> func, pparam_t pparams, kwparam_t kwparams)
        : ASTNode(NODE_CALL), m_func(std::move(func)), m_pparams(std::move(pparams)),
          m_kwparams(std::move(kwparams)) { }

    PycRef<ASTNode> func() const { return m_func; }              // 获取被调用函数
    const pparam_t& pparams() const { return m_pparams; }        // 获取位置参数
    const kwparam_t& kwparams() const { return m_kwparams; }     // 获取关键字参数
    PycRef<ASTNode> var() const { return m_var; }                // 获取可变位置参数
    PycRef<ASTNode> kw() const { return m_kw; }                  // 获取可变关键字参数

    bool hasVar() const { return m_var != nullptr; }             // 是否有可变位置参数
    bool hasKW() const { return m_kw != nullptr; }               // 是否有可变关键字参数

    void setVar(PycRef<ASTNode> var) { m_var = std::move(var); } // 设置可变位置参数
    void setKW(PycRef<ASTNode> kw) { m_kw = std::move(kw); }     // 设置可变关键字参数

private:
    PycRef<ASTNode> m_func;      // 被调用函数节点
    pparam_t m_pparams;          // 位置参数列表
    kwparam_t m_kwparams;        // 关键字参数列表
    PycRef<ASTNode> m_var;       // 可变位置参数（*args）
    PycRef<ASTNode> m_kw;        // 可变关键字参数（**kwargs）
};


// 导入语句节点类
class ASTImport : public ASTNode {
public:
    typedef std::list<PycRef<ASTStore>> list_t;

    ASTImport(PycRef<ASTNode> name, PycRef<ASTNode> fromlist)
        : ASTNode(NODE_IMPORT), m_name(std::move(name)), m_fromlist(std::move(fromlist)) { }

    PycRef<ASTNode> name() const { return m_name; }           // 获取导入模块名
    list_t stores() const { return m_stores; }                // 获取存储列表
    void add_store(PycRef<ASTStore> store) { m_stores.emplace_back(std::move(store)); }  // 添加存储

    PycRef<ASTNode> fromlist() const { return m_fromlist; }   // 获取from导入列表

private:
    PycRef<ASTNode> m_name;      // 模块名节点
    list_t m_stores;             // 存储列表

    PycRef<ASTNode> m_fromlist;  // from导入列表节点
};


// 元组节点类
class ASTTuple : public ASTNode {
public:
    typedef std::vector<PycRef<ASTNode>> value_t;

    ASTTuple(value_t values)
        : ASTNode(NODE_TUPLE), m_values(std::move(values)),
          m_requireParens(true) { }

    const value_t& values() const { return m_values; }           // 获取元组值列表
    void add(PycRef<ASTNode> name) { m_values.emplace_back(std::move(name)); }  // 添加元素

    void setRequireParens(bool require) { m_requireParens = require; }  // 设置是否需要括号
    bool requireParens() const { return m_requireParens; }              // 是否需要括号

private:
    value_t m_values;         // 值列表
    bool m_requireParens;     // 是否需要括号标志
};


// 列表节点类
class ASTList : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> value_t;

    ASTList(value_t values)
        : ASTNode(NODE_LIST), m_values(std::move(values)) { }

    const value_t& values() const { return m_values; }  // 获取列表值

private:
    value_t m_values;  // 值列表
};

// 集合节点类
class ASTSet : public ASTNode {
public:
    typedef std::deque<PycRef<ASTNode>> value_t;

    ASTSet(value_t values)
        : ASTNode(NODE_SET), m_values(std::move(values)) { }

    const value_t& values() const { return m_values; }  // 获取集合值

private:
    value_t m_values;  // 值队列
};

// 映射节点类（字典）
class ASTMap : public ASTNode {
public:
    typedef std::list<std::pair<PycRef<ASTNode>, PycRef<ASTNode>>> map_t;

    ASTMap() : ASTNode(NODE_MAP) { }

    void add(PycRef<ASTNode> key, PycRef<ASTNode> value)  // 添加键值对
    {
        m_values.emplace_back(std::move(key), std::move(value));
    }

    const map_t& values() const { return m_values; }  // 获取映射键值对

private:
    map_t m_values;  // 键值对列表
};

// 关键字名称映射节点类
class ASTKwNamesMap : public ASTNode {
public:
    typedef std::list<std::pair<PycRef<ASTNode>, PycRef<ASTNode>>> map_t;

    ASTKwNamesMap() : ASTNode(NODE_KW_NAMES_MAP) { }

    void add(PycRef<ASTNode> key, PycRef<ASTNode> value)  // 添加键值对
    {
        m_values.emplace_back(std::move(key), std::move(value));
    }

    const map_t& values() const { return m_values; }  // 获取关键字名称映射

private:
    map_t m_values;  // 键值对列表
};

// 常量映射节点类
class ASTConstMap : public ASTNode {
public:
    typedef std::vector<PycRef<ASTNode>> values_t;

    ASTConstMap(PycRef<ASTNode> keys, const values_t& values)
        : ASTNode(NODE_CONST_MAP), m_keys(std::move(keys)), m_values(std::move(values)) { }

    const PycRef<ASTNode>& keys() const { return m_keys; }    // 获取键列表
    const values_t& values() const { return m_values; }       // 获取值列表

private:
    PycRef<ASTNode> m_keys;    // 键节点
    values_t m_values;         // 值列表
};


// 下标操作节点类
class ASTSubscr : public ASTNode {
public:
    ASTSubscr(PycRef<ASTNode> name, PycRef<ASTNode> key)
        : ASTNode(NODE_SUBSCR), m_name(std::move(name)), m_key(std::move(key)) { }

    PycRef<ASTNode> name() const { return m_name; }  // 获取名称节点
    PycRef<ASTNode> key() const { return m_key; }    // 获取键节点

private:
    PycRef<ASTNode> m_name;  // 名称节点（被索引的对象）
    PycRef<ASTNode> m_key;   // 键节点（索引值）
};


// 打印语句节点类
class ASTPrint : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> values_t;

    ASTPrint(PycRef<ASTNode> value, PycRef<ASTNode> stream = {})
        : ASTNode(NODE_PRINT), m_stream(std::move(stream)), m_eol()
    {
        if (value != nullptr)
            m_values.emplace_back(std::move(value));
        else
            m_eol = true;  // 只有换行
    }

    values_t values() const { return m_values; }         // 获取打印值列表
    PycRef<ASTNode> stream() const { return m_stream; }  // 获取输出流
    bool eol() const { return m_eol; }                   // 是否以换行结束

    void add(PycRef<ASTNode> value) { m_values.emplace_back(std::move(value)); }  // 添加打印值
    void setEol(bool eol) { m_eol = eol; }                                       // 设置换行标志

private:
    values_t m_values;        // 打印值列表
    PycRef<ASTNode> m_stream; // 输出流节点
    bool m_eol;               // 换行结束标志
};


// 转换操作节点类
class ASTConvert : public ASTNode {
public:
    ASTConvert(PycRef<ASTNode> name)
        : ASTNode(NODE_CONVERT), m_name(std::move(name)) { }

    PycRef<ASTNode> name() const { return m_name; }  // 获取转换名称

private:
    PycRef<ASTNode> m_name;  // 转换名称节点
};


// 关键字节点类
class ASTKeyword : public ASTNode {
public:
    enum Word {
        KW_PASS,        // pass 关键字
        KW_BREAK,       // break 关键字  
        KW_CONTINUE     // continue 关键字
    };

    ASTKeyword(Word key) : ASTNode(NODE_KEYWORD), m_key(key) { }

    Word key() const { return m_key; }           // 获取关键字类型
    const char* word_str() const;                // 获取关键字字符串表示

private:
    Word m_key;  // 关键字类型
};


// 抛出异常节点类
class ASTRaise : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> param_t;

    ASTRaise(param_t params) : ASTNode(NODE_RAISE), m_params(std::move(params)) { }

    const param_t& params() const { return m_params; }  // 获取异常参数

private:
    param_t m_params;  // 异常参数列表
};


// 执行语句节点类
class ASTExec : public ASTNode {
public:
    ASTExec(PycRef<ASTNode> stmt, PycRef<ASTNode> glob, PycRef<ASTNode> loc)
        : ASTNode(NODE_EXEC), m_stmt(std::move(stmt)), m_glob(std::move(glob)),
          m_loc(std::move(loc)) { }

    PycRef<ASTNode> statement() const { return m_stmt; }  // 获取执行语句
    PycRef<ASTNode> globals() const { return m_glob; }    // 获取全局变量
    PycRef<ASTNode> locals() const { return m_loc; }      // 获取局部变量

private:
    PycRef<ASTNode> m_stmt;  // 执行语句节点
    PycRef<ASTNode> m_glob;  // 全局变量节点
    PycRef<ASTNode> m_loc;   // 局部变量节点
};


// 代码块节点类
class ASTBlock : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> list_t;

    // 代码块类型枚举
    enum BlkType {
        BLK_MAIN,        // 主块
        BLK_IF,          // if 块
        BLK_ELSE,        // else 块  
        BLK_ELIF,        // elif 块
        BLK_TRY,         // try 块
        BLK_CONTAINER,   // 容器块
        BLK_EXCEPT,      // except 块
        BLK_FINALLY,     // finally 块
        BLK_WHILE,       // while 块
        BLK_FOR,         // for 块
        BLK_WITH,        // with 块
        BLK_ASYNCFOR     // 异步 for 块
    };

    ASTBlock(BlkType blktype, int end = 0, int inited = 0)
        : ASTNode(NODE_BLOCK), m_blktype(blktype), m_end(end), m_inited(inited) { }

    BlkType blktype() const { return m_blktype; }           // 获取块类型
    int end() const { return m_end; }                       // 获取结束位置
    const list_t& nodes() const { return m_nodes; }         // 获取节点列表
    list_t::size_type size() const { return m_nodes.size(); }  // 获取节点数量
    void removeFirst();        // 移除第一个节点
    void removeLast();         // 移除最后一个节点
    void append(PycRef<ASTNode> node) { m_nodes.emplace_back(std::move(node)); }  // 添加节点
    const char* type_str() const;  // 获取类型字符串表示

    virtual int inited() const { return m_inited; }         // 是否已初始化
    virtual void init() { m_inited = 1; }                   // 初始化
    virtual void init(int init) { m_inited = init; }        // 带参数的初始化

    void setEnd(int end) { m_end = end; }                   // 设置结束位置

private:
    BlkType m_blktype;  // 块类型
    int m_end;          // 结束位置
    list_t m_nodes;     // 节点列表

protected:
    int m_inited;       // 初始化标志
};


// 条件块节点类
class ASTCondBlock : public ASTBlock {
public:
    // 初始化条件枚举
    enum InitCond {
        UNINITED,      // 未初始化
        POPPED,        // 已弹出
        PRE_POPPED     // 预弹出
    };

    ASTCondBlock(ASTBlock::BlkType blktype, int end, PycRef<ASTNode> cond,
                 bool negative = false)
        : ASTBlock(blktype, end), m_cond(std::move(cond)), m_negative(negative) { }

    PycRef<ASTNode> cond() const { return m_cond; }      // 获取条件表达式
    bool negative() const { return m_negative; }         // 是否为否定条件

private:
    PycRef<ASTNode> m_cond;     // 条件节点
    bool m_negative;            // 否定标志
};


// 迭代块节点类
class ASTIterBlock : public ASTBlock {
public:
    ASTIterBlock(ASTBlock::BlkType blktype, int start, int end, PycRef<ASTNode> iter)
        : ASTBlock(blktype, end), m_iter(std::move(iter)), m_idx(), m_comp(), m_start(start) { }

    PycRef<ASTNode> iter() const { return m_iter; }             // 获取迭代器
    PycRef<ASTNode> index() const { return m_idx; }             // 获取索引
    PycRef<ASTNode> condition() const { return m_cond; }        // 获取条件
    bool isComprehension() const { return m_comp; }             // 是否为推导式
    int start() const { return m_start; }                       // 获取起始位置

    void setIndex(PycRef<ASTNode> idx) { m_idx = std::move(idx); init(); }  // 设置索引
    void setCondition(PycRef<ASTNode> cond) { m_cond = std::move(cond); }   // 设置条件
    void setComprehension(bool comp) { m_comp = comp; }                     // 设置推导式标志

private:
    PycRef<ASTNode> m_iter;  // 迭代器节点
    PycRef<ASTNode> m_idx;   // 索引节点
    PycRef<ASTNode> m_cond;  // 条件节点
    bool m_comp;             // 推导式标志
    int m_start;             // 起始位置
};

// 容器块节点类
class ASTContainerBlock : public ASTBlock {
public:
    ASTContainerBlock(int finally, int except = 0)
        : ASTBlock(ASTBlock::BLK_CONTAINER, 0), m_finally(finally), m_except(except) { }

    bool hasFinally() const { return m_finally != 0; }   // 是否有finally块
    bool hasExcept() const { return m_except != 0; }     // 是否有except块
    int finally() const { return m_finally; }            // 获取finally位置
    int except() const { return m_except; }              // 获取except位置

    void setExcept(int except) { m_except = except; }    // 设置except位置

private:
    int m_finally;  // finally块位置
    int m_except;   // except块位置
};

// with块节点类
class ASTWithBlock : public ASTBlock {
public:
    ASTWithBlock(int end)
        : ASTBlock(ASTBlock::BLK_WITH, end) { }

    PycRef<ASTNode> expr() const { return m_expr; }   // 获取表达式
    PycRef<ASTNode> var() const { return m_var; }     // 获取变量

    void setExpr(PycRef<ASTNode> expr) { m_expr = std::move(expr); init(); }  // 设置表达式
    void setVar(PycRef<ASTNode> var) { m_var = std::move(var); }              // 设置变量

private:
    PycRef<ASTNode> m_expr;  // 表达式节点
    PycRef<ASTNode> m_var;   // 变量节点（可选值）
};

// 推导式节点类
class ASTComprehension : public ASTNode {
public:
    typedef std::list<PycRef<ASTIterBlock>> generator_t;

    ASTComprehension(PycRef<ASTNode> result)
        : ASTNode(NODE_COMPREHENSION), m_result(std::move(result)) { }

    PycRef<ASTNode> result() const { return m_result; }               // 获取结果表达式
    generator_t generators() const { return m_generators; }           // 获取生成器列表

    void addGenerator(PycRef<ASTIterBlock> gen) {
        m_generators.emplace_front(std::move(gen));  // 添加生成器
    }

private:
    PycRef<ASTNode> m_result;        // 结果表达式节点
    generator_t m_generators;        // 生成器列表
};

// 加载构建类节点类
class ASTLoadBuildClass : public ASTNode {
public:
    ASTLoadBuildClass(PycRef<PycObject> obj)
        : ASTNode(NODE_LOADBUILDCLASS), m_obj(std::move(obj)) { }

    PycRef<PycObject> object() const { return m_obj; }  // 获取对象

private:
    PycRef<PycObject> m_obj;  // 对象引用
};

// 可等待对象节点类
class ASTAwaitable : public ASTNode {
public:
    ASTAwaitable(PycRef<ASTNode> expr)
        : ASTNode(NODE_AWAITABLE), m_expr(std::move(expr)) { }

    PycRef<ASTNode> expression() const { return m_expr; }  // 获取表达式

private:
    PycRef<ASTNode> m_expr;  // 表达式节点
};

// 格式化值节点类（用于f-string）
class ASTFormattedValue : public ASTNode {
public:
    // 转换标志枚举
    enum ConversionFlag {
        NONE = 0,           // 无转换
        STR = 1,            // str() 转换
        REPR = 2,           // repr() 转换  
        ASCII = 3,          // ascii() 转换
        CONVERSION_MASK = 0x03,  // 转换掩码

        HAVE_FMT_SPEC = 4,  // 有格式说明符
    };

    ASTFormattedValue(PycRef<ASTNode> val, ConversionFlag conversion,
                      PycRef<ASTNode> format_spec)
        : ASTNode(NODE_FORMATTEDVALUE),
          m_val(std::move(val)),
          m_conversion(conversion),
          m_format_spec(std::move(format_spec))
    { }

    PycRef<ASTNode> val() const { return m_val; }                     // 获取值
    ConversionFlag conversion() const { return m_conversion; }        // 获取转换标志
    PycRef<ASTNode> format_spec() const { return m_format_spec; }     // 获取格式说明符

private:
    PycRef<ASTNode> m_val;            // 值节点
    ConversionFlag m_conversion;      // 转换标志
    PycRef<ASTNode> m_format_spec;    // 格式说明符节点
};

// 连接字符串节点类（同ASTList）
class ASTJoinedStr : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> value_t;

    ASTJoinedStr(value_t values)
        : ASTNode(NODE_JOINEDSTR), m_values(std::move(values)) { }

    const value_t& values() const { return m_values; }  // 获取值列表

private:
    value_t m_values;  // 值列表
};

// 注解变量节点类
class ASTAnnotatedVar : public ASTNode {
public:
    ASTAnnotatedVar(PycRef<ASTNode> name, PycRef<ASTNode> type)
        : ASTNode(NODE_ANNOTATED_VAR), m_name(std::move(name)), m_type(std::move(type)) { }

    PycRef<ASTNode> name() const noexcept { return m_name; }         // 获取变量名
    PycRef<ASTNode> annotation() const noexcept { return m_type; }   // 获取类型注解

private:
    PycRef<ASTNode> m_name;  // 变量名节点
    PycRef<ASTNode> m_type;  // 类型注解节点
};

// 三元操作节点类
class ASTTernary : public ASTNode
{
public:
    ASTTernary(PycRef<ASTNode> if_block, PycRef<ASTNode> if_expr,
               PycRef<ASTNode> else_expr)
        : ASTNode(NODE_TERNARY), m_if_block(std::move(if_block)),
          m_if_expr(std::move(if_expr)), m_else_expr(std::move(else_expr)) { }

    PycRef<ASTNode> if_block() const noexcept { return m_if_block; }    // 获取条件块
    PycRef<ASTNode> if_expr() const noexcept { return m_if_expr; }      // 获取if表达式
    PycRef<ASTNode> else_expr() const noexcept { return m_else_expr; }  // 获取else表达式

private:
    PycRef<ASTNode> m_if_block;   // 条件块节点（包含"condition"和"negative"）
    PycRef<ASTNode> m_if_expr;    // if表达式节点
    PycRef<ASTNode> m_else_expr;  // else表达式节点
};

#endif