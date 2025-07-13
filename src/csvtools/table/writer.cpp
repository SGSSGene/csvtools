// SPDX-FileCopyrightText: 2006-2023, Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2023, Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause
//#include "../detail/buffered_writer.h"
//#include "../detail/file_writer.h"
//#include "../detail/stream_writer.h"
//#include "../detail/zlib_file_writer.h"
#include "ivio/detail/buffered_writer.h"
#include "ivio/detail/file_writer.h"
#include "ivio/detail/stream_writer.h"
#include "ivio/detail/zlib_file_writer.h"

#include "writer.h"

#include <cassert>
#include <fmt/format.h>

namespace ivio {

template <>
struct writer_base<table::writer>::pimpl {
    using Writers = std::variant<file_writer,
                                 buffered_writer<zlib_file_writer>,
                                 stream_writer,
                                 buffered_writer<zlib_stream_writer>
                                 >;

    Writers writer;

    std::string linePrefix;
    std::string lineSuffix;
    std::string entrySeparator;
    bool firstLineHeader;

    std::vector<table::record> records;
    std::vector<size_t> longestEntry;

    std::string buffer;
    pimpl(std::filesystem::path output, std::string linePrefix, std::string lineSuffix, std::string entrySeparator, bool firstLineHeader)
        : writer {[&]() -> Writers {
            return file_writer{output};
        }()}
        , linePrefix {linePrefix}
        , lineSuffix {lineSuffix}
        , entrySeparator {entrySeparator}
        , firstLineHeader {firstLineHeader}
    {}

    pimpl(std::ostream& output, std::string linePrefix, std::string lineSuffix, std::string entrySeparator, bool firstLineHeader)
        : writer {[&]() -> Writers {
            return stream_writer{output};
        }()}
        , linePrefix {linePrefix}
        , lineSuffix {lineSuffix}
        , entrySeparator {entrySeparator}
        , firstLineHeader {firstLineHeader}
    {}
};

}

namespace ivio::table {

writer::writer(config config_)
    : writer_base{std::visit([&](auto& p) {
        return std::make_unique<pimpl>(p,
                                       config_.linePrefix,
                                       config_.lineSuffix,
                                       config_.entrySeparator,
                                       config_.firstLineHeader
        );
    }, config_.output)}
{
}

writer::~writer() {
    close();
}

void writer::write(record_view record) {
    assert(pimpl_);
    auto& records      = pimpl_->records;
    auto& longestEntry = pimpl_->longestEntry;

    records.emplace_back(record);
    longestEntry.resize(std::max(longestEntry.size(), records.back().entries.size()));
    for (size_t i{0}; i < records.back().entries.size(); ++i) {
        longestEntry[i] = std::max(longestEntry[i], records.back().entries[i].size());
    }
}

void writer::close() {
    if (pimpl_) {
        auto& records      = pimpl_->records;
        auto& longestEntry = pimpl_->longestEntry;

        std::visit([&](auto& writer) {
            auto buffer = std::string{};
            for (size_t i{0}; i < records.size(); ++i) {
                buffer.clear();
                auto& record = records[i];
                buffer = pimpl_->linePrefix;
                for (size_t j{0}; j < record.entries.size(); ++j) {
                    if (j > 0) buffer += pimpl_->entrySeparator;
                    buffer += fmt::format("{: >{}}", record.entries[j], longestEntry[j]);
                }
                buffer += pimpl_->lineSuffix;
                buffer += '\n';

                if (i == 0 && pimpl_->firstLineHeader) {
                    buffer += pimpl_->linePrefix;

                    for (size_t j{0}; j < record.entries.size(); ++j) {
                        if (j > 0) buffer += pimpl_->entrySeparator;
                        buffer += fmt::format("{:->{}}", "", longestEntry[j]);
                    }
                    buffer += pimpl_->lineSuffix;
                    buffer += '\n';
                }

                writer.write(buffer);
            }
        }, pimpl_->writer);
    }

    pimpl_.reset();
}

static_assert(record_writer_c<writer>);

}
