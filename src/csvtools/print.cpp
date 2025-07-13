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
enum class OutputType { Table, CSV, Latex };
auto cliOutputType = clice::Argument {
    .parent = &cliCmd,
    .args   = {"--ot", "--output_type"},
    .desc   = "type of the output file. Available: table, csv, latex",
    .value  = OutputType::Table,
    .mapping = {{
        {"table", OutputType::Table},
        {"csv",   OutputType::CSV},
        {"latex", OutputType::Latex},
    }},
};
auto cliColumnOrder = clice::Argument {
    .parent = &cliCmd,
    .args   = {"--order"},
    .desc   = "order of the columns. If no columns given, ignore this parameter.",
    .value  = std::vector<std::string>{},
};
auto cliCustomPrint = clice::Argument {
    .parent = &cliCmd,
    .args   = {"--fmt"},
    .desc   = "custom format",
    .value  = std::vector<std::string>{},
};
auto cliFilterPrint = clice::Argument {
    .parent = &cliCmd,
    .args   = {"--filter"},
    .desc   = "format if filter fits",
    .value  = std::vector<std::string>{},
};
auto cliTransformPrint = clice::Argument {
    .parent = &cliCmd,
    .args   = {"--transform"},
    .desc   = "transforms the value",
    .value  = std::vector<std::string>{},
};

struct Rect {
    size_t startRow{0}, endRow{std::numeric_limits<size_t>::max()};
    size_t startCol{0}, endCol{std::numeric_limits<size_t>::max()};

    bool isInRange(size_t row, size_t col) const {
        bool valid = true;
        valid = valid && startRow <= row && row <= endRow;
        valid = valid && startCol <= col && col <= endCol;
        return valid;
    }
};

        // parse number
auto parseNumberRange(std::string s, size_t min, size_t max) -> std::tuple<size_t, size_t> {
    if (auto iter = s.find("-"); iter != std::string::npos) {
        auto start = s.substr(0, iter);
        size_t startI = min;
        auto end   = s.substr(iter+1);
        size_t endI = max;

        if (start.size()) {
            startI = std::stoi(start);
        }
        if (end.size()) {
            endI = std::stoi(end);
        }
        return {startI, endI};
    } else if (s.size()) {
        auto v = std::stoi(s);
        return {v, v};
    }
    return {0, max};
};


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

        // make table quadratic
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
            width = values.size();
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
        if (cliColumnOrder && cliColumnOrder->size() > 0) {
            auto ranges = std::vector<std::tuple<std::string, size_t, size_t>>{};
            for (auto e : *cliColumnOrder) {
                if (e == "id") {
                    ranges.emplace_back(e, 0, 1);
                    continue;
                }
                if (e.starts_with("id ")) {
                    auto start = std::stoi(e.substr(3));
                    ranges.emplace_back("id", start, 1);
                    continue;
                }
                auto [start, end] = parseNumberRange(e, 0, width-1);
                if (start >= width || end >= width) {
                    throw std::runtime_error{fmt::format("invalid order range {}-{} max value allowed is {}", start, end, width-1)};
                }
                ranges.emplace_back("range", start, end);
            }

            auto vec = std::vector<std::vector<std::string>>{};
            for (size_t row{}; row < values.size(); ++row) {
                auto const& in_entries = values[row];
                auto out_entries = std::vector<std::string>{};
                for (auto [type, start, end] : ranges) {
                    if (type == "id") {
                        out_entries.push_back(std::to_string(row*end + start));
                    } else if (type == "range") {
                        for (; start <= end; ++start) {
                            out_entries.push_back(in_entries[start]);
                        }
                    }
                }
                vec.emplace_back(std::move(out_entries));
            }
            std::swap(vec, values);
        }

        auto parseNextF = [](std::string& s) {
            auto iter = s.find(":");
            if (iter == std::string::npos) {
                throw std::runtime_error{"filter must be of format: \"<rnr>:<cnr>:<filter>:<fmt>\""};
            }
            auto ret = s.substr(0, iter);
            s = s.substr(iter+1);
            return ret;
        };



        if (cliTransformPrint) {
            using Transform = std::function<std::string(size_t row, size_t col, std::string)>;
            auto transformMap = std::vector<std::tuple<Rect, Transform>>{};
            for (auto l : *cliTransformPrint) {
                auto rowsStr      = parseNextF(l);
                auto colsStr      = parseNextF(l);
                auto transformStr = l;

                // parse which columns to apply this to
                auto [rstart, rend] = parseNumberRange(rowsStr, 0, std::numeric_limits<size_t>::max());
                auto [cstart, cend] = parseNumberRange(colsStr, 0, width-1);
                auto rect = Rect {
                    rstart, rend,
                    cstart, cend,
                };


                auto filter = [=](size_t row, size_t col, std::string s) {
                    if (transformStr.starts_with("scale ")) {
                        auto per = std::stod(transformStr.substr(6));
                        auto v   = std::stod(s);
                        s = fmt::format("{}", v*per);
                    }
                    return s;
                };

                transformMap.emplace_back(rect, filter);
            }

            for (size_t row{0}; row < values.size(); ++row) {
                auto& entries = values[row];
                for (size_t col{0}; col < entries.size(); ++col) {
                    auto& e = entries[col];
                    for (auto [rect, transform] : transformMap) {
                        if (!rect.isInRange(row, col)) continue;
                        try {
                            e = transform(row, col, e);
                        } catch(...){}
                    }
                }
            }
        }

        auto cachedValues = std::unordered_map<std::string, double>{};

        auto cachedValue_fetch_or_run = [&](std::string id, auto cb) {
            auto iter = cachedValues.find(id);
            if (iter == cachedValues.end()) {
                auto v = cb();
                cachedValues[id] = v;
                iter = cachedValues.find(id);
            }
            return iter->second;
        };
        auto cachedValue_fetch_or_cacc = [&](size_t col, size_t start, size_t end, auto init, auto bin) {
            for (size_t i{start}; i <= end; ++i) {
                auto& entries = values[i];
                auto v = std::stod(entries[col]);
                init = bin(v, init);
            }
            return init;
        };

        auto cachedValue_fetch_or_racc = [&](size_t row, size_t start, size_t end, auto init, auto bin) {
            for (size_t i{start}; i < std::min(values[row].size(), end); ++i) {
                auto const& e = values[row][i];
                auto v = std::stod(e);
                init = bin(v, init);
            }
            return init;
        };

        auto cachedValue_cmin = [&](size_t col, size_t start, size_t end) -> double {
            return cachedValue_fetch_or_run(fmt::format("cmin{}:{}-{}", col, start, end), [&]() {
                return cachedValue_fetch_or_cacc(col, start, end, std::numeric_limits<double>::max(), [](double lhs, double rhs) {
                    return std::min(lhs, rhs);
                });
            });
        };

        auto cachedValue_cmax = [&](size_t col, size_t start, size_t end) -> double {
            return cachedValue_fetch_or_run(fmt::format("cmax{}:{}-{}", col, start, end), [&]() {
                return cachedValue_fetch_or_cacc(col, start, end, std::numeric_limits<double>::lowest(), [](double lhs, double rhs) {
                    return std::max(lhs, rhs);
                });
            });
        };

        auto cachedValue_rmin = [&](size_t row, size_t start, size_t end) -> double {
            return cachedValue_fetch_or_run(fmt::format("rmin{}:{}-{}", row, start, end), [&]() {
                return cachedValue_fetch_or_racc(row, start, end, std::numeric_limits<double>::max(), [](double lhs, double rhs) {
                    return std::min(lhs, rhs);
                });
            });
        };

        auto cachedValue_rmax = [&](size_t row, size_t start, size_t end) -> double {
            return cachedValue_fetch_or_run(fmt::format("rmax{}:{}-{}", row, start, end), [&]() {
                return cachedValue_fetch_or_racc(row, start, end, std::numeric_limits<double>::lowest(), [](double lhs, double rhs) {
                    return std::max(lhs, rhs);
                });
            });
        };

        if (cliFilterPrint) {
            using Filter = std::function<std::string(size_t row, size_t col, std::string)>;
            auto filterMap = std::vector<std::tuple<Rect, Filter>>{};
            for (auto l : *cliFilterPrint) {
                auto rowsStr   = parseNextF(l);
                auto colsStr   = parseNextF(l);
                auto filterStr = parseNextF(l);
                auto suffix    = l;

                // parse which columns to apply this to
                auto [rstart, rend] = parseNumberRange(rowsStr, 0, std::numeric_limits<size_t>::max());
                auto [cstart, cend] = parseNumberRange(colsStr, 0, width-1);
                auto rect = Rect {
                    rstart, rend,
                    cstart, cend,
                };


                auto filter = [=](size_t row, size_t col, std::string s) {
                    if (filterStr == "true") {
                        s = fmt::format(fmt::runtime(suffix), s);
                    }
                    if (filterStr == "cmin") {
                        if (std::stod(s) == cachedValue_cmin(col, rstart, rend)) {
                            s = fmt::format(fmt::runtime(suffix), s);
                        }
                    }
                    if (filterStr == "cmax") {
                        if (std::stod(s) == cachedValue_cmax(col, rstart, rend)) {
                            s = fmt::format(fmt::runtime(suffix), s);
                        }
                    }
                    if (filterStr.starts_with("cmin ")) {
                        auto per = std::stod(filterStr.substr(5));
                        if (std::stod(s)*per <= cachedValue_cmin(col, rstart, rend)) {
                            s = fmt::format(fmt::runtime(suffix), s);
                        }
                    }
                    if (filterStr.starts_with("cmax ")) {
                        auto per = std::stod(filterStr.substr(5));
                        if (std::stod(s) >= cachedValue_cmax(col, rstart, rend)*per) {
                            s = fmt::format(fmt::runtime(suffix), s);
                        }
                    }
                    if (filterStr == "rmin") {
                        if (std::stod(s) == cachedValue_rmin(row, cstart, cend)) {
                            s = fmt::format(fmt::runtime(suffix), s);
                        }
                    }
                    if (filterStr == "rmax") {
                        if (std::stod(s) == cachedValue_rmax(row, cstart, cend)) {
                            s = fmt::format(fmt::runtime(suffix), s);
                        }
                    }
                    if (filterStr.starts_with("rmin ")) {
                        auto per = std::stod(filterStr.substr(5));
                        if (std::stod(s)*per <= cachedValue_rmin(row, cstart, cend)) {
                            s = fmt::format(fmt::runtime(suffix), s);
                        }
                    }
                    if (filterStr.starts_with("rmax ")) {
                        auto per = std::stod(filterStr.substr(5));
                        if (std::stod(s) >= cachedValue_rmax(row, cstart, cend)*per) {
                            s = fmt::format(fmt::runtime(suffix), s);
                        }
                    }
                    if (filterStr == "float") {
                        auto v = std::stod(s);
                        s = fmt::format(fmt::runtime(suffix), v);
                    }

                    return s;
                };

                filterMap.emplace_back(rect, filter);
            }

            for (size_t row{0}; row < values.size(); ++row) {
                auto& entries = values[row];
                for (size_t col{0}; col < entries.size(); ++col) {
                    auto& e = entries[col];
                    for (auto [rect, filter] : filterMap) {
                        if (!rect.isInRange(row, col)) continue;
                        try {
                            e = filter(row, col, e);
                        } catch(...){}
                    }
                }
            }
        }



        if (cliCustomPrint) {
            auto customFmt = std::unordered_map<size_t, std::string>{};
            for (auto l : *cliCustomPrint) {
                auto iter = l.find(":");
                if (iter == std::string::npos) {
                    throw std::runtime_error{"custom format must be of format: \"<nr>:<fmt>\""};
                }
                auto prefix = l.substr(0, iter);
                auto suffix = l.substr(iter+1);
                if (prefix.size()) {
                    auto col = std::stoi(prefix);
                    customFmt[col] = suffix;
                } else {
                    for (size_t i{0}; i < width; ++i) {
                        customFmt[i] = suffix;
                    }
                }
            }
            for (auto& entries : values) {
                for (size_t col{0}; col < entries.size(); ++col) {
                    auto& e = entries[col];
                    if (auto iter = customFmt.find(col); iter != customFmt.end()) {
                        e = fmt::format(fmt::runtime(iter->second), e);
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
        } else if (*cliOutputType == OutputType::Latex) {
            auto writer = ivio::table::writer {{
                .output = std::cout,
                .linePrefix      = "",
                .lineSuffix      = "\\\\",
                .entrySeparator  = " & ",
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
