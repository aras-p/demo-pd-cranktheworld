This is an implementation of radix sort in C and C++.

The radix sort implentation also include a 11 bit radix sort which uses less
passes than the standard 8 bit radix sort, and a floating point key radix sort,
as described by Michael Herf:

 * http://stereopsis.com/radix.html

The radix sort interface is designed to perform no memory allocations, and does
not write the final sorted values to an output buffer, but rather returns which
buffer the final result resides in. The rationaly here is the callig code can
decide if it's appropriate to allocate temporary space, or to copy the results
to a specified buffer. If so desired a simple wrapper could be written to
simplify the interface at the expense of some performance and memory
allocation.

There is a test program which calls each radix sort function with a small
array to sort, and a perf program which performs a number of iterations
of each radix sort algorithm for different input sizes. In the case of C++
the same data is sorted by the sort provided by each language's respective
standard library for comparison purposes.

## Prerequisites

The test and perf applications require [CMake](http://www.cmake.org) to
generate build files.

## Compiling and running

CMake is used to generate Makefiles that will build the C and C++
implementations of the Radix sort. There are scripts to aide with CMake
build generation in the build directory, for example:

~~~
cd build
./generate-linux64.sh
~~~

Running this script will create debug (unoptimised) and release (optimised)
build directories. Make needs to be invoked inside the debug or release
directory:

~~~
cd linux64-make/release
make
~~~

This will generate executables in bench and test subdirectories.

## License

This software is licensed under the zlib license, see the LICENSE file for
details.
