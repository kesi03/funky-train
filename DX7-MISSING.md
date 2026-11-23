# DX7 completeness report

No `.syx` was generated. High-level voice descriptions cannot be converted into
a genuine DX7 voice dump: the 155-byte voice block needs six complete operator
records (rates, levels, keyboard scaling, detune, waveform, velocity sensitivity),
plus pitch envelope and LFO data. Fill these fields in the `.vforge` source and
set `dx7.complete = true` to enable a real SysEx encoder.

| Voice | Complete | Missing |
|-------|----------|---------|
| WRDUPBAS | no | operator envelopes, keyboard scaling, detune, OP5/OP6 |
| STRNGBAS | no | operator envelopes, levels, scaling |
| GAPBASS | no | operator envelopes, levels, scaling |
| ZAPPBASS | no | operator envelopes, levels, scaling |
| CANDYBEL | no | operator envelopes, keyboard scaling, OP6 |
| CRYSTBEL | no | operator envelopes, levels, scaling |
| FMBELLPD | no | operators, envelopes, scaling |
| MARIMBA1 | no | operators, envelopes, scaling |
| STRNGBRS | no | operators, envelopes, scaling |
| OBBRASS1 | no | operators, envelopes, scaling |
| OBBRASS2 | no | operators, envelopes, scaling |
| HITBRASS | no | operators, envelopes, scaling |
| CANDYEP | no | operators, envelopes, scaling |
| DXFUNKEP | no | operators, envelopes, scaling |
| SLOWEP | no | operators, envelopes, scaling |
| BRITEEP | no | operators, envelopes, scaling |
| FUNKCLAV | no | operator envelopes, scaling, OP5/OP6 |
| WAHCLAV | no | operators, envelopes, scaling |
| DIRTYCLV | no | operators, envelopes, scaling |
| SNAPCLAV | no | operators, envelopes, scaling |
| JUNOCOMP | no | operators, envelopes, scaling |
| SINGLELF | no | operators, envelopes, scaling |
| STYLEPOLY | no | operators, envelopes, scaling |
| SHALAPAD | no | operators, envelopes, scaling |
| CS80STR | no | operators, envelopes, scaling |
| ATLSTRNG | no | operators, envelopes, scaling |
| FUNKPAD1 | no | operators, envelopes, scaling |
| MIDNPAD | no | operators, envelopes, scaling |
| MINIFUNK | no | operator envelopes, levels, scaling, OP5/OP6 |
| TALKLEAD | no | operators, envelopes, scaling |
| ZAPPLEAD | no | operators, envelopes, scaling |
| CAMEOSTK | no | this is a layered performance patch; no single DX7 voice |
| BT ROTARY | no | drawbar organ is additive, not a DX7 voice |
| BT FUNK B3 | no | drawbar organ is additive, not a DX7 voice |
