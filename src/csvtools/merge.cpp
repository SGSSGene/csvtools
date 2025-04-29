// SPDX-FileCopyrightText: 2023 Gottlieb+Freitag <info@gottliebtfreitag.de>
// SPDX-License-Identifier: CC0-1.0
#include <clice/clice.h>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/std.h>
#include <ivio/csv/reader.h>
#include <ivio/csv/writer.h>
#include "table/writer.h"
#include <iostream>

namespace {
void app();
auto cliCmd    = clice::Argument{ .args   = "merge",
                                  .desc   = "merges same sized csv tables",
                                  .value  = std::vector<std::filesystem::path>{},
                                  .cb     = &app,
};

auto cliMergeMode = clice::Argument{ .parent = &cliCmd,
                                     .desc   = "select mode (min, max)",
                                     .value  = std::string{"min"}
};

void app() {
    if (cliCmd->size() == 0) {
        fmt::print("No files given\n");
        return;
    }
    using File = std::vector<ivio::csv::record>;
    auto files = std::vector<File>{};

    // load all files
    for (auto p : *cliCmd) {
        auto reader = ivio::csv::reader{{.input = p,
                                         .delimiter = ','
        }};
        files.emplace_back();
        for (auto record : reader) {
            files.back().push_back(record);
        }
    }

    //
    auto mergeFunc = std::function<void(std::string&, std::string const&)>{};
    mergeFunc = [](std::string& current, std::string const& test) {
        current = std::min(current, test);
    };

    // merge down to smallest value
    for (auto const& file : files) {
        for (size_t row{0}; row < file.size(); ++row) {
            for (size_t col{0}; col < file[row].entries.size(); ++col) {
                auto const& entry = file[row].entries[col];
                auto& baseEntry = files[0][row].entries[col];
                mergeFunc(baseEntry, entry);
            }
        }
    }


    {
        auto writer = ivio::csv::writer{{.output = std::cout,
        }};

        for (auto record : files[0]) {
            writer.write({
                .entries = record.entries,
            });
        }
    }
}
}
