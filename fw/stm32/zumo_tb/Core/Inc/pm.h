/*
 * ui.h
 *
 *  Created on: Jan 16, 2020
 *      Author: maik
 */

#ifndef INC_PM_H_
#define INC_PM_H_

#define UBAT_MIN (4*1150)
#define UBAT_MID (4*1200)
#define UBAT_FULL (4*1300)
#define UBAT_MAX (4*1400)
#define U_CHARGE_MIN (UBAT_MAX+3000)
#define U_CHARGE_MAX (20000)
#if defined(UBAT_MIN) && defined(UBAT_MAX)
#define UBAT_OK ( u_bat>=UBAT_MIN && u_bat<=UBAT_MAX )
#endif
#define UCHARGE_OK ( u_charge>=U_CHARGE_MIN && u_charge<=U_CHARGE_MAX )

void pm_early_init();
void pm_init();
void pm_exit();
void pm_loop();

#endif /* INC_PM_H_ */
