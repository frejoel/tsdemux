# libts

MPEG-TS (ISO/IEC 13818-1) demultiplexer.

This library is intended for content analysis and content demultiplexing in
applications such as adaptive video streaming.

See the LICENSE file for licensing information.

# Installation
## Dependencies
There are currently no external dependencies required to build this library.

It should be noted that development and testing by the author takes place on
Linux (Debian), and to date, no testing has been done on Little Endian
Architectures.

## Linux Systems
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

To run all unit tests validating the build:
```
  make check
```
The unit tests will abort as soon as an error is encountered leaving a message
in the terminal.

# Contributing
Before making Pull Requests into this repository for review, please complete
the following steps.

## Code Style
Run a pass on the source code to ensure basic common 'linux' styling using
'astyle':
```
 sudo apt-get install astyle
 astyle --style=linux -n src/*.h src/*.c
```

## Unit Tests
Make sure all unit tests are passing, and provide new or modify existing unit
tests, which cover code changes being made.

Check the unit tests:
```
 make check
```
