#ifndef __VTTY_PS_H__
#define __VTTY_PS_H__

#include "vtty.h"
#include "vtty_task.h"

class VttyPS : public VttyCommand
{
public:
    const std::string execute() override
    {
        // 忽略相关输入参数
        auto task_info{vtty->__vtty_task->get_task_info()};
        uint32_t killed_count{0};

        *vtty << "TID\tTask Name\tState\tCreate Time\tArguments\n"
              << std::endl;

        for (const auto &task_itme : task_info)
        {
            VttyTask::task_id_t task_id;                                    // 任务id
            std::string cmd_name;                                           // 命令名称
            std::string args;                                               // 命令参数
            std::chrono::time_point<std::chrono::system_clock> create_time; // 创建时间
            VttyTask::task_state_t task_state;                              // 任务状态

            std::tie(task_id, cmd_name, args, create_time, task_state) = task_itme;
            if (cmd_name == "ps")
                continue;
            *vtty << task_id << "\t" << cmd_name
                  << "\t " << VttyTask::state_to_str(task_state)
                  << "\t " << std::chrono::duration_cast<std::chrono::milliseconds>(create_time.time_since_epoch()).count()
                  << "\t" << args << "\n"
                  << std::endl;
            ++killed_count;
        }

        if (killed_count == 0)
        {
            return "No task is running.";
        }

        return "";
    }

    static const std::string help()
    {
        return R"(Show the running task list.

Usage:
    ps

Examples:
    ps
)";
    }

    using VttyCommand::VttyCommand; // inherit constructor
};

#endif // __VTTY_PS_H__
