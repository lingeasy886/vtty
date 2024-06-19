// @brief 打印的输入输出管理功能
#ifndef __VTTY_IN_OUT_H__
#define __VTTY_IN_OUT_H__

#include "vtty_io_impl.h"

#include <string>
#include <cstddef>
#include <queue>
#include <condition_variable>

class VttyIO
{
    friend class Vtty;

public:
    enum class input_state_t
    {
        NIL,          // 没有输入
        VALID,        // 输入正常
        OUT_OF_RANGE, // 命令超长
        TIMEOUT,      // 命令超时
        READ_TIMEOUT, // 读取超时，在调用者使用read函数时，并且有时间超时限制，如果超时，将会返回INPUT_READ_TIMEOUT
    };

public:
    VttyIO() = delete;
    VttyIO(const VttyIO &) = delete;
    VttyIO(VttyIO &&) = delete;
    VttyIO &operator=(const VttyIO &) = delete;
    VttyIO &operator=(VttyIO &&) = delete;

    VttyIO(VttyIOImpl &vtty_io_impl);

    ~VttyIO();

    /// @brief 打印信息
    /// @param str 需要打印的信息
    void print(const std::string &str);

    /// @brief 打印信息
    /// @param str
    void print(const char *str);

    /// @brief 打印信息
    /// @param str 需要打印的信息
    /// @param len 需要打印的长度
    void print(const char *str, const size_t len);

    /// @brief 读取当前队列中等待读取的数据长度
    /// @return 队列长度
    const size_t available() const;

    /// @brief 读取一条指令
    /// @param timeout_ms 超时等待时长，默认为0ms，不等待
    /// @return 返回一条指令
    ///         {"", INPUT_NIL}：表示没有读取到数据
    ///         {"", INPUT_READ_TIMEOUT}：表示等待了超时后，没有读取到数据
    ///         {"...", INPUT_VALID ...}：表示成功的读取到了数据
    /// @note 测试发现，在ESP32上似乎不能支持waint_for功能，调用时会立刻返回，所以目前只使用wait实现无限长时间的等待
    const std::pair<std::string, input_state_t> read(const uint32_t timeout_ms = 0);

    // TODO:未来增加处理连续输入数据的功能，start_write, stop_write之间可以连续输入数据

    /// @brief 清空全部缓存
    void flush();

    /// @brief 清空输入缓存
    void flush_in();

    /// @brief 清空输出缓存
    void flush_out();

private:
    bool __is_running{true}; // 控制线程的运行状态

    std::condition_variable __in_cv;
    std::mutex __in_mtx;
    std::vector<char> __in_buffer;                                // 输入缓存，输入必须是单线程的
    std::queue<std::pair<std::string, input_state_t>> __in_queue; // 输入队列，将解析出来的命令放入队列中，分类是<一条指令，指令状态>

    std::condition_variable __out_cv;
    std::mutex __out_mtx;
    std::queue<std::string> __out_queue;

    uint32_t __command_len_max{1024};                // 最大可输入命令长度，保函参数列在内的长度
    uint32_t __input_timeout_interval_ms{10 * 1000}; // 超时等待时长，默认为10秒
    uint32_t __read_tick_time_on_prev_empty_ms{100}; // 读取输入的间隔时长，默认为100ms

    /// @brief 删除字符串末端的空白符号
    /// @param s 需要处理的字符串
    /// @return 处理后的字符串
    inline std::string __rtrim(const std::string &s);

    /// @brief 将数据压入到可用队列中，等待读取后处理
    /// @param command 压入的命令
    /// @param input_state 输入状态
    void __enqueue_command(std::string &command, const input_state_t input_state);

    /// @brief 从输入读取数据，并写入到缓存中
    /// @return 是否有数据被写入
    bool __append_data_to_in_buffer();

    /// @brief NODE:处理状态变量，只提供给__in_handle内部使用，当数据是分次到达时，此方法可以提供处理效率，避免重复处理
    bool __in_double_quote{false}; // 是否在双引号内
    std::string __current;

    /// @brief 读取输入
    /// @return 是否有数据被读取
    void __readline();

    /// @brief 处理输入
    ///        Return = CR = 13 = ‘\x0d’
    ///        NewLine = LF = 10 = ‘\x0a’
    ///        当接收到<CR>时会被识别为命令结果，如果其后还有<LF>，将会被删除
    ///        包裹在单引号和双引号内的数据，将不会进行命令解析
    ///        同时处理Windows (\r\n) 和 Unix/Linux/macOS (\n) 的换行输入
    ///        处理后的数据会被写入到__in_queue队列中，等待使用函数的调用
    void __in_handle();

    /// @brief 处理输出
    void __out_handle();

    VttyIOImpl &__vtty_io_impl;
};

#endif // !__VTTY_IN_OUT_H__
