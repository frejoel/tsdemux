# libts
MPEG-TS (ISO/IEC 13818-1) demultiplexer.

## Overview
This library is intended for content analysis and content demultiplexing in
applications such as adaptive video streaming which tools such as ffmpeg may
not be possible or suitable.

This software is licensed under an MIT license.
See the LICENSE file for licensing information.

## Multi-Threading
This library is single threaded and not thread-safe when attempting to
manipulate a single Demux Context across multiple threads.

It is thread-safe if content is demultiplexed using a separate Demux Context per
thread.

The demux example, `examples/demux.c`, creates a single Demux Context on a
single (main) thread.

## Documentation
The `src/tsdemux.h` header file contains the public API documentation.
This header is generated and distributed with the Library files.

The best place to get started would be the examples which are in the `examples`
directory.
The examples are heavily documented tutorials with instructions on how to work
with the library.

# Installation
## Dependencies
There are currently no external dependencies required to build this library.

It should be noted that development and testing by the author takes place on
Linux (Debian), and to date, no compilation and testing has been on attempted
on Little Endian Architectures.

## Linux
From a terminal:
```
  make
```
Creates a `bin` directory containing static library and header files.
This is no `install` step within the Makefile.

To clean the repository and start a build from scratch:
```
  make clean
  make
```

To build and run all unit tests validating the build:
```
  make check
```
The unit tests will abort as soon as an error is encountered leaving a message
in the terminal.

# Examples
Examples showing how to use the library as in the `examples` directory.
To build the examples:
```
 make examples
```
The examples are also build when using the `all` target.

The executables are built directly in the `examples` directory alongside the
example source code.
The examples and expecting to be run from the `examples` directory.
```
 cd examples
 ./demux.o
```

# Contributing
Before making Pull Requests into this repository for review, please complete
the following steps.

## Code Style
Run a pass on the source code to ensure basic common 'linux' styling using
`astyle`:
```
 sudo apt-get install astyle
 make style
```

## Unit Tests
Make sure all unit tests are passing, and provide new or modify existing unit
tests, which cover code changes being made.

Build unit tests:
```
 make tests
```

Build and run unit tests:
```
 make check
```
