# Symbols — `tests/hardware/rfb_activity`

DIRECTORY=tests/hardware/rfb_activity
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

This checkpoint exercises only the extracted synchronized RFB activity
rendezvous on top of the qualified physical Wire and logical byte channel.

| Name | Kind | Owner | Scope | Description |
|---|---|---|---|---|
| `RFB_PAYLOAD_BYTES` | macro | synthetic RFB workload | file | Uses 328-byte logical updates to mirror the HW1-sized inbound update. |
| `PRE_BLOCKED_ROUNDS` / `POST_BLOCKED_ROUNDS` | macro family | wake workload | file | 128 blocked activity-wake cycles before and after long idle. |
| `consumer_context_t` | structure | consumer thread | file | Carries flow ownership plus test-only startup/progress/done rendezvous IDs and failure state. |
| `consumer_thread` | function | parser-side simulation | file static | Snapshots activity, exercises activity-before-wait, then repeatedly blocks/wakes/reads exact 328-byte updates. |
| `wait_activity_armed` | function | discriminator coordination | file static | Observes the activity waiter state only while holding the flow queue lock. |
| `run_blocked_round` | function | wake cycle | file static | Requires a real armed waiter before Pi is allowed to send the next RFB frame. |
| `start_consumer_thread` / `wait_consumer_dormant` | function family | EE thread lifecycle | file static | Start and prove dormancy of the real consumer EE thread. |
| `main` | function | checkpoint coordinator | entry point | Performs Q4, activity-before-wait, 128 blocked wakes, 60-second armed idle wake, 128 more wakes, final state proof, and success exit. |
