# Source Manifest / Verification Notes

Current public lineage: CYD V8 / Expanded CYD client.

Known package behaviour:
- CYD is scanner and authoritative local rule engine;
- external DB lookups are non-blocking/supplementary;
- UART 460800 8N1;
- local rules continue when DB is offline;
- busy mode uses rolling unique-device rate and 5-advertisement median RSSI for external lookup servicing;
- raw observed BLE MAC addresses may be transient in RAM/UART but are not intentionally persisted.

Known prior package SHA-256 for `CYD V8 1 of 2.zip`:
`a8c89389c28109926b94e7a9136fd440efef7b9d7eb9961e9bd84b2042aa946d`
