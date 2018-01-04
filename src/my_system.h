/*
 * my_system.h
 *
 *  Created on: Jan 4, 2018
 *      Author: htc02
 */

#ifndef MY_SYSTEM_H_
#define MY_SYSTEM_H_



enum DPPSignals {
    EAT_SIG = Q_USER_SIG, /* published by Table to let a philosopher eat */
    DONE_SIG,       /* published by Philosopher when done eating */
    PAUSE_SIG,      /* published by BSP to pause serving forks */
    SERVE_SIG,      /* published by BSP to serve re-start serving forks */
    TEST_SIG,       /* published by BSP to test the application */
    MAX_PUB_SIG,    /* the last published signal */

    HUNGRY_SIG,     /* posted direclty to Table from hungry Philo */
    TIMEOUT_SIG,    /* used by Philosophers for time events */
    MAX_SIG         /* the last signal */
};

#if ((QP_VERSION < 580) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8)))
#error qpc version 5.8.0 or higher required
#endif

/*${Events::TableEvt} ......................................................*/
typedef struct {
/* protected: */
    QEvt super;

/* public: */
    uint8_t philoNum;
} TableEvt;


/* number of philosophers */
#define N_PHILO ((uint8_t)5)

/*${AOs::Philo_ctor} .......................................................*/
void Philo_ctor(void);

extern QMActive * const AO_Philo[N_PHILO];


/*${AOs::Table_ctor} .......................................................*/
void Table_ctor(void);

extern QActive * const AO_Table;


#ifdef qxk_h
    void Test1_ctor(void);
    extern QXThread * const XT_Test1;
    void Test2_ctor(void);
    extern QXThread * const XT_Test2;
#endif /* qxk_h */

#endif /* MY_SYSTEM_H_ */
