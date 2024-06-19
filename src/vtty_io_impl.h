#ifndef __VTTY_IO_IMPL_H__
#define __VTTY_IO_IMPL_H__

#include <stdint.h>
#include <vector>
#include <string>

/// @brief 用户需要继承该类，实现自己的vtty IO操作
class VttyIOImpl
{
public:
    virtual ~VttyIOImpl(){};

    /// @brief 最终输出到显示的内容
    /// @param data 显示的数据
    /// @param len 数据长度
    virtual void print(const char *data, uint32_t len) = 0;

    /// @brief 谋取输入缓存中数据长度，用于判读是否可以读取数据
    /// @return 数据长度
    virtual uint32_t available() = 0;

    /// @brief 从设备读取数据
    /// @param data 接收数据的地址
    /// @param len 数据的长度
    /// @return 实际读取到的数据长度
    virtual uint32_t read(char *data, uint32_t len) = 0;

    /// @brief 清空输入到系统的缓存
    ///        NODE:函数会在初始化和退出时调用此功能，可能会对其它函数的打印产生影响
    virtual void flush_in() {};

    /// @brief 清空输出到系统的缓存
    ///        NODE:函数会在初始化和退出时调用此功能，可能会对其它函数的打印产生影响
    virtual void flush_out() {};
};

#endif
