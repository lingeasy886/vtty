#ifndef __VTTY_DFLT_CMD_CFG_H__
#define __VTTY_DFLT_CMD_CFG_H__

#include <string>
#include <tuple>
#include "vtty_cmd_info.h"

// NOTE: 默认支持的命令
#include "commands/vtty_vtty.h"
#include "commands/vtty_help.h"
#include "commands/vtty_signal.h"
#include "commands/vtty_kill.h"
#include "commands/vtty_ps.h"
#include "commands/vtty_boot_time.h"
#include "commands/vtty_reboot.h"
#include "commands/vtty_echo.h"

static auto __vtty_dflt_cmd{
    std::make_tuple(
        vtty_cmd_info_t<VttyVtty>("vtty"),
        vtty_cmd_info_t<VttyHelp>("help"),
        vtty_cmd_info_t<VttySignal>("signal"),
        vtty_cmd_info_t<VttyKill>("kill"),
        vtty_cmd_info_t<VttyPS>("ps"),
        vtty_cmd_info_t<VttyBootTime>("btime"),
        vtty_cmd_info_t<VttyReboot>("reboot"),
        vtty_cmd_info_t<VttyEcho>("echo"))
    // NOTE: 可以在此处理增加默认支持的命令
    // NOTE: 对于不需要的命令，可以将其对应的行进行注释
};

decltype(__vtty_dflt_cmd) &vtty_dflt_cmd()
{
    return __vtty_dflt_cmd;
}

#endif // !__VTTY_DFLT_CMD_CFG_H__
