/*
 * File synopsis:
 * Owns deterministic IOP bootstrap, base controller-service module loading,
 * bounded application-thread delay mechanics, and the final OSDSYS system-menu
 * exit path.
 *
 * Context: docs/reconstruction/ISSUE7_MINIMAL_CORE.md, "PS2 system and
 * private-Ethernet platform seam"; docs/CLEAN_ARCHITECTURE.md, "PS2 platform
 * mechanisms".
 */

#include <delaythread.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <kernel.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>

#include <limits.h>

#include "ps2_system.h"

extern unsigned char SIO2MAN_irx[];
extern unsigned int size_SIO2MAN_irx;
extern unsigned char PADMAN_irx[];
extern unsigned int size_PADMAN_irx;

int pstvnc_ps2_system_prepare_iop(void)
{
    /*
     * Resetting the IOP discards whatever module/RPC state the launching
     * environment left behind. Re-establishing that foundation makes startup
     * deterministic before PS-to-VNC loads its own network modules.
     */
    sceSifInitRpc(0);

    while (!SifIopReset("", 0)) {
        /* The qualified startup waits until the reset request is accepted. */
    }

    while (!SifIopSync()) {
        /* The IOP must be synchronized before loading product modules. */
    }

    sceSifInitRpc(0);

    if (SifLoadFileInit() < 0)
        return -1;

    if (SifInitIopHeap() < 0)
        return -1;

    if (sbv_patch_enable_lmb() < 0)
        return -1;

    /*
     * Controller services are part of the deterministic IOP foundation rather
     * than pad interpretation. SIO2MAN owns the serial-controller transport
     * used by PADMAN, so its load order must precede PADMAN.
     *
     * Both images come from the pinned PS2SDK build instead of inheriting ROM
     * or launcher module state. The pad owner can therefore use ordinary
     * libpad against one known controller-service pair.
     */
    if (SifExecModuleBuffer(
            SIO2MAN_irx,
            size_SIO2MAN_irx,
            0,
            NULL,
            NULL) < 0)
        return -1;

    if (SifExecModuleBuffer(
            PADMAN_irx,
            size_PADMAN_irx,
            0,
            NULL,
            NULL) < 0)
        return -1;

    return 0;
}

int pstvnc_ps2_system_delay_us(unsigned int microseconds)
{
    if (microseconds > (unsigned int)INT_MAX)
        return -1;

    if (microseconds == 0)
        return 0;

    return
        DelayThread((int)microseconds) < 0
        ? -1
        : 0;
}

void pstvnc_ps2_system_exit_to_menu(void)
{
    LoadExecPS2("rom0:OSDSYS", 0, NULL);

    /*
     * Returning from OSDSYS launch is not a valid continuation path. Park the
     * thread instead of running on in partially shut-down product state.
     */
    SleepThread();
}
