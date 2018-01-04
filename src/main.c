//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/Trace.h"

#include "../externals/qpc/include/qpc.h"

#include "stm32f10x.h"
#include "core_cm3.h"
#include "my_system.h"

// ----------------------------------------------------------------------------
//
// Semihosting STM32F1 empty sample (trace via ITM).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the ITM output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

#define BSP_TICKS_PER_SEC    1000U

enum KernelAwareISRs {
    SYSTICK_PRIO = QF_AWARE_ISR_CMSIS_PRI, /* see NOTE00 */
    /* ... */
    MAX_KERNEL_AWARE_CMSIS_PRI /* keep always last */
};

/* ISRs used in this project ===============================================*/
void SysTick_Handler(void) {
    QXK_ISR_ENTRY();   /* inform QXK about entering an ISR */

#ifdef Q_SPY
    {
        tmp = SysTick->CTRL; /* clear SysTick_CTRL_COUNTFLAG */
        QS_tickTime_ += QS_tickPeriod_; /* account for the clock rollover */
    }
#endif

    QF_TICK_X(0U, &l_SysTick); /* process time events for rate 0 */

    QXK_ISR_EXIT();  /* inform QXK about exiting an ISR */
}

/* BSP functions ===========================================================*/
void BSP_init(void) {
    /* NOTE: SystemInit() already called from the startup code
    *  but SystemCoreClock needs to be updated
    */
    SystemCoreClockUpdate();

    /* NOTE: SystemInit() already called from the startup code
    *  but SystemCoreClock needs to be updated
    */
    SystemCoreClockUpdate();
}
/* QF callbacks ============================================================*/
void QF_onStartup(void) {
    /* set up the SysTick timer to fire at BSP_TICKS_PER_SEC rate */
    SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);

    /* assing all priority bits for preemption-prio. and none to sub-prio. */
    NVIC_SetPriorityGrouping(0U);

    /* set priorities of ALL ISRs used in the system, see NOTE00
    *
    * !!!!!!!!!!!!!!!!!!!!!!!!!!!! CAUTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    * Assign a priority to EVERY ISR explicitly by calling NVIC_SetPriority().
    * DO NOT LEAVE THE ISR PRIORITIES AT THE DEFAULT VALUE!
    */
    NVIC_SetPriority(SysTick_IRQn,   SYSTICK_PRIO);
    /* ... */

    /* enable IRQs... */
#ifdef Q_SPY
    NVIC_EnableIRQ(USART2_IRQn); /* USART2 interrupt used for QS-RX */
#endif
}

/*..........................................................................*/
void QF_onCleanup(void) {
}
/*..........................................................................*/
void QXK_onIdle(void) {
    QF_INT_DISABLE();
    QF_INT_ENABLE();

#ifdef Q_SPY
    QS_rxParse();  /* parse all the received bytes */

    if ((USART2->SR & USART_FLAG_TXE) != 0) { /* TXE empty? */
        uint16_t b;

        QF_INT_DISABLE();
        b = QS_getByte();
        QF_INT_ENABLE();

        if (b != QS_EOD) {  /* not End-Of-Data? */
            USART2->DR  = (b & 0xFFU);  /* put into the DR register */
        }
    }
#elif defined NDEBUG
    /* Put the CPU and peripherals to the low-power mode.
    * you might need to customize the clock management for your application,
    * see the datasheet for your particular Cortex-M MCU.
    */
    /* !!!CAUTION!!!
    * The WFI instruction stops the CPU clock, which unfortunately disables
    * the JTAG port, so the ST-Link debugger can no longer connect to the
    * board. For that reason, the call to __WFI() has to be used with CAUTION.
    *
    * NOTE: If you find your board "frozen" like this, strap BOOT0 to VDD and
    * reset the board, then connect with ST-Link Utilities and erase the part.
    * The trick with BOOT(0) is it gets the part to run the System Loader
    * instead of your broken code. When done disconnect BOOT0, and start over.
    */
    //__WFI(); /* Wait-For-Interrupt */
#endif
}

/*..........................................................................*/
void Q_onAssert(char const *module, int loc) {
    /*
    * NOTE: add here your application-specific error handling
    */
    (void)module;
    (void)loc;
    puts("[HIEUTC] Ek!\r\n");
    QS_ASSERTION(module, loc, (uint32_t)10000U); /* report assertion to QS */
    NVIC_SystemReset();
}
/* local "naked" thread object .............................................*/
static QXThread l_test1;
static QXThread l_test2;
static QXMutex l_mutex;
static QXSemaphore l_sema;

/* global pointer to the test thread .......................................*/
QXThread * const XT_Test1 = &l_test1;
QXThread * const XT_Test2 = &l_test2;

/*..........................................................................*/
static void Thread1_run(QXThread * const me) {

    QXMutex_init(&l_mutex, 3U);

    (void)me;
    for (;;) {
        float volatile x;
        puts("[HIEUTC] [Thread1] 1st step\r\n.");
        /* wait on a semaphore (BLOCK with timeout) */
        int i = QXK_ISR_CONTEXT_();
        if(i == 0) {
        	puts("[HIEUTC] EkEk\r\n");
        }
        (void)QXSemaphore_wait(&l_sema, BSP_TICKS_PER_SEC);
        //BSP_ledOn();
        puts("[HIEUTC3] [Thread1] 2nd step\r\n.");
        QXMutex_lock(&l_mutex, QXTHREAD_NO_TIMEOUT); /* lock the mutex */
        puts("[HIEUTC3] [Thread1] 3rd step\r\n.");
        /* some flating point code to exercise the VFP... */
        x = 1.4142135F;
        x = x * 1.4142135F;
        QXMutex_unlock(&l_mutex);
        puts("[HIEUTC3] [Thread1] 4th step\r\n.");

        QXThread_delay(BSP_TICKS_PER_SEC/7);  /* BLOCK */
        puts("[HIEUTC3] [Thread1] 5th step\r\n.");

        /* publish to thread2 */
        //QF_PUBLISH(Q_NEW(QEvt, TEST_SIG), &l_test1);
    }
}

/*..........................................................................*/
void Test1_ctor(void) {
    QXThread_ctor(&l_test1, Q_XTHREAD_CAST(&Thread1_run), 0U);
}
/*..........................................................................*/
static void Thread2_run(QXThread * const me) {

    /* subscribe to the test signal */
    QActive_subscribe(&me->super, TEST_SIG);

    /* initialize the semaphore before using it
    * NOTE: the semaphore is initialized in the highest-priority thread
    * that uses it. Alternatively, the semaphore can be initialized
    * before any thread runs.
    */
    QXSemaphore_init(&l_sema,
                     0U,  /* count==0 (signaling semaphore) */
                     1U); /* max_count==1 (binary semaphore) */

    for (;;) {
        QEvt const *e;

        /* some flating point code to exercise the VFP... */
        float volatile x;
        x = 1.4142135F;
        x = x * 1.4142135F;

        /* wait on the internal event queue (BLOCK) with timeout */
        puts("[HIEUTC3] [Thread2] 1st step\r\n.");
        e = QXThread_queueGet(BSP_TICKS_PER_SEC/2);
        puts("[HIEUTC3] [Thread2] 2nd step\r\n.");
        //BSP_ledOff();

        if (e != (QEvt *)0) { /* event actually delivered? */
        	puts("[HIEUTC3] [Thread2] 3rd.1 step\r\n.");
            QF_gc(e); /* recycle the event manually! */
        }
        else {
        	puts("[HIEUTC3] [Thread2] 3rd.2 step\r\n.");
            QXThread_delay(BSP_TICKS_PER_SEC/2);  /* wait more (BLOCK) */
            puts("[HIEUTC3] [Thread2] 3rd.3 step\r\n.");
            QXSemaphore_signal(&l_sema); /* signal Thread1 */
            puts("[HIEUTC3] [Thread2] 3rd.4 step\r\n.");
        }
    }
}

/*..........................................................................*/
void Test2_ctor(void) {
    QXThread_ctor(&l_test2, Q_XTHREAD_CAST(&Thread2_run), 0U);
}
int
main(int argc, char* argv[])
{
  // At this stage the system clock should have already been configured
  // at high speed.

	static QEvt const *tableQueueSto[N_PHILO];
	static QEvt const *philoQueueSto[N_PHILO][N_PHILO];
	static QSubscrList subscrSto[MAX_PUB_SIG];
	static QF_MPOOL_EL(TableEvt) smlPoolSto[2*N_PHILO]; /* small pool */

	/* stacks and queues for the extended test threads */
	static void const *test1QueueSto[5];
	static uint64_t test1StackSto[64];
	static void const *test2QueueSto[5];
	static uint64_t test2StackSto[64];
    Test1_ctor(); /* instantiate the Test1 extended thread */
    Test2_ctor();

	QF_init();    /* initialize the framework and the underlying RT kernel */

	    /* initialize publish-subscribe... */
	    QF_psInit(subscrSto, Q_DIM(subscrSto));

	    /* initialize event pools... */
	    QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));

	    /* initialize the Board Support Package
	    * NOTE: BSP_init() is called *after* initializing publish-subscribe and
	    * event pools, to make the system ready to accept SysTick interrupts.
	    * Unfortunately, the STM32Cube code that must be called from BSP,
	    * configures and starts SysTick.
	    */
	    BSP_init();
	    QXSemaphore_init(&l_sema,
	                         0U,  /* count==0 (signaling semaphore) */
	                         1U); /* max_count==1 (binary semaphore) */
	    //QXMutex_init(&l_mutex, 3U);
	    /* start the extended thread */
		QXTHREAD_START(XT_Test1,                 /* Thread to start */
					  (uint_fast8_t)1U,          /* QP priority of the thread */
					  test1QueueSto,             /* message queue storage */
					  Q_DIM(test1QueueSto),      /* message length [events] */
					  test1StackSto,             /* stack storage */
					  sizeof(test1StackSto),     /* stack size [bytes] */
					  (QEvt *)0);                /* initialization event */
		QXTHREAD_START(XT_Test2,                 /* Thread to start */
		                  (uint_fast8_t)(N_PHILO + 2), /* QP priority of the thread */
		                  test2QueueSto,             /* message queue storage */
		                  Q_DIM(test2QueueSto),      /* message length [events] */
		                  test2StackSto,             /* stack storage */
		                  sizeof(test2StackSto),     /* stack size [bytes] */
		                  (QEvt *)0);                /* initialization event */

		QF_run();
  return 0;
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
