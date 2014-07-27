pkg(egl).

met(egl, _) :- isfile('/usr/include/EGL/egl.h').

meet(egl, _) :- bash('sudo apt-get install libegl1-mesa-dev -y').
