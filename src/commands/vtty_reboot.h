#ifndef __VTTY_REBOOT_H__
#define __VTTY_REBOOT_H__

#include "vtty.h"
#include "vtty_task.h"

#ifdef ESP32
#include "esp_system.h"
#define RESTART_FUNCTION esp_restart
#elif defined(STM32)
#if defined(__CORTEX_M3)
#include "core_cm3.h"
#elif defined(__CORTEX_M4)
#include "core_cm4.h"
#endif
#define RESTART_FUNCTION NVIC_SystemReset
#else
#define RESTART_FUNCTION
#endif

class VttyReboot : public VttyCommand
{
public:
    const std::string execute() override
    {
        RESTART_FUNCTION();
        return "The system does not support this reboot command!"; // 如果不支持重启，则返回错误信息
    }

    static const std::string help()
    {
        return R"(The system will immediately reboot!

Usage:
    reboot

Example:
    reboot  ;The system will immediately reboot!
)";
    }

    using VttyCommand::VttyCommand; // inherit constructor
};

#endif // __VTTY_REBOOT_H__
