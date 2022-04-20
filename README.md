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

# DIY

Sensor: https://www.cfsensor.com/static/upload/file/20220412/XGZP6897D%20Pressure%20Sensor%20Module%20V2.4.pdf

![connect](https://user-images.githubusercontent.com/441764/164128297-fd4ba289-7b09-4e65-8114-cd06ab20dc8d.png)

_Watch for the notch on pin1, ultra-important to detect this small **notch on pin1** to know where to connect the 4 wires (pin2 3.3, pin8 GND, pin7 SCL, pin6 SDA)_

3.3V I2C (esp32 = pin 22 SCL, 21 SDA):

![esp32](https://user-images.githubusercontent.com/441764/164128492-48395a59-3325-4550-ae02-de5d36a436ce.png)
