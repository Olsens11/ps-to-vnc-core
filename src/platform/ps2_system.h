/*
 * File synopsis:
 * Declares the PS2 system-lifecycle seam that establishes the deterministic
 * IOP/controller-service foundation, owns bounded application-thread delays,
 * and owns final OSDSYS convergence.
 *
 * Context: docs/reconstruction/ISSUE7_MINIMAL_CORE.md, "PS2 system and
 * private-Ethernet platform seam"; docs/CLEAN_ARCHITECTURE.md, "PS2 platform
 * mechanisms".
 */

#ifndef PSTVNC_PS2_SYSTEM_H
#define PSTVNC_PS2_SYSTEM_H

/*
 * Establish the ordinary PS2SDK IOP state plus the embedded SIO2MAN/PADMAN
 * controller services required before feature initialization. Returns 0 on
 * success and -1 on failure.
 */
int pstvnc_ps2_system_prepare_iop(void);

/*
 * Yield the application/main thread for a bounded number of microseconds.
 *
 * Scheduling mechanics remain a platform responsibility; application code uses
 * this seam to avoid busy-spinning while responsive nonblocking RFB receive
 * service reports an ordinary idle condition.
 */
int pstvnc_ps2_system_delay_us(
    unsigned int microseconds);

/*
 * Converge to the ordinary PS2 system menu. If LoadExecPS2 unexpectedly
 * returns, park the current thread rather than continuing in unknown state.
 */
void pstvnc_ps2_system_exit_to_menu(void);

#endif
