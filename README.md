# SEMI

A minimalist programming language designed for embedding within applications.

> **NOTICE**: Semi is currently in very early development. The language and its APIs are subject to change without notice.


## Design Principles

* Semi is **Predictable**: The syntax is designed to be concise and easy to understand. Variable shadowing is prohibited. Errors must be handled explicitly, not buried in exception blocks.
* Semi is **Data Oriented**: Semi encourages a data-centric approach with first-class support for algebraic data types and structural typing. By favoring composition over inheritance, it helps you write code that is more flexible and resilient to change.
* Semi is **Minimal**: Semi keeps the builds lean by making the standard library optional. The entire runtime, including the compiler and virtual machine, is under 50KiB when compiled to WebAssembly and gzipped.
* Semi is **Portable**: Semi is C11-compliant and is designed to be compiled and run anywhere.


## Getting Started

Semi does not have any dependencies beyond a C11 compiler. To build the Semi compiler and runtime, simply clone the repository and run `make`. Two binaries will be produced that assist quickly verifying code changes: `semi` (the ad-hoc CLI) and `dis` (the bytecode disassembler). It also generates a static library `libsemi.a` that can be linked into your own applications. The APIs are defined in `semi/semi.h`.

To generate release builds, add `BUILD_MODE=release` to make arguments.

```shell
make BUILD_MODE=release
make BUILD_MODE=release WASM=1 wasm # For WebAssembly target
```

This will enable optimizations and disable debug information.

Currently there's not much to configure, but `include/semi/config.h` is the place where you can tweak compile-time options.

### WebAssembly Target

We treat WebAssembly as a first-class target for Semi. To build the WebAssembly version of Semi, you need to have [Emscripten](https://emscripten.org/) installed. Once you have Emscripten set up, run `make WASM=1 wasm` in the root directory. This will produce `build/wasm.wasm` and `build/wasm.js`. They power the demo in our [homepage](https://semi-lang.dev).


## License

Semi is licensed under the [Mozilla Public License 2.0](https://www.mozilla.org/en-US/MPL/2.0/).

## Security

For any security vulnerabilities, please report them to security at our domain, where our domain is semi-lang dot dev.