# Function Graph Explorer

This is an educational tool for the exploration of function graphs, the Fourier Transform, filter design and audio signal analysis.

*Remark:*

This project is in an early stage. Functionality is still incomplete and might change rapidly.

![screenshot3](doc/screenshot3.jpg)

# Concept

The data model is based on the idea of chaining mathematical functions.
A function in this chain may use the output of a preceding function as input.
Each function may be considered one step in a processing pipeline and - depending on the context and the application - represents an operation such as a generator, a filter, some transformation or data analysis (e.g. Fourier Transform).

# Technologies

This is programming exercise about development of user applications and graphical user interface design in C++.

Technologies:

- [Qt](https://www.qt.io/): Modern, platform independent Graphical User Interfaces
- [exprtk](https://www.partow.net/programming/exprtk/index.html): Software Library for mathematical expressions

# Build

## Build With Conan

You need [conan](https://conan.io/) installed. Issuing the following command will fetch external dependencies if necessary and build the project:

    $ conan profile detect --force
    $ ./scripts/conan_build.fish

# Run

    $ ./scripts/run.fish

# Clean Output

    $ ./scripts/clean.fish

# Build and Run Tests

    $ ./scripts/conan_test.fish
