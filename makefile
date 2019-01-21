
jdk = ./jdk

javac = $(jdk)/bin/javac
jmod  = $(jdk)/bin/jmod
jlink = $(jdk)/bin/jlink

gcc = gcc
strip = strip

build = ./build
bin = $(build)/bin
conf = $(build)/conf
classes = $(build)/classes
jmods = $(build)/jmods
dist = $(build)/dist


all: $(dist)

demo:
	mkdir -p bin lib
	ln -s ../build/dist/bin/exe bin/cmd
	bin/cmd


$(dist): $(jmods)/exe.jmod
	$(jlink) --module-path $(jmods) --add-modules exe.base --output $@

$(jmods)/exe.jmod: $(classes)/module-info.class $(bin)/exe
	$(jmod) create --main-class exe.Main --class-path $(classes) --cmds $(bin) $@

$(classes)/module-info.class: src/exe/module-info.java $(classes)/exe/Main.class
	$(javac) -d $(classes) $<

$(classes)/exe/Main.class: src/exe/Main.java
	$(javac) -d $(classes) -sourcepath src $<

$(bin)/exe: src/main.c
	$(gcc) -Wall -Os -Ijdk/include -Ijdk/include/linux -ldl -o $@ $<
	$(strip) -s $@


clean:
	rm -fr $(dist)
	find $(build) -type f -exec rm {} ';'

$(build):
	mkdir -pv $@/{bin,classes,jmods}

