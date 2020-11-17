progname=compute_shader
builddir=build
srcdir=src
libs=-Llib -Iinclude -lGL -lGLEW -lglfw

sourcefiles=${srcdir}/Camera.cpp ${srcdir}/Main.cpp

makebuilddir= if [ ! -e ${builddir} ];then mkdir ${builddir};fi

.PHONY: build debug clean

default: build

build:
	${makebuilddir}
	g++ -std=c++11 -O2 -o ${builddir}/${progname} ${sourcefiles} ${libs}
debug:
	${makebuilddir}
	g++ -g -std=c++11 -o ${builddir}/${progname}_debug ${sourcefiles} ${libs}
	gdb ${builddir}/${progname}_debug
clean:
	rm build/*
