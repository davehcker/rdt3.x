This program builds a reliable data transfer protocol (rdt) over UDP (which is not guaranteed to be reliable at all).

A simple rdt would be to send a packet, wait for an ACK from receiver. But because the transport layer is not reliable, we can miss the ACK packets. To deal with this issue, we put a timer in place every time we send a packet. If the timer expires, (and we don't receive any ACK), then we resend the data. This is a complete rdt.

Despite being complete, our rdt is extremely slow and is a send and wait protocol. We can optimize it by sending multiple packets at once, and still ensuring that all the packets get to the receiver. We implement this using a control window.

A control window maintains a fixed window (cwnd, in terms of number of unacked packets). We send cwnd number of packets and maintain a buffer for the unacked packets. As we get an acknowledgement for the packets, we clear the buffer, load new packets, and send them to the receiver; essentially sliding the windows within which we send the data.