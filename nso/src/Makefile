CFLAGS:= -g

test_wifi: test_wifi.o wifi_ops.o
	gcc -o test_wifi test_wifi.o wifi_ops.o $(CFLAGS)

test_wifi.o: packet.h nic_ops_intf.h wifi_ops.h

wifi_ops.o: packet.h nic_ops_intf.h wifi_ops.h

.PHONY: clean
clean:
	rm -rf test_wifi *.o
