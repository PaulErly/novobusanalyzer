# CAN DBC smoke sample

Use `dbc_smoke.dbc` for a quick x64 CAN bridge smoke test.

Expected coverage:
- standard CAN message
- extended CAN message
- 8-byte DLC
- little-endian signal
- big-endian signal
- unsigned signal
- signed signal
- scale / offset
- unit
- value table

Manual smoke checklist:
1. Launch `build_busmaster_clean\Sources\BUSMASTER\Application\Debug\NovoBusAnalyzer.exe`.
2. Open `CAN -> Database -> Associate`.
3. Select `Tests\Samples\CAN\dbc_smoke.dbc`.
4. Confirm the trace log shows the x64 CAN bridge path and a successful DBF conversion.
5. Open the CAN transmit window.
6. Confirm the imported message names and signal names appear.
7. Edit a signal in `Signal Details` and confirm it no longer asserts.
8. Save a configuration, close BUSMASTER, reopen the saved config, and confirm the associated DBC is restored.

Runtime log:
- `%TEMP%\NovoBusAnalyzer_dbc_import.log`

