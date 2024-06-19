#ifndef __VTTY_CMD_INFO_H__
#define __VTTY_CMD_INFO_H__

#include <string>

template <typename CMD_TYP>
struct vtty_cmd_info_t
{
    using cmd_type = CMD_TYP;
    std::string name{};
    vtty_cmd_info_t(std::string name_) : name(name_){};
};

template <typename T>
struct is_vtty_cmd_info_t : std::false_type
{
};

template <typename CMD_TYP>
struct is_vtty_cmd_info_t<vtty_cmd_info_t<CMD_TYP>> : std::true_type
{
};

#endif //__VTTY_CMD_INFO_H__
