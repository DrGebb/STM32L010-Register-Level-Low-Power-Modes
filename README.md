# STM32L010F4P6 Bare-Metal Low-Power & Clock Management

A register-level demonstration project for the **STM32L010F4P6** (Cortex-M0+, Category 1) microcontroller. 

The goal of this project is to demonstrate direct hardware manipulation using **CMSIS** register definitions and bit-manipulation macros (`SET_BIT`, `CLEAR_BIT`, `MODIFY_REG`, `READ_BIT`, `WRITE_REG`) without relying on HAL or LL drivers. All hardware sequences strictly follow the **RM0451** Reference Manual and Cortex-M0+ architectural guidelines.

## Development Environment

* **IDE:** VS Code
* **Toolchain:** Keil MDK v6 (Arm Compiler 6) with **CMSIS-Toolbox** (`csolution` / `cbuild`)
* **Target MCU:** STM32L010F4P6 (TSSOP-20, 16 KB Flash, 2 KB RAM)

## Implemented Modules & Features

### 1. Clock Control (`RCC` & `FLASH`)
* **System Clock Scaling:** Safe transitions between internal (`MSI`, `HSI16`), external (`HSE Bypass` on `CK_IN`), and `PLL` clock sources up to 32 MHz.
* **Flash Latency Synchronization:** Proper `FLASH_ACR` Wait States (`LATENCY`) and Prefetch (`PRFTEN`) management with mandatory read-back verification before/after frequency transitions.
* **Ultra-Low-Power Downclocking:** Switching to 65.536 kHz (`MSI Range 0`) with bus prescaler adjustments.
* **Clock Verification:** `MCO` pin configuration with prescaler setup for oscilloscope verification.

### 2. Power Management (`PWR` & Core `SCB`)
* **Dynamic Voltage Scaling:** Core voltage regulation across Range 1 (1.8V), Range 2 (1.5V), and Range 3 (1.2V) synchronized via `VOSF` polling.
* **Low-Power Run & Sleep Modes:** Regulator transitions (`LPSDSR` / `LPRUN`), peripheral clock gating in Sleep (`IOPSMENR` / `AHBSMENR`), Flash power-down (`SLEEP_PD`), and `Sleep-On-Exit` configuration.
* **Stop & Standby Modes:** Ultra-low-power entry (`ULP`, `FWU`), race-free `WFI` entry using `PRIMASK`, and `WFE` (`SEVONPEND`) event-driven wakeup sequences.
* **Wakeup Hygiene:** Hierarchical clearing of pending wakeup flags (`RTC` -> `EXTI` -> `NVIC` -> `PWR`) with required bus synchronization barriers (`__DSB`, `__ISB`).

### 3. Real-Time Clock (`RTC`)
* **Calendar & Backup Domain:** `LSI` initialization with two-step prescaler writes (`RTC_PRER`), write-protection key handling (`RTC_WPR`), and state retention check (`INITS`) across resets.
* **Periodic Auto-Wakeup:** `Stop` mode wakeup using the 16-bit Wakeup Timer (`WUT`) via `EXTI Line 20` in Event mode (zero ISR overhead).
* **Hardware Alarm & Tamper:** `Standby` wakeup with hardware output routing to `PA2` (`RTC_ALARM`), backup register (`BKPxR`) retention, and `Tamper 2` (`PA0`) detection with automatic timestamping (`TAMPTS`) and hardware backup erase.
* **Safe Calendar Readout:** Shadow register resynchronization (`RSF`) after `Stop` mode and double-read polling in `BYPSHAD` mode when `f_APB1 < 7 * f_RTCCLK`.

### 4. Low-Power UART (`LPUART1`)
* **Dual-Clock Domain Operation:** Configuration using `HSI16` or `LSE` (32.768 kHz at 9600 baud) independent of `PCLK`.
* **Autonomous Stop Mode Wakeup:** On-demand `HSI16` activation upon start-bit detection and wakeup on `RXNE` (`EXTI Line 28`).
* **Hardware Address Filtering:** `Mute Mode` + `Stop Mode` integration to wake the CPU only when a matching 7-bit node address is received on the bus.