# How to build project
In project directory run this commands
```
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
# Usage

## Create service
Now we generate server-service and client-binary file </br>
To start server service use command from build directory:
```
.\server\Release\server.exe install
``` 
Now we can check that service is available using command in VS promt:
```
sc queryex state=all type=service | findstr ServerSvc
```
To start this service just use:
```
net start ServerSvc
```
Where ServerSvc -- name of created service</br>
## Connection to server
If you want to try client connection use :
```
.\client\Release\client.exe <ip_address>
```
Where ip_address -- ip address of server (e.g. on local machine this address would be 127.0.0.1) </br>
## Delete service
To stop this service use
```
net stop ServerSvc
```
And after that you can delete this service:
```
.\server\Release\server.exe delete
```