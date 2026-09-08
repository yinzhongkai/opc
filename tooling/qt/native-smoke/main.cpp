#include <cstdio>

int main()
{
    static_assert(sizeof(void *) == 8, "T-012 requires an x64 binary");
    std::puts("SPACE_RHYTHM_MSVC_X64_SMOKE");
    return 0;
}
