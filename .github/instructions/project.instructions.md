---
applyTo: "**/*.c,**/*.cpp,**/*.h"
description: "Conventions and Guidelines for Coding in this Project"
---
# Project coding standards for this project

Guidelines to follow at all times:
- Always keep your changes limited to what explicitly mentioned
- Don't use any 3rd party libraries/framework/code unless explicitly requested
- Focus on simplicity both in terms of design/architecture and implementation
- Code has to be easy to reason about, and easy to extend/change in the future
- Validate that you understand the question correctly by re-iterating what's asked of you but with different words
- Double-check your work before responding, think outside the box and consider tradeoffs and pros/cons.
- Consider how maintainable your changes will be, always try to create as maintainable code as possible
- Take a step back if needed and retry if you can think of a better solution
- Simple is better than complicated
- All code should be treated as production code
- Don't add any comments unless the next line is really complicated and hard to understand
- Don't create new abstractions unless absolutely required for fulfilling what's outlined above
- Most important, try to write simple and easily understood code
- For particularly tricky/large changes, divide your work into multiple milestones, and work through them one by one, and since you have a limit how big single commits can be, make sure to make multiple commits instead of one large one.

# Bulid, Run, and Test
- Use the provided build system to compile and run the project
- To build the binaries `./build/dis` and `./build/semi`, run the following command in the terminal:
  ```sh
  make
  ```
- To build the tests, run:
  ```sh
  make ./build/test_runner
  ```
  And then run the tests using:
  ```sh
  ./build/test_runner
  ```
  This is a googletest runner. Use flags like `--gtest_filter` to run specific tests. For example:
  ```sh
  ./build/test_runner --gtest_filter="VMInstructionFunctionCallTest.*"
  ```
  or
  ```sh
  ./build/test_runner --gtest_filter="VMInstructionFunctionCallTest.*"
  ```
- For debugging, DO NOT create temporary programs. Instead, use the existing test framework to add tests that can be run and debugged easily.
- To check the disassembly of a semi program, use './build/dis'. For example:
  ```sh
  echo 'b := List[1, 2]' | ./build/dis -in -
  ```
- We have a DSL for verifying the output of the compiler. Check out `./doc/test_dsl.md` for more information.
- DO NOT automatically generate tests unless explicitly requested.