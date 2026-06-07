# CAN DBC smoke sample

Use these files for quick x64 CAN bridge smoke tests:

- `dbc_smoke.dbc`
- `dbc_smoke_a.dbc`
- `dbc_smoke_b.dbc`

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
3. Select `Tests\Samples\CAN\dbc_smoke_a.dbc` or `dbc_smoke_b.dbc`.
4. Confirm the trace log shows the x64 CAN bridge path and a successful DBF conversion.
5. Open the CAN transmit window.
6. Confirm the imported message names and signal names appear.
7. Repeat `CAN -> Database -> Associate` for the second sample and confirm both message sets are available together.
8. Edit a signal in `Signal Details` and confirm it no longer asserts.
9. Save a configuration, close BUSMASTER, reopen the saved config, and confirm all associated DBCs are restored.

Runtime log:
- `%TEMP%\NovoBusAnalyzer_dbc_import.log`
