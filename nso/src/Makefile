CFLAGS:= -g
LDFLAGS:= -lpthread

test_fwding: test_fwding.o fwding.o
	gcc -o test_fwd test_fwding.o fwding.o $(CFLAGS) $(LDFLAGS)

test_arp: test_arp.o arp.o
	gcc -o test_arp test_arp.o arp.o $(CFLAGS) $(LDFLAGS)

test_wifi: test_wifi.o wifi_ops.o
	gcc -o test_wifi test_wifi.o wifi_ops.o $(CFLAGS)

test_wifi.o: packet.h nic_ops.h wifi_ops.h

wifi_ops.o: packet.h nic_ops.h wifi_ops.h

test_arp.o: arp.h

arp.o: arp.h

test_fwding.o: fwding.h nso_if.h

fwding.o: fwding.h

.PHONY: clean
clean:
	- rm -rf test_wifi *.o test_arp test_fwd
