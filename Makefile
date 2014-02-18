C = gcc
#ARCH = -m32
ARCH = -m64
CFLAGS = -Wall -Wextra -Werror $(ARCH) -std=gnu99 -I . -I/usr/include/lua5.1  -Isrc -pg
LFLAGS = $(ARCH) -pg
#PLATFORM_LIBS = -L/usr/lib/i386-linux-gnu -L/usr/local/lib/i386-linux-gnu
PLATFORM_LIBS = -L/usr/lib/x86_64-linux-gnu -L/usr/local/lib/x86_64-linux-gnu
LIBS = -L/usr/lib -L/usr/local/lib $(PLATFORM_LIBS) -lGL -lGLU -lEGL -llua -lm -lX11 -lpthread -ldl
EXECUTABLE = vitae
include Makelist
OBJS = $(SRCS:src/%.c=bin/release/%.o)
OBJS_DBG = $(SRCS:src/%.c=bin/debug/%.o)
OBJS_PROF = $(SRCS:src/%.c=bin/profile/%.o)
include Makemoon
MOON_LUA = $(MOON_SRCS:%.moon=SpaceSim/lua/compiled/%.lua)

all : $(EXECUTABLE)_release

# pull in dependency info for *existing* .o files
#-include $(OBJS:.o=.d)
-include $(SRCS:src/%.c=bin/release/%.d)
-include $(SRCS:src/%.c=bin/debug/%.d)
-include $(SRCS:src/%.c=bin/profile/%.d)

.PHONY : clean cleandebug android cleanandroid

clean :
	@echo "--- Removing Object Files ---"
	@find bin/release -name '*.o' -exec rm -vf {} \;
	@find bin/release -name '*.d' -exec rm -vf {} \;
	@echo "--- Removing Executable ---"
	@-rm -vf $(EXECUTABLE);

cleandebug : 
	@echo "--- Removing Object Files ---"
	@find bin/debug -name '*.o' -exec rm -vf {} \;
	@find bin/debug -name '*.d' -exec rm -vf {} \;
	@echo "--- Removing Debug Executable ---"
	@-rm -vf $(EXECUTABLE)_debug;

android : $(MOON_LUA)
	@echo "--- Building Native Code for Android NDK ---"
	@ndk-build -C android NDK_DEBUG=1 APP_OPTIM=debug
	@echo "--- Compiling Android Java and packaging APK ---"
	@ant debug -f android/build.xml
	@echo "--- Installing APK to device ---"
	@android/install.sh

cleanandroid :
	@echo "--- Cleaning Android ---"
	@find /home/nick/Projects/Vitae/android/obj/local/armeabi/objs-debug -name '*.o' -exec rm {} \;
	@ant clean -f android/build.xml

android_release : 
	@echo "--- Building Native Code for Android NDK ---"
	@ndk-build -C android NDK_DEBUG=0 APP_OPTIM=release
	@echo "--- Compiling Android Java and packaging APK ---"
	@ant release -v -f android/build.xml
	@echo "--- Signing apk ---"
	@cp android/bin/vitae-release-unsigned.apk android/bin/vitae-release-unaligned.apk
	@jarsigner -verbose -sigalg MD5withRSA -digestalg SHA1 -keystore android-release-key.keystore android/bin/vitae-release-unaligned.apk vitae
	@echo "--- running zipalign ---"
	@rm android/bin/vitae-release.apk
	@zipalign -v 4 android/bin/vitae-release-unaligned.apk android/bin/vitae-release.apk
	@echo "--- Installing APK to device ---"
	@android/install_release.sh

cleanandroid_release :
	@echo "--- Cleaning Android ---"
	@find /home/nick/Projects/Vitae/android/obj/local/armeabi/objs -name '*.o' -exec rm {} \;
	@ant clean -f android/build.xml

$(EXECUTABLE)_release : $(SRCS) $(OBJS) $(MOON_LUA)
	@echo "- Linking $@"
	@$(C) $(LFLAGS) -O2 -o $(EXECUTABLE)_release $(OBJS) $(LIBS)

profile : $(EXECUTABLE)_profile

$(EXECUTABLE)_profile : $(SRCS) $(OBJS_PROF) $(MOON_LUA)
	@echo "- Linking $@"
	@$(C) -g $(LFLAGS) -O2 -o $(EXECUTABLE)_profile $(OBJS_PROF) $(LIBS)

debug : $(EXECUTABLE)_debug

$(EXECUTABLE)_debug : $(SRCS) $(OBJS_DBG) $(MOON_LUA)
	@echo "- Linking $@"
	@$(C) -g $(LFLAGS) -o $(EXECUTABLE)_debug $(OBJS_DBG) $(LIBS)

bin/debug/%.o : src/%.c
#	Calculate the directory required and create it
	@mkdir -pv `echo "$@" | sed -e 's/\/[^/]*\.o//'`
	@echo "- Compiling $@"
	@$(C) -g $(CFLAGS) -MD -D DEBUG -c -o $@ $<

bin/profile/%.o : src/%.c
#	Calculate the directory required and create it
	@mkdir -pv `echo "$@" | sed -e 's/\/[^/]*\.o//'`
	@echo "- Compiling $@"
	@$(C) $(CFLAGS) -g -O2 -MD -c -o $@ $<

bin/release/%.o : src/%.c
#	Calculate the directory required and create it
	@mkdir -pv `echo "$@" | sed -e 's/\/[^/]*\.o//'`
	@echo "- Compiling $@"
	@$(C) $(CFLAGS) -O2 -MD -c -o $@ $<

SpaceSim/lua/compiled/%.lua : SpaceSim/moon/%.moon
	@echo "compiling $< to $@"
	@(cd SpaceSim/moon && moonc -t ../../SpaceSim/lua/compiled `echo "$<" | sed -e 's/SpaceSim\/moon\///'`)

