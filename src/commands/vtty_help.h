#ifndef __VTTY_HELP_H__
#define __VTTY_HELP_H__

#include "vtty_command_impl.h"

class VttyHelp : public VttyCommand
{
public:
    const std::string execute() override
    {
        // 如果没有参数
        if (args.size() == 1)
        {
            // 打印全部内容
            auto cmd_lis{vtty->command_list()};
            *vtty << "No.\tCommand Name" << "\n"
                  << std::endl;
            int count{1};
            for (auto &cmd : cmd_lis)
            {
                *vtty << count++ << '\t' << cmd << '\n';
            }
            *vtty << std::endl; // 输出结果
            return "";
        }
        // 如果参数超出指定数据
        if (args.size() > 2)
        {
            return "Wrong parameter, can only receive one parameter at a time.";
        }
        // 打印指定函数的文档
        if (args.at(1) == "help")
        {
            return help();
        }
        // 打印指定函数的帮助
        if (vtty->command_exist(args.at(1)))
        {
            return vtty->command_help(args.at(1));
        }
        return args.at(1) + " is not a command.";
    }

    static const std::string help()
    {
        return R"(Print help information of the command.

Note:
    If no command name is given, print all commands.

Usage:
    help [command name]

Example:
    help
    help kill
    help btime
)";
    }

    using VttyCommand::VttyCommand; // inherit constructor
};

#endif // !__VTTY_HELP_H__
