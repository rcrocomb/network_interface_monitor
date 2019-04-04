<pre>
$ ./main
main: Signal handler installed.
network_stats: Building stat to filename map
network_stats: Got interface stats path as '/sys/class/net/eth0/statistics/'
main: Setting Rx stats to update
network_stats: For 'RX_BYTES': opening stats file @ '/sys/class/net/eth0/statistics/rx_bytes'
network_stats: For 'RX_BYTES': got file descriptor as 3
network_stats: For 'RX_PACKETS': opening stats file @ '/sys/class/net/eth0/statistics/rx_packets'
network_stats: For 'RX_PACKETS': got file descriptor as 4
main: Setting Tx stats to update
network_stats: For TX_BYTES: opening stats file @ '/sys/class/net/eth0/statistics/tx_bytes'
network_stats: For 'TX_BYTES': got file descriptor as 5
network_stats: For TX_PACKETS: opening stats file @ '/sys/class/net/eth0/statistics/tx_packets'
network_stats: For 'TX_PACKETS': got file descriptor as 6
main: 1554359699 : Rx bytes: 315490928269 -> 315490945259 : 16990
main: 1554359699 : Tx bytes: 11930829096 -> 11930829866 : 770
main: 1554359699 : Rx packets: 250892712 -> 250892726 : 14
main: 1554359699 : Tx packets: 91998198 -> 91998209 : 11
main: 1554359700 : Rx bytes: 315490945259 -> 315490961564 : 16305
main: 1554359700 : Tx bytes: 11930829866 -> 11930830356 : 490
main: 1554359700 : Rx packets: 250892726 -> 250892738 : 12
main: 1554359700 : Tx packets: 91998209 -> 91998216 : 7
main: 1554359701 : Rx bytes: 315490961564 -> 315490979468 : 17904
main: 1554359701 : Tx bytes: 11930830356 -> 11930831960 : 1604
main: 1554359701 : Rx packets: 250892738 -> 250892757 : 19
main: 1554359701 : Tx packets: 91998216 -> 91998232 : 16
main: 1554359702 : Rx bytes: 315490979468 -> 315490997442 : 17974
main: 1554359702 : Tx bytes: 11930831960 -> 11930832800 : 840
main: 1554359702 : Rx packets: 250892757 -> 250892771 : 14
main: 1554359702 : Tx packets: 91998232 -> 91998244 : 12
main: 1554359703 : Rx bytes: 315490997442 -> 315491013598 : 16156
main: 1554359703 : Tx bytes: 11930832800 -> 11930833570 : 770
main: 1554359703 : Rx packets: 250892771 -> 250892785 : 14
main: 1554359703 : Tx packets: 91998244 -> 91998255 : 11
main: 1554359704 : Rx bytes: 315491013598 -> 315491030738 : 17140
main: 1554359704 : Tx bytes: 11930833570 -> 11930834130 : 560
main: 1554359704 : Rx packets: 250892785 -> 250892797 : 12
main: 1554359704 : Tx packets: 91998255 -> 91998263 : 8
^CCaught signal 2: SIGINT -- no error detected
SIGINT received: synchronous shutdown.
main: 1554359704 : Rx bytes: 315491030738 -> 315491046138 : 15400
main: 1554359704 : Tx bytes: 11930834130 -> 11930834690 : 560
main: 1554359704 : Rx packets: 250892797 -> 250892808 : 11
main: 1554359704 : Tx packets: 91998263 -> 91998271 : 8
network_stats: Shutting down
</pre>
