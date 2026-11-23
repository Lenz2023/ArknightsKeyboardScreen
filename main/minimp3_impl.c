// minimp3 实现单独放 C 文件里编译（放 C++ TU 里会因类型/别名规则出问题）
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD
#define MINIMP3_ONLY_MP3
#include "minimp3.h"
