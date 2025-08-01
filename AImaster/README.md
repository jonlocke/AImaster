# AImaster Project

This project is a C++ application that communicates via a serial port and executes commands based on received input. The application listens for specific commands and performs actions such as listing directory contents or displaying the current user.

## Files

- **main.cpp**: Contains the main application code, including serial port communication and command execution.
- **Makefile**: Automates the build process for the C++ project.
- **README.md**: Documentation for the project.

## Build Instructions

To build the project, use the provided Makefile. Run the following command in the terminal:

```
make
```

This will compile the `main.cpp` file and create the executable named `AImaster`.

## Clean Up

To remove the generated files (object files and the executable), run:

```
make clean
```

This will clean up the project directory by removing all compiled files.