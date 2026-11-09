#include "RTE_Components.h"
#include CMSIS_device_header

//Disabling and enabling hardware write protection for RTC registers
#define RTC_UNLOCK_WRITE_PROTECTION()                                          \
  do {                                                                         \
    WRITE_REG(RTC->WPR, 0xCAU);                                                \
    WRITE_REG(RTC->WPR, 0x53U);                                                \
  } while (0)

#define RTC_LOCK_WRITE_PROTECTION() WRITE_REG(RTC->WPR, 0xFFU)

// Safe clearing of rc_w0 flags in the RTC->ISR register without affecting other flags or the INIT bit 
#define RTC_CLEAR_ISR_FLAGS(FLAGS)                                             \
  WRITE_REG(RTC->ISR, (~(uint32_t)(FLAGS)) | READ_BIT(RTC->ISR, RTC_ISR_INIT))


int main() {

  for (;;) {
  }
}

// Configure SYSCLK to 32 MHz from internal HSI16 via PLL
void Clock_Config_Max32MHz_HSI_PLL(void) {
  // Enable PWR clock and ensure APB1 write completion
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);

  // Switch VCORE to Range 1 (1.8V) and wait for stabilization
  while (READ_BIT(PWR->CSR, PWR_CSR_VOSF) != 0U) {
  }
  MODIFY_REG(PWR->CR, PWR_CR_VOS, PWR_CR_VOS_0);
  while (READ_BIT(PWR->CSR, PWR_CSR_VOSF) != 0U) {
  }

  // Enable Prefetch and 1 Wait State for Flash, then verify
  SET_BIT(FLASH->ACR, FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY);
  while (READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY) == 0U) {
  }

  // Enable HSI16 (without /4 divider) and wait until ready
  CLEAR_BIT(RCC->CR, RCC_CR_HSIDIVEN);
  SET_BIT(RCC->CR, RCC_CR_HSION);
  while (READ_BIT(RCC->CR, RCC_CR_HSIRDY) == 0U) {
  }

  // Switch to HSI16 if PLL is currently used as SYSCLK
  if (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL) {
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_HSI);
    while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
    }
  }

  // Disable PLL before configuration
  CLEAR_BIT(RCC->CR, RCC_CR_PLLON);
  while (READ_BIT(RCC->CR, RCC_CR_PLLRDY) != 0U) {
  }

  // Set AHB, APB1, and APB2 prescalers to DIV1
  MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2,
             RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1);

  // Configure PLL: 16 MHz (HSI) * 4 / 2 = 32 MHz
  MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL | RCC_CFGR_PLLDIV,
             RCC_CFGR_PLLSRC_HSI | RCC_CFGR_PLLMUL4 | RCC_CFGR_PLLDIV2);

  // Enable PLL and wait until locked
  SET_BIT(RCC->CR, RCC_CR_PLLON);
  while (READ_BIT(RCC->CR, RCC_CR_PLLRDY) == 0U) {
  }

  // Switch SYSCLK to PLL and wait for status update
  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
  while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
  }
}

// Configure SYSCLK to 32 MHz from external 8 MHz clock (HSE Bypass on CK_IN)
// via PLL
void Clock_Config_Max32MHz_HSE_Bypass_PLL(void) {
  // Safely switch SYSCLK to MSI before disabling PLL and HSE
  SET_BIT(RCC->CR, RCC_CR_MSION);
  while (READ_BIT(RCC->CR, RCC_CR_MSIRDY) == 0U) {
  }
  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_MSI);
  while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_MSI) {
  }

  // Disable PLL and HSE before reconfiguration
  CLEAR_BIT(RCC->CR, RCC_CR_PLLON);
  while (READ_BIT(RCC->CR, RCC_CR_PLLRDY) != 0U) {
  }
  CLEAR_BIT(RCC->CR, RCC_CR_HSEON);
  while (READ_BIT(RCC->CR, RCC_CR_HSERDY) != 0U) {
  }

  // Switch VCORE to Range 1 (1.8V) and wait for stabilization
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  while (READ_BIT(PWR->CSR, PWR_CSR_VOSF) != 0U) {
  }
  MODIFY_REG(PWR->CR, PWR_CR_VOS, PWR_CR_VOS_0);
  while (READ_BIT(PWR->CSR, PWR_CSR_VOSF) != 0U) {
  }

  // Enable Prefetch and 1 Wait State for Flash, then verify
  SET_BIT(FLASH->ACR, FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY);
  while (READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY) == 0U) {
  }

  // Enable HSE in Bypass mode (HSEBYP must be set while HSEON == 0)
  SET_BIT(RCC->CR, RCC_CR_HSEBYP);
  SET_BIT(RCC->CR, RCC_CR_HSEON);
  while (READ_BIT(RCC->CR, RCC_CR_HSERDY) == 0U) {
  }

  // Set AHB, APB1, and APB2 prescalers to DIV1
  MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2,
             RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1);

  // Configure PLL: 8 MHz (HSE) * 8 / 2 = 32 MHz
  MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL | RCC_CFGR_PLLDIV,
             RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMUL8 | RCC_CFGR_PLLDIV2);

  // Enable PLL and wait until locked
  SET_BIT(RCC->CR, RCC_CR_PLLON);
  while (READ_BIT(RCC->CR, RCC_CR_PLLRDY) == 0U) {
  }

  // Switch SYSCLK to PLL and wait for status update
  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
  while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
  }
}

// Downclock to 65.536 kHz (MSI Range 0), APB1 /2, 0 WS, and VCORE Range 3 (1.2V)
void Clock_Config_UltraLowPower_MSI_65kHz_Range3(void) {
  // Enable MSI and set frequency to 65.536 kHz (Range 0)
  SET_BIT(RCC->CR, RCC_CR_MSION);
  while (READ_BIT(RCC->CR, RCC_CR_MSIRDY) == 0U) {
  }
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_MSIRANGE, RCC_ICSCR_MSIRANGE_0);

  // Switch SYSCLK to MSI and wait for status update
  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_MSI);
  while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_MSI) {
  }

  // Set AHB = DIV1, APB1 = DIV2, APB2 = DIV1 and read back HPRE before lowering
  // VOS
  MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2,
             RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1);
  (void)READ_BIT(RCC->CFGR, RCC_CFGR_HPRE);

  // Disable unused high-speed oscillators
  CLEAR_BIT(RCC->CR, RCC_CR_PLLON | RCC_CR_HSION | RCC_CR_HSEON);

  // Disable Flash Prefetch and Wait State (0 WS), then verify
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_LATENCY | FLASH_ACR_PRFTEN);
  while (READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY) != 0U) {
  }

  // Switch VCORE to Range 3 (1.2V) and wait for stabilization
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  while (READ_BIT(PWR->CSR, PWR_CSR_VOSF) != 0U) {
  }
  MODIFY_REG(PWR->CR, PWR_CR_VOS, PWR_CR_VOS_0 | PWR_CR_VOS_1);
  while (READ_BIT(PWR->CSR, PWR_CSR_VOSF) != 0U) {
  }
}

// Output SYSCLK / 16 on PA9 (or PA8) MCO pin for oscilloscope verification
void Clock_Enable_MCO_PA8_SysclkDiv16(void) {
  // Enable GPIOA clock
  SET_BIT(RCC->IOPENR, RCC_IOPENR_IOPAEN);
  (void)READ_BIT(RCC->IOPENR, RCC_IOPENR_IOPAEN);

  // Configure PA8 as AF0 (MCO), Push-Pull, Very High Speed, No Pull
  CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_OT_8);
  CLEAR_BIT(GPIOA->PUPDR, GPIO_PUPDR_PUPD8);
  MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEED8, GPIO_OSPEEDER_OSPEED8);
  MODIFY_REG(GPIOA->AFR[1], GPIO_AFRH_AFSEL8, 0x00000000U);
  MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE8, GPIO_MODER_MODE8_1);

  // Configure MCO prescaler (/16) while MCO output is disabled
  MODIFY_REG(RCC->CFGR, RCC_CFGR_MCOPRE | RCC_CFGR_MCOSEL,
             RCC_CFGR_MCOPRE_DIV16);

  // Enable SYSCLK output on MCO
  MODIFY_REG(RCC->CFGR, RCC_CFGR_MCOSEL, RCC_CFGR_MCOSEL_SYSCLK);
}

// Enter Low-Power Run mode (MSI 131.072 kHz, VCORE Range 2, LP Regulator)
void PWR_Enter_LowPowerRunMode(void) {
  // Enable PWR clock
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);

  // Enable MSI, switch SYSCLK to MSI, and set Range 1 (131.072 kHz max for LP
  // Run)
  SET_BIT(RCC->CR, RCC_CR_MSION);
  while (READ_BIT(RCC->CR, RCC_CR_MSIRDY) == 0U) {
  }
  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_MSI);
  while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_MSI) {
  }
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_MSIRANGE, RCC_ICSCR_MSIRANGE_1);

  // Disable unused high-speed oscillators
  CLEAR_BIT(RCC->CR, RCC_CR_PLLON | RCC_CR_HSION | RCC_CR_HSEON);

  // Set 0 Wait State for Flash and verify
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_LATENCY | FLASH_ACR_PRFTEN);
  while (READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY) != 0U) {
  }

  // Ensure VCORE is in Range 2 (1.5V, required for LP Run)
  while (READ_BIT(PWR->CSR, PWR_CSR_VOSF) != 0U) {
  }
  MODIFY_REG(PWR->CR, PWR_CR_VOS, PWR_CR_VOS_1);
  while (READ_BIT(PWR->CSR, PWR_CSR_VOSF) != 0U) {
  }

  // Switch regulator to Low-Power mode (LPSDSR must be set before LPRUN)
  SET_BIT(PWR->CR, PWR_CR_LPSDSR);
  __DSB();
  SET_BIT(PWR->CR, PWR_CR_LPRUN);

  // Wait until regulator enters Low-Power mode
  while (READ_BIT(PWR->CSR, PWR_CSR_REGLPF) == 0U) {
  }
}

// Exit Low-Power Run mode back to normal Run mode (2.1 MHz MSI)
void PWR_Exit_LowPowerRunMode(void) {
  // Switch regulator back to Main mode (LPSDSR must NOT be cleared yet)
  CLEAR_BIT(PWR->CR, PWR_CR_LPRUN);

  // Wait until Main regulator is ready (REGLPF == 0)
  while (READ_BIT(PWR->CSR, PWR_CSR_REGLPF) != 0U) {
  }

  // Safely clear LPSDSR after regulator stabilization
  CLEAR_BIT(PWR->CR, PWR_CR_LPSDSR);

  // Restore default MSI frequency (2.097 MHz, Range 5)
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_MSIRANGE, RCC_ICSCR_MSIRANGE_5);
}

// Enter Sleep-Now mode via WFI
void PWR_Enter_SleepMode_WFI(void) {
  // Enable PWR clock and keep regulator in Main mode
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  CLEAR_BIT(PWR->CR, PWR_CR_LPSDSR);

  // Select normal Sleep-Now mode (SLEEPDEEP = 0, SLEEPONEXIT = 0)
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);

  // Clear pending EXTI and NVIC flags before sleep
  WRITE_REG(EXTI->PR, 0xFFFFFFFFUL);
  __DSB();
  NVIC_ClearPendingIRQ(EXTI0_1_IRQn);
  NVIC_ClearPendingIRQ(EXTI2_3_IRQn);
  NVIC_ClearPendingIRQ(EXTI4_15_IRQn);
  NVIC_ClearPendingIRQ(RTC_IRQn);

  // Enter Sleep mode
  __DSB();
  __WFI();
  __ISB();
}

// Enter Sleep-On-Exit mode (MCU sleeps automatically after each ISR)
void PWR_Enter_SleepOnExitMode(void) {
  // Enable PWR clock and keep regulator in Main mode
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  CLEAR_BIT(PWR->CR, PWR_CR_LPSDSR);

  // Select Sleep-On-Exit mode (SLEEPDEEP = 0, SLEEPONEXIT = 1)
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  SET_BIT(SCB->SCR, SCB_SCR_SLEEPONEXIT_Msk);

  // Clear pending EXTI and NVIC flags before sleep
  WRITE_REG(EXTI->PR, 0xFFFFFFFFUL);
  __DSB();
  NVIC_ClearPendingIRQ(EXTI0_1_IRQn);
  NVIC_ClearPendingIRQ(EXTI2_3_IRQn);
  NVIC_ClearPendingIRQ(EXTI4_15_IRQn);
  NVIC_ClearPendingIRQ(RTC_IRQn);

  // Enter Sleep mode
  __DSB();
  __WFI();
  __ISB();
}

// Enter Low-Power Sleep mode with Flash power-down and WFE (SEVONPEND) wakeup
void PWR_Enter_LowPowerSleepMode_WFE_SevOnPend(void) {
  // Enable PWR clock
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);

  // Switch SYSCLK to 131.072 kHz (MSI Range 1) before lowering voltage
  SET_BIT(RCC->CR, RCC_CR_MSION);
  while (READ_BIT(RCC->CR, RCC_CR_MSIRDY) == 0U) {
  }
  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_MSI);
  while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_MSI) {
  }
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_MSIRANGE, RCC_ICSCR_MSIRANGE_1);

  // Disable unused high-speed oscillators and Flash latency (0 WS)
  CLEAR_BIT(RCC->CR, RCC_CR_PLLON | RCC_CR_HSION | RCC_CR_HSEON);
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_LATENCY | FLASH_ACR_PRFTEN);
  while (READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY) != 0U) {
  }

  // Switch VCORE to Range 2 (1.5V, required for LP Sleep)
  while (READ_BIT(PWR->CSR, PWR_CSR_VOSF) != 0U) {
  }
  MODIFY_REG(PWR->CR, PWR_CR_VOS, PWR_CR_VOS_1);
  while (READ_BIT(PWR->CSR, PWR_CSR_VOSF) != 0U) {
  }

  // Power-down Flash and gate NVM interface clock during Sleep
  SET_BIT(FLASH->ACR, FLASH_ACR_SLEEP_PD);
  CLEAR_BIT(RCC->AHBSMENR, RCC_AHBSMENR_MIFSMEN);

  // Configure regulator for Low-Power mode during Sleep
  CLEAR_BIT(PWR->CR, PWR_CR_LPRUN);
  SET_BIT(PWR->CR, PWR_CR_LPSDSR);

  // Enable SEVONPEND for WFE wakeup on any pending IRQ
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);
  SET_BIT(SCB->SCR, SCB_SCR_SEVONPEND_Msk);

  // Clear pending EXTI and NVIC flags
  WRITE_REG(EXTI->PR, 0xFFFFFFFFUL);
  __DSB();
  NVIC_ClearPendingIRQ(EXTI0_1_IRQn);
  NVIC_ClearPendingIRQ(EXTI2_3_IRQn);
  NVIC_ClearPendingIRQ(EXTI4_15_IRQn);
  NVIC_ClearPendingIRQ(RTC_IRQn);

  // Clear internal Event Register and enter Low-Power Sleep via WFE
  __DSB();
  __SEV();
  __WFE();
  __WFE();
  __ISB();

  // Restore normal state after wakeup
  CLEAR_BIT(PWR->CR, PWR_CR_LPSDSR);
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_SLEEP_PD);
  SET_BIT(RCC->AHBSMENR, RCC_AHBSMENR_MIFSMEN);
}

// Enter Stop mode (LP Regulator, ULP + Fast Wakeup, HSI16 wakeup clock)
void PWR_Enter_StopMode_UltraLowPower(void) {
  // Enable PWR clock
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);

  // Select HSI16 (16 MHz) as wakeup clock from Stop mode
  SET_BIT(RCC->CFGR, RCC_CFGR_STOPWUCK);

  // Clear RTC wakeup/alarm/tamper flags if backup domain is open (takes 2 APB
  // cycles)
  if (READ_BIT(PWR->CR, PWR_CR_DBP) != 0U) {
    uint32_t rtc_flags = RTC_ISR_ALRAF | RTC_ISR_ALRBF | RTC_ISR_WUTF |
                         RTC_ISR_TSF | RTC_ISR_TSOVF | RTC_ISR_TAMP2F |
                         RTC_ISR_TAMP3F;
    WRITE_REG(RTC->ISR, (~rtc_flags) | READ_BIT(RTC->ISR, RTC_ISR_INIT));
    while (READ_BIT(RTC->ISR, rtc_flags) != 0U) {
    }
  }

  // Clear all pending EXTI and NVIC flags
  WRITE_REG(EXTI->PR, 0xFFFFFFFFUL);
  __DSB();
  NVIC_ClearPendingIRQ(EXTI0_1_IRQn);
  NVIC_ClearPendingIRQ(EXTI2_3_IRQn);
  NVIC_ClearPendingIRQ(EXTI4_15_IRQn);
  NVIC_ClearPendingIRQ(RTC_IRQn);

  // Configure Stop mode: PDDS = 0, LP Regulator, ULP = 1, FWU = 1, and clear
  // WUF
  MODIFY_REG(PWR->CR,
             PWR_CR_PDDS | PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU |
                 PWR_CR_CWUF,
             PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU | PWR_CR_CWUF);

  // Wait 2 SYSCLK cycles for WUF to clear
  while (READ_BIT(PWR->CSR, PWR_CSR_WUF) != 0U) {
  }

  // Set SLEEPDEEP in Cortex-M0+ core
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPONEXIT_Msk);
  SET_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);

  // Enter Stop mode
  __DSB();
  __WFI();
  __ISB();

  // Immediately clear SLEEPDEEP after wakeup
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);

  // Clear WUF and wait for VREFINT stabilization before using analog
  // peripherals
  SET_BIT(PWR->CR, PWR_CR_CWUF);
  while (READ_BIT(PWR->CSR, PWR_CSR_VREFINTRDYF) == 0U) {
  }
}

// Enter Standby mode with rising-edge wakeup on PA0 (WKUP1)
void PWR_Enter_StandbyMode_WKUP1_PA0(void) {
  // Enable PWR clock
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);

  // Enable WKUP1 pin (PA0) with internal pull-down
  SET_BIT(PWR->CSR, PWR_CSR_EWUP1);

  // Clear RTC flags if backup domain is open
  if (READ_BIT(PWR->CR, PWR_CR_DBP) != 0U) {
    uint32_t rtc_flags = RTC_ISR_ALRAF | RTC_ISR_ALRBF | RTC_ISR_WUTF |
                         RTC_ISR_TSF | RTC_ISR_TSOVF | RTC_ISR_TAMP2F |
                         RTC_ISR_TAMP3F;
    WRITE_REG(RTC->ISR, (~rtc_flags) | READ_BIT(RTC->ISR, RTC_ISR_INIT));
    while (READ_BIT(RTC->ISR, rtc_flags) != 0U) {
    }
  }

  // Clear all pending EXTI and NVIC flags
  WRITE_REG(EXTI->PR, 0xFFFFFFFFUL);
  __DSB();
  NVIC_ClearPendingIRQ(EXTI0_1_IRQn);
  NVIC_ClearPendingIRQ(EXTI2_3_IRQn);
  NVIC_ClearPendingIRQ(EXTI4_15_IRQn);
  NVIC_ClearPendingIRQ(RTC_IRQn);

  // Select Standby mode (PDDS = 1), disable VREFINT (ULP = 1), clear WUF and
  // SBF
  MODIFY_REG(PWR->CR, PWR_CR_PDDS | PWR_CR_ULP | PWR_CR_CWUF | PWR_CR_CSBF,
             PWR_CR_PDDS | PWR_CR_ULP | PWR_CR_CWUF | PWR_CR_CSBF);

  // Wait 2 SYSCLK cycles for WUF to clear
  while (READ_BIT(PWR->CSR, PWR_CSR_WUF) != 0U) {
  }

  // Set SLEEPDEEP and power off VCORE domain
  SET_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  __DSB();
  __WFI();
  __ISB();

  while (1) {
  }
}

// Check if MCU woke up from Standby mode and clear SBF/WUF flags
uint8_t PWR_CheckAndClear_StandbyWakeup(void) {
  // Enable PWR clock
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);

  // Check Standby flag (SBF)
  if (READ_BIT(PWR->CSR, PWR_CSR_SBF) != 0U) {
    // Clear SBF and WUF flags and wait for WUF to clear
    SET_BIT(PWR->CR, PWR_CR_CSBF | PWR_CR_CWUF);
    while (READ_BIT(PWR->CSR, PWR_CSR_WUF) != 0U) {
    }
    return 1U;
  }

  return 0U;
}

// Keep SWD debugger clock active during Sleep, Stop, and Standby modes
void DBG_Enable_DebugInLowPowerModes(void) {
  // Enable DBGMCU clock on APB2
  SET_BIT(RCC->APB2ENR, RCC_APB2ENR_DBGEN);
  (void)READ_BIT(RCC->APB2ENR, RCC_APB2ENR_DBGEN);

  // Enable debug support in Sleep, Stop, and Standby modes
  SET_BIT(DBGMCU->CR,
          DBGMCU_CR_DBG_SLEEP | DBGMCU_CR_DBG_STOP | DBGMCU_CR_DBG_STANDBY);
}

// Clear all pending wakeup flags in strict hierarchy: RTC -> EXTI -> NVIC ->
// PWR
void PWR_ClearAllPendingWakeupSources(void) {
  // Enable PWR clock
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);

  // Clear RTC flags (rc_w0) if backup domain is open and wait 2 APB cycles
  if (READ_BIT(PWR->CR, PWR_CR_DBP) != 0U) {
    uint32_t rtc_flags = RTC_ISR_ALRAF | RTC_ISR_ALRBF | RTC_ISR_WUTF |
                         RTC_ISR_TSF | RTC_ISR_TSOVF | RTC_ISR_TAMP2F |
                         RTC_ISR_TAMP3F;
    WRITE_REG(RTC->ISR, (~rtc_flags) | READ_BIT(RTC->ISR, RTC_ISR_INIT));
    while (READ_BIT(RTC->ISR, rtc_flags) != 0U) {
    }
  }

  // Clear all pending EXTI line flags and ensure bus write completion
  WRITE_REG(EXTI->PR, 0xFFFFFFFFUL);
  __DSB();

  // Clear pending IRQs in NVIC
  NVIC_ClearPendingIRQ(EXTI0_1_IRQn);
  NVIC_ClearPendingIRQ(EXTI2_3_IRQn);
  NVIC_ClearPendingIRQ(EXTI4_15_IRQn);
  NVIC_ClearPendingIRQ(RTC_IRQn);

  // Clear WUF and SBF flags in PWR and wait 2 SYSCLK cycles for WUF to clear
  SET_BIT(PWR->CR, PWR_CR_CWUF | PWR_CR_CSBF);
  while (READ_BIT(PWR->CSR, PWR_CSR_WUF) != 0U) {
  }
}

// Race-free Stop mode entry using PRIMASK (Joseph Yiu pattern)
void PWR_Enter_StopMode_RaceFree(void) {
  // Mask interrupts via PRIMASK (WFI still wakes up on pending NVIC IRQ)
  __disable_irq();

  // Safely clear RTC, EXTI, NVIC, and PWR wakeup flags without ISR preemption
  PWR_ClearAllPendingWakeupSources();

  // Configure Stop mode: PDDS = 0, LP Regulator, ULP = 1, FWU = 1
  MODIFY_REG(PWR->CR, PWR_CR_PDDS | PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU,
             PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU);

  // Set SLEEPDEEP in Cortex-M0+ core
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPONEXIT_Msk);
  SET_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);

  // Enter Stop mode
  __DSB();
  __WFI();
  __ISB();

  // Clear SLEEPDEEP and WUF before executing ISR
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  SET_BIT(PWR->CR, PWR_CR_CWUF);

  // Unmask interrupts to service the wakeup IRQ
  __enable_irq();
}
#define RTC_UNLOCK_WRITE_PROTECTION()                                          \
  do {                                                                         \
    WRITE_REG(RTC->WPR, 0xCAU);                                                \
    WRITE_REG(RTC->WPR, 0x53U);                                                \
  } while (0)

#define RTC_LOCK_WRITE_PROTECTION() WRITE_REG(RTC->WPR, 0xFFU)

#define RTC_CLEAR_ISR_FLAGS(FLAGS)                                             \
  WRITE_REG(RTC->ISR, (~(uint32_t)(FLAGS)) | READ_BIT(RTC->ISR, RTC_ISR_INIT))

// Initialize RTC with LSI (~37 kHz) only if calendar is not already running
void RTC_Init_LSI_SmartCalendar(void) {
  // Enable PWR clock and unlock backup domain write protection (DBP)
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  SET_BIT(PWR->CR, PWR_CR_DBP);

  // Enable LSI oscillator and wait until ready
  SET_BIT(RCC->CSR, RCC_CSR_LSION);
  while (READ_BIT(RCC->CSR, RCC_CSR_LSIRDY) == 0U) {
  }

  // If calendar is already initialized, just resynchronize shadow registers and
  // exit
  if (READ_BIT(RTC->ISR, RTC_ISR_INITS) != 0U) {
    RTC_CLEAR_ISR_FLAGS(RTC_ISR_RSF);
    while (READ_BIT(RTC->ISR, RTC_ISR_RSF) == 0U) {
    }
    return;
  }

  // Select LSI as RTCCLK source and enable RTC
  MODIFY_REG(RCC->CSR, RCC_CSR_RTCSEL, RCC_CSR_RTCSEL_LSI);
  SET_BIT(RCC->CSR, RCC_CSR_RTCEN);

  // Unlock RTC registers write protection
  RTC_UNLOCK_WRITE_PROTECTION();

  // Enter initialization mode and wait for INITF flag
  SET_BIT(RTC->ISR, RTC_ISR_INIT);
  while (READ_BIT(RTC->ISR, RTC_ISR_INITF) == 0U) {
  }

  // Configure prescalers in two separate writes (37000 Hz / (125 * 296) = 1 Hz)
  WRITE_REG(RTC->PRER, 295U);
  WRITE_REG(RTC->PRER, (124U << RTC_PRER_PREDIV_A_Pos) | 295U);

  // Select 24-hour format
  CLEAR_BIT(RTC->CR, RTC_CR_FMT);

  // Set initial time in BCD: 23:59:50
  WRITE_REG(RTC->TR, (2U << RTC_TR_HT_Pos) | (3U << RTC_TR_HU_Pos) |
                         (5U << RTC_TR_MNT_Pos) | (9U << RTC_TR_MNU_Pos) |
                         (5U << RTC_TR_ST_Pos) | (0U << RTC_TR_SU_Pos));

  // Set initial date in BCD: Friday (5), 06 November 2026
  WRITE_REG(RTC->DR, (2U << RTC_TR_HT_Pos) | (6U << RTC_DR_YU_Pos) |
                         (5U << RTC_DR_WDU_Pos) | (1U << RTC_DR_MT_Pos) |
                         (1U << RTC_DR_MU_Pos) | (0U << RTC_DR_DT_Pos) |
                         (6U << RTC_DR_DU_Pos));

  // Exit initialization mode and lock RTC registers
  CLEAR_BIT(RTC->ISR, RTC_ISR_INIT);
  RTC_LOCK_WRITE_PROTECTION();

  // Wait for shadow registers synchronization
  RTC_CLEAR_ISR_FLAGS(RTC_ISR_RSF);
  while (READ_BIT(RTC->ISR, RTC_ISR_RSF) == 0U) {
  }
}

// Configure RTC Wakeup timer and enter Stop mode with WFE wakeup (no ISR
// overhead)
void RTC_StopMode_PeriodicWakeup_WFE(uint16_t period_sec) {
  // Enable PWR clock, unlock DBP and RTC write protection
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  SET_BIT(PWR->CR, PWR_CR_DBP);
  RTC_UNLOCK_WRITE_PROTECTION();

  // Disable Wakeup timer and wait for WUTWF flag (~2 RTCCLK cycles)
  CLEAR_BIT(RTC->CR, RTC_CR_WUTE);
  while (READ_BIT(RTC->ISR, RTC_ISR_WUTWF) == 0U) {
  }

  // Set auto-reload value (period = WUT + 1 seconds)
  WRITE_REG(RTC->WUTR, (uint32_t)(period_sec - 1U));

  // Select 1 Hz ck_spre clock (WUCKSEL = 100) and enable WUTIE and WUTE
  MODIFY_REG(RTC->CR, RTC_CR_WUCKSEL,
             RTC_CR_WUCKSEL_2 | RTC_CR_WUTIE | RTC_CR_WUTE);
  RTC_LOCK_WRITE_PROTECTION();

  // Clear all pending RTC flags and wait 2 APB cycles
  uint32_t rtc_flags = RTC_ISR_ALRAF | RTC_ISR_ALRBF | RTC_ISR_WUTF |
                       RTC_ISR_TSF | RTC_ISR_TSOVF | RTC_ISR_TAMP2F |
                       RTC_ISR_TAMP3F;
  RTC_CLEAR_ISR_FLAGS(rtc_flags);
  while (READ_BIT(RTC->ISR, rtc_flags) != 0U) {
  }

  // Configure EXTI Line 20 (RTC Wakeup) in Event mode on rising edge
  CLEAR_BIT(EXTI->IMR, EXTI_IMR_IM20);
  SET_BIT(EXTI->EMR, EXTI_EMR_EM20);
  SET_BIT(EXTI->RTSR, EXTI_RTSR_RT20);

  // Clear all pending EXTI and NVIC flags before sleep
  WRITE_REG(EXTI->PR, 0xFFFFFFFFUL);
  __DSB();
  NVIC_ClearPendingIRQ(EXTI0_1_IRQn);
  NVIC_ClearPendingIRQ(EXTI2_3_IRQn);
  NVIC_ClearPendingIRQ(EXTI4_15_IRQn);
  NVIC_ClearPendingIRQ(RTC_IRQn);

  // Configure Stop mode (LP Regulator, ULP, Fast Wakeup) and wait for WUF to
  // clear
  MODIFY_REG(PWR->CR,
             PWR_CR_PDDS | PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU |
                 PWR_CR_CWUF,
             PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU | PWR_CR_CWUF);
  while (READ_BIT(PWR->CSR, PWR_CSR_WUF) != 0U) {
  }

  // Set SLEEPDEEP and enter Stop mode via WFE pattern
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPONEXIT_Msk);
  SET_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  __DSB();
  __SEV();
  __WFE();
  __WFE();
  __ISB();

  // Restore core state and clear wakeup flags after waking up
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  RTC_CLEAR_ISR_FLAGS(RTC_ISR_WUTF);
  while (READ_BIT(RTC->ISR, RTC_ISR_WUTF) != 0U) {
  }
  WRITE_REG(EXTI->PR, EXTI_PR_PIF20);
  SET_BIT(PWR->CR, PWR_CR_CWUF);
}

// Save counter to BKP0R, route Alarm A output to PA2, and enter Standby mode
void RTC_Standby_With_BackupReg_And_PA2_AlarmOutput(void) {
  // Enable PWR clock and unlock backup domain
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  SET_BIT(PWR->CR, PWR_CR_DBP);

  // Increment wakeup counter in non-volatile backup register BKP0R
  WRITE_REG(RTC->BKP0R, READ_REG(RTC->BKP0R) + 1U);

  RTC_UNLOCK_WRITE_PROTECTION();

  // Disable Alarm A and wait until register update is allowed
  CLEAR_BIT(RTC->CR, RTC_CR_ALRAE);
  while (READ_BIT(RTC->ISR, RTC_ISR_ALRAWF) == 0U) {
  }

  // Trigger Alarm A when seconds == 00 (ignore date, hours, minutes)
  WRITE_REG(RTC->ALRMAR, RTC_ALRMAR_MSK4 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK2 |
                             (0U << RTC_ALRMAR_ST_Pos) |
                             (0U << RTC_ALRMAR_SU_Pos));

  // Route Alarm A flag to PA2 (Push-Pull, Active High) and enable Alarm A
  SET_BIT(RTC->OR, RTC_OR_ALARMOUTTYPE);
  MODIFY_REG(RTC->CR, RTC_CR_OSEL | RTC_CR_POL | RTC_CR_COE,
             RTC_CR_OSEL_0 | RTC_CR_ALRAIE | RTC_CR_ALRAE);

  RTC_LOCK_WRITE_PROTECTION();

  // Clear all RTC flags and wait 2 APB cycles
  uint32_t rtc_flags = RTC_ISR_ALRAF | RTC_ISR_ALRBF | RTC_ISR_WUTF |
                       RTC_ISR_TSF | RTC_ISR_TSOVF | RTC_ISR_TAMP2F |
                       RTC_ISR_TAMP3F;
  RTC_CLEAR_ISR_FLAGS(rtc_flags);
  while (READ_BIT(RTC->ISR, rtc_flags) != 0U) {
  }

  // Clear all pending EXTI and NVIC flags
  WRITE_REG(EXTI->PR, 0xFFFFFFFFUL);
  __DSB();
  NVIC_ClearPendingIRQ(EXTI0_1_IRQn);
  NVIC_ClearPendingIRQ(EXTI2_3_IRQn);
  NVIC_ClearPendingIRQ(EXTI4_15_IRQn);
  NVIC_ClearPendingIRQ(RTC_IRQn);

  // Configure Standby mode, clear WUF/SBF, and wait for WUF to clear
  MODIFY_REG(PWR->CR, PWR_CR_PDDS | PWR_CR_ULP | PWR_CR_CWUF | PWR_CR_CSBF,
             PWR_CR_PDDS | PWR_CR_ULP | PWR_CR_CWUF | PWR_CR_CSBF);
  while (READ_BIT(PWR->CSR, PWR_CSR_WUF) != 0U) {
  }

  // Enter Standby mode
  SET_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  __DSB();
  __WFI();
  __ISB();

  while (1) {
  }
}

// Read calendar via shadow registers (BYPSHAD = 0) after waking up from Stop
// mode
void RTC_ReadCalendar_AfterStopMode(uint32_t *subsec, uint32_t *time_bcd,
                                    uint32_t *date_bcd) {
  // Ensure backup domain and RTC write protection are unlocked to clear RSF
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  SET_BIT(PWR->CR, PWR_CR_DBP);
  RTC_UNLOCK_WRITE_PROTECTION();

  // Ensure shadow registers are enabled (BYPSHAD = 0) and clear stale RSF flag
  CLEAR_BIT(RTC->CR, RTC_CR_BYPSHAD);
  RTC_CLEAR_ISR_FLAGS(RTC_ISR_RSF);
  RTC_LOCK_WRITE_PROTECTION();

  // Wait for shadow registers to update on next RTCCLK cycle
  while (READ_BIT(RTC->ISR, RTC_ISR_RSF) == 0U) {
  }

  // Read in strict order: SSR -> TR (locks shadow registers) -> DR (unlocks
  // shadow registers)
  *subsec = READ_REG(RTC->SSR);
  *time_bcd = READ_REG(RTC->TR);
  *date_bcd = READ_REG(RTC->DR);
}

// Read calendar directly from counters (BYPSHAD = 1) when APB1 clock < 7 *
// RTCCLK (LP Run)
void RTC_ReadCalendar_BypassShadow_LowPowerRun(uint32_t *subsec,
                                               uint32_t *time_bcd,
                                               uint32_t *date_bcd) {
  uint32_t ssr1, tr1, dr1;
  uint32_t ssr2, tr2, dr2;

  // Enable BYPSHAD if not already set
  if (READ_BIT(RTC->CR, RTC_CR_BYPSHAD) == 0U) {
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
    (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
    SET_BIT(PWR->CR, PWR_CR_DBP);
    RTC_UNLOCK_WRITE_PROTECTION();
    SET_BIT(RTC->CR, RTC_CR_BYPSHAD);
    RTC_LOCK_WRITE_PROTECTION();
  }

  // Read twice until two consecutive reads match to prevent asynchronous
  // rollover corruption
  do {
    ssr1 = READ_REG(RTC->SSR);
    tr1 = READ_REG(RTC->TR);
    dr1 = READ_REG(RTC->DR);

    ssr2 = READ_REG(RTC->SSR);
    tr2 = READ_REG(RTC->TR);
    dr2 = READ_REG(RTC->DR);
  } while ((ssr1 != ssr2) || (tr1 != tr2) || (dr1 != dr2));

  *subsec = ssr1;
  *time_bcd = tr1;
  *date_bcd = dr1;
}

// Enable Tamper 2 (PA0) with hardware auto-erase of BKPxR and automatic
// Timestamp on tamper event
void RTC_Enable_Tamper2_PA0_WithTimestamp_And_AutoErase(uint32_t secret_key) {
  // Enable PWR clock and unlock backup domain
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  SET_BIT(PWR->CR, PWR_CR_DBP);

  // Disable Tamper 2 before changing trigger polarity (prevents spurious
  // TAMP2F)
  CLEAR_BIT(RTC->TAMPCR, RTC_TAMPCR_TAMP2E);

  // Configure Tamper 2: edge detection, rising edge on PA0, auto-erase BKPxR,
  // save Timestamp
  MODIFY_REG(RTC->TAMPCR,
             RTC_TAMPCR_TAMPFLT | RTC_TAMPCR_TAMP2TRG |
                 RTC_TAMPCR_TAMP2NOERASE | RTC_TAMPCR_TAMP2MF,
             RTC_TAMPCR_TAMPTS | RTC_TAMPCR_TAMPIE);

  // Enable Tamper 2 detection
  SET_BIT(RTC->TAMPCR, RTC_TAMPCR_TAMP2E);

  // Clear any spurious Tamper/Timestamp flags and wait 2 APB cycles
  uint32_t tamp_flags = RTC_ISR_TAMP2F | RTC_ISR_TSF | RTC_ISR_TSOVF;
  RTC_CLEAR_ISR_FLAGS(tamp_flags);
  while (READ_BIT(RTC->ISR, tamp_flags) != 0U) {
  }

  // Store secret key into BKP1R AFTER enabling and clearing Tamper 2
  WRITE_REG(RTC->BKP1R, secret_key);

  // Configure EXTI Line 19 (Tamper/Timestamp) in Event mode on rising edge for
  // Stop wakeup
  CLEAR_BIT(EXTI->IMR, EXTI_IMR_IM19);
  SET_BIT(EXTI->EMR, EXTI_EMR_EM19);
  SET_BIT(EXTI->RTSR, EXTI_RTSR_RT19);
  WRITE_REG(EXTI->PR, EXTI_PR_PIF19);
}

// Initialize LPUART1 (PA2 = TX, PA3 = RX, AF6) clocked by HSI16 for 9600 baud
void LPUART1_Init_HSI16_9600Baud(void) {
  // Enable HSI16 oscillator and wait until ready
  CLEAR_BIT(RCC->CR, RCC_CR_HSIDIVEN);
  SET_BIT(RCC->CR, RCC_CR_HSION);
  while (READ_BIT(RCC->CR, RCC_CR_HSIRDY) == 0U) {
  }

  // Select HSI16 as LPUART1 kernel clock (LPUART1SEL = 10) before enabling UE
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPUART1SEL, RCC_CCIPR_LPUART1SEL_1);

  // Enable GPIOA and LPUART1 clocks
  SET_BIT(RCC->IOPENR, RCC_IOPENR_IOPAEN);
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN);

  // Configure PA2 (TX) and PA3 (RX) as Alternate Function 6 (AF6 = 0x06),
  // Pull-Up
  MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3,
             GPIO_PUPDR_PUPD2_0 | GPIO_PUPDR_PUPD3_0);
  MODIFY_REG(GPIOA->AFR[0], GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3,
             (6U << GPIO_AFRL_AFSEL2_Pos) | (6U << GPIO_AFRL_AFSEL3_Pos));
  MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE2 | GPIO_MODER_MODE3,
             GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);

  // Disable LPUART1 before configuration (UE = 0)
  CLEAR_BIT(LPUART1->CR1, USART_CR1_UE);

  // Configure 8N1 (M = 00, PCE = 0, STOP = 00)
  CLEAR_BIT(LPUART1->CR1, USART_CR1_M0 | USART_CR1_M1 | USART_CR1_PCE);
  CLEAR_BIT(LPUART1->CR2, USART_CR2_STOP);

  // Set BRR for 9600 baud from 16 MHz HSI16: (256 * 16000000) / 9600 = 0x682AA
  // (>= 0x300)
  WRITE_REG(LPUART1->BRR, 0x000682AAUL);

  // Enable LPUART1, Transmitter, and Receiver
  SET_BIT(LPUART1->CR1, USART_CR1_UE | USART_CR1_TE | USART_CR1_RE);

  // Wait for TEACK and REACK hardware acknowledgement from kernel clock domain
  while (READ_BIT(LPUART1->ISR, USART_ISR_TEACK | USART_ISR_REACK) !=
         (USART_ISR_TEACK | USART_ISR_REACK)) {
  }
}

// Ultra-low-power LPUART1 init clocked by LSE (32.768 kHz) at 9600 baud (0x369)
void LPUART1_Init_LSE_9600Baud(void) {
  // Enable PWR clock, unlock DBP, and start LSE crystal (32.768 kHz)
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
  SET_BIT(PWR->CR, PWR_CR_DBP);
  SET_BIT(RCC->CSR, RCC_CSR_LSEON);
  while (READ_BIT(RCC->CSR, RCC_CSR_LSERDY) == 0U) {
  }

  // Select LSE as LPUART1 kernel clock (LPUART1SEL = 11)
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPUART1SEL,
             RCC_CCIPR_LPUART1SEL_0 | RCC_CCIPR_LPUART1SEL_1);

  // Enable LPUART1 clock and disable UE before configuration
  SET_BIT(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN);
  (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN);
  CLEAR_BIT(LPUART1->CR1, USART_CR1_UE);

  // Set BRR for 9600 baud from 32.768 kHz LSE (Table 112: BRR = 0x369)
  WRITE_REG(LPUART1->BRR, 0x00000369UL);

  // Keep LSE clock enabled in Stop mode (mandatory for 9600 baud wakeup on LSE)
  SET_BIT(LPUART1->CR3, USART_CR3_UCESM);

  // Enable LPUART1, Transmitter, and Receiver, then wait for TEACK and REACK
  SET_BIT(LPUART1->CR1, USART_CR1_UE | USART_CR1_TE | USART_CR1_RE);
  while (READ_BIT(LPUART1->ISR, USART_ISR_TEACK | USART_ISR_REACK) !=
         (USART_ISR_TEACK | USART_ISR_REACK)) {
  }
}

// Enter Stop mode and wake up on ANY incoming byte via LPUART1 (WUS = RXNE)
uint8_t LPUART1_StopMode_WakeupOnByte(void) {
  // Wait until any ongoing transmission (TC == 1) or reception (BUSY == 0)
  // finishes
  while (READ_BIT(LPUART1->ISR, USART_ISR_TC) == 0U) {
  }
  while (READ_BIT(LPUART1->ISR, USART_ISR_BUSY) != 0U) {
  }

  // Ensure receiver is acknowledged (REACK == 1)
  while (READ_BIT(LPUART1->ISR, USART_ISR_REACK) == 0U) {
  }

  // Disable UE to configure WUS[1:0] = 11 (Wakeup on RXNE)
  CLEAR_BIT(LPUART1->CR1, USART_CR1_UE);
  MODIFY_REG(LPUART1->CR3, USART_CR3_WUS, USART_CR3_WUS_0 | USART_CR3_WUS_1);

  // Enable LPUART1 in Stop mode (UESM = 1), WUF interrupt (WUFIE = 1), and
  // re-enable UE
  SET_BIT(LPUART1->CR3, USART_CR3_WUFIE);
  SET_BIT(LPUART1->CR1, USART_CR1_UESM | USART_CR1_UE);
  while (READ_BIT(LPUART1->ISR, USART_ISR_REACK) == 0U) {
  }

  // Flush any stale RX data and clear LPUART1 WUF/error flags
  WRITE_REG(LPUART1->RQR, USART_RQR_RXFRQ);
  WRITE_REG(LPUART1->ICR,
            USART_ICR_WUCF | USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NCF);

  // Configure EXTI Line 28 (LPUART1 wakeup) in Event mode
  CLEAR_BIT(EXTI->IMR, EXTI_IMR_IM28);
  SET_BIT(EXTI->EMR, EXTI_EMR_EM28);

  // Clear all pending wakeup sources and enter Stop mode via WFE
  PWR_ClearAllPendingWakeupSources();
  MODIFY_REG(PWR->CR, PWR_CR_PDDS | PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU,
             PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU);

  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPONEXIT_Msk);
  SET_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  __DSB();
  __SEV();
  __WFE();
  __WFE();
  __ISB();

  // Disable UESM and clear WUF after waking up from Stop mode
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  CLEAR_BIT(LPUART1->CR1, USART_CR1_UESM);
  WRITE_REG(LPUART1->ICR, USART_ICR_WUCF);
  SET_BIT(PWR->CR, PWR_CR_CWUF);

  // Return the received byte that woke up the MCU (reading RDR clears RXNE)
  return (uint8_t)READ_REG(LPUART1->RDR);
}

// Trick: Mute Mode + Stop Mode hardware address filter (wakes up MCU ONLY on
// matching 7-bit address)
void LPUART1_StopMode_WakeupOnAddressMatch(uint8_t node_addr_7bit) {
  // Wait for ongoing transfers to complete
  while (READ_BIT(LPUART1->ISR, USART_ISR_TC) == 0U) {
  }
  while (READ_BIT(LPUART1->ISR, USART_ISR_BUSY) != 0U) {
  }

  // Disable UE to configure Address Match wakeup and Mute mode
  CLEAR_BIT(LPUART1->CR1, USART_CR1_UE);

  // Set 7-bit address detection (ADDM7 = 1) and node address in ADD[7:0] (MSB
  // must be 1 on wire)
  MODIFY_REG(LPUART1->CR2, USART_CR2_ADD | USART_CR2_ADDM7,
             ((uint32_t)(node_addr_7bit & 0x7FU) << USART_CR2_ADD_Pos) |
                 USART_CR2_ADDM7);

  // Select Wakeup from Mute mode on Address Mark (WAKE = 1) and enable Mute
  // mode (MME = 1)
  SET_BIT(LPUART1->CR1, USART_CR1_WAKE | USART_CR1_MME);

  // Select Wakeup from Stop mode on Address Match (WUS[1:0] = 00) and enable
  // WUFIE
  CLEAR_BIT(LPUART1->CR3, USART_CR3_WUS);
  SET_BIT(LPUART1->CR3, USART_CR3_WUFIE);

  // Enable UESM and UE, then wait for REACK
  SET_BIT(LPUART1->CR1, USART_CR1_UESM | USART_CR1_UE);
  while (READ_BIT(LPUART1->ISR, USART_ISR_REACK) == 0U) {
  }

  // Flush RX (RXNE must be 0 when entering Mute + Stop) and force Mute mode via
  // MMRQ
  WRITE_REG(LPUART1->RQR, USART_RQR_RXFRQ | USART_RQR_MMRQ);
  WRITE_REG(LPUART1->ICR,
            USART_ICR_WUCF | USART_ICR_CMCF | USART_ICR_ORECF | USART_ICR_FECF);

  // Wait until receiver enters Mute mode (RWU == 1)
  while (READ_BIT(LPUART1->ISR, USART_ISR_RWU) == 0U) {
  }

  // Enable EXTI Line 28 (LPUART1) in Event mode
  CLEAR_BIT(EXTI->IMR, EXTI_IMR_IM28);
  SET_BIT(EXTI->EMR, EXTI_EMR_EM28);

  // Clear all pending wakeup flags and enter Stop mode
  PWR_ClearAllPendingWakeupSources();
  MODIFY_REG(PWR->CR, PWR_CR_PDDS | PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU,
             PWR_CR_LPSDSR | PWR_CR_ULP | PWR_CR_FWU);

  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPONEXIT_Msk);
  SET_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  __DSB();
  __SEV();
  __WFE();
  __WFE();
  __ISB();

  // MCU wakes up ONLY when (0x80 | node_addr_7bit) is received on RX!
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  CLEAR_BIT(LPUART1->CR1, USART_CR1_UESM);
  WRITE_REG(LPUART1->ICR, USART_ICR_WUCF);
  SET_BIT(PWR->CR, PWR_CR_CWUF);
}