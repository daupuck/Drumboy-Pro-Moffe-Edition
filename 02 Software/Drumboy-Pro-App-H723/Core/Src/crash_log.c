/* v2.09: crash record + watchdog. See crash_log.h. */

#include "crash_log.h"

#include "main.h"

#define CRASH_MAGIC 0xC4A5B007u

typedef struct {
  uint32_t magic;
  CrashInfo info;
  uint32_t check; /* ~magic ^ type ^ pc: guards against random DTCM content after power-on */
} CrashRecord;

/* .DTCM is NOLOAD: not zeroed by the startup code, so the record survives a reset. */
__attribute__((section(".DTCM"))) static volatile CrashRecord crashRecord;

static uint32_t CrashLog_Check(uint32_t type, uint32_t pc)
{
  return ~CRASH_MAGIC ^ type ^ pc;
}

static void CrashLog_Store(uint32_t type, uint32_t pc, uint32_t lr)
{
  const uint32_t cfsr = SCB->CFSR;
  uint32_t addr = 0;
  if (cfsr & SCB_CFSR_BFARVALID_Msk) addr = SCB->BFAR;
  else if (cfsr & SCB_CFSR_MMARVALID_Msk) addr = SCB->MMFAR;

  crashRecord.info.type = type;
  crashRecord.info.pc = pc;
  crashRecord.info.lr = lr;
  crashRecord.info.cfsr = cfsr;
  crashRecord.info.hfsr = SCB->HFSR;
  crashRecord.info.addr = addr;
  crashRecord.check = CrashLog_Check(type, pc);
  crashRecord.magic = CRASH_MAGIC;
  __DSB();
}

void CrashLog_Fault(uint32_t* frame, uint32_t type)
{
  /* Exception frame: r0 r1 r2 r3 r12 lr pc xpsr */
  CrashLog_Store(type, frame[6], frame[5]);
  NVIC_SystemReset();
}

void CrashLog_Reset(uint32_t type, uint32_t pc)
{
  __disable_irq();
  CrashLog_Store(type, pc, 0);
  NVIC_SystemReset();
}

int CrashLog_Take(CrashInfo* out)
{
  int found = 0;
  const uint32_t rsr = RCC->RSR;

  if ((crashRecord.magic == CRASH_MAGIC) &&
      (crashRecord.check == CrashLog_Check(crashRecord.info.type, crashRecord.info.pc)) &&
      ((rsr & RCC_RSR_PORRSTF) == 0)) {
    out->type = crashRecord.info.type;
    out->pc = crashRecord.info.pc;
    out->lr = crashRecord.info.lr;
    out->cfsr = crashRecord.info.cfsr;
    out->hfsr = crashRecord.info.hfsr;
    out->addr = crashRecord.info.addr;
    found = 1;
  } else if (rsr & RCC_RSR_IWDG1RSTF) {
    out->type = CRASH_WATCHDOG;
    out->pc = 0;
    out->lr = 0;
    out->cfsr = 0;
    out->hfsr = 0;
    out->addr = 0;
    found = 1;
  }

  crashRecord.magic = 0;
  RCC->RSR |= RCC_RSR_RMVF; /* clear the reset flags for the next boot */
  return found;
}

/* IWDG1 runs from the ~32 kHz LSI: prescaler /256 and reload 4095 = about 32 s. */
void Watchdog_Start(void)
{
  DBGMCU->APB4FZ1 |= DBGMCU_APB4FZ1_DBG_IWDG1; /* stop it while a debugger halts the core */
  IWDG1->KR = 0xCCCCu;                          /* start (also turns the LSI on) */
  IWDG1->KR = 0x5555u;                          /* unlock PR / RLR */
  IWDG1->PR = 6u;                               /* /256 */
  IWDG1->RLR = 0x0FFFu;
  for (uint32_t i = 0; (IWDG1->SR != 0u) && (i < 1000000u); i++) {
  }
  IWDG1->KR = 0xAAAAu; /* reload */
}

void Watchdog_Kick(void)
{
  IWDG1->KR = 0xAAAAu;
}
