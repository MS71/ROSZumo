/*
 * ui.h
 *
 *  Created on: Jan 16, 2020
 *      Author: maik
 */

#ifndef INC_PM_H_
#define INC_PM_H_

#define UBAT_MIN_CHARGE (4*400)
#define UBAT_MIN (4*1150)
#define UBAT_MIN_ON (4*1000)
#define UBAT_MID (4*1200)
#define UBAT_CHARGE_A (4*1250)
#define UBAT_FULL (4*1300)
#define UBAT_MAX (4*1375)
#define UBAT_CHARGE_MAX (4*1380)
//#define UBAT_MAX (4*1500)
#define U_CHARGE_MIN (UBAT_MID)
#define U_CHARGE_MAX (8000)
#define U_CHARGE_MAX_DIFF (1700)
#if defined(UBAT_MIN) && defined(UBAT_MAX)
#define UBAT_OK ( u_bat>=UBAT_MIN && u_bat<=UBAT_MAX )
//#define UBAT_CHARGE_OK ( u_bat>=UBAT_MIN_CHARGE && u_bat<=UBAT_MAX )
//#define UBAT_SIMPLE_CHARGE_OK ( u_bat<UBAT_SIMPLE_CHARGE )
#endif
#define UCHARGE_OK ( u_charge>=U_CHARGE_MIN && u_charge<=U_CHARGE_MAX )

#define PWRCNTDWN_START	  2
#define PWR_TMPON_TIME	  2

#define CHARGE_ON_PRERIOD 30
#define CHARGE_ON_TIME     3

void pm_early_init();
void pm_init();
void pm_exit();
void pm_loop();

#endif /* INC_PM_H_ */
