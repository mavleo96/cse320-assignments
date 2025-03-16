# CSE 320 - Systems Fundamentals Assignments 

This repository contains programming assignments for CSE 320 Systems Fundamentals II course at Stony Brook University by Prof. Eugene Stark. The course focuses on programming in C and explores systems-level concepts including memory management, process control, and system calls.

## Repository Structure
```
.
├── ACADEMICHONESTY.md
├── README.md
├── hw0/
├── hw1/
├── hw2/
├── hw3/
├── hw4/
├── hw5/
├── .github-ci.yml
└── .gitignore
```

## Assignments

### Assignment 0: Development Environment Setup
Introduction to Linux environment setup, Git, and essential C development tools.

### Assignment 1: Introduction to C Programming
Implementation of string manipulation and bitwise operations in C with focus on memory safety.

### Assignment 2: Debugging, Fixing, and Optimizing
Debugging and fixing memory-related issues in existing C code using GDB and Valgrind.

### Assignment 3: Dynamic Memory Allocator
Implementation of a custom memory allocator (malloc/free) with various allocation strategies.

### Assignment 4: Parallel 'Cook'
Development of multi-level cooking process with multi-processing while handling signal and IO redirection.

### Assignment 5: PBX Server
Implementation of a Private Branch Exchange (PBX) server for handling concurrent phone connections.

> Note: Each assignment directory contains its own README.md with specific requirements and instructions.

## Development Environment

### VM Requirements
- VirtualBox 6.1.27 or higher
- 10GB minimum free disk space (40GB recommended)
- 64-bit hardware virtualization support
- Linux Mint 22 "Wilma" (provided in course VM)

### Course Tools
- GCC compiler suite
- GNU Make build system
- GDB debugger
- Valgrind memory checker
- Git version control

### VM Setup
The course provides a pre-configured Linux Mint VM (CSE320_Fall24.ova). For detailed setup instructions and VM download, refer to hw0/README.md.

## Building Projects

Each assignment follows a standard structure:
```bash
hwX/
├── include/     # Header files
├── src/        # Source files
├── tests/      # Test cases
└── Makefile    # Build configuration
```

To build a project:
```bash
cd hwX
make clean all
```

## Academic Integrity

This repository contains academic coursework. All work must be your own. Review [ACADEMICHONESTY.md](ACADEMICHONESTY.md) for the complete academic integrity policy.

## License

All rights reserved. This project is for educational purposes only and part of the CSE 320 coursework.