CFLAGS:= -g
LDFLAGS:= -lpthread

test: test_nbr test_fwding test_arp test_wifi test_nso

test_nso: test_nso.o nso.o wifi_ops.o fwding.o neighbor.o arp.o nso_aaa.o nso_tsd.o

test_nbr: test_nbr.o neighbor.o wifi_ops.o
	gcc -o test_nbr test_nbr.o neighbor.o wifi_ops.o $(CFLAGS) $(LDFLAGS)

test_fwding: test_fwding.o fwding.o wifi_ops.o
	gcc -o test_fwd test_fwding.o fwding.o wifi_ops.o $(CFLAGS) $(LDFLAGS)

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

test_nbr.o: neighbor.h nso_if.h

neighbor.o: neighbor.h

.PHONY: clean
clean:
	- rm -rf test_wifi *.o test_arp test_fwd test_nbr
