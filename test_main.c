#include <stdio.h>
#include "../wma_core.h"
int main(int argc, char **argv) {
    if (argc < 3) return 2;
    int r = wma_decode_to_wav(argv[1], argv[2]);
    printf("result %d\n", r);
    return r == 0 ? 0 : 1;
}
