To be clear - this is a proof of concept project from 2013. It is not maintained and has not been tested in a very long time.

-------------------------

WinTap is a packet sniffer and soft-tap developed to mirror packets flowing through an Ethernet interface. It is purely based on the NDIS which allows us to sniff packets and rewrite them to a second interface acting as a soft-tap. WinTap consists of two components,

    1. NDIS 6.0 based protocol driver.
    2. User mode soft-tap.

These two act in tandem to create a soft-tap, where the protocol driver sniffs the traffic and delivers to the user mode application. User mode application does the redirection logic and returns the packets to be rewritten to second interface.

Please read the description.html for more details.
