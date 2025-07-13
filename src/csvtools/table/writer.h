// SPDX-FileCopyrightText: 2006-2023, Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2023, Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

//#include "../detail/concepts.h"
//#include "../detail/writer_base.h"
#include "ivio/detail/concepts.h"
#include "ivio/detail/writer_base.h"

#include "record.h"

#include <filesystem>
#include <functional>
#include <ostream>
#include <unordered_map>
#include <variant>

namespace ivio::table {

struct writer : writer_base<writer> {
    using record_view = table::record_view; //!doc: see record_writer_c<writer> concept

    struct config {
        // Source: file or stream
        std::variant<std::filesystem::path, std::reference_wrapper<std::ostream>> output;

        // How a line should start
        std::string linePrefix {"| "};

        // How a line should end
        std::string lineSuffix {" |"};

        // alternative suffix for specific rows
        std::unordered_map<size_t, std::string> lineAltSuffix;

        // How each entry should be separated
        std::string entrySeparator {" | "};

        // If the first entry is a header line
        bool firstLineHeader {false};
    };

    writer(config config);
    ~writer();

    //!doc: see record_writer_c<writer> concept
    void write(record_view record);

    //!doc: see record_writer_c<writer> concept
    void close();
};

static_assert(record_writer_c<writer>);

}
