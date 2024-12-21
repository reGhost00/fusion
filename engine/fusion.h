#pragma once
#include "core/_base.h"
#include "core/array.h"
#include "core/file.h"
#include "core/hash2.h"
#include "core/logger.h"
#include "core/main.h"
#include "core/memory.h"
#include "core/misc.h"
#include "core/object.h"

#include "platform/glfw.h"
#include "platform/misc.linux.h"
#include "platform/misc.window.h"
#include "platform/sdl.h"

#include "renderer/context.h"
#include "renderer/misc.h"
#include "renderer/shader.h"

/**
 * 2024-11-08
 * 尝试使用 C++ 编译器
 * 但是出现大量不兼容，包括但不限于:
 *   mimalloc 内部报错：使用 extren "C" 与模板不兼容
 *   typedef struct XXX 语法不兼容
 *   各种其它标准库里的语法错误
 * 反正很多错误，考虑支持 C++ 只是方便添加 DirectX12 的支持
 * 但是现在已经有 Vulkan 了，就不需要了
 */