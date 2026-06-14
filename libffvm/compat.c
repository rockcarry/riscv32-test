/* picolibc 缺少的兼容函数 */
#include "libffvm.h"
void usleep(int us) { mdelay(us / 1000 + 1); }
