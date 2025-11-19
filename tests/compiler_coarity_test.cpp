// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "instruction_verifier.hpp"
#include "test_common.hpp"

using namespace InstructionVerifier;

class CompilerCoarityTest : public CompilerTest {};

// ============================================================================
// Category 1: Basic Terminal If-Else
// ============================================================================

TEST_F(CompilerCoarityTest, CompleteIfElseWithValueReturns) {
    const char* source = R"(
        fn test() {
            if true { return 1 } else { return 2 }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Complete if-else with returns should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: FunctionProto arity=0 coarity=1 maxStackSize=1 -> @test
)");
}

TEST_F(CompilerCoarityTest, CompleteIfElseWithEmptyReturns) {
    const char* source = R"(
        fn test() {
            if true { return } else { return }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Complete if-else with empty returns should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @test
)");
}

TEST_F(CompilerCoarityTest, IncompleteIfRequiresExplicitReturn) {
    const char* source = R"(
        fn test() {
            if true { return 1 }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_NE(result, 0) << "Incomplete if should fail to compile";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_MISSING_RETURN_STATEMENT);
}

// ============================================================================
// Category 2: If-Elif-Else Chains
// ============================================================================

TEST_F(CompilerCoarityTest, CompleteIfElifElseAllReturning) {
    const char* source = R"(
        fn test(a, b) {
            if a { return 1 } elif b { return 2 } else { return 3 }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Complete if-elif-else should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: FunctionProto arity=2 coarity=1 maxStackSize=3 -> @test
)");
}

TEST_F(CompilerCoarityTest, IfElifElseWithMissingReturnInMiddle) {
    const char* source = R"(
        fn test(a, b, c) {
            if a { return 1 } elif b { } elif c { return 1 }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_NE(result, 0) << "If-elif-else with missing return should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_MISSING_RETURN_STATEMENT);
}

TEST_F(CompilerCoarityTest, IfElifWithoutElseIsNonTerminal) {
    const char* source = R"(
        fn test(a, b) {
            if a { return 1 } elif b { return 1 }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_NE(result, 0) << "If-elif without else should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_MISSING_RETURN_STATEMENT);
}

// ============================================================================
// Category 3: Nested If-Else
// ============================================================================

TEST_F(CompilerCoarityTest, NestedCompleteIfElseInAllBranches) {
    const char* source = R"(
        fn test(outer, inner) {
            if outer {
                if inner { return 1 } else { return 2 }
            } else {
                return 3
            }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Nested complete if-else should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: FunctionProto arity=2 coarity=1 maxStackSize=3 -> @test
)");
}

TEST_F(CompilerCoarityTest, IncompleteInnerIfMakesOuterNonTerminal) {
    const char* source = R"(
        fn test(outer, inner) {
            if outer {
                if inner { return 1 }
            } else {
                return 2
            }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_NE(result, 0) << "Incomplete inner if should make function fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_MISSING_RETURN_STATEMENT);
}

// ============================================================================
// Category 4: For Loops and Control Flow
// ============================================================================

TEST_F(CompilerCoarityTest, ReturnInsideForLoopDoesNotMakeFunctionTerminal) {
    const char* source = R"(
        fn test() {
            for i in 1..10 {
                return 1
            }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_NE(result, 0) << "Return in for loop should not make function terminal";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_MISSING_RETURN_STATEMENT);
}

TEST_F(CompilerCoarityTest, ForLoopAfterTerminalIfElseIsUnreachable) {
    const char* source = R"(
        fn test() {
            if true { return 1 } else { return 1 }
            for i in 1..10 { x := i }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Unreachable for loop after terminal if-else should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: Range start=1 end=10 step=1
K[1]: FunctionProto arity=0 coarity=1 maxStackSize=3 -> @test
)");
}

TEST_F(CompilerCoarityTest, ForLoopWithCompleteIfElseInsideStillNonTerminal) {
    const char* source = R"(
        fn test() {
            for i in 1..10 {
                if i > 5 { return 1 } else { return 2 }
            }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_NE(result, 0) << "Complete if-else in loop doesn't make function terminal";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_MISSING_RETURN_STATEMENT);
}

TEST_F(CompilerCoarityTest, TerminalIfElseAfterForLoop) {
    const char* source = R"(
        fn test() {
            for i in 1..10 { x := i }
            if true { return 1 } else { return 1 }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Terminal if-else after for loop should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: Range start=1 end=10 step=1
K[1]: FunctionProto arity=0 coarity=1 maxStackSize=3 -> @test
)");
}

// ============================================================================
// Category 5: For Loop Nested in If-Else
// ============================================================================

TEST_F(CompilerCoarityTest, ForLoopInIfBranchWithReturnAfter) {
    const char* source = R"(
        fn test(cond) {
            if cond {
                for i in 1..10 { }
                return 1
            } else {
                return 1
            }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "For loop followed by return in branch should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: Range start=1 end=10 step=1
K[1]: FunctionProto arity=1 coarity=1 maxStackSize=4 -> @test
)");
}

TEST_F(CompilerCoarityTest, ForLoopInIfBranchWithoutReturnAfter) {
    const char* source = R"(
        fn test(cond) {
            if cond {
                for i in 1..10 { return 1 }
            } else {
                return 1
            }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_NE(result, 0) << "For loop without return after should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_MISSING_RETURN_STATEMENT);
}

// ============================================================================
// Category 6: If-Else Nested in For Loop
// ============================================================================

TEST_F(CompilerCoarityTest, CompleteIfElseInsideForLoopFunctionNeedsReturn) {
    const char* source = R"(
        fn test() {
            for i in 1..10 {
                if i > 5 { x := 1 } else { y := 2 }
            }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Function with no returns should compile with coarity=0";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: Range start=1 end=10 step=1
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=3 -> @test
)");
}

TEST_F(CompilerCoarityTest, ReturnInOneForLoopIterationPath) {
    const char* source = R"(
        fn test() {
            for i in 1..10 {
                if i > 5 { return 1 }
            }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_NE(result, 0) << "Incomplete if in loop should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_MISSING_RETURN_STATEMENT);
}

// ============================================================================
// Category 7: Unreachable Code
// ============================================================================

TEST_F(CompilerCoarityTest, StatementsAfterTerminalIfElse) {
    const char* source = R"(
        fn test() {
            if true { return 1 } else { return 1 }
            x := 2
            y := 3
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Unreachable statements after terminal if-else should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: FunctionProto arity=0 coarity=1 maxStackSize=2 -> @test
)");
}

TEST_F(CompilerCoarityTest, AnotherIfElseAfterTerminalIfElse) {
    const char* source = R"(
        fn test(a, b) {
            if a { return 1 } else { return 1 }
            if b { z := 3 } else { w := 4 }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Unreachable if-else after terminal should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: FunctionProto arity=2 coarity=1 maxStackSize=3 -> @test
)");
}

// ============================================================================
// Category 8: Functions Without Returns
// ============================================================================

TEST_F(CompilerCoarityTest, FunctionWithNoReturns) {
    const char* source = R"(
        fn test() {
            x := 1
            for i in 1..10 { y := i }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Function without returns should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: Range start=1 end=10 step=1
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=4 -> @test
)");
}

TEST_F(CompilerCoarityTest, IfElseWithNoReturns) {
    const char* source = R"(
        fn test() {
            if true { x := 1 } else { y := 2 }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "If-else without returns should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @test
)");
}

// ============================================================================
// Category 9: Error Cases
// ============================================================================

TEST_F(CompilerCoarityTest, InconsistentReturnsInSameFunction) {
    const char* source = R"(
        fn test() {
            if true {
                return 1
            }
            return
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_NE(result, 0) << "Inconsistent returns should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_INCONSISTENT_RETURN_COUNT);
}

TEST_F(CompilerCoarityTest, OneBranchWithValueOneWithout) {
    const char* source = R"(
        fn test() {
            if true { return 1 } else { }
            return 1
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "One branch with return, one without, explicit return at end should compile";

    VerifyModule(module, R"(
[Instructions] (ignored)
[Instructions:test] (ignored)

[Constants]
K[0]: FunctionProto arity=0 coarity=1 maxStackSize=1 -> @test
)");
}
