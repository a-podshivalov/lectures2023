== Пример построения сети 6LoWPAN поверх BLE ==

Используйте несколько nRF52-DK с прошивкой `examples-miem/6loble`. Здесь все действия выполняются вручную, для автоматизированного построения топологии сети можно использовать модуль `nimble_autoconn`, входящий в состав ОС Riot.

Для установления соединения между устройствами на "ведомом" устройств выполняем команду ble adv, указывая желаемое название устройства:
```
> ble adv bleRIOT
success: advertising this node as 'bleRIOT'
```
Запустив "объявления", на "ведущем" устройстве выполняем ble scan, видим список устройств с названием bleRIOT:
```
> ble scan
scanning (for 500ms) ...
done
[ 0] C0:D7:7F:91:C8:B2 (public)  [IND] "undefined", adv_msg_cnt: 2, adv_int: 393us, last_rssi: -87
[ 1] ED:11:AA:3C:3C:24 (random)  [IND] "bleRIOT", adv_msg_cnt: 6, adv_int: 52767us, last_rssi: -35
[ 2] E9:C9:CE:F7:68:DE (random)  [IND] "bleRIOT", adv_msg_cnt: 6, adv_int: 52380us, last_rssi: -33
```
На ведущем устройстве выполняем ble connect, указывая номер ведомого в этом списке:
```
> ble connect 1
initiated connection procedure with ED:11:AA:3C:3C:24
> event: handle 0 -> CONNECTED as MASTER (ED:11:AA:3C:3C:24)
ble connect 2
initiated connection procedure with E9:C9:CE:F7:68:DE
> event: handle 1 -> CONNECTED as MASTER (E9:C9:CE:F7:68:DE)
```
В этот же момент оконечные устройства сообщат об успешном соединении:
```
event: handle 0 -> CONNECTED as SLAVE (C1:E2:B9:42:08:D3)
```
После успешного соединения работают link-local адреса, и можно, например, подсмотрев командой ifconfig эти адреса у оконечных устройств, выполнить ping - например, на ведомом устройстве ifconfig выведет что-то в таком духе:
```
> ifconfig
Iface  8  HWaddr: E9:C9:CE:F7:68:DE
          L2-PDU:1280  MTU:1280  HL:64  RTR
          6LO  IPHC
          Source address length: 6
          Link type: wireless
          inet6 addr: fe80::e9c9:ceff:fef7:68de  scope: link  VAL
          inet6 group: ff02::2
          inet6 group: ff02::1
          inet6 group: ff02::1:fff7:68de

          Statistics for Layer 2
            RX packets 0  bytes 0
            TX packets 0 (Multicast: 0)  bytes 28000
            TX succeeded 0 errors 0
          Statistics for IPv6
            RX packets 2  bytes 112
            TX packets 8 (Multicast: 8)  bytes 448
            TX succeeded 8 errors 0
```
После этого выполняем команду ping на ведущем устройстве, указывая этот link-local адрес:
```
> ping fe80::e9c9:ceff:fef7:68de
12 bytes from fe80::e9c9:ceff:fef7:68de%8: icmp_seq=0 ttl=64 time=58.957 ms
12 bytes from fe80::e9c9:ceff:fef7:68de%8: icmp_seq=1 ttl=64 time=54.265 ms
12 bytes from fe80::e9c9:ceff:fef7:68de%8: icmp_seq=2 ttl=64 time=99.186 ms

--- fe80::e9c9:ceff:fef7:68de PING statistics ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max = 54.265/70.802/99.186 ms
```
Можно сделать и наоборот - пропинговать ведущее устройство с ведомого, а вот маршрутизация пока не работает, и если выполнить ping на _другом_ узле сети - то результат будет очевидно отрицательным:
```
> ping fe80::e9c9:ceff:fef7:68de

--- fe80::e9c9:ceff:fef7:68de PING statistics ---
3 packets transmitted, 0 packets received, 100% packet loss
```
Для полноценной маршрутизации нам надо запустить на всех устройствах протокол маршрутизации RPL. Для этого сначала выполняем везде команду rpl init. Аргументом у этой команды служит номер интерфейса, который можно определить с помощью ifconfig:
```
> ifconfig
Iface  8  HWaddr: ED:11:AA:3C:3C:24
          L2-PDU:1280  MTU:1280  HL:64  RTR
          6LO  IPHC
          Source address length: 6
          Link type: wireless
          inet6 addr: fe80::ed11:aaff:fe3c:3c24  scope: link  VAL
          inet6 group: ff02::2
          inet6 group: ff02::1
          inet6 group: ff02::1:ff3c:3c24
          inet6 group: ff02::1a

          Statistics for Layer 2
            RX packets 0  bytes 0
            TX packets 0 (Multicast: 0)  bytes 28154
            TX succeeded 0 errors 0
          Statistics for IPv6
            RX packets 10  bytes 554
            TX packets 19 (Multicast: 16)  bytes 1046
            TX succeeded 19 errors 0

> rpl init 8
successfully initialized RPL on interface 8
```

Далее на корневом устройстве добавляем IP-адрес маршрутизатора - например, такой:
```
> ifconfig 8 add 2001:db8::1
success: added 2001:db8::1/64 to interface 8
```
И затем - инициализируем на нем же "корень" DODAG (графа маршрутизации):
```
> rpl root 1 2001:db8::1
successfully added a new RPL DODAG
```
Очень быстро (буквально за секунду) граф маршрутизации построится, а у остальных устройств появится "глобальный" адрес с тем же префиксом, что у "корня":
```
> ifconfig
Iface  8  HWaddr: ED:11:AA:3C:3C:24
          L2-PDU:1280  MTU:1280  HL:64  RTR
          6LO  IPHC
          Source address length: 6
          Link type: wireless
          inet6 addr: fe80::ed11:aaff:fe3c:3c24  scope: link  VAL
          inet6 addr: 2001:db8::ed11:aaff:fe3c:3c24  scope: global  VAL
          inet6 group: ff02::2
          inet6 group: ff02::1
          inet6 group: ff02::1:ff3c:3c24
          inet6 group: ff02::1a

          Statistics for Layer 2
            RX packets 0  bytes 0
            TX packets 0 (Multicast: 0)  bytes 29011
            TX succeeded 0 errors 0
          Statistics for IPv6
            RX packets 24  bytes 1742
            TX packets 36 (Multicast: 32)  bytes 2516
            TX succeeded 36 errors 0
```
Можно проверить функционирование маршрутизации с помощью ping:
```
> ping 2001:db8::ed11:aaff:fe3c:3c24
12 bytes from 2001:db8::ed11:aaff:fe3c:3c24: icmp_seq=0 ttl=63 time=120.712 ms
12 bytes from 2001:db8::ed11:aaff:fe3c:3c24: icmp_seq=1 ttl=63 time=109.664 ms
12 bytes from 2001:db8::ed11:aaff:fe3c:3c24: icmp_seq=2 ttl=63 time=148.078 ms

--- 2001:db8::ed11:aaff:fe3c:3c24 PING statistics ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max = 109.664/126.151/148.078 ms
```
Обратите внимание, что тут у нас уже два "хопа", с соответствующим увеличением round-trip time.

На функционирование RPL и построенный с его помощью граф можно посмотреть соответствующей командой, например, на одном из оконечных устройств:
```
> rpl
instance table: [X]
parent table:   [X]     [ ]     [ ]

instance [1 | Iface: 8 | mop: 2 | ocp: 0 | mhri: 256 | mri 0]
        dodag [2001:db8::1 | R: 512 | OP: Router | PIO: on | TR(I=[8,20], k=10, c=1)]
                parent [addr: fe80::c1e2:b9ff:fe42:8d3 | rank: 256]
```
В примере есть еще и команда udp, с помощью которой можно запустить на одном из устройств (корневом) сервер, печатающий дампы принятых пакетов, а на оконечных устройствах - эти пакеты передавать, например так:

(на корневом устройстве)
```
> udp server start 1234
Success: started UDP server on port 1234
```
(на оконечном устройстве - эта команда отправит 100 пакетов со словом hello с интервалом 1000 мс)
```
> udp send 2001:db8::1 1234 hello 100 1000
Success: sent 5 byte(s) to [2001:db8::1]:1234
Success: sent 5 byte(s) to [2001:db8::1]:1234
Success: sent 5 byte(s) to [2001:db8::1]:1234
Success: sent 5 byte(s) to [2001:db8::1]:1234
```
Сервер будет выводить в консоль содержимое принятых пакетов:
```
PKTDUMP: data received:
~~ SNIP  0 - size:   5 byte, type: NETTYPE_UNDEF (0)
00000000  68  65  6C  6C  6F
~~ SNIP  1 - size:   8 byte, type: NETTYPE_UDP (4)
   src-port:  1234  dst-port:  1234
   length: 13  cksum: 0x8478
~~ SNIP  2 - size:  40 byte, type: NETTYPE_IPV6 (2)
traffic class: 0x00 (ECN: 0x0, DSCP: 0x00)
flow label: 0x00000
length: 13  next header: 17  hop limit: 63
source address: 2001:db8::ed11:aaff:fe3c:3c24
destination address: 2001:db8::1
~~ SNIP  3 - size:  20 byte, type: NETTYPE_NETIF (-1)
if_pid: 8  rssi: -32768  lqi: 0
flags: 0x0
src_l2addr: E9:C9:CE:F7:68:DE
dst_l2addr: C1:E2:B9:42:08:D3
~~ PKT    -  4 snips, total size:  73 byte
PKTDUMP: data received:
~~ SNIP  0 - size:   5 byte, type: NETTYPE_UNDEF (0)
00000000  68  65  6C  6C  6F
~~ SNIP  1 - size:   8 byte, type: NETTYPE_UDP (4)
   src-port:  1234  dst-port:  1234
   length: 13  cksum: 0x364b
~~ SNIP  2 - size:  40 byte, type: NETTYPE_IPV6 (2)
traffic class: 0x00 (ECN: 0x0, DSCP: 0x00)
flow label: 0x00000
length: 13  next header: 17  hop limit: 64
source address: 2001:db8::e9c9:ceff:fef7:68de
destination address: 2001:db8::1
~~ SNIP  3 - size:  20 byte, type: NETTYPE_NETIF (-1)
if_pid: 8  rssi: -32768  lqi: 0
flags: 0x0
src_l2addr: E9:C9:CE:F7:68:DE
dst_l2addr: C1:E2:B9:42:08:D3
~~ PKT    -  4 snips, total size:  73 byte
PKTDUMP: data received:
~~ SNIP  0 - size:   5 byte, type: NETTYPE_UNDEF (0)
00000000  68  65  6C  6C  6F
~~ SNIP  1 - size:   8 byte, type: NETTYPE_UDP (4)
   src-port:  1234  dst-port:  1234
   length: 13  cksum: 0x8478
~~ SNIP  2 - size:  40 byte, type: NETTYPE_IPV6 (2)
traffic class: 0x00 (ECN: 0x0, DSCP: 0x00)
flow label: 0x00000
length: 13  next header: 17  hop limit: 63
source address: 2001:db8::ed11:aaff:fe3c:3c24
destination address: 2001:db8::1
~~ SNIP  3 - size:  20 byte, type: NETTYPE_NETIF (-1)
if_pid: 8  rssi: -32768  lqi: 0
flags: 0x0
src_l2addr: E9:C9:CE:F7:68:DE
dst_l2addr: C1:E2:B9:42:08:D3
~~ PKT    -  4 snips, total size:  73 byte
```