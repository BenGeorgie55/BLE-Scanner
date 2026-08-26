# Source Manifest / Verification Notes

Database coprocessor v1:
- classic ESP32 Dev Module
- maximum 4 MB design target
- no PSRAM required
- no BLE scanning
- no Wi-Fi scanning
- framed UART protocol with CRC16-CCITT
- 460800 baud, 8N1
- no persistent observed-device storage
- `generate_company_db.py` can generate `company_ids_generated.h`

Known prior generated package SHA-256:
`3650a253ff924e8a127b7457c9b4aa271432d5e1a5606066781f6e1becb05f26`
