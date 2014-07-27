pkg(libgl).

met(libgl, _) :- isfile('/usr/include/GL/gl.h'), check_dpkg('libglu1-mesa-dev').

meet(libgl, _) :- bash('sudo apt-get install libgl1-mesa-dev -y'),
					bash('sudo apt-get install libglu1-mesa-dev -y').
