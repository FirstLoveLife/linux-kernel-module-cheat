#include <stdio.h>
#include <inttypes.h>

#include <lkmc.h>
#include <lkmc/gicv3.h>

#define IRQ_FOUND          (0)
#define IRQ_NOT_FOUND      (1)

void enable_irq(void) {
	__asm__ __volatile__("msr DAIFClr, %0\n\t" : : "i" (LKMC_SYSREG_BITS_DAIF_IRQ)  : "memory");
}

void disable_irq(void) {
	__asm__ __volatile__("msr DAIFSet, %0\n\t" : : "i" (LKMC_SYSREG_BITS_DAIF_IRQ)  : "memory");
}

/* Processor status word. */
void psw_disable_and_save_interrupt(uint64_t *pswp) {
	uint64_t psw;
	psw = lkmc_sysreg_read_daif();
	disable_irq();
	*pswp = psw;
}

/* Processor status word. */
void psw_restore_interrupt(uint64_t *pswp) {
	uint64_t psw;
	psw = *pswp;
	lkmc_sysreg_write_daif(psw);
}

void lkmc_vector_trap_handler(
    LkmcVectorExceptionFrame *exception __attribute__((unused))
) {
    uint64_t psw;
    irq_no irq;
    int rc;
    if ((exception->exc_type & 0xff) == LKMC_VECTOR_IRQ_SPX) {
        psw_disable_and_save_interrupt(&psw);
        rc = gic_v3_find_pending_irq(exception, &irq);
        if (rc != IRQ_FOUND)  {
            puts("IRQ not found!");
            goto restore_irq_out;
        } else {
            printf("IRQ found: 0x%" PRIX32 "\n", irq);
        }
        gicd_disable_int(irq);
        gic_v3_eoi(irq);
        printf("CNTVCT_EL0 0x%" PRIX64 "\n", lkmc_sysreg_read_cntvct_el0());
        gicd_enable_int(irq);
restore_irq_out:
        psw_restore_interrupt(&psw);
    }
}

void enable_cntv(void) {
    lkmc_sysreg_write_cntv_ctl_el0(lkmc_sysreg_read_cntv_ctl_el0() | LKMC_CNTV_CTL_ENABLE);
}

int main(void) {
    /* Initial state. */
    lkmc_sysreg_print_cntv_ctl_el0();
    lkmc_sysreg_print_cntfrq_el0();
    lkmc_sysreg_print_cntv_cval_el0();

    /* Get the counter value many times to watch the time pass. */
    lkmc_sysreg_print_cntvct_el0();
    lkmc_sysreg_print_cntvct_el0();
    lkmc_sysreg_print_cntvct_el0();

    gic_v3_initialize();
    {
        /*uint64_t ticks, current_cnt;*/
        /*uint32_t cntfrq;*/
        lkmc_sysreg_write_cntfrq_el0(1);
        /*ticks = cntfrq;*/
        /*current_cnt = lkmc_sysreg_read_cntvct_el0();*/
        /*lkmc_sysreg_write_cntv_cval_el0(current_cnt + ticks);*/
        enable_cntv();
        enable_irq();
    }
    while (1) {
        LKMC_WFI;
    }
    return 0;
}
