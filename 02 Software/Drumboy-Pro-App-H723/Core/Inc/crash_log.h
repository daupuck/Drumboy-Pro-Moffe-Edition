#ifndef __CRASH_LOG_H
#define __CRASH_LOG_H

/* v2.09: crashes and hangs restart the device instead of freezing it silently.
 *
 * - CPU faults (HardFault, MemManage, BusFault, UsageFault), NMI and Error_Handler() write a
 *   small record to DTCM and reset. DTCM is NOLOAD in the app and unused by the bootloader, so
 *   the record survives the reset and the next boot shows it on the screen (where and why).
 * - An independent watchdog (IWDG1, ~32 s) restarts a main loop that stopped running. The main
 *   loop and the few long blocking waits (SD card insert, sample loading) call Watchdog_Kick().
 *   The watchdog is frozen while a debugger halts the CPU.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  CRASH_NONE = 0,
  CRASH_HARDFAULT = 1,
  CRASH_MEMMANAGE = 2,
  CRASH_BUSFAULT = 3,
  CRASH_USAGEFAULT = 4,
  CRASH_ERROR_HANDLER = 5,
  CRASH_NMI = 6,
  CRASH_WATCHDOG = 7 /* not written by a handler: read from the reset flags at boot */
};

typedef struct {
  uint32_t type; /* CRASH_* */
  uint32_t pc;   /* faulting instruction (Error_Handler: its caller) */
  uint32_t lr;
  uint32_t cfsr; /* SCB->CFSR */
  uint32_t hfsr; /* SCB->HFSR */
  uint32_t addr; /* SCB->BFAR or MMFAR when valid, else 0 */
} CrashInfo;

/* Called by the fault handlers with the stacked exception frame. Never returns. */
void CrashLog_Fault(uint32_t* frame, uint32_t type);
/* Records a crash with the given pc and resets. Never returns. */
void CrashLog_Reset(uint32_t type, uint32_t pc);

/* At boot: fills `out` with the last crash (including a watchdog reset seen in the reset
 * flags), clears the record and the reset flags. Returns 1 when there was one. */
int CrashLog_Take(CrashInfo* out);

void Watchdog_Start(void);
void Watchdog_Kick(void);

#ifdef __cplusplus
}
#endif

#endif
