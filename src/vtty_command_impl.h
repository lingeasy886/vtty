#ifndef __VTTY_COMMAND_IMPL_H__
#define __VTTY_COMMAND_IMPL_H__

#include <stdint.h>
#include <vector>
#include <string>
#include <chrono>
#include "vtty_sig.h"

class Vtty;

/// @brief  用户需要继承该类，实现自己的命令
/// @note   init:非必须，初始化函数，在执行execute前将会调用此函数
/// @note   execute:必须，提供给vtty自动调用的接口，当用户输入命令时，会调用此函数
/// @note   on_kill:非必须，提供给vtty自动调用的接口，当命令被强制关闭时，会调用此函数
/// @note   help:非必须，但建议，提供给vtty自动调用的接口，当用户输入命令时，会调用此函数
/// @note   ~VttyCommand:非必须，处理析构时相关善后工作
/// @note   args:此处为任务创建时传入的参数
/// @note   task_id:此处为任务创建时传入的任务id
/// @note   create_time:此处为任务创建时的时间
/// @note   vtty:此处为vtty对象，保存调用对象，提供给命令使用，该参数在vtty自动填充
/// @warning   请勿修改此文件，否则会导致程序崩溃！！！
class VttyCommand
{
public:
    /// @brief 初始化功能，在运行execute前将会调用此函数
    /// @warning 如果异常阻塞，会导致execute无法运行
    virtual void init() {}

    /// @brief 执行命令，通过tty输入的命令
    /// @return 返回运行完成的结果，可以为空，空时将不把打印任务内容，过程中如果需要打印，可以主动调用输出功能
    virtual const std::string execute() = 0;

    /// @brief 处理发过过来的信号
    /// @return 返回处理结果，可以为空
    /// @warning 此函数应立即返回结果，不可行动耗时较长的任务
    virtual void on_sig(const sig_type_t sig_id, std::string sig_ms) {}

    /// @brief 获取命令帮助，通过tty输入的命令
    /// @return 返回命令帮助
    static const std::string help() { return "No help information to display."; }

    /// @brief 在运行结束后，或使用kill将任务删除时，会自动调用此函数进行析构，可以适当增加相应的处理功能
    virtual ~VttyCommand() = default;

    /// @brief 不能使用默认构造函数
    VttyCommand() = delete;

    /// @brief 绑定调用者
    /// @warning 请不要修改此函数的实现
    VttyCommand(Vtty *vtty_, const std::vector<std::string> args_, uint32_t task_id_)
        : vtty(vtty_), args(args_), task_id(task_id_), create_time(std::chrono::system_clock::now()) {}

protected:
    friend class Vtty;
    friend class VttyTask;

    /// @brief vtty对象，保存调用对象，提供给命令使用，该参数在vtty自动填充
    /// @warning 使用时，请不要修改此指针，更不能删除此指针，否则会导致程序崩溃！！！
    /// @note 例如，输出数据，类型python的输出：(*vtty).print("hello world", 123);
    ///       或者，输出数据，支持std::cout相同方式，std::endl时输出：(*vtty) << "hello world" << 123 << std::endl;
    ///       或者，拼接数据：std::string str = (*vtty).f("hello world", 123);
    Vtty *vtty{nullptr};

    /// @brief 接收到的参数列表
    /// @note 此参数在任务调度时由系统自动填写，不需要修改
    const std::vector<std::string> args;

    /// @brief 当前任务的ID号
    /// @note 此参数在任务调度时由系统自动填写，不需要修改
    const uint32_t task_id;

    /// @brief 任务创建时间
    /// @note 此参数在任务调度时由系统自动填写，不需要修改
    const std::chrono::time_point<std::chrono::system_clock> create_time;

    /// @brief 任务完成（结束）时间
    /// @note 此参数在任务调度时由系统自动填写，不需要修改
    std::chrono::time_point<std::chrono::system_clock> finish_time;
};

#endif // __VTTY_COMMAND_IMPL_H__
