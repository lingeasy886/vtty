#include "vtty_io.h"

#include <thread>
#include <algorithm>

VttyIO::VttyIO(VttyIOImpl &vtty_io_impl) : __vtty_io_impl(vtty_io_impl)
{
    // 清空输入输出缓存
    __vtty_io_impl.flush_in();
    __vtty_io_impl.flush_out();

    // 启动输入输出线程
    __is_running = true;
    std::thread(&VttyIO::__in_handle, this).detach();
    std::thread(&VttyIO::__out_handle, this).detach();
}

VttyIO::~VttyIO()
{
    // 退出输入输出线程
    __is_running = false;
    __in_cv.notify_one();
    __out_cv.notify_one();

    // 清空输入输出缓存
    __vtty_io_impl.flush_in();
    __vtty_io_impl.flush_out();
}

inline std::string VttyIO::__rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(" \t\n\r\f\v");
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

void VttyIO::__enqueue_command(std::string &command, const input_state_t input_state)
{
    if (command.empty())
        return;

    std::unique_lock<std::mutex> lock(__in_mtx);
    __in_queue.emplace(__rtrim(command), input_state); // 将处理好的字符串放入输入队列
    __in_cv.notify_one();                              // 通知读取线程有新的数据
}

bool VttyIO::__append_data_to_in_buffer()
{
    auto buf_remain = __vtty_io_impl.available();
    if (buf_remain == 0)
    {
        return false; // 没有可读数据
    }

    // 扩展 vector 以容纳新数据
    auto current_len = __in_buffer.size();
    __in_buffer.resize(buf_remain + current_len);

    // 从设备读取数据到 vector 的末尾
    auto bytes_read = __vtty_io_impl.read(__in_buffer.data() + current_len, buf_remain);

    // 如果实际读取的数据少于请求的数据，调整 vector 的大小
    if (bytes_read < buf_remain)
    {
        __in_buffer.resize(current_len + bytes_read);
    }

    return bytes_read != 0;
}

void VttyIO::__readline()
{
    uint32_t pos = 0;

    while (pos < __in_buffer.size())
    {
        char ch = __in_buffer[pos];

        if (ch == '"')
        {
            __in_double_quote = !__in_double_quote; // 双引号内的换行符被忽略
            __current += ch;
        }
        else if (ch == '\r' || ch == '\n') // 对于回车符(\r)或换行符(\n)，处理逻辑
        {
            if (!__in_double_quote)
            {
                if (!__current.empty())
                {
                    std::string trimmed_cmd = __rtrim(__current);
                    if (!trimmed_cmd.empty())
                    {
                        __enqueue_command(trimmed_cmd, input_state_t::VALID);
                    }
                }
                __current.clear();

                // 处理换行符组合（\r\n 或 \n\r）
                if (ch == '\r' && pos + 1 < __in_buffer.size() && __in_buffer[pos + 1] == '\n')
                {
                    pos++;
                }
                else if (ch == '\n' && pos + 1 < __in_buffer.size() && __in_buffer[pos + 1] == '\r')
                {
                    pos++;
                }
            }
            else
            {
                __current += ch;
            }
        }
        else
        {
            __current += ch;
        }

        if (__current.length() >= __command_len_max)
        {
            // 处理超长字符串
            __enqueue_command(__current, input_state_t::OUT_OF_RANGE);
            __current.clear();
        }

        pos++;
    }

    // 清除已处理的部分
    __in_buffer.erase(__in_buffer.begin(), __in_buffer.begin() + pos);
}

void VttyIO::__in_handle()
{
    int32_t timeout_tick_count{0};
    while (__is_running)
    {
        // 尝试从设备读取数据
        if (!__append_data_to_in_buffer())
        {
            ++timeout_tick_count; // 计数自增
            if (timeout_tick_count >= (__input_timeout_interval_ms / __read_tick_time_on_prev_empty_ms))
            {
                // 输入超时
                if (!__current.empty()) // 超时，且已经处理了一部分数据时
                {
                    // 将当前的处理状态重置，等待后续用户的输入
                    __enqueue_command(__current, input_state_t::TIMEOUT);
                    __in_buffer.clear();
                    __in_double_quote = false;
                    __current.clear();
                }
                timeout_tick_count = 0;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(__read_tick_time_on_prev_empty_ms)); // 等待下一次处理
            continue;                                                                                  // 直接跳过本次循环
        }

        timeout_tick_count = 0; // 重置超时计数
        __readline(); // 处理数据
    }
}

void VttyIO::__out_handle()
{
    while (__is_running)
    {
        std::unique_lock<std::mutex> lock(__out_mtx); // 锁定互斥量
        __out_cv.wait(lock, [this]()
                      { return !__out_queue.empty(); }); // 等待条件满足：数据队列非空

        if (__out_queue.empty())
        {
            lock.unlock(); // 解锁互斥量
            continue;      // 队列空，继续等待
        }

        std::string str = __out_queue.front();
        __out_queue.pop();
        lock.unlock(); // 解锁互斥量

        __vtty_io_impl.print(str.c_str(), str.length()); // 打印信息
    }
}

void VttyIO::print(const std::string &str)
{
    std::unique_lock<std::mutex> lock(__out_mtx);
    __out_queue.push(str);
    __out_cv.notify_one();
}

void VttyIO::print(const char *str)
{
    print(std::string(str));
}

void VttyIO::print(const char *str, const size_t len)
{
    print(std::string(str, len));
}

const size_t VttyIO::available() const
{
    return __in_queue.size();
}

const std::pair<std::string, VttyIO::input_state_t> VttyIO::read(const uint32_t timeout_ms)
{
    std::unique_lock<std::mutex> lock(__in_mtx);

    // 直接返回的处理方式
    if (timeout_ms == 0)
    {
        if (__in_queue.empty())
        {
            lock.unlock(); // 解锁互斥量
            return {"", input_state_t::NIL};
        }
        std::pair<std::string, input_state_t> cmd{__in_queue.front()};
        __in_queue.pop();
        lock.unlock(); // 解锁互斥量
        return cmd;
    }

    /// NOTE:测试发现，在ESP32上似乎不能支持waint_for功能，调用时会立刻返回，所以目前只使用wait实现无限长时间的等待
    // 等待特定时长，或外部线程通知，为下一次判断做准备
    // #ifdef __ESP32__
    __in_cv.wait(lock, [this]
                 { return !__in_queue.empty(); });

    if (!__in_queue.empty())
    {
        std::pair<std::string, input_state_t> cmd{__in_queue.front()};
        __in_queue.pop();
        lock.unlock(); // 解锁互斥量
        return cmd;
    }

    lock.unlock(); // 解锁互斥量
    return {"", input_state_t::READ_TIMEOUT};
    // #else
    //     // 否则等待特定时长，或外部线程通知
    //     bool read_timeout{__in_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms)) == std::cv_status::timeout};
    //     // 否则直接返回队列中的数据
    //     if (!__in_queue.empty())
    //     {
    //         std::pair<std::string, input_state_t> cmd{__in_queue.front()};
    //         __in_queue.pop();
    //         lock.unlock(); // 解锁互斥量
    //         return cmd;
    //     }
    //     // 等待一段时间后，任然没有数据
    //     lock.unlock(); // 解锁互斥量
    //     return {"", read_timeout ? input_state_t::READ_TIMEOUT : input_state_t::NIL};
    // #endif
}

void VttyIO::flush()
{
    flush_in();
    flush_out();
}

void VttyIO::flush_in()
{
    while (!__in_queue.empty())
    {
        __in_queue.pop();
    }

    __in_buffer.clear();

    __in_double_quote = false;
    __current.clear();
}

void VttyIO::flush_out()
{
    while (!__out_queue.empty())
    {
        __out_queue.pop();
    }
}
