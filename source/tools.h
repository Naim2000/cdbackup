#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

enum {
    input_up        = 0x0001,
    input_down      = 0x0002,
    input_left      = 0x0004,
    input_right     = 0x0008,
    input_a         = 0x0010,
    input_b         = 0x0020,
    input_x         = 0x0040,
    input_y         = 0x0080,
    input_start     = 0x0100,
    input_select    = 0x0200,
    input_home      = 0x0400,
};

extern uint16_t input_btns;
#define input_pressed(x) (input_btns & x)

extern uint32_t wii_down;
extern uint16_t gcn_down;

__attribute((constructor))
void init_video();
void clearln();
void input_scan(void);
void quit(int ret);

bool confirmation(const char* message, unsigned int wait_time);

__attribute((weak))
void OSReport(const char* fmt, ...) {}
static inline void* memalign32(size_t size) { return aligned_alloc(0x20, size); }
