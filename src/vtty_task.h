#ifndef __VTTY_TASK_H__
#define __VTTY_TASK_H__

#include "vtty_command_impl.h"
#include "vtty_sig.h"

#include <list>
#include <tuple>
#include <string>
#include <condition_variable>
#include <type_traits>
#include <functional>
#include <future>
#include <queue>
#include <chrono>

class Vtty;

class VttyTask
{
    friend class Vtty;

public:
    enum class task_state_t
    {
        RUNNING,   // 任务正在执行
        WAITING,   // 任务等待执行，还没有被调用时
        COMPLETED, // 任务正常结束
        DELETED,   // 删除的任务
        DEATHED,   // 任务死亡，异常的线程执行，目前并没有实现相关的功能
    };

    using task_id_t = uint32_t; // 任务ID别名

    /// @brief 用户执行命令时的类型
    /// @note 参数分别是args、detached、task_id
    using vcmd_create_t = std::function<VttyCommand *(const std::vector<std::string>, const uint32_t)>;

    /// @brief 用户在功能类中实现的帮助函数
    using vcmd_help_t = std::function<const std::string()>;

public:
    VttyTask(Vtty *vtty, const int max_task_count = 20);
    ~VttyTask();

    /// @brief 命令描述
    struct cmd_desc_t
    {
        std::string name;   // 命令名称
        vcmd_help_t help;   // 帮助信息
        vcmd_create_t vcmd; // 命令对象工厂函数

        cmd_desc_t(std::string _name, vcmd_create_t _vcmd, vcmd_help_t _help)
            : name(_name), vcmd(_vcmd), help(_help){};
    };

    struct task_info_t
    {
        cmd_desc_t &cmd_desc; // 命令描述

        /// @brief 在任务被创建的时候，不会立刻生成，会在调用时生成此任务对象，减少对内存的消耗
        VttyCommand *cmd{nullptr};                 // 命令对象
        std::string args_raw;                      // 命令参数（用户输入值）
        task_state_t state{task_state_t::WAITING}; // 任务状态

        std::queue<std::pair<sig_type_t, std::string>> sig_queue; // 信号队列

        /// @brief 记录任务信息
        /// @param _task_id 任务ID
        /// @param _cmd_desc 用户注册的命令描述
        /// @param _args_raw 用户输出的原始数据
        /// @param _args_vec 将参数进行拆分后的参数数据
        task_info_t(task_id_t _task_id, cmd_desc_t &_cmd_desc,
                    const std::string &_args_raw, const std::vector<std::string> &_args_vec, bool _detached)
            : cmd_desc(_cmd_desc), args_raw(_args_raw)
        {
            cmd = _cmd_desc.vcmd(_args_vec, _task_id);
        }

        /// @brief 拷贝构造函数
        task_info_t(task_info_t &&other)
            : cmd_desc(other.cmd_desc),
              args_raw(other.args_raw),
              state(other.state),
              sig_queue(other.sig_queue),
              cmd(other.cmd)
        {
            other.cmd = nullptr;
        }

        /// @brief 析构函数
        /// @details 释放命令对象
        ~task_info_t()
        {
            if (cmd)
            {
                delete cmd;
                cmd = nullptr;
            }
        }
    };

    /// @brief 创建新任务
    /// @param cmd_desc 命令描述
    /// @return 是否增加成功
    const uint32_t new_task(cmd_desc_t &cmd_desc, const std::string args_raw, const std::vector<std::string> &args);

    /// @brief 删除任务
    /// @param task_id 任务id
    /// @return false表示删除失败
    const bool del_task(const task_id_t task_id);

    /// @brief 删除任务
    /// @param cmd_name 命令名称
    /// @return 任务id，空表示没有可删除的目标任务
    const std::vector<task_id_t> del_task(const std::string &cmd_name);

    /// @brief 获取任务id列表
    /// @param cmd_name 命令名称
    /// @return 任务id列表
    const std::vector<task_id_t> get_task_id_list(const std::string &cmd_name);

    /// @brief 发送消息到指定任务中
    /// @param task_id_list 任务id序列
    /// @param msg_type 消息类型
    /// @param msg 消息内容
    const void sig_task(const std::vector<task_id_t> &task_id_list, const sig_type_t msg_type, const std::string &msg);

    /// @brief 发送消息到指定任务中
    /// @param task_id 任务id
    /// @param msg_type 消息类型
    /// @param msg 消息内容
    const void sig_task(const task_id_t task_id, const sig_type_t msg_type, const std::string &msg);

    /// @brief 获取任务信息
    /// @return std::tuple<task_id, cmd_name, args, create_time, task_state_t>
    /// @note   task_id: 任务id
    /// @note   cmd_name: 命令名称
    /// @note   args: 命令参数
    /// @note   create_time: 是否在独立线程中运行
    /// @note   task_state: 任务状态
    const std::vector<std::tuple<task_id_t, std::string, std::string, std::chrono::time_point<std::chrono::system_clock>, task_state_t>> get_task_info();

    /// @brief 查询指定的任务是否正在运行
    /// @param cmd_name 命令名称
    /// @return true/false
    bool task_exists(const std::string &cmd_name);

    /// @brief 查询指定的任务是否正在运行
    /// @param task_id 命令id
    /// @return true/false
    bool task_exists(const task_id_t task_id);

    /// @brief 将运行状态转换成字符串输出
    /// @param state 任务状态
    static const std::string state_to_str(task_state_t state);

private:
#if __cplusplus >= 201703L
    inline static uint32_t __task_count{0}; // 任务ID，所有VTTY共享一个task计数
#else
    static uint32_t __task_count; // 任务ID，所有VTTY共享一个task计数
#endif

    const int __max_task_count{20}; // 最大任务数
    bool __running{true};

    std::mutex __task_mtx;              // 任务锁
    std::condition_variable __task_cv;  // 循环处理条件变量
    std::list<task_info_t> __task_list; // 所有任务列表

    Vtty *__vtty{nullptr}; // 创建任务管理的发起者

    /// @brief 任务管理循环
    void __handler();

    /// @brief 任务模板
    /// @param task 任务描述
    void __task_template(task_info_t &task);
};

#endif // __VTTY_TASK_H__
