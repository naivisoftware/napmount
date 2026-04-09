/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <utility/errorstate.h>
#include <string>
#include <utility/dllexport.h>

namespace nap
{
    namespace utility
    {
        namespace rc
        {
            constexpr int success = 0;          ///< syscall executed correct
            constexpr int openFailure = 10;     ///< syscall couldn't open pipe
        }

        /**
         * Perform a system call and capture the output
         * @param command The command to execute
         * @param outResponse The output of the command
         * @return command exit code, 10 if pipe could not be opened
         */
        NAPAPI int syscall(const std::string& command, std::string& outResponse);
    }
}