/*
 * Copyright (C) 2023 Eduardo Rocha
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <bitset>
#include <cstdint>

#include "3rd_party/better_enums/enums.h"

namespace capybot
{
namespace bnet
{

template <typename Enum>
constexpr Enum max_loop(Enum accumulator, size_t index)
{
    return index >= Enum::_size()                 ? accumulator
           : Enum::_values()[index] > accumulator ? max_loop<Enum>(Enum::_values()[index], index + 1)
                                                  : max_loop<Enum>(accumulator, index + 1);
}

template <typename Enum>
constexpr Enum max()
{
    return max_loop<Enum>(Enum::_values()[0], 1);
}

BETTER_ENUM(ActionExecutionStatus, uint32_t,
            // Completed
            SUCCESS = 0, // action completed successfully, or token is in a passive place
            FAILURE,     // action completed with failure
            ERROR,       // an error occurred when executing the action

            // (likely) In progress
            IN_PROGRESS,    // action is still in progress, it did not finish in the current epoch
            QUERRY_TIMEOUT, // failure to get action status, i.e., action impl did not return status within the epoch
            NOT_STARTED     // action callable task is in the thread pool execution queue, but it has not started yet
)

using ActionExecutionStatusSet = std::bitset<max<ActionExecutionStatus>()._to_integral() + 1>;

struct ActionExecutionResult
{
    uint64_t tokenId; // TODO: ptr or iterator
    ActionExecutionStatus status;
};

} // namespace bnet
} // namespace capybot
