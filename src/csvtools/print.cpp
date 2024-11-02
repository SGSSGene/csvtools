// SPDX-FileCopyrightText: 2023 Gottlieb+Freitag <info@gottliebtfreitag.de>
// SPDX-License-Identifier: CC0-1.0
#include <clice/clice.h>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/std.h>
#include <ivio/csv/reader.h>
#include "table/writer.h"
#include <iostream>

namespace {
void app();
auto cliCmd    = clice::Argument{ .args   = "print",
                                  .desc   = "pretty print this csv table",
                                  .value  = std::vector<std::filesystem::path>{},
                                  .cb     = &app,
};

void printLatexTable(std::vector<std::string> entries) {
}

void app() {
    if (cliCmd->size() == 0) {
        fmt::print("No files given\n");
        return;
    }
    for (auto p : *cliCmd) {
        auto reader = ivio::csv::reader{{.input = p,
                                         .delimiter = ','
        }};

        auto writer = ivio::table::writer{{.output = std::cout,
                                           .firstLineHeader = true,
        }};

        for (auto record : reader) {
            writer.write({
                .entries = record.entries,
            });
        }
    }
}
}
