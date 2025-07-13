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
auto cliCmd    = clice::Argument {
    .args   = "print",
    .desc   = "pretty print this csv table",
    .value  = std::vector<std::filesystem::path>{},
    .cb     = &app,
};
auto cliHeader = clice::Argument {
    .parent = &cliCmd,
    .args   = {"-h", "--header"},
    .desc   = "treats the first line as a header",
};
auto cliDelimiter = clice::Argument {
    .parent = &cliCmd,
    .args   = {"-d", "--delimiter"},
    .desc   = "delimiter used in input file",
    .value  = ',',
};
auto cliTranspose = clice::Argument {
    .parent = &cliCmd,
    .args   = {"-t", "--transpose"},
    .desc   = "transpose",
};
auto cliNoTrim = clice::Argument {
    .parent = &cliCmd,
    .args   = {"--no-trim"},
    .desc   = "do not trim white spaces around entries",
};
auto cliUseMapping = clice::Argument {
    .parent = &cliCmd,
    .args   = {"--mapping"},
    .desc   = "A CSV file, which defines mapping",
    .value  = std::filesystem::path{},
};
enum class OutputType { Table, CSV };
auto cliOutputType = clice::Argument {
    .parent = &cliCmd,
    .args   = {"--ot", "--output_type"},
    .desc   = "type of the output file. Available: table, csv",
    .value  = OutputType::Table,
    .mapping = {{
        {"table", OutputType::Table},
        {"csv",   OutputType::CSV},
    }},
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
                                         .delimiter = *cliDelimiter,
                                         .trim      = !cliNoTrim,
        }};

        auto values = std::vector<std::vector<std::string>>{};
        size_t width{};
        for (auto record : reader) {
            values.emplace_back(record.entries | std::ranges::to<std::vector>());
            width = std::max(width, values.back().size());
        }

        // make table quadtratic
        for (size_t y{0}; y < values.size(); ++y) {
            values[y].resize(width);
        }

        if (cliTranspose) {
            auto vec = std::vector<std::vector<std::string>>{};
            vec.resize(width, std::vector<std::string>(values.size()));
            for (size_t y{0}; y < values.size(); ++y) {
                for (size_t x{0}; x < width; ++x) {
                    vec[x][y] = values[y][x];
                }
            }
            std::swap(vec, values);
        }

        auto mapping = std::unordered_map<std::string, std::string>{};
        if (cliUseMapping) {
            auto reader = ivio::csv::reader{{
                .input = *cliUseMapping,
                .delimiter = ',',
                .trim = false,
            }};
            for (auto record : reader) {
                if (record.entries.size() == 2) {
                    mapping[record.entries[0]] = record.entries[1];
                }
            }

            for (size_t y{0}; y < values.size(); ++y) {
                for (size_t x{0}; x < values[y].size(); ++x) {
                    auto& v = values[y][x];
                    if (auto iter = mapping.find(v); iter != mapping.end()) {
                        v = iter->second;
                    }
                }
            }
        }

        if (*cliOutputType == OutputType::Table) {
            auto writer = ivio::table::writer {{
                .output = std::cout,
                .firstLineHeader = cliHeader,
            }};
            for (auto& record : values) {
                writer.write({
                    .entries = record,
                });
            }
        } else if (*cliOutputType == OutputType::CSV) {
            auto writer = ivio::table::writer {{
                .output = std::cout,
                .linePrefix      = "",
                .lineSuffix      = "",
                .entrySeparator  = ", ",
                .firstLineHeader = cliHeader,
            }};
            for (auto& record : values) {
                writer.write({
                    .entries = record,
                });
            }
        }
    }
}
}
