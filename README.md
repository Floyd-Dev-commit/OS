# OS
Operation System Code

Linux Operating System Internals & Process Management
A comprehensive collection of system-level programming projects focused on Linux kernel exploration, process synchronization, and file system architecture. These experiments were conducted on Ubuntu 16.04 using the Linux 4.16.3 kernel source.

Exp1: Kernel Compilation & Custom System Call
This project demonstrates the process of extending the Linux kernel functionality. It involves modifying the kernel source to register and implement a new system call, mysetnice. This system call allows user-space applications to interface with the kernel's scheduler by reading or modifying the nice value and priority (prio) of a specific process identified by its PID. The implementation includes modifications to the system call table, header files, and the core service routine.

Exp2: Loadable Kernel Modules (LKM)
This section focuses on kernel-space programming through Loadable Kernel Modules. Two distinct modules were developed:

Task Enumerator: A module that traverses the task list to extract and display metadata for all kernel threads, including task name, PID, execution state, and priority.

Process Genealogy: A parameterized module that takes a PID as input and reconstructs the process's family tree, identifying its parent, siblings, and children directly from kernel data structures.

Exp3: Process Management & Inter-Process Communication (IPC)
This project explores various paradigms for process creation and communication in a Linux environment:

Shell Simulation: A basic command-line interpreter that manages process lifecycles using fork() and exec() primitives.

Synchronized Pipe Communication: Multi-process coordination using anonymous pipes and POSIX semaphores to ensure data integrity during concurrent execution.

Message Queue Systems: A 2-sender/2-receiver model utilizing Linux message queues for asynchronous data exchange.

Shared Memory Management: A sophisticated IPC model using mapped shared memory. It features a custom-built static linked list to manage memory blocks, optimizing space allocation and enabling efficient communication between four independent processes.

Exp4: Virtual File System Implementation
A standalone, multi-level directory file system based on the FAT (File Allocation Table) architecture. The system operates on a memory-mapped virtual disk and ensures data persistence by saving the entire file system state to a local host file upon exit. Key features include:

Directory Management: Support for hierarchical structures with commands for navigation, creation, and a recursive deletion algorithm for non-empty directories.

File Operations: A complete API for file lifecycle management, including creation, deletion, and access control.

Advanced I/O: Support for multiple write modes, including Truncate, Overwrite, and Append, implemented through a robust pointer-based read/write mechanism.

Environment & Requirements
Operating System: Ubuntu 16.04 LTS

Kernel: Linux Source 4.16.3

Tools: GCC, Make, GDB

Parts of works finished on Windows using Visual Studio 2019 IDE.



Due to Github 25MB Limit，Linux Kernel == 4.16.3 not included.
