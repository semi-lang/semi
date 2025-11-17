// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <set>
#include <string>

#include "instruction_verifier.hpp"
#include "test_common.hpp"

using namespace InstructionVerifier;

class CompilerCollectionInitializerTest : public CompilerTest {};

TEST_F(CompilerCollectionInitializerTest, EmptyListInitializer) {
    const char* input = "List[]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_NEW_COLLECTION   A=0x00 B=0x06 C=0x00 kb=T kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, EmptyDictInitializer) {
    const char* input = "Dict[]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_NEW_COLLECTION   A=0x00 B=0x07 C=0x00 kb=T kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, ListInitializerSingleElement) {
    const char* input = "List[1]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_NEW_COLLECTION            A=0x00 B=0x06 C=0x01 kb=T kc=F
1: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2: OP_APPEND_LIST               A=0x00 B=0x01 C=0x01 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, ListInitializerMultipleElements) {
    const char* input = "List[1, 2, 3]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_NEW_COLLECTION            A=0x00 B=0x06 C=0x03 kb=T kc=F
1: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2: OP_LOAD_INLINE_INTEGER       A=0x02 K=0x0002 i=T s=T
3: OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0003 i=T s=T
4: OP_APPEND_LIST               A=0x00 B=0x01 C=0x03 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, ListInitializerWithTrailingComma) {
    const char* input = "List[1, 2, 3,]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_NEW_COLLECTION            A=0x00 B=0x06 C=0x03 kb=T kc=F
1: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2: OP_LOAD_INLINE_INTEGER       A=0x02 K=0x0002 i=T s=T
3: OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0003 i=T s=T
4: OP_APPEND_LIST               A=0x00 B=0x01 C=0x03 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, ListInitializerExactly16Elements) {
    const char* input = "List[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0:  OP_NEW_COLLECTION            A=0x00 B=0x06 C=0x10 kb=T kc=F
1:  OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2:  OP_LOAD_INLINE_INTEGER       A=0x02 K=0x0002 i=T s=T
3:  OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0003 i=T s=T
4:  OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0004 i=T s=T
5:  OP_LOAD_INLINE_INTEGER       A=0x05 K=0x0005 i=T s=T
6:  OP_LOAD_INLINE_INTEGER       A=0x06 K=0x0006 i=T s=T
7:  OP_LOAD_INLINE_INTEGER       A=0x07 K=0x0007 i=T s=T
8:  OP_LOAD_INLINE_INTEGER       A=0x08 K=0x0008 i=T s=T
9:  OP_LOAD_INLINE_INTEGER       A=0x09 K=0x0009 i=T s=T
10: OP_LOAD_INLINE_INTEGER       A=0x0A K=0x000A i=T s=T
11: OP_LOAD_INLINE_INTEGER       A=0x0B K=0x000B i=T s=T
12: OP_LOAD_INLINE_INTEGER       A=0x0C K=0x000C i=T s=T
13: OP_LOAD_INLINE_INTEGER       A=0x0D K=0x000D i=T s=T
14: OP_LOAD_INLINE_INTEGER       A=0x0E K=0x000E i=T s=T
15: OP_LOAD_INLINE_INTEGER       A=0x0F K=0x000F i=T s=T
16: OP_LOAD_INLINE_INTEGER       A=0x10 K=0x0010 i=T s=T
17: OP_APPEND_LIST               A=0x00 B=0x01 C=0x10 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, ListInitializer17ElementsRequiresBatching) {
    const char* input = "List[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0:  OP_NEW_COLLECTION            A=0x00 B=0x06 C=0x11 kb=T kc=F
1:  OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2:  OP_LOAD_INLINE_INTEGER       A=0x02 K=0x0002 i=T s=T
3:  OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0003 i=T s=T
4:  OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0004 i=T s=T
5:  OP_LOAD_INLINE_INTEGER       A=0x05 K=0x0005 i=T s=T
6:  OP_LOAD_INLINE_INTEGER       A=0x06 K=0x0006 i=T s=T
7:  OP_LOAD_INLINE_INTEGER       A=0x07 K=0x0007 i=T s=T
8:  OP_LOAD_INLINE_INTEGER       A=0x08 K=0x0008 i=T s=T
9:  OP_LOAD_INLINE_INTEGER       A=0x09 K=0x0009 i=T s=T
10: OP_LOAD_INLINE_INTEGER       A=0x0A K=0x000A i=T s=T
11: OP_LOAD_INLINE_INTEGER       A=0x0B K=0x000B i=T s=T
12: OP_LOAD_INLINE_INTEGER       A=0x0C K=0x000C i=T s=T
13: OP_LOAD_INLINE_INTEGER       A=0x0D K=0x000D i=T s=T
14: OP_LOAD_INLINE_INTEGER       A=0x0E K=0x000E i=T s=T
15: OP_LOAD_INLINE_INTEGER       A=0x0F K=0x000F i=T s=T
16: OP_LOAD_INLINE_INTEGER       A=0x10 K=0x0010 i=T s=T
17: OP_APPEND_LIST               A=0x00 B=0x01 C=0x10 kb=F kc=F
18: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0011 i=T s=T
19: OP_APPEND_LIST               A=0x00 B=0x01 C=0x01 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, ListInitializer32ElementsRequiresDoubleBatching) {
    const char* input =
        "List[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, "
        "17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0:  OP_NEW_COLLECTION            A=0x00 B=0x06 C=0x20 kb=T kc=F
1:  OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2:  OP_LOAD_INLINE_INTEGER       A=0x02 K=0x0002 i=T s=T
3:  OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0003 i=T s=T
4:  OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0004 i=T s=T
5:  OP_LOAD_INLINE_INTEGER       A=0x05 K=0x0005 i=T s=T
6:  OP_LOAD_INLINE_INTEGER       A=0x06 K=0x0006 i=T s=T
7:  OP_LOAD_INLINE_INTEGER       A=0x07 K=0x0007 i=T s=T
8:  OP_LOAD_INLINE_INTEGER       A=0x08 K=0x0008 i=T s=T
9:  OP_LOAD_INLINE_INTEGER       A=0x09 K=0x0009 i=T s=T
10: OP_LOAD_INLINE_INTEGER       A=0x0A K=0x000A i=T s=T
11: OP_LOAD_INLINE_INTEGER       A=0x0B K=0x000B i=T s=T
12: OP_LOAD_INLINE_INTEGER       A=0x0C K=0x000C i=T s=T
13: OP_LOAD_INLINE_INTEGER       A=0x0D K=0x000D i=T s=T
14: OP_LOAD_INLINE_INTEGER       A=0x0E K=0x000E i=T s=T
15: OP_LOAD_INLINE_INTEGER       A=0x0F K=0x000F i=T s=T
16: OP_LOAD_INLINE_INTEGER       A=0x10 K=0x0010 i=T s=T
17: OP_APPEND_LIST               A=0x00 B=0x01 C=0x10 kb=F kc=F
18: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0011 i=T s=T
19: OP_LOAD_INLINE_INTEGER       A=0x02 K=0x0012 i=T s=T
20: OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0013 i=T s=T
21: OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0014 i=T s=T
22: OP_LOAD_INLINE_INTEGER       A=0x05 K=0x0015 i=T s=T
23: OP_LOAD_INLINE_INTEGER       A=0x06 K=0x0016 i=T s=T
24: OP_LOAD_INLINE_INTEGER       A=0x07 K=0x0017 i=T s=T
25: OP_LOAD_INLINE_INTEGER       A=0x08 K=0x0018 i=T s=T
26: OP_LOAD_INLINE_INTEGER       A=0x09 K=0x0019 i=T s=T
27: OP_LOAD_INLINE_INTEGER       A=0x0A K=0x001A i=T s=T
28: OP_LOAD_INLINE_INTEGER       A=0x0B K=0x001B i=T s=T
29: OP_LOAD_INLINE_INTEGER       A=0x0C K=0x001C i=T s=T
30: OP_LOAD_INLINE_INTEGER       A=0x0D K=0x001D i=T s=T
31: OP_LOAD_INLINE_INTEGER       A=0x0E K=0x001E i=T s=T
32: OP_LOAD_INLINE_INTEGER       A=0x0F K=0x001F i=T s=T
33: OP_LOAD_INLINE_INTEGER       A=0x10 K=0x0020 i=T s=T
34: OP_APPEND_LIST               A=0x00 B=0x01 C=0x10 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, DictInitializerSinglePair) {
    const char* input = "Dict[1: 10]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_NEW_COLLECTION            A=0x00 B=0x07 C=0x01 kb=T kc=F
1: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2: OP_LOAD_INLINE_INTEGER       A=0x02 K=0x000A i=T s=T
3: OP_APPEND_MAP                A=0x00 B=0x01 C=0x01 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, DictInitializerMultiplePairs) {
    const char* input = "Dict[1: 10, 2: 20, 3: 30]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_NEW_COLLECTION            A=0x00 B=0x07 C=0x03 kb=T kc=F
1: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2: OP_LOAD_INLINE_INTEGER       A=0x02 K=0x000A i=T s=T
3: OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0002 i=T s=T
4: OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0014 i=T s=T
5: OP_LOAD_INLINE_INTEGER       A=0x05 K=0x0003 i=T s=T
6: OP_LOAD_INLINE_INTEGER       A=0x06 K=0x001E i=T s=T
7: OP_APPEND_MAP                A=0x00 B=0x01 C=0x03 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, DictInitializerWithTrailingComma) {
    const char* input = "Dict[1: 10, 2: 20,]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_NEW_COLLECTION            A=0x00 B=0x07 C=0x02 kb=T kc=F
1: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2: OP_LOAD_INLINE_INTEGER       A=0x02 K=0x000A i=T s=T
3: OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0002 i=T s=T
4: OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0014 i=T s=T
5: OP_APPEND_MAP                A=0x00 B=0x01 C=0x02 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, DictInitializerExactly8Pairs) {
    const char* input = "Dict[1: 10, 2: 20, 3: 30, 4: 40, 5: 50, 6: 60, 7: 70, 8: 80]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0:  OP_NEW_COLLECTION            A=0x00 B=0x07 C=0x08 kb=T kc=F
1:  OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2:  OP_LOAD_INLINE_INTEGER       A=0x02 K=0x000A i=T s=T
3:  OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0002 i=T s=T
4:  OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0014 i=T s=T
5:  OP_LOAD_INLINE_INTEGER       A=0x05 K=0x0003 i=T s=T
6:  OP_LOAD_INLINE_INTEGER       A=0x06 K=0x001E i=T s=T
7:  OP_LOAD_INLINE_INTEGER       A=0x07 K=0x0004 i=T s=T
8:  OP_LOAD_INLINE_INTEGER       A=0x08 K=0x0028 i=T s=T
9:  OP_LOAD_INLINE_INTEGER       A=0x09 K=0x0005 i=T s=T
10: OP_LOAD_INLINE_INTEGER       A=0x0A K=0x0032 i=T s=T
11: OP_LOAD_INLINE_INTEGER       A=0x0B K=0x0006 i=T s=T
12: OP_LOAD_INLINE_INTEGER       A=0x0C K=0x003C i=T s=T
13: OP_LOAD_INLINE_INTEGER       A=0x0D K=0x0007 i=T s=T
14: OP_LOAD_INLINE_INTEGER       A=0x0E K=0x0046 i=T s=T
15: OP_LOAD_INLINE_INTEGER       A=0x0F K=0x0008 i=T s=T
16: OP_LOAD_INLINE_INTEGER       A=0x10 K=0x0050 i=T s=T
17: OP_APPEND_MAP                A=0x00 B=0x01 C=0x08 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, DictInitializer9PairsRequiresBatching) {
    const char* input = "Dict[1: 10, 2: 20, 3: 30, 4: 40, 5: 50, 6: 60, 7: 70, 8: 80, 9: 90]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0:  OP_NEW_COLLECTION            A=0x00 B=0x07 C=0x09 kb=T kc=F
1:  OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2:  OP_LOAD_INLINE_INTEGER       A=0x02 K=0x000A i=T s=T
3:  OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0002 i=T s=T
4:  OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0014 i=T s=T
5:  OP_LOAD_INLINE_INTEGER       A=0x05 K=0x0003 i=T s=T
6:  OP_LOAD_INLINE_INTEGER       A=0x06 K=0x001E i=T s=T
7:  OP_LOAD_INLINE_INTEGER       A=0x07 K=0x0004 i=T s=T
8:  OP_LOAD_INLINE_INTEGER       A=0x08 K=0x0028 i=T s=T
9:  OP_LOAD_INLINE_INTEGER       A=0x09 K=0x0005 i=T s=T
10: OP_LOAD_INLINE_INTEGER       A=0x0A K=0x0032 i=T s=T
11: OP_LOAD_INLINE_INTEGER       A=0x0B K=0x0006 i=T s=T
12: OP_LOAD_INLINE_INTEGER       A=0x0C K=0x003C i=T s=T
13: OP_LOAD_INLINE_INTEGER       A=0x0D K=0x0007 i=T s=T
14: OP_LOAD_INLINE_INTEGER       A=0x0E K=0x0046 i=T s=T
15: OP_LOAD_INLINE_INTEGER       A=0x0F K=0x0008 i=T s=T
16: OP_LOAD_INLINE_INTEGER       A=0x10 K=0x0050 i=T s=T
17: OP_APPEND_MAP                A=0x00 B=0x01 C=0x08 kb=F kc=F
18: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0009 i=T s=T
19: OP_LOAD_INLINE_INTEGER       A=0x02 K=0x005A i=T s=T
20: OP_APPEND_MAP                A=0x00 B=0x01 C=0x01 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, DictInitializer16PairsRequiresDoubleBatching) {
    const char* input =
        "Dict[1: 10, 2: 20, 3: 30, 4: 40, 5: 50, 6: 60, 7: 70, 8: 80, "
        "9: 90, 10: 100, 11: 110, 12: 120, 13: 130, 14: 140, 15: 150, 16: 160]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0:  OP_NEW_COLLECTION            A=0x00 B=0x07 C=0x10 kb=T kc=F
1:  OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2:  OP_LOAD_INLINE_INTEGER       A=0x02 K=0x000A i=T s=T
3:  OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0002 i=T s=T
4:  OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0014 i=T s=T
5:  OP_LOAD_INLINE_INTEGER       A=0x05 K=0x0003 i=T s=T
6:  OP_LOAD_INLINE_INTEGER       A=0x06 K=0x001E i=T s=T
7:  OP_LOAD_INLINE_INTEGER       A=0x07 K=0x0004 i=T s=T
8:  OP_LOAD_INLINE_INTEGER       A=0x08 K=0x0028 i=T s=T
9:  OP_LOAD_INLINE_INTEGER       A=0x09 K=0x0005 i=T s=T
10: OP_LOAD_INLINE_INTEGER       A=0x0A K=0x0032 i=T s=T
11: OP_LOAD_INLINE_INTEGER       A=0x0B K=0x0006 i=T s=T
12: OP_LOAD_INLINE_INTEGER       A=0x0C K=0x003C i=T s=T
13: OP_LOAD_INLINE_INTEGER       A=0x0D K=0x0007 i=T s=T
14: OP_LOAD_INLINE_INTEGER       A=0x0E K=0x0046 i=T s=T
15: OP_LOAD_INLINE_INTEGER       A=0x0F K=0x0008 i=T s=T
16: OP_LOAD_INLINE_INTEGER       A=0x10 K=0x0050 i=T s=T
17: OP_APPEND_MAP                A=0x00 B=0x01 C=0x08 kb=F kc=F
18: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0009 i=T s=T
19: OP_LOAD_INLINE_INTEGER       A=0x02 K=0x005A i=T s=T
20: OP_LOAD_INLINE_INTEGER       A=0x03 K=0x000A i=T s=T
21: OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0064 i=T s=T
22: OP_LOAD_INLINE_INTEGER       A=0x05 K=0x000B i=T s=T
23: OP_LOAD_INLINE_INTEGER       A=0x06 K=0x006E i=T s=T
24: OP_LOAD_INLINE_INTEGER       A=0x07 K=0x000C i=T s=T
25: OP_LOAD_INLINE_INTEGER       A=0x08 K=0x0078 i=T s=T
26: OP_LOAD_INLINE_INTEGER       A=0x09 K=0x000D i=T s=T
27: OP_LOAD_INLINE_INTEGER       A=0x0A K=0x0082 i=T s=T
28: OP_LOAD_INLINE_INTEGER       A=0x0B K=0x000E i=T s=T
29: OP_LOAD_INLINE_INTEGER       A=0x0C K=0x008C i=T s=T
30: OP_LOAD_INLINE_INTEGER       A=0x0D K=0x000F i=T s=T
31: OP_LOAD_INLINE_INTEGER       A=0x0E K=0x0096 i=T s=T
32: OP_LOAD_INLINE_INTEGER       A=0x0F K=0x0010 i=T s=T
33: OP_LOAD_INLINE_INTEGER       A=0x10 K=0x00A0 i=T s=T
34: OP_APPEND_MAP                A=0x00 B=0x01 C=0x08 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, MixingListAndMapSyntaxColonInList) {
    const char* input = "List[1, 2: 20]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN);
}

TEST_F(CompilerCollectionInitializerTest, MixingListAndMapSyntaxNoColonInDict) {
    const char* input = "Dict[1: 10, 2]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN);
}

TEST_F(CompilerCollectionInitializerTest, MixingListAndMapSyntaxStartWithListThenDict) {
    const char* input = "List[1, 2, 3: 30]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN);
}

TEST_F(CompilerCollectionInitializerTest, MixingListAndMapSyntaxStartWithDictThenList) {
    const char* input = "Dict[1: 10, 2: 20, 3]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN);
}

TEST_F(CompilerCollectionInitializerTest, ListInitializerWithComplexExpressions) {
    const char* input = "List[1 + 2, 3 * 4, 5 - 6]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_NEW_COLLECTION            A=0x00 B=0x06 C=0x03 kb=T kc=F
1: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0003 i=T s=T
2: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x000C i=T s=T
3: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=F
4: OP_APPEND_LIST               A=0x00 B=0x01 C=0x03 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, DictInitializerWithComplexExpressions) {
    const char* input = "Dict[1 + 1: 10 * 2, 2 - 1: 20 + 5]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_NEW_COLLECTION            A=0x00 B=0x07 C=0x02 kb=T kc=F
1: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0002 i=T s=T
2: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0014 i=T s=T
3: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
4: OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0019 i=T s=T
5: OP_APPEND_MAP                A=0x00 B=0x01 C=0x02 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, NestedListInitializers) {
    const char* input = "List[List[1, 2], List[3, 4]]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0:  OP_NEW_COLLECTION            A=0x00 B=0x06 C=0x02 kb=T kc=F
1:  OP_NEW_COLLECTION            A=0x01 B=0x06 C=0x02 kb=T kc=F
2:  OP_LOAD_INLINE_INTEGER       A=0x02 K=0x0001 i=T s=T
3:  OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0002 i=T s=T
4:  OP_APPEND_LIST               A=0x01 B=0x02 C=0x02 kb=F kc=F
5:  OP_NEW_COLLECTION            A=0x02 B=0x06 C=0x02 kb=T kc=F
6:  OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0003 i=T s=T
7:  OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0004 i=T s=T
8:  OP_APPEND_LIST               A=0x02 B=0x03 C=0x02 kb=F kc=F
9:  OP_APPEND_LIST               A=0x00 B=0x01 C=0x02 kb=F kc=F
)");
}

TEST_F(CompilerCollectionInitializerTest, NestedDictInitializers) {
    const char* input = "Dict[1: Dict[10: 100], 2: Dict[20: 200]]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    VerifyCompilation(&compiler, R"(
[Instructions]
0:  OP_NEW_COLLECTION            A=0x00 B=0x07 C=0x02 kb=T kc=F
1:  OP_LOAD_INLINE_INTEGER       A=0x01 K=0x0001 i=T s=T
2:  OP_NEW_COLLECTION            A=0x02 B=0x07 C=0x01 kb=T kc=F
3:  OP_LOAD_INLINE_INTEGER       A=0x03 K=0x000A i=T s=T
4:  OP_LOAD_INLINE_INTEGER       A=0x04 K=0x0064 i=T s=T
5:  OP_APPEND_MAP                A=0x02 B=0x03 C=0x01 kb=F kc=F
6:  OP_LOAD_INLINE_INTEGER       A=0x03 K=0x0002 i=T s=T
7:  OP_NEW_COLLECTION            A=0x04 B=0x07 C=0x01 kb=T kc=F
8:  OP_LOAD_INLINE_INTEGER       A=0x05 K=0x0014 i=T s=T
9:  OP_LOAD_INLINE_INTEGER       A=0x06 K=0x00C8 i=T s=T
10: OP_APPEND_MAP                A=0x04 B=0x05 C=0x01 kb=F kc=F
11: OP_APPEND_MAP                A=0x00 B=0x01 C=0x02 kb=F kc=F
)");
}
