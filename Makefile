OPTS=-K -M3
CFLAGS=-DAT386 -DVPIX -DWEITEK -DMERGE -DFASTFS -DPGREAD -DISCEXTMAP -DHBTCP -DXWIN -D_POSIX_SOURCE -DPOSIX_SUPPORT -DPOSIX_JC

all: Driver.o

#el.s: el.c
#	$(CC) $(OPTS) $(CFLAGS) -O -DINKERNEL -S -c el.c

el.o: el.c
	$(CC) $(OPTS) $(CFLAGS) -O -DINKERNEL -c el.c

Driver.o: el.o
	ld -r -x -o Driver.o el.o
#	mv el.o Driver.o

clean:
	rm -f Driver.o el.o el.s
	rm -f el.i el.txt

install:
	cp el.h /usr/include/sys/el.h
	cp el.menu /usr/admin/menu/packagemgmt/tcpipmgmt/hbtcpmgmt/
	/etc/conf/bin/idinstall -a -k el

update:
	cp el.h /usr/include/sys/el.h
	cp el.menu /usr/admin/menu/packagemgmt/tcpipmgmt/hbtcpmgmt/
	/etc/conf/bin/idinstall -u -k el

build:
	/etc/conf/bin/idbuild

dirty:
	/lib/idcpp el.c > el.i
	/lib/idcomp el.i > el.txt
