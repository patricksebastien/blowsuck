# Blow / Suck
Wireless breath controller

- using esp32 (node 32s)
- pressure sensor i2c (24bit) = cfsensor XGZP6897D
- wireless (via udp)
- runs on battery
- pd act as a udp server and relay to osc or midi
- web interface to configure ssid + udp server / port

# Configure 
Push BOOT button (GPIO 0 on node 32s) to configure wifi credentials and UDP server IP / port. Connect to Access point: *blowsuckAP* and open http://192.168.0.4

# Medias
![image](https://user-images.githubusercontent.com/441764/152018330-60fa6280-8540-4b97-8292-34e2692896ad.png)

![photo1643218006](https://user-images.githubusercontent.com/441764/152017938-cecceb2d-7439-45f2-8e83-aee7932defc1.jpeg)

![photo1643218013](https://user-images.githubusercontent.com/441764/152017958-75eab607-ca6a-4fcd-848e-e17fa4ee799d.jpeg)
