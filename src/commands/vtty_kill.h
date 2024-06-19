#ifndef __VTTY_KILL_H__
#define __VTTY_KILL_H__

#include "vtty.h"

#include <charconv>

/// @note 实现vtty kill命令
/// @note 参数说明：
/// @note -i task_id    ;指定要杀死的任务id
/// @note -s task_name  ;指定要杀死的任务名
/// @note [null]        ; 会优先尝试使用-i方式杀死任务
class VttyKill : public VttyCommand
{
public:
    const std::string execute() override
    {
        // 如果参数少于3，将认为是错误指令，不进行更新
        if (args.size() != 3)
        {
            return "The parameter input format is illegal. Please refer to the help.";
        }

        // 处理数据类型的输入
        if (args.at(1) == "-i")
        {
            std::string task_id_str{};
            task_id_str = args.at(2);
            uint32_t task_id{UINT32_MAX};
#if __cplusplus >= 201402L
            std::from_chars(args.at(1).data(), args.at(1).data() + args.at(1).size(), task_id);
#else
            if (!task_id_str.empty())
            {
                for (char const &c : task_id_str)
                {
                    if (!std::isdigit(c))
                    {
                        goto KILL_TASK_ID_NULL; // 如果字符串为非数字，跳转到KILL_TASK_ID_NULL标签处
                    }
                }
                task_id = std::atoi(task_id_str.c_str());
            }
        KILL_TASK_ID_NULL:
#endif
            if (task_id == UINT32_MAX)
            {
                return "The parameter input format is illegal. Please refer to the help.";
            }
            if (vtty->__vtty_task->del_task(task_id))
            {
                return "The task with id " + std::to_string(task_id) + " is killed.";
            }
            return "The task with id " + std::to_string(task_id) + " is not exist.";
        }
        // 处理字符类型的输入
        if (args.at(1) == "-s")
        {
            std::string task_name{args.at(2)};
            // 删除字符串中可能出现的空格符号
            if (task_name.at(0) == '"' && task_name.at(task_name.size() - 1) == '"')
            {
                task_name = task_name.substr(1, task_name.size() - 2);
            }

            if (task_name.empty())
            {
                return "Missing parameters.";
            }
            // 尝试删除任务
            auto kill_task_result{vtty->__vtty_task->del_task(task_name)};
            if (kill_task_result.empty())
            {
                return "No tasks were killed.";
            }
            *vtty << "The following tasks have been killed:";
            for (const auto &value : kill_task_result)
            {
                *vtty << value << " ";
            }
            *vtty << "\n" << std::endl;

            return "";
        }
        return "The parameter input format is illegal. Please refer to the help.";
    }

    static const std::string help()
    {
        return R"(Kill a task by task_id or task_name.

Usage: 
    kill [-i task_id] [-s task_name]

Options:
    -i task_id    ;Specify the task ID to be killed
    -s task_name  ;Specify the task name to be killed

Examples:
    kill -i 123
    kill -s "my_task"
)";
    }

    using VttyCommand::VttyCommand; // inherit constructor
};

#endif // __VTTY_KILL_H__
