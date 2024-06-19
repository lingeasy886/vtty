#ifndef __VTTY_PRINT_H__
#define __VTTY_PRINT_H__

#include "vtty.h"

class VttyEcho : public VttyCommand
{
public:
    const std::string execute() override
    {
        vtty->println(args.at(1));
        return "";
    }

    static const std::string help()
    {
        return R"(Display the received data in the console.
If there are spaces in the output information, use double quotation marks.

Usage:
    echo [data]
    
Example:
    echo "Hello World!"
)";
    }

    using VttyCommand::VttyCommand; // inherit constructor
};

#endif // __VTTY_PRINT_H__