# udp
Broadcasts Pi status (temperature etc) as simple CSV-style message to specified UDP port

Accepts 1 command-line parameter:
  - port to broadcast on
  
e.g.:
  ./pistatus 12345

To compile and link:
  cc pistatus.c -o pistatus
