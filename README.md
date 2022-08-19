# My Reliable Protocol

***My Reliable Protocol*** is built on top of UDP and provides reliable communication with re-transmission if messages are dropped. However, it can have out-of-order delivery as well as duplicate messages.

More details regarding data structures and functions to make this protocol possible are listed [here](https://github.com/Cypre55/My-Reliable-Protocol/blob/master/Documentation.txt).

### Testing

*user1.c* acts as a server which transmits every character of a string as a message. The server will intentionally drop messages with probability *p* to test the reliabilty.  *user2.c* is the client and receives the message.  By comparing the strings, we can observe the out-of-order and duplicate messages.

### Usage

```bash
# To run user1.c
make run1

# To run user2.c
make run2
```

### Folder Structure

```bash
├── Documentation.txt
├── librsocket.a
├── Makefile
├── README.md
├── rsocket.c
├── rsocket.h
├── user1.c
└── user2.c
```

### Credits

Built by **Satwik Chappidi** and **Nikhil Tudaha**