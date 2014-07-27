pkg(luarocks).

met(luarocks, _) :- which('luarocks').

meet(luarocks, _) :- bash('sudo apt-get install luarocks -y').
