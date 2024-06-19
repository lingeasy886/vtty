#ifndef __VTTY_SIG_H__
#define __VTTY_SIG_H__

#include <string>
#include <vector>

enum class sig_type_t
{
    msg,     // 通用发送消息
    start,   // 开始任务通知
    suspend, // 挂起任务通知
    resume,  // 恢复任务通知
    stop,    // 停止任务
    exit,    // 退出任务消息
};

#endif // __VTTY_SIG_H__
