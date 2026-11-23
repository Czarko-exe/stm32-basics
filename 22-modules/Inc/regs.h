#ifndef REGS_H
#define REGS_H

#include <stdint.h>

#define RCC_AHB1ENR		(*(volatile uint32_t *)0x40023830)
#define GPIOA_MODER		(*(volatile uint32_t *)0x40020000)
#define GPIOB_MODER		(*(volatile uint32_t *)0x40020400)
#define RCC_APB1ENR		(*(volatile uint32_t *)0x40023840)
#define GPIOB_AFRH		(*(volatile uint32_t *)0x40020424)
#define GPIOB_OTYPER	(*(volatile uint32_t *)0x40020404)

#define SYST_CSR		(*(volatile uint32_t *)0xE000E010)
#define SYST_RVR		(*(volatile uint32_t *)0xE000E014)
#define SYST_CVR		(*(volatile uint32_t *)0xE000E018)

#define RCC_APB2ENR		(*(volatile uint32_t *)0x40023844)
#define SPI1_CR1		(*(volatile uint32_t *)0x40013000)
#define SPI1_SR			(*(volatile uint32_t *)0x40013008)
#define SPI1_DR			(*(volatile uint32_t *)0x4001300C)
#define GPIOB_AFRL		(*(volatile uint32_t *)0x40020420)
#define GPIOB_BSRR		(*(volatile uint32_t *)0x40020418)
#define GPIOA_BSRR		(*(volatile uint32_t *)0x40020018)
#define SPI1_CR2		(*(volatile uint32_t *)0x40013004)

#define I2C1_CR1		(*(volatile uint32_t *)0x40005400)
#define I2C1_CR2		(*(volatile uint32_t *)0x40005404)
#define I2C1_DR			(*(volatile uint32_t *)0x40005410)
#define I2C1_SR1		(*(volatile uint32_t *)0x40005414)
#define I2C1_SR2		(*(volatile uint32_t *)0x40005418)
#define I2C1_CCR		(*(volatile uint32_t *)0x4000541C)
#define I2C1_TRISE		(*(volatile uint32_t *)0x40005420)

#define DMA2_S3CR		(*(volatile uint32_t *)0x40026458)
#define DMA2_S3NDTR		(*(volatile uint32_t *)0x4002645C)
#define DMA2_S3PAR		(*(volatile uint32_t *)0x40026460)
#define DMA2_S3M0AR		(*(volatile uint32_t *)0x40026464)
#define DMA2_S3FCR		(*(volatile uint32_t *)0x4002646C)
#define DMA2_LISR		(*(volatile uint32_t *)0x40026400)
#define DMA2_LIFCR		(*(volatile uint32_t *)0x40026408)

#endif
