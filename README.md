## TTGO T-Display NetMon

A network monitor that pings google dns every half second to check internet connection.

+ orange while building connection to router
  + may intermediate black because reboots while not able to connect
+ red if ping fails
  + screen stays
+ green when ping succeeds
  + dimming screen to pitch black after 5 seconds 

Shows time of last event in case it got time from internet timeserver.

Toggle by 
* button1 to history (circle buffered)

or 
* by button2 to the current state.


## Ideas (no priority sort order)
* show RSSI in case of event
* more fancy interface
* invoke an URL in case of event to trigger external devices (maybe take a photo of a street in case of interferences from vehicles)
* directly take a photo (device with display an cam exists from TTGO)
* replace loops and wait for by event driven design for WiFi and Ping
* use tasks
* in conjunction use sleep mode and its events to consume less power
* sounding alarm
