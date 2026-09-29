# Symbols — `src/platform`

DIRECTORY=src/platform
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

This directory owns the PS2-specific system and private-Ethernet foundation
currently admitted to the clean integration line. The files were imported from
ledge and are preserved byte-for-byte at checkpoint 01 authority.

| Name | Kind | File | Owner | Scope | Description | Context |
|---|---|---|---|---|---|---|
| `SIO2MAN_irx` / `size_SIO2MAN_irx` | linker seam | ps2_system.c | IOP bootstrap | external | Embedded SIO2MAN image and byte count used during deterministic controller-service startup. | checkpoint 01 platform foundation |
| `PADMAN_irx` / `size_PADMAN_irx` | linker seam | ps2_system.c | IOP bootstrap | external | Embedded PADMAN image and byte count loaded after SIO2MAN. | checkpoint 01 platform foundation |
| `pstvnc_ps2_system_prepare_iop` | function | ps2_system.[ch] | PS2 system | public | Resets/synchronizes the IOP, initializes loading support, patches LMB, and loads controller-service modules. | deterministic startup |
| `pstvnc_ps2_system_delay_us` | function | ps2_system.[ch] | PS2 system | public | Performs one bounded EE-thread delay in microseconds. | generic platform scheduling seam |
| `pstvnc_ps2_system_exit_to_menu` | function | ps2_system.[ch] | PS2 system | public | Transfers control to OSDSYS and parks if that transfer unexpectedly returns. | checkpoint success exit |
| `DEV9_irx` / `size_DEV9_irx` | linker seam | ps2_network.c | Ethernet bootstrap | external | Embedded DEV9 module image and byte count. | private Ethernet |
| `NETMAN_irx` / `size_NETMAN_irx` | linker seam | ps2_network.c | Ethernet bootstrap | external | Embedded NETMAN module image and byte count. | private Ethernet |
| `SMAP_irx` / `size_SMAP_irx` | linker seam | ps2_network.c | Ethernet bootstrap | external | Embedded SMAP module image and byte count. | private Ethernet |
| `link_wait_alarm` | function | ps2_network.c | Ethernet link wait | file static | Wakes the startup thread after one bounded link-wait interval. | carrier acquisition |
| `link_is_up` | function | ps2_network.c | Ethernet link wait | file static | Queries NETMAN for current Ethernet link state. | carrier acquisition |
| `ps2_network_connect_server` | function | ps2_network.c | TCP descriptor creation | file static | Creates and connects one caller-owned TCP socket to a fixed endpoint. | private endpoints |
| `pstvnc_ps2_network_init` | function | ps2_network.[ch] | PS2 network | public | Loads DEV9/NETMAN/SMAP, initializes NETMAN, and configures the fixed private IPv4 link. | checkpoint 01 |
| `pstvnc_ps2_network_wait_link` | function | ps2_network.[ch] | PS2 network | public | Waits for Ethernet carrier using bounded alarm-driven sleeps. | checkpoint 01 |
| `pstvnc_ps2_network_connect_pstv` | function | ps2_network.[ch] | PS2 network | public | Opens a caller-owned TCP descriptor to the Pi PSTV endpoint on port 5902. | future Wire rung |
| `pstvnc_ps2_network_connect_management` | function | ps2_network.[ch] | PS2 network | public | Opens a caller-owned TCP descriptor to the Pi management/test endpoint on port 5959. | checkpoint 01 |
| `pstvnc_ps2_network_close` | function | ps2_network.[ch] | PS2 network | public | Closes a valid descriptor that is still caller-owned. | descriptor ownership |
Public endpoint macros in `ps2_network.h`:
- `PSTVNC_PS2_LOCAL_IP` = `192.168.50.2`
- `PSTVNC_PS2_NETMASK` = `255.255.255.0`
- `PSTVNC_PS2_GATEWAY_IP` = `192.168.50.1`
- `PSTVNC_PS2_PSTV_SERVER_IP` / `PORT` = `192.168.50.1:5902`
- `PSTVNC_PS2_MANAGEMENT_SERVER_IP` / `PORT` = `192.168.50.1:5959`

Include guards are ordinary header mechanics and are not expanded into separate
table rows.
