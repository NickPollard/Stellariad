% Download SDK
pkg(androiddl).
met(androiddl, _) :- isfile('~/Download/android-sdk_r23.0.2-linux.tgz').
meet(androiddl, _) :- bash('mkdir -p ~/Download && wget http://dl.google.com/android/android-sdk_r23.0.2-linux.tgz && mv android-sdk_r23.0.2-linux.tgz ~/Download/').

pkg(androidunpack).

% Unpack and install SDK
met(androidunpack, _) :- isfile('~/android/android-sdk-linux/platform-tools/adb').
meet(androidunpack, _) :- bash('mkdir -p ~/android && tar xzvf ~/Download/android-sdk_r23.0.2-linux.tgz -C ~/android && ~/android/android-sdk-linux/tools/android update sdk --no-ui && sudo apt-get install libc6-i386 lib32stdc++6 lib32gcc1 lib32ncurses5 lib32z1').
depends(androidunpack, _, [androiddl]).

% Download NDK
pkg(android_ndk_dl).
met(android_ndk_dl, _) :- isfile('~/Download/android-ndk32-r10-linux-x86_64.tar.bz2').
meet(android_ndk_dl, _) :- bash('mkdir -p ~/Download && wget http://dl.google.com/android/ndk/android-ndk32-r10-linux-x86_64.tar.bz2 && mv android-ndk32-r10-linux-x86_64.tar.bz2 ~/Download/').

% Unpack and install SDK
pkg(android_ndk_unpack).
met(android_ndk_unpack, _) :- isfile('~/android/android-ndk-r10/ndk-build').
meet(android_ndk_unpack, _) :- bash('mkdir -p ~/android && tar xjvf ~/Download/android-ndk32-r10-linux-x86_64.tar.bz2 -C ~/android').
depends(android_ndk_unpack, _, [android_ndk_dl]).

%Ant
pkg(ant).
met(ant, _) :- which('ant').
meet(ant, _) :- bash('sudo apt-get install ant -y').

pkg(android).
met(android, _) :- met(android_ndk_unpack, _), met(androidunpack, _), met(ant, _).
depends(android, _, [android_ndk_unpack, androidunpack, ant]).
