# Symbols — `tests/hardware/rfb_channel`

DIRECTORY=tests/hardware/rfb_channel
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

This checkpoint exercises only the imported logical RFB byte channel on top of
the qualified physical Wire foundation.

| Name | Kind | Owner | Scope | Description |
|---|---|---|---|---|
| `QUEUE_CAPACITY` | macro | logical queue fixture | file | Uses a 1024-byte ring to force wraparound under synthetic traffic. |
| `FRAME_CAPACITY` | macro | physical receive buffer | file | Accepts any valid Wire payload up to 8192 bytes. |
| `expected_byte` / `validate_bytes` | function family | synthetic byte stream | file static | Independently define and verify the opaque logical RFB stream. |
| `wait_until_readable` / `receive_frame` | function family | physical readiness | file static | Reuse the qualified select-based Wire receive path. |
| `receive_rfb_data` | function | producer path | file static | Receives one channel-1 DATA frame and commits it into the real RFB channel. |
| `read_available_and_validate` | function | consumer path | file static | Exercises partial consumption and validates byte order across wrap. |
| `read_exact_and_validate` | function | consumer path | file static | Exercises exact consumption and validates the logical stream. |
| `run_queue_sequence` | function | queue semantics | file static | Proves wraparound, zero-length commit, exact-read atomicity, activity generation, and residual discard. |
| `run_idle_wake` | function | post-idle logical receive | file static | Holds 60 s idle, commits new RFB DATA, verifies generation/read, and reports empty polls. |
| `main` | function | checkpoint coordinator | entry point | Initializes Q4/queue, runs pre-idle semantics plus post-idle commit/read, then exits on success. |
