# HTTP Censorship Proxy

Author: Viet An Truong

Email me at:
<a href="mailto:vietan124@gmail.com">email</a>

To compile, type:
```
$ g++ cenProxyC.cpp -o proxy
```

To run transparent proxy (this will not block any content), type:
```
$ ./proxy
```

To run proxy with blocked keywords, type:
```
$ ./proxy <keywords>
```

For example:
```
$ ./proxy spongebob
```
or
```
$ ./proxy spongebob curling
```

Configure a browser to connect to proxy by using local host IP address and port 12401