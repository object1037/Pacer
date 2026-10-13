#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// 12 MHz external crystal (not the 24 MHz default).
#define FUNCONF_USE_HSE 1
#define HSE_VALUE 12000000
// CH32V003 HSE path uses PLL x2: 12 MHz -> 24 MHz system clock.
#define FUNCONF_SYSTEM_CORE_CLOCK 24000000

#endif
