/*
 * File synopsis:
 * Declares PS2 private-link setup plus creation/closure of caller-owned TCP
 * descriptors for fixed product endpoints. PSTV descriptors may later transfer
 * to Transport; management descriptors remain independent higher-owner sockets.
 *
 * Context: docs/reconstruction/ISSUE7_MINIMAL_CORE.md, "PS2 system and
 * private-Ethernet platform seam"; docs/ledge/LEDGE_ARCHITECTURE_OVERLAY.md;
 * docs/ledge/LEDGE_FOREMAN_STATE.md, R31.
 */

#ifndef PSTVNC_PS2_NETWORK_H
#define PSTVNC_PS2_NETWORK_H

#define PSTVNC_PS2_LOCAL_IP "192.168.50.2"
#define PSTVNC_PS2_NETMASK "255.255.255.0"
#define PSTVNC_PS2_GATEWAY_IP "192.168.50.1"
#define PSTVNC_PS2_PSTV_SERVER_IP "192.168.50.1"
#define PSTVNC_PS2_PSTV_SERVER_PORT 5902
#define PSTVNC_PS2_MANAGEMENT_SERVER_IP "192.168.50.1"
#define PSTVNC_PS2_MANAGEMENT_SERVER_PORT 5959

int pstvnc_ps2_network_init(void);
int pstvnc_ps2_network_wait_link(void);

/* Return one caller-owned PSTV descriptor. Transport adoption follows. */
int pstvnc_ps2_network_connect_pstv(void);

/* Return one caller-owned descriptor for the fixed private management service. */
int pstvnc_ps2_network_connect_management(void);

/* Close only a descriptor still owned by the caller (never an adopted one). */
void pstvnc_ps2_network_close(int socket_fd);

#endif
