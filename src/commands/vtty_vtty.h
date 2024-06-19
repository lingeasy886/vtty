#ifndef __VTTY_INFO_H__
#define __VTTY_INFO_H__

#include "vtty.h"
#include "vtty_task.h"
#include "vtty_version.h"

class VttyVtty : public VttyCommand
{
public:
    const std::string execute() override
    {

        if (args.size() == 1)
        {
            return "VTTY VERSION " + std::string(VTTY_VERSION_STRING) + "\n" +
                   "VTTY VERCODE " + std::to_string(VTTY_VERSION) + "\n" +
                   "VTTY VERDATE " + VTTY_VERSION_DATE + "\n";
        }

        if (args[1] == "-version")
        {
            return VTTY_VERSION_STRING;
        }
        else if (args[1] == "-vercode")
        {
            return std::to_string(VTTY_VERSION);
        }
        else if (args[1] == "-verdate")
        {
            return VTTY_VERSION_DATE;
        }
        return "vtty: invalid arguments";
    }

    static const std::string help()
    {
        return R"(View the current version information of vtty.

Usage:
    vtty <-version|-vercode|-verdate>

Options:
    -version    ; view the current version information of vtty.
    -vercode    ; view the current version code of vtty.
    -verdate    ; view the current version date of vtty.

Examples:
    vtty    ; view the current version information of vtty.
    vtty -vercode    ; view the current version code of vtty.
    vtty -verdate    ; view the current version date of vtty.
)";
    }

    using VttyCommand::VttyCommand; // inherit constructor
};

#endif // __VTTY_INFO_H__
