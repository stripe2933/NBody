//
// Created by gomkyung2 on 2023/09/04.
//

#pragma once

#include <optional>

namespace Dialog{
    template <typename ResultType>
    struct Dialog{
        bool is_open = false;

        virtual ~Dialog() noexcept = default;

        /**
         * Open dialog and get dialog result.
         * @param name Name of dialog, which is also used for ImGui identifier.
         * @return \p std::nullopt if dialog is waiting for user input or closed without result, otherwise \p std::optional of result.
         */
        virtual std::optional<ResultType> open(const char *name) = 0;
    };
};