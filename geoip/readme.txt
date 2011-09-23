This folder contains useful parts of MaxMind's GeoIP C API <http://www.maxmind.com/app/c>.

Header includes have been slightly changed.
The inet_pton implementation for Windows has been rewritten to use WSAStringToAddressA (the
getaddrinfo variant in the original file was causing crashes).
