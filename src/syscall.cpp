#include "syscall.h"

// Pipe open / close macros
#ifdef __linux__
	#define POPEN(cmd)		popen(cmd, "r");
	#define PCLOSE(pipe)	pclose(pipe);
#else
	#define POPEN(cmd)		_popen(cmd, "r")
	#define PCLOSE(pipe)	_pclose(pipe)
#endif

namespace nap
{
    namespace utility
    {
        int syscall(const std::string& command, std::string& outResponse)
		{
			// Run cmd and open pipe
			FILE* pipe = POPEN(command.c_str());
            if(pipe == nullptr)
            	return rc::openFailure;

            // Read from stdout
            outResponse.clear(); char buffer[128];
            while(fgets(buffer, sizeof(buffer), pipe) != nullptr)
                outResponse += buffer;

            // Erase newline
            if(!outResponse.empty())
                outResponse.erase(std::remove(outResponse.end() - 1, outResponse.end(), '\n'), outResponse.cend());

			// Close pipe and get process wait exit code
			int rc = PCLOSE(pipe);
#ifdef __linux__
			if (WIFEXITED(rc))
				rc = WEXITSTATUS(rc);
#endif
        	return rc;
        }
    }
}
