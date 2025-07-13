# SPDX-FileCopyrightText: 2024 Simon Gene Gottlieb
# SPDX-License-Identifier: CC0-1.0
cmake_minimum_required (VERSION 3.16)

project (csvtools LANGUAGES CXX C
                 VERSION 1.0.0)

include(cmake/CPM.cmake)
CPMAddPackage("gh:SGSSGene/cpm.dependencies@1.0.1")
CPMLoadDependenciesFile("${CMAKE_CURRENT_SOURCE_DIR}/cpm.dependencies")

add_subdirectory(src/csvtools)
