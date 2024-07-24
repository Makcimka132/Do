<h1 align="center">The Rave Programming Language</h1>
<p align="center">
<a href="https://github.com/Ttimofeyka/Rave/releases/latest">
<img src="https://img.shields.io/github/v/release/Ttimofeyka/Rave.svg" alt="Latest Release">
</a>
<br>
<a href="https://discord.gg/AfEtyArvsM">
<img src="https://img.shields.io/discord/872555146968698950?color=7289DA&label=Discord&logo=discord&logoColor=white" alt="Discord">
</a>
</p>
<br/>

Rave is a statically typed, compiled, procedural, general-purpose programming language.

## "Hello, world!" Example

```nasm
import <std/io>

void main {
    std::println("Hello, world!");
}
```

## Advantages

* Fast compilation
* Cross-platform features (for example, working with threads)
* Support for many platforms as target
* Using LLVM for great optimizations

For maximum performance, use the `-Ofast` or `-O3 --noChecks`. Also, don't forget to compile std with these flags using `--recompileStd -Ofast`.

## Dependencies

* `llvm-16`
**You can also use LLVM from 11 to 15.**
* `clang` or `gcc`
* C++ compiler (with support of C++17 and higher)
* Make
* MinGW (if you need cross-compilation or you are using Windows)

## Building/Running

To install dependencies, you can try running `install.sh` (Arch Linux/Void Linux/Ubuntu/Debian) or `install.bat` (only Windows 64-bit using [choco](https://chocolatey.org))

If the installer does not work well on your system, you can try to install all the dependencies yourself.

After install write `make` in the Rave directory.

You can compile, for example, "Hello world!" example using `./rave examples/hello_world.rave -o hello_world` in directory with Rave.
To run this example after compiling, try `./examples/hello_world`.

### Cross-compilation programs from Linux for Windows

You just need to set the compiler "i686-w64-mingw32-gcc-win32" in options.json, and add "-t i686-win32" to your build command.

## Specifications

The specifications is in `specifications` directory - [link](https://github.com/Ttimofeyka/Rave/blob/main/specifications/intro.md).

## Troubleshooting errors

### Segmentation fault during compile-time

Often caused by incorrect syntax or misuse of builtin instructions.
We're continuously working to minimize these occurrences.

### SSE/SSE2/SSE3/AVX as not a recognized features

If you encounter warnings about unrecognized features, disable them in options.json (set `sse` and `avx` to `0`) or use command-line options: `-noSSE`, `-noSSE2`, `-noSSE3`, `-noAVX`, `-noAVX2`.

### Division by zero

The compiler defines any division by zero as **undefined behavior** in order to optimize the work with mathematics.
Always check the numbers if you are not sure before making a division or calling a mathematical function from standard library.

## Useful links

<a href="https://github.com/Ttimofeyka/Rave/blob/main/bindings.md">Bindings</a>

<a href="https://discord.gg/AfEtyArvsM">Discord</a>

<a href="https://ravelang.space">Web-site</a>