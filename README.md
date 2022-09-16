# cpp-search-server

## Description

The search server provides parsing of search queries, document search, sorting of results by specified criteria, removal of duplicates. Server operation is accelerated due to multithreading and the use of the std::string_view class.

## Instructions for use

Download the archive with the project to the local directory of your PC. It is necessary to have a C++ compiler (GCC, Clang, etc.) or an IDE with C++ support.  We recommend using the latest versions of compilers or IDE. 

To compile a project and build an executable file (using the GCC compiler as an example), run on the command line:
```
g++ *.cpp –o server
```
Next, run the executable file "server" on the command line.

An example of using a search server (adding documents, searching by specified criteria, removing duplicates, etc.) is contained in the "main" file. If necessary, delete the lines with examples or comment out.

## System requirements

1. GCC version at least 11.2.0 (or another C++ compiler).

2. The C++17 standard.

## Plans for completion

1. Create a graphical interface

2. Implement parsing of websites on the Internet

## Technology stack

1. GCC 11.2.0

2. C++17 (STL)