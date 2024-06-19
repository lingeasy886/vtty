#include "vtty.h"
#include "vtty_dflt_cmd_cfg.h"

#include <thread>
#include <algorithm>
#include <tuple>
#include <array>

const bool Vtty::bind_command(const std::string &cmd_str, const VttyTask::vcmd_create_t cmd_fun, const VttyTask::vcmd_help_t help)
{
    // 检查命令是否已经存在
    if (command_exist(cmd_str))
    {
        return false;
    }
    __vtty_cmd_arr.push_back(VttyTask::cmd_desc_t{cmd_str, cmd_fun, help});
    return true;
}

const bool Vtty::unbind_command(const std::string &cmd_str)
{
    // 使用std::remove_if配合lambda表达式来查找并移除指定命令
    auto it = std::remove_if(__vtty_cmd_arr.begin(), __vtty_cmd_arr.end(),
                             [&cmd_str](const VttyTask::cmd_desc_t &cmd_desc)
                             {
                                 return cmd_desc.name == cmd_str;
                             });

    // 检查是否有元素被移除
    if (it != __vtty_cmd_arr.end())
    {
        // 查询是否还有任务在执行
        if (__vtty_task->task_exists(cmd_str))
        {
            *this << cmd_str << " is running, can't be unbinded!\n"
                  << std::endl;
            return false;
        }
        // 如果没有正在执行的任务，删除对应的bind对象
        __vtty_cmd_arr.erase(it);
        print("Command \"" + cmd_str + "\" unbinded!\n");
        return true;
    }
    return false;
}

const bool Vtty::command_exist(const std::string &cmd_str)
{
    // 使用C++11范围for循环和std::find_if从<algorithm>来查找命令
    auto it = std::find_if(__vtty_cmd_arr.begin(), __vtty_cmd_arr.end(),
                           [&cmd_str](const VttyTask::cmd_desc_t &cmd_desc)
                           {
                               return cmd_desc.name == cmd_str;
                           });
    // 如果迭代器没有到达末尾，我们找到了命令
    return it != __vtty_cmd_arr.end();
}

const std::vector<std::string> Vtty::command_list() const
{
    // 准备一个空的vector，用于存储结果
    std::vector<std::string> result;

    // 遍历__vtty_cmd_arr，并将每个元素的cmd和auto_start_thread添加到结果vector中
    for (const auto &cmd_desc : __vtty_cmd_arr)
    {
        result.emplace_back(cmd_desc.name);
    }

    return result; // 返回填充好的vector
}

const std::string Vtty::command_help(const std::string &cmd_str) const
{
    // 使用C++11范围for循环和std::find_if从<algorithm>来查找命令
    auto it = std::find_if(__vtty_cmd_arr.begin(), __vtty_cmd_arr.end(),
                           [&cmd_str](const VttyTask::cmd_desc_t &cmd_desc)
                           {
                               return cmd_desc.name == cmd_str;
                           });
    // 如果迭代器没有到达末尾，我们找到了命令
    if (it != __vtty_cmd_arr.end())
    {
        return it->help(); // 返回帮助信息
    }
    return "Unable to find relevant help documents!";
}

const std::vector<std::string> Vtty::split_command_str(const std::string &str)
{
    std::vector<std::string> result;
    std::string current;
    bool in_quotes = false;
    char quote_char = '\0'; // 当前使用的引号类型（单引号或双引号）

    for (size_t i = 0; i < str.size(); ++i)
    {
        char ch = str[i];

        if (ch == '"' || ch == '\'')
        {
            if (in_quotes && quote_char == ch)
            {
                // 当前在引号内且遇到了匹配的引号
                in_quotes = false;
                result.push_back(current);
                current.clear();
            }
            else if (!in_quotes)
            {
                // 当前不在引号内，进入引号状态
                in_quotes = true;
                quote_char = ch;
                if (!current.empty())
                {
                    result.push_back(current);
                    current.clear();
                }
            }
            else
            {
                // 嵌套的引号，按普通字符处理
                current += ch;
            }
        }
        else if (std::isspace(ch) && !in_quotes)
        {
            if (!current.empty())
            {
                result.push_back(current);
                current.clear();
            }
        }
        else
        {
            current += ch;
        }
    }

    if (!current.empty())
    {
        result.push_back(current);
    }

    return result;
}

const bool Vtty::exec(const std::string &cmd_str)
{
    std::vector<std::string> cmd_vec = split_command_str(cmd_str);
    std::string cmd_name = cmd_vec.front();

    auto it = std::find_if(__vtty_cmd_arr.begin(), __vtty_cmd_arr.end(),
                           [&cmd_name](const VttyTask::cmd_desc_t &cmd_desc)
                           {
                               return cmd_desc.name == cmd_name;
                           });

    // 检查命令是否已经存在
    if (it != __vtty_cmd_arr.end())
    {
        auto re = __vtty_task->new_task(*it, cmd_str, cmd_vec); // 创建新任务
        if (re > 0)
        {
            return true;
        }

        this->println("The task limit has been reached"); // 输出错误信息
        return false;
    }
    *this << cmd_name << " not found!\n"
          << std::endl; // 输出错误信息
    return true;
}

Vtty::Vtty(VttyIOImpl &vtty_io_impl, bool detach_mode) : __detach_mode(detach_mode)
{
    // 绑定默认命令
    bind_command(vtty_dflt_cmd());

    // 启动任务管理功能
    __vtty_task = new VttyTask{this};

    // 启动收发功能
    __vtty_io = new VttyIO{vtty_io_impl};

    // 启动命令解析功能
    if (detach_mode)
    {
        std::thread(&Vtty::__parsing_scripts, this).detach(); // 分离线程
    }
}

void Vtty::handler()
{
    if (__detach_mode)
        return;
    __parsing_scripts();
}

Vtty::~Vtty()
{
    delete __vtty_io;
    delete __vtty_task;
    __running = false;
}

void Vtty::println()
{
    __vtty_io->print("\n");
}

Vtty &Vtty::operator<<(std::ostream &(*pf)(std::ostream &))
{
    // 当我们遇到 std::endl 时，输出缓存的内容，然后清空缓存
    if (pf == static_cast<std::ostream &(*)(std::ostream &)>(std::endl))
    {
        __vtty_io->print(__cout_buffer.str());
        __cout_buffer.str(""); // 清空缓存
        __cout_buffer.clear(); // 清除错误标志
    }
    return *this; // 允许链式调用
}

void Vtty::__parsing_scripts()
{
    while (__running)
    {
        std::string cmd_str{""};
        VttyIO::input_state_t state{VttyIO::input_state_t::NIL};
        tie(cmd_str, state) = __vtty_io->read(UINT32_MAX); // 一直等待，直到有数据

        switch (state)
        {
        case VttyIO::input_state_t::VALID:
            exec(cmd_str);
            break;
        case VttyIO::input_state_t::NIL:
            // NOTE:不处理
            break;
        case VttyIO::input_state_t::OUT_OF_RANGE:
            *this << "The input instruction length exceeds the maximum receiving range.\n"
                  << std::endl;
            break;
        case VttyIO::input_state_t::TIMEOUT:
            *this << "The input instruction timeout.\n"
                  << std::endl;
            break;
        case VttyIO::input_state_t::READ_TIMEOUT:
            *this << "The input instruction read timeout.\n"
                  << std::endl;
            break;
        default:
            break;
        }
    }
}