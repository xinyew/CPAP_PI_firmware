/*
 * CPAP PI Sensor Control board early init.
 *
 * The MDBT50Q-P1M module runs in high-voltage mode (VDDH = DVDD = 3.3 V)
 * and the REG0 DCDC inductor (L4) is unpopulated, so the GPIO rail (VDD)
 * comes from REG0 in LDO mode. UICR REGOUT0 defaults to 1.8 V and is
 * wiped back to that default by every `west flash --recover`.
 *
 * All I2C/INT/reset pull-ups on this board go to DVDD (3.3 V) and the six
 * MS5611 SPI inputs are supplied at 3.3 V, so GPIO MUST run at 3.3 V:
 * program REGOUT0 = 3V3 on first boot after an erase, then reset for the
 * new setting to take effect.
 */

#include <zephyr/init.h>
#include <hal/nrf_power.h>

void board_early_init_hook(void)
{
	if ((nrf_power_mainregstatus_get(NRF_POWER) ==
	     NRF_POWER_MAINREGSTATUS_HIGH) &&
	    ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
	     (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos))) {

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}

		NRF_UICR->REGOUT0 =
		    (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
		    (UICR_REGOUT0_VOUT_3V3 << UICR_REGOUT0_VOUT_Pos);

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}

		/* a reset is required for changes to take effect */
		NVIC_SystemReset();
	}
}
