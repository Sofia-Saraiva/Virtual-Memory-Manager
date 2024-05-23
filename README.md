# 💻 Virtual Memory Manager

This project implements a virtual memory manager in C, as part of the exercises presented in *Operating System Concepts*, Silberschatz, A. et al, 10th edition.

### 🔍 Description

The virtual memory manager simulates the translation of logical addresses to physical addresses using page tables and TLB (Translation Lookaside Buffer). It supports two page replacement algorithms: FIFO (First-In-First-Out) and LRU (Least Recently Used). The TLB uses FIFO for page translations.

## 💡 Features
- Virtual memory translation from logical addresses to physical addresses.
- Page replacement algorithms: FIFO and LRU.
- TLB management using FIFO.
- The frames in physical memory are filled from 0 to 127.

## Installation

1. Clone the repository:

    ```bash
    git clone https://github.com/Sofia-Saraiva/virtual-memory-manager.git
    ```

2. Navigate to the project directory:

    ```bash
    cd virtual-memory-manager/src
    ```

3. Compile the code using Makefile:

    ```bash
    make vm
    ```

4. Execute the program with command-line arguments:

    ```bash
    ./vm address.txt lru
    ```

    Replace `address.txt` with the path to your input file containing logical addresses, and `lru` with either `fifo` or `lru` to select the page replacement algorithm.

## 📄 Output

The program will generate an output file named `correct.txt` containing the translated physical addresses and other statistics, following the provided format for validation.

## 🛠️ Dependencies 

- GCC 13.2.0 or higher
- Linux operating system

## 👏 Acknowledgements

This project was developed for the Infrastructure of Software course in the third semester at Cesar School.
