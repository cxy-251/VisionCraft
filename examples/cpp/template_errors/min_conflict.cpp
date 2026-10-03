// 去掉 DeviceLink 里 std::min<int> 的 <int> 会怎样：两个参数类型不同，编译器推不出 T
#include <algorithm>

#define VC_IMAGE_CHUNK 1000u      // 协议头文件里的定义：带 u，是 unsigned int

int chunkLength(int remaining)
{
    return std::min(VC_IMAGE_CHUNK, remaining);
}
