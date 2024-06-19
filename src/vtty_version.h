#ifndef __VTTY_VERSION_H__
#define __VTTY_VERSION_H__

// 版本信息
#define VTTY_MAJOR_VERSION 1
#define VTTY_MINOR_VERSION 0
#define VTTY_PATCH_VERSION 0

// 辅助宏，用于将宏值转换成字符串
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x) // 新增加的宏，确保参数值先被展开

// 辅助宏，用于将版本号转换成整数
#define VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

// 拼接版本号字符串
#define VTTY_VERSION_STRING (TOSTRING(VTTY_MAJOR_VERSION) "." TOSTRING(VTTY_MINOR_VERSION) "." TOSTRING(VTTY_PATCH_VERSION))
// 计算版本数值
#define VTTY_VERSION VERSION_VAL(VTTY_MAJOR_VERSION, VTTY_MINOR_VERSION, VTTY_PATCH_VERSION)

#define VTTY_VERSION_DATE "2023-05-29"

#endif // __VTTY_VERSION_H__
