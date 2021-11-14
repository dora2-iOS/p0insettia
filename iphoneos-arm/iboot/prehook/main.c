/*
 * prehook payload
 * copyright (C) 2021/11/14 sauRdev
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "lib/aeabi.h"
#include "lib/putc.h"
#include "lib/printf.h"

#include "drivers/drivers.h"
#include "drivers/display/display.h"

#define IBOOT_SIZE      0x48000
#define PAYLOAD_BASE    0xbfc00000
#define TRAMPOLINE      0xdeadbeef

typedef void (*dt_load_t)(void);
typedef void (*mount_and_boot_system_t)(void);

#include "patchfinder.h"

uintptr_t* framebuffer_address;
uintptr_t* base_address;
unsigned int display_width;
unsigned int display_height;

get_env_uint_t _get_env_uint;
get_env_t _get_env;

static void print_banner() {
    fb_print_row('=');
    printf(":: p0insettia pre-patcher payload\n");
    printf("::\n");
    printf("::   BUILD_VERSION: 1.1 [1B241]\n");
#ifdef DEBUG
    printf("::   BUILD_STYLE: DEBUG\n");
#else
    printf("::   BUILD_STYLE: RELEASE\n");
#endif
    printf("::   Copyright 2021, dora2ios.\n");
    fb_print_row('=');
    printf("* Thanks to:\n");
    printf("axi0mX\n");
    printf("geohot\n");
    printf("iH8sn0w\n");
    printf("JonathanSeals\n");
    printf("planetbeing\n");
    printf("posixninja\n");
    printf("qwertyoruiopz\n");
    printf("synackuk\n");
    printf("xerub\n");
    fb_print_row('-');
}

void *memcpy(void *dst, const void *src, size_t len)
{
    const char *s = src;
    char       *d = dst;
    int        pos = 0, dir = 1;
    
    if (d > s) {
        dir = -1;
        pos = len - 1;
    }
    
    while (len--) {
        d[pos] = s[pos];
        pos += dir;
    }
    
    return dst;
}

uint32_t make_bl(int blx, int pos, int tgt)
{
    int delta;
    unsigned short pfx;
    unsigned short sfx;
    unsigned int omask = 0xF800;
    unsigned int amask = 0x7FF;
    if (blx) { /* untested */
        omask = 0xE800;
        amask = 0x7FE;
        pos &= ~3;
    }
    delta = tgt - pos - 4; /* range: 0x400000 */
    pfx = 0xF000 | ((delta >> 12) & 0x7FF);
    sfx =  omask | ((delta >>  1) & amask);
    return (unsigned int)pfx | ((unsigned int)sfx << 16);
}

#include "payload.h"

int
_main()
{
    dt_load_t dt_load;
    mount_and_boot_system_t mount_and_boot_system;
    uint32_t pre_dt_load;
    uint32_t pre_mount;
    uint32_t hook_point;
    
    base_address = find_base_address();
    _get_env_uint = find_get_env_uint();
    framebuffer_address = find_framebuffer_address();
    _get_env = find_get_env();
    display_width = find_display_width();
    display_height = find_display_height();
    
    drivers_init((uint32_t*)framebuffer_address, display_width, display_height);
    
    pre_dt_load = (uint32_t)find_dt_load(base_address, IBOOT_SIZE);
    pre_dt_load = pre_dt_load - 0x28 + 1;
    
    pre_mount = (uint32_t)find_mount_and_boot_system(base_address, IBOOT_SIZE);
    pre_mount = pre_mount + 0x8 + 1;
    
    dt_load = (dt_load_t)pre_dt_load;
    mount_and_boot_system = (mount_and_boot_system_t)pre_mount;
    
    hook_point = (uint32_t)find_hook_point(base_address, IBOOT_SIZE);
    hook_point += 6;
    
    *(uint32_t*)hook_point = make_bl(1, hook_point, PAYLOAD_BASE);
    
    print_banner();
    
#ifdef DEBUG
    printf("BASE_ADDRESS: 0x%x\n", base_address);
    printf("GET_ENV_UINT: 0x%x\n", _get_env_uint);
    printf("FRAMEBUFFER: 0x%x\n", framebuffer_address);
    printf("GET_ENV: 0x%x\n", _get_env);
    printf("DISPLAY: %d, %d\n", display_width, display_height);
    
    printf("Found dt_load: 0x%x\n", dt_load);
    printf("Found mount_and_boot_system: 0x%x\n", mount_and_boot_system);
    printf("Found hook_point: 0x%x\n", hook_point);
#else
    printf("Found dt_load\n");
    printf("Found mount_and_boot_system\n");
    printf("Found hook_point\n");
#endif
    
    memcpy((void*)(PAYLOAD_BASE), (void*)(TRAMPOLINE), (uint32_t)(payload_len));
    
    dt_load();
    
    printf("Waiting KernelCache...\n");
    
    mount_and_boot_system();
    
    return 0;
}