#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <time.h>

#define printerr(fmt, ...)               \
    do                                   \
{                                        \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    fflush(stderr);                      \
} while (0)

uint64_t get_time_tick()
{
    struct timespec temp;
    clock_gettime(CLOCK_REALTIME, &temp);
    return((temp.tv_sec * 1000000000) + temp.tv_nsec);
}

#define POWERBASE               0x1C238000

#define TE_SYS_TOTAL_SLOT       0x300
#define TE_SOC_PKG_SLOT         0x304
#define TE_C_AND_G_SLOT         0x308
#define TE_CPU_P_SLOT           0x30c
#define TE_CPU_E_SLOT           0x310
#define TE_VCORE_SLOT           0x314
#define TE_VDDQ_SLOT            0x318
#define TE_CHR_SLOT             0x31c
#define TE_GPC_OUT_SLOT         0x320
#define TE_TOTAL_GPU_OUT_SLOT   0x324
#define TE_GPC_IN_SLOT          0x328
#define TE_TOTAL_GPU_IN_SLOT    0x32c
#define TE_TOTAL_SYS_IN_SLOT    0x330
#define TE_DLA_IN_SLOT          0x334
#define TE_PREREG_IN_SLOT       0x338

#define TE_MAX			0x400

#define GET_COUNTER(TE_COUNTER)		\
{					\
    read_result = *((volatile uint32_t *)(map_base+TE_COUNTER)); \
    printf("%08" PRIu64 ", ", read_result);	\
}

int main(int argc, char **argv)
{
    int fd=0, i=0;
    void *map_base=NULL;
    uint64_t target=0;
    int access_size = 4; /* Assume 32-bit value */
    unsigned int pagesize = (unsigned)sysconf(_SC_PAGESIZE);
    unsigned int map_size = pagesize;
    unsigned offset=0;
    uint64_t read_result = -1;

    target = POWERBASE;

    offset = (unsigned int)(target & (pagesize - 1));
    if (offset + TE_MAX > pagesize)
    {
        map_size += pagesize; /* map the next page */
    }

    printerr("Address: %#" PRIX64 " op.size=%d offset=%x map_size=%d\n", target, access_size, offset, map_size);
    if (offset & (access_size - 1))
    {
        printerr("ERROR: address not aligned on %d!\n", access_size);
        exit(2);
    }

    fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd == -1)
    {
        printerr("Error opening /dev/mem (%d) : %s\n", errno, strerror(errno));
        exit(1);
    }

    map_base = mmap(0, map_size, PROT_READ, MAP_SHARED,
            fd,
            target & ~((typeof(target))pagesize - 1));
    if (map_base == (void *)-1)
    {
        printerr("Error mapping (%d) : %s\n", errno, strerror(errno));
        exit(1);
    }
    printf("Memory mapped at address %p.\n", map_base);

    for(i=0; i<25; i++) {
        printf("%lu", get_time_tick());
        GET_COUNTER(TE_SYS_TOTAL_SLOT);
        GET_COUNTER(TE_SOC_PKG_SLOT);
        GET_COUNTER(TE_C_AND_G_SLOT);
        GET_COUNTER(TE_CPU_P_SLOT);
        GET_COUNTER(TE_CPU_E_SLOT);
        GET_COUNTER(TE_VCORE_SLOT);
        GET_COUNTER(TE_VDDQ_SLOT);
        GET_COUNTER(TE_CHR_SLOT);
        GET_COUNTER(TE_GPC_OUT_SLOT);
        GET_COUNTER(TE_TOTAL_GPU_OUT_SLOT);
        GET_COUNTER(TE_GPC_IN_SLOT);
        GET_COUNTER(TE_TOTAL_GPU_IN_SLOT);
        GET_COUNTER(TE_TOTAL_SYS_IN_SLOT);
        GET_COUNTER(TE_DLA_IN_SLOT);
        GET_COUNTER(TE_PREREG_IN_SLOT);
        printf("\n");
        usleep(500000);
    }
    fflush(stdout);

    if (munmap(map_base, map_size) != 0)
    {
        printerr("ERROR munmap (%d) %s\n", errno, strerror(errno));
    }

    close(fd);

    return 0;
}
