#ifndef __VTTY_BOOT_TIME_H__
#define __VTTY_BOOT_TIME_H__

#include "vtty.h"
#include "vtty_task.h"

class VttyBootTime : public VttyCommand
{
public:
    const std::string execute() override
    {
        if (args.size() == 1)
        {
            return "system startup " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) + " ms";
        }
        else if (args.size() == 2)
        {
            const std::string &arg = args[1];
            if (arg == "-ms")
            {
                return "system startup " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) + " ms";
            }
            else if (arg == "-s")
            {
                return "system startup " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()) + " s";
            }
            else if (arg == "-m")
            {
                return "system startup " + std::to_string(std::chrono::duration_cast<std::chrono::minutes>(std::chrono::system_clock::now().time_since_epoch()).count()) + " m";
            }
            else if (arg == "-h")
            {
                return "system startup " + std::to_string(std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count()) + " h";
            }
            else
            {
                return "Invalid argument.";
            }
        }
        return "Can only receive at most one parameter.";
    }

    static const std::string help()
    {
        return R"(Returns the system startup time.

Usage:
    btime [-ms|-s|-m|-h]

Options:
    -ms : milliseconds
    -s  : seconds
    -m  : minutes
    -h  : hours

Examples:
    btime
    btime -ms
    btime -s
    btime -m
    btime -h
)";
    }

    using VttyCommand::VttyCommand; // inherit constructor
};

#endif // !__VTTY_BOOT_TIME_H__