#include <errno.h>
#include <isr.h>

#include <plat/inc/cmsis.h>
#include <plat/inc/exti.h>
#include <plat/inc/pwr.h>

struct StmExti
{
    volatile uint32_t IMR;
    volatile uint32_t EMR;
    volatile uint32_t RTSR;
    volatile uint32_t FTSR;
    volatile uint32_t SWIER;
    volatile uint32_t PR;
};

#define EXTI ((struct StmExti*)EXTI_BASE)

void extiEnableIntLine(const enum ExtiLine line, enum ExtiTrigger trigger)
{
    if (trigger == EXTI_TRIGGER_BOTH) {
        EXTI->RTSR |= (1UL << line);
        EXTI->FTSR |= (1UL << line);
    } else if (trigger == EXTI_TRIGGER_RISING) {
        EXTI->RTSR |= (1UL << line);
        EXTI->FTSR &= ~(1UL << line);
    } else if (trigger == EXTI_TRIGGER_FALLING) {
        EXTI->RTSR &= ~(1UL << line);
        EXTI->FTSR |= (1UL << line);
    }

    /* Clear pending interrupt */
    extiClearPendingLine(line);

    /* Enable hardware interrupt */
    EXTI->IMR |= (1UL << line);
}

void extiDisableIntLine(const enum ExtiLine line)
{
    EXTI->IMR &= ~(1UL << line);
}

void extiClearPendingLine(const enum ExtiLine line)
{
    EXTI->PR |= (1UL << line);
}

bool extiIsPendingLine(const enum ExtiLine line)
{
    return (EXTI->PR & (1UL << line)) ? true : false;
}

struct ExtiInterrupt
{
    struct ChainedInterrupt base;
    IRQn_Type irq;
};

static void extiInterruptEnable(struct ChainedInterrupt *irq)
{
    struct ExtiInterrupt *exti = container_of(irq, struct ExtiInterrupt, base);
    NVIC_EnableIRQ(exti->irq);
}

static void extiInterruptDisable(struct ChainedInterrupt *irq)
{
    struct ExtiInterrupt *exti = container_of(irq, struct ExtiInterrupt, base);
    NVIC_DisableIRQ(exti->irq);
}

#define DECLARE_SHARED_EXTI(i) {            \
    .base = {                               \
        .enable = extiInterruptEnable,      \
        .disable = extiInterruptDisable,    \
    },                                      \
    .irq = i,                               \
}

static struct ExtiInterrupt gInterrupts[] = {
    DECLARE_SHARED_EXTI(EXTI0_IRQn),
    DECLARE_SHARED_EXTI(EXTI1_IRQn),
    DECLARE_SHARED_EXTI(EXTI2_IRQn),
    DECLARE_SHARED_EXTI(EXTI3_IRQn),
    DECLARE_SHARED_EXTI(EXTI4_IRQn),
    DECLARE_SHARED_EXTI(EXTI9_5_IRQn),
    DECLARE_SHARED_EXTI(EXTI15_10_IRQn),
};

static inline struct ExtiInterrupt *extiForIrq(IRQn_Type n)
{
    if (n >= EXTI0_IRQn && n <= EXTI4_IRQn)
        return &gInterrupts[n - EXTI0_IRQn];
    if (n == EXTI9_5_IRQn)
        return &gInterrupts[ARRAY_SIZE(gInterrupts) - 2];
    if (n == EXTI15_10_IRQn)
        return &gInterrupts[ARRAY_SIZE(gInterrupts) - 1];
    return NULL;
}

static void extiIrqHandler(IRQn_Type n)
{
    struct ExtiInterrupt *exti = extiForIrq(n);
    dispatchIsr(&exti->base);
}

#define DEFINE_SHARED_EXTI_ISR(i)           \
    void EXTI##i##_IRQHandler(void);        \
    void EXTI##i##_IRQHandler(void) {       \
        extiIrqHandler(EXTI##i##_IRQn);     \
    }                                       \

DEFINE_SHARED_EXTI_ISR(0)
DEFINE_SHARED_EXTI_ISR(1)
DEFINE_SHARED_EXTI_ISR(2)
DEFINE_SHARED_EXTI_ISR(3)
DEFINE_SHARED_EXTI_ISR(4)
DEFINE_SHARED_EXTI_ISR(9_5)
DEFINE_SHARED_EXTI_ISR(15_10)

int extiChainIsr(IRQn_Type n, struct ChainedIsr *isr)
{
    struct ExtiInterrupt *exti = extiForIrq(n);
    if (!exti)
        return -EINVAL;

    chainIsr(&exti->base, isr);
    return 0;
}

int extiUnchainIsr(IRQn_Type n, struct ChainedIsr *isr)
{
    struct ExtiInterrupt *exti = extiForIrq(n);
    if (!exti)
        return -EINVAL;

    unchainIsr(&exti->base, isr);
    return 0;
}
