// SPDX-FileCopyrightText: 2023 Gottlieb+Freitag <info@gottliebtfreitag.de>
// SPDX-License-Identifier: CC0-1.0
#include <clice/clice.h>
#include <iostream>

namespace {
auto cliHelp = clice::Argument{ .args   = "--help",
                                .desc   = "prints the help page",
                                .cb     = []{ std::cout << clice::generateHelp(); exit(0); },
};
}

int main(int argc, char** argv) {
    if (auto failed = clice::parse(argc, argv); failed) {
        std::cerr << "parsing failed: " << *failed << "\n";
        return 1;
    }
    return 0;
}
