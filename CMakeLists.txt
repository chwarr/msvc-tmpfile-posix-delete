# Copyright (c) 2026 Microsoft
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 4.0)
project("msvc-tmpfile-posix-delete")

add_library(wil INTERFACE IMPORTED)
target_include_directories(wil SYSTEM INTERFACE "wil/include")
add_library(WIL::WIL ALIAS wil)

add_executable("msvc-tmpfile-posix-delete")
target_sources(
    "msvc-tmpfile-posix-delete"
    PRIVATE main.cpp
    PRIVATE FILE_SET HEADERS FILES "NtSetInformationFile.h")    
target_compile_features("msvc-tmpfile-posix-delete" PRIVATE cxx_std_20)
target_compile_definitions("msvc-tmpfile-posix-delete" PRIVATE NO_MIN_MAX WIN32_LEAN_AND_MEAN)
target_link_libraries("msvc-tmpfile-posix-delete" PRIVATE WIL::WIL)
