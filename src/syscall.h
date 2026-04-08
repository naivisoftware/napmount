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