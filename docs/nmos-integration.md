# NMOS integration

This iteration contains no MQTT transport, IS-04 registration, IS-05 connection management, IS-07 event decoding, or AMWA conformance claim.

The intended future data flow is:

```text
NMOS IS-07 Producer -> MQTT broker -> validated event decoder -> tally state -> NeoPixel renderer
```

The MQTT layer should use bounded reconnect attempts, explicit topics/sources, authenticated and encrypted transport where the deployment supports it, and must never let malformed events reach rendering code. IS-07 event payloads require structure and type validation before a source mapping may alter tally state.

IS-07 transport does not define a universal PROGRAM/PREVIEW meaning or the colors in this firmware. A future integration must document the event-source mapping for each deployment. KATE-NMOS may be used for later integration tests, but this repository contains no KATE-specific implementation.
