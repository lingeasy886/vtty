#ifndef __VTTY_SIGNAL_H__
#define __VTTY_SIGNAL_H__

#include "vtty_sig.h"
#include "vtty.h"

class VttySignal : public VttyCommand
{
public:
    const std::string execute() override
    {
        if (args.size() < 3)
        {
            return help(); // 参数长度不对，直接返回帮助信息
        }

        // 解析信号类型参数
        sig_type_t sig_tye{sig_type_t::msg};
        if (args[2] == "msg")
        {
            sig_tye = sig_type_t::msg;
        }
        else if (args[2] == "start")
        {
            sig_tye = sig_type_t::start;
        }
        else if (args[2] == "suspend")
        {
            sig_tye = sig_type_t::suspend;
        }
        else if (args[2] == "resume")
        {
            sig_tye = sig_type_t::resume;
        }
        else if (args[2] == "stop")
        {
            sig_tye = sig_type_t::stop;
        }
        else if (args[2] == "exit")
        {
            sig_tye = sig_type_t::exit;
        }
        else
        {
            return "Invalid signal name.";
        }

        std::string msg_data{args.size() == 4 ? args[3] : "" };

        std::string task_typ{args[1].substr(0, 2)};
        if (task_typ == "-i")
        {
            int task_id = std::stoi(args[1].substr(2));
            if (task_id < 1)
            {
                return "Invalid task id.";
            }
            if (!vtty->__vtty_task->task_exists(task_id))
            {
                return "Task not found.";
            }
            vtty->__vtty_task->sig_task(task_id, sig_tye, msg_data);
        }
        else if (task_typ == "-s")
        {
            std::string task_name = args[1].substr(2);
            if (task_name.empty())
            {
                return "Invalid task name :." + task_name;
            }
            auto task_id_list{vtty->__vtty_task->get_task_id_list(task_name)};
            if (task_id_list.empty())
            {
                return "Task not found.";
            }
            vtty->__vtty_task->sig_task(task_id_list, sig_tye, msg_data);
        }
        else
        {

            return "Invalid task id or name.";
        }
        return "";
    }

    static const std::string help()
    {
        return R"(Send a signal to an running task.

Usage:
    signal <-iTaskId|-sTaskName> <Signal Name> [message]

Options:
    <-iTaskId> :    The task id to signal.
    <-sTaskName> :  The task name to signal.
    <Signal Name> : The signal name to send.
    [message] :     The message to send with the signal.

Signal Name List:
    msg :      Message signal.
    start :    Start signal.
    suspend :  Suspend signal.
    resume :   Resume signal.
    stop :     Stop signal.
    exit :     Exit signal.

Example:
    signal -i1 msg "hello world"
    signal -sps msg "hello world"
    signal -s1 exit
)";
    }

    using VttyCommand::VttyCommand; // inherit constructor
};

#endif // __VTTY_SIGNAL_H__