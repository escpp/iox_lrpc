NPROC=$(shell nproc)
PREFIX:=$(shell pwd)/build/dist
PREFIX:=

x86:
	bash tools/x86/build.sh ${PREFIX}
	cd build && make -j${NPROC} 
mp135:
	bash tools/mp135/build.sh ${PREFIX}
	cd build && make -j${NPROC}
install:
	cd build && make install
	$(shell [ -d build/dist ] && tree build/dist -h)
clean:
	rm build -rfv
