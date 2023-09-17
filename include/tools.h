#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define input_up		(1 << 0x0)
#define input_down		(1 << 0x1)
#define input_left		(1 << 0x2)
#define input_right		(1 << 0x3)

#define input_a			(1 << 0x4)
#define input_b			(1 << 0x5)
#define input_x			(1 << 0x6)
#define input_y			(1 << 0x7)

#define input_start		(1 << 0x8)
#define input_select	(1 << 0x9)

#define input_home		(1 << 0xB)

extern uint16_t input_btns;
#define input_pressed(x) (input_btns & x)

extern uint32_t wii_down;
extern uint16_t gcn_down;

void init_video(int row, int col);
void input_scan(void);

int quit(int ret);

bool confirmation(const char* message, unsigned int wait_time);

// extern __attribute__((weak)) void OSReport(const char* fmt, ...) {}
extern void* malloc(size_t bytes);
extern void* memalign(size_t alignment, size_t size);
extern unsigned int sleep(unsigned int seconds);
