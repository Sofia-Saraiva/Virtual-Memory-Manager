CFLAGS = -Wall -Werror

.PHONY: clean

vm: vm.c
	@gcc $(CFLAGS) -o vm vm.c

clean:
	@rm -f correct.txt vm
	