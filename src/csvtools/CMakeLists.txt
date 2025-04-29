# SPDX-FileCopyrightText: 2024 Simon Gene Gottlieb
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required (VERSION 3.25)

add_executable(csvtools
    main.cpp
    print.cpp
    merge.cpp
    table/writer.cpp
)
target_link_libraries(csvtools
    PUBLIC
    clice::clice
    fmt::fmt
    ivio::ivio
)
target_compile_features(csvtools PUBLIC cxx_std_20)
