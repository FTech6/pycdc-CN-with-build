#ifndef _PYC_ASTREE_H
#define _PYC_ASTREE_H

#include "ASTNode.h"

// 抽象语法树 (AST)
PycRef<ASTNode> BuildFromCode(PycRef<PycCode> code, PycModule* mod);

// 从 AST 生成 Python 代码
void print_src(PycRef<ASTNode> node, PycModule* mod, std::ostream& pyc_output);

// 反编译: Python 代码
void decompyle(PycRef<PycCode> code, PycModule* mod, std::ostream& pyc_output);

#endif
