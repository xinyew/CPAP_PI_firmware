/*
 * CPAP PI Sensor Control board early init — self-heals the two UICR
 * words that a full chip erase destroys:
 *
 * 1. APPROTECT (hardened silicon, nRF52840 rev 3 / build code F0+):
 *    SystemInit copies UICR->APPROTECT into the APPROTECT soft branch
 *    at every boot; the erased value (0xFF) locks the debug port, and
 *    raw J-Link flashing tools (nRF Connect Programmer with a bare
 *    zephyr.hex) do not restore the 0x5A "HwDisabled" byte the way
 *    nrfutil/nrfjprog do. Result: firmware runs but no probe can
 *    attach. Restore 0x5A in UICR (future boots) and software-open
 *    the branch immediately (this boot). Detected at runtime via the
 *    MDK configuration-249 check, so pre-rev-3 silicon — where 0x5A
 *    would mean the OPPOSITE (protected) — is left untouched.
 *
 * 2. REGOUT0: the module runs in high-voltage mode (VDDH = DVDD =
 *    3.3 V) with the REG0 DCDC inductor (L4) unpopulated, so the
 *    GPIO rail (VDD) comes from REG0 in LDO mode. REGOUT0 defaults
 *    to 1.8 V after an erase, while all pull-ups and the MS5611 SPI
 *    inputs sit at 3.3 V — program 3V3 and reset for it to latch.
 */

#include <zephyr/init.h>
#include <hal/nrf_power.h>
#include <mdk/nrf52_erratas.h>

static void uicr_write(volatile uint32_t *reg, uint32_t val)
{
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		;
	}

	*reg = val;

	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		;
	}
}

void board_early_init_hook(void)
{
	/* --- 1. APPROTECT: keep the debug port open (hardened silicon) --- */
#if NRF52_CONFIGURATION_249_PRESENT
	if (nrf52_configuration_249() &&
	    ((NRF_UICR->APPROTECT & UICR_APPROTECT_PALL_Msk) !=
	     (UICR_APPROTECT_PALL_HwDisabled << UICR_APPROTECT_PALL_Pos))) {

		/* Program only the PALL byte; leave reserved bits erased */
		uicr_write(&NRF_UICR->APPROTECT,
			   0xFFFFFF00UL | UICR_APPROTECT_PALL_HwDisabled);

		/* Open the soft branch now — SystemInit already ran with
		 * the erased UICR value, so redo its job for this session
		 * instead of waiting for the next reset.
		 */
		NRF_APPROTECT->DISABLE = APPROTECT_DISABLE_DISABLE_SwDisable;
	}
#endif

	/* --- 2. REGOUT0: GPIO rail must be 3.3 V (see header comment) --- */
	if ((nrf_power_mainregstatus_get(NRF_POWER) ==
	     NRF_POWER_MAINREGSTATUS_HIGH) &&
	    ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
	     (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos))) {

		uicr_write(&NRF_UICR->REGOUT0,
			   (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
			   (UICR_REGOUT0_VOUT_3V3 << UICR_REGOUT0_VOUT_Pos));

		/* a reset is required for the new voltage to take effect */
		NVIC_SystemReset();
	}
}
