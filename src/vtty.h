#ifndef __VTTY_H__
#define __VTTY_H__

#include "vtty_io.h"
#include "vtty_task.h"
#include "vtty_cmd_info.h"

#include <string>
#include <functional>
#include <vector>
#include <list>
#include <iostream>
#include <sstream>
#include <type_traits>

class VttyTask;
class VttyCommand;
class VttyIOImpl;
class Vtty
{
public:
    /// @brief 构造函数
    /// @param vtty_io_impl 输入输出接口
    /// @param detach_mode 是否使用分离模式，分离模式下，vtty不会启动处理，需要外部调用handler来处理vtty的输入输出
    Vtty(VttyIOImpl &vtty_io_impl, bool detach_mode = true);
    virtual ~Vtty();

    Vtty() = delete; // 禁止默认构造函数

    /// @brief 禁止拷贝和赋值
    Vtty(const Vtty &) = delete;
    Vtty &operator=(const Vtty &) = delete;
    Vtty(Vtty &&) = delete;
    Vtty &operator=(Vtty &&) = delete;

    /// @brief 增加一个处理过程，该处理过程会绑定一个命令，当命令被触发时，会执行该处理过程
    /// @param cmd_str 命令的名称
    /// @param cmd_fun 命令的处理过程，由用户自己实现，bind时实例化，ubind时销毁
    /// @param help 命令的帮助信息，使用help查看时，显示此部分信息
    /// @return 是否成功
    const bool bind_command(const std::string &cmd_str, const VttyTask::vcmd_create_t cmd_fun, const VttyTask::vcmd_help_t help);

    template <typename _CMD_TYP,
              typename = std::enable_if<std::is_base_of<VttyCommand, _CMD_TYP>::value>>
    const bool bind_command(const std::string &cmd_str)
    {
        VttyTask::vcmd_create_t cmd_fun = [this](const std::vector<std::string> args_, uint32_t task_id_) -> VttyCommand *
        {
            return dynamic_cast<VttyCommand *>(new _CMD_TYP(this, args_, task_id_));
        };
        return bind_command(cmd_str, cmd_fun, std::bind(&_CMD_TYP::help));
    }

    /// @brief 增加一个命令列表
    /// @param _CMD_IDX 命令列表索引
    /// @param Args 命令类型
    /// @param cmd_tpl 命令列表
    template <std::size_t _CMD_IDX = 0, typename... Args>
    void bind_command(const std::tuple<Args...> &cmd_tpl)
    {
        if constexpr (_CMD_IDX < sizeof...(Args))
        {
            auto &cmd_info = std::get<_CMD_IDX>(cmd_tpl);
            using CmdType = typename std::remove_reference<decltype(cmd_info)>::type::cmd_type;
            bind_command<CmdType>(cmd_info.name);
            bind_command<_CMD_IDX + 1>(cmd_tpl);
        }
    }

    /// @brief 解除一个处理过程，该处理过程会绑定一个命令，当命令被触发时，会执行该处理过程
    /// @param cmd_str 命令的名称
    /// @return 是否成功
    const bool unbind_command(const std::string &cmd_str);

    /// @brief 判断命令是否已经增加到任务中
    /// @param cmd_str 命令的名称
    /// @return 是否存在
    const bool command_exist(const std::string &cmd_str);

    /// @brief 获取全部命令列表
    /// @return 所有命令状态分别为(名称)
    const std::vector<std::string> command_list() const;

    /// @brief 获取命令帮助信息
    /// @param cmd_str 命令的名称
    /// @return 命令的帮助信息
    const std::string command_help(const std::string &cmd_str) const;

    /// @brief 分离命令和参数，参数根据空格进行分离
    /// @param str 输入的字符串
    /// @return 命令和参数的分离结果
    const std::vector<std::string> split_command_str(const std::string &str);

    /// @brief 执行一条指令
    /// @param cmd_name 指令名称
    /// @param args 指令的参数
    /// @return 是否被增加到任务列表中
    const bool exec(const std::string &cmd_name);

    /// @brief 在非分离模式时，提供给外部调用
    /// @note 外部调用时需要保证其其后没有需要处理的任务
    /// @warning 此函数会阻塞，使用时需要注意，如果初始化时的运行模式为分离运行，将直接返回
    void handler();

    // --------提供一种类似于python的f函数功能--------
    // 基本情形：当没有参数时，什么也不做并返回空字符串
    static std::string f() { return ""; }

    // 递归展开函数模板，处理至少一个参数
    template <typename T, typename... Args>
    static std::string f(const T &first, const Args &...args)
    {
        std::stringstream ss;
        ss << first;      // 处理当前参数
        ss << f(args...); // 递归处理剩余参数
        return ss.str();
    }

    // --------提供类似于python的print输出方式--------
    // 递归终止函数，当没有参数时结束递归
    void print() {};

    // 变参模板递归展开函数，处理至少一个参数
    template <typename T, typename... Args>
    void print(const T &first, const Args &...args)
    {
        __print_arg(first); // 处理并输出当前参数
        print(args...);     // 递归处理剩余参数
    }

    // --------提供类似于python的print输出方式--------
    // 递归终止函数，当没有参数时结束递归
    void println();

    // 变参模板递归展开函数，处理至少一个参数
    template <typename T, typename... Args>
    void println(const T &first, const Args &...args)
    {
        __print_arg(first); // 处理并输出当前参数
        println(args...);   // 递归处理剩余参数
    }

    // --------提供类似于std::cout形式的输出方式--------
    // 处理 std::endl，只有当接收到std::endl时才会输出
    Vtty &operator<<(std::ostream &(*pf)(std::ostream &));

    // 重载插入运算符以接受任意类型的数据
    template <typename T>
    Vtty &operator<<(const T &data)
    {
        __cout_buffer << data; // 将数据写入缓存
        return *this;          // 允许链式调用
    }

private:
    bool __running{true};
    bool __detach_mode{true};

    std::vector<VttyTask::cmd_desc_t> __vtty_cmd_arr; // 函数描述列表

    VttyIO *__vtty_io{nullptr};
    VttyTask *__vtty_task{nullptr};

    std::stringstream __cout_buffer; // 用于接收调用者使用"<<"写入的数据

    // 辅助函数，处理单个参数输出
    template <typename T>
    void __print_arg(const T &value)
    {
        std::stringstream ss;
        ss << value;                // 将值转换成字符串
        __vtty_io->print(ss.str()); // 使用自定义的输出方法
    }

    /// @brief 解析相关脚本，并生成命令
    void __parsing_scripts();

    friend class VttyKill;   // kill命令可以进行任务操作
    friend class VttyPS;     // ps命令可以进行任务操作
    friend class VttySignal; // signal命令可以进行任务操作
};

#endif
