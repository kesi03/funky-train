from pathlib import Path
p = Path('firmware/funky-train/main.c')
L = p.read_text(encoding='utf-8').splitlines()
del L[118:167]
p.write_text('\n'.join(L) + '\n', encoding='utf-8')
s = Path('firmware/funky-train/synth.c')
t = s.read_text(encoding='utf-8').replace('FT_SYNTH_PATCHES', 'FT_PATCH_COUNT')
s.write_text(t, encoding='utf-8')
