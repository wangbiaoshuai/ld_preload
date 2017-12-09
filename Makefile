libunlink.so: unlink.c
	gcc unlink.c -shared -fPIC -Wl,-soname,libunlink.so.1 \
		-o libunlink.so.1.0.0 -ldl -D_GNU_SOURCE
clean:
	rm -rf libunlink.so* file_protected cems_proc a.out

package:
	@mkdir -p ./file_protected
	cp ./install.sh ./file_protected
	cp ./uninstall.sh ./file_protected
	cp ./ld.so.preload ./file_protected
	cp ./libunlink.so.1.0.0 ./file_protected

test:
	gcc -o cems_proc test.c
	gcc test.c
