% Moonscript Marelle
pkg(moonscript).

met(moonscript, _) :- which('moon').

meet(moonscript, _) :- bash('sudo luarocks install moonscript').

depends(moonscript, _, [luarocks]).
