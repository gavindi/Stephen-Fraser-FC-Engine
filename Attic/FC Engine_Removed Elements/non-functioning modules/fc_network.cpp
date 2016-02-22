#include <winsock2.h>
#include "..\fc_source\fc_h.h"

#define maxpacketsize 512
//extern console *con;

// ********************************************************************************
// ***								Common Code									***
// ********************************************************************************
WSADATA wsaData;
addresslist localaddrbuf;
bool serverok = false;
bool clientok = false;
bool netinitialized = false;

SOCKET testclientsocket,testserversocket;
sockaddr_in testlocal,testfrom;
	
char *getneterror(dword errorcode)
{	errorcode&=0x7fffffff;
	switch(errorcode)
	{	case 0: return "No Error";
		case net_tcpipna-net_fatal			: return "TCP/IP or UDP/IP protocols are not available";
		case net_cantresolveaddr-net_fatal	: return "Can't resolve host address";
		case net_opensocketerror-net_fatal	: return "Error opening socket";
		case net_connectfailed-net_fatal	: return "Connection to host failed";
		case net_hostlogin-net_fatal		: return "Failed to send login request to host";
		case net_serverfull-net_fatal		: return "The server is full";
		case net_servererror-net_fatal		: return "The server failed to create a connection";
		case net_invalidresponse-net_fatal	: return "The server issued an unexpected response to a login request";
		case net_toofewconnections-net_fatal: return "Too few connections were requested when creating the server instance (minimum = 1)";
		case net_bindfailed-net_fatal		: return "Failed to bind socket";
		case net_listenfailed-net_fatal		: return "Could not listen on socket";
		case net_acceptfailed-net_fatal		: return "Failed to accept a connection on the socket";
		case net_ioctlfailed-net_fatal		: return "Failed to set Input/Output mode on socket";
		case net_timeout-net_fatal			: return "A Timeout error occured";
	}
	return "Unknown Error";
}

void reportNetError(void)
{	int error = WSAGetLastError();
	msg("Network Error",buildstr("%i",error));
	WSASetLastError(0);	
}

bool inittestserver(word port)
{	// Perform a firewall safety check - Server
	testserversocket = socket(AF_INET, SOCK_DGRAM,0); 
	if (testserversocket == INVALID_SOCKET) reportNetError();//return false;

	testlocal.sin_family = AF_INET;
	testlocal.sin_addr.s_addr = INADDR_ANY;
	testlocal.sin_port = htons(port);
	if (bind(testserversocket,(struct sockaddr*)&testlocal,sizeof(testlocal) ) != 0) 
	{	closesocket(testserversocket);
		return false;
	}

	u_long argp = 1;
	if (ioctlsocket(testserversocket,FIONBIO,&argp)) 
	{	closesocket(testserversocket);
		return false;
	}
	
	return true;
}

bool inittestclient(word port)
{	// Perform a firewall CONNECT safety check (crash prevention)
	hostent *host = gethostbyname("Localhost");
	if (host == NULL) return false;

	testclientsocket = socket(AF_INET,SOCK_DGRAM,0); // Open a socket
	if (testclientsocket<0) return false;
	
	sockaddr_in server;
	memfill(&server,0,sizeof(server));
	memcpy(&server.sin_addr,host->h_addr,host->h_length);
	server.sin_family = host->h_addrtype;
	server.sin_port = htons(port);

	long retval = connect(testclientsocket,(struct sockaddr*)&server,sizeof(server));
	if (retval == SOCKET_ERROR) return false;

	// Send initial test packet
	retval = send(testclientsocket,"test",5,0);
	if (retval == SOCKET_ERROR) return false;
	return true;
}

bool initnetwork(word port)
{	if (netinitialized)	
	{	msg("Networking Error","An attempt was made to initialize the networking module more than once");
	}
	int fromlen=sizeof(testfrom);

#ifdef PLATFORM_win32
	if (WSAStartup(0x202,&wsaData)) 
	{	reportNetError();
		WSACleanup();
		return false;
	}
#endif
	netinitialized = true;

	serverok = inittestserver(port);
	clientok = inittestclient(port);

	if (!clientok) serverok = false;

	if (clientok && serverok)
	{	// Receive initial test packet
		char buf[10];
		long retval = WSAEWOULDBLOCK;
		dword timeouttime = globalTimer + currentTimerHz * 2;
		while (retval==WSAEWOULDBLOCK && globalTimer<timeouttime)
		{	checkmsg();
			retval = recvfrom(testserversocket,buf,10,0,(struct sockaddr *)&testfrom,&fromlen);
			if (retval == SOCKET_ERROR || retval == 0 || txtcmp(buf,"test")!=0) 
			{	serverok = false;
				closesocket(testserversocket);
			}
		}
		if (globalTimer>=timeouttime)
		{	serverok = false;
			closesocket(testserversocket);
		}
	}

	if (serverok) closesocket(testserversocket);
	if (clientok) closesocket(testclientsocket);
	return true;
}

addresslist *getlocalIP(void)
{	long i;

	if (!netinitialized)
	{	return NULL;
	}

	hostent *h = gethostbyname("LocalHost");
	if (!h) return NULL;
	hostent *host = gethostbyname(h->h_name);
	if (!host) return NULL;

	i=0;
	while (i<10 && host->h_addr_list[i])
	{	IPaddress *hostaddr = (IPaddress *)host->h_addr_list[i];
		localaddrbuf.IP[i].b1 = hostaddr->b1;
		localaddrbuf.IP[i].b2 = hostaddr->b2;
		localaddrbuf.IP[i].b3 = hostaddr->b3;
		localaddrbuf.IP[i].b4 = hostaddr->b4;
		i++;
	}
	localaddrbuf.numIP = i;
	localaddrbuf.name = h->h_name;
	return &localaddrbuf;
}

// ********************************************************************************
// ***								Client Code									***
// ********************************************************************************
struct netclientoem
{	struct	hostent *host;			// Details about the server (IP / port)
	SOCKET  conn_socket;			// Connection socket to the host
	long	socket_type;			// Type of socket being used
	long	refnum;					// The server's reference number for this connection
	byte	Buffer[maxpacketsize];	// Buffer to hold incomming data
	long	bytesinqueue, bytespertick;	// Bandwidth variables
};

netclient::netclient(bool reliable,char *requeststring,char *host,word port,dword bandwidth,void (*_receiver)(netclient *client,byte packettype,byte *data,long size))
{	reliable = false;

	if (!clientok)
	{	connectionerror = net_tcpipna;
		return;
	}
	receiver = _receiver;
	netclientoem *oem = (netclientoem *)fcalloc(sizeof(netclientoem),"Network Client Connection");
	oemdata = oem;

	if (reliable) oem->socket_type = SOCK_STREAM;	// TCP
			else  oem->socket_type = SOCK_DGRAM;	// UDP

	// Attempt to detect if we should call gethostbyname() or gethostbyaddr()
	if (isalpha(host[0])) 
	{   // server address is a name
		oem->host = gethostbyname(host);
	}	else  
	{	// Convert nnn.nnn address to a usable one
		dword addr = inet_addr(host);
		oem->host = gethostbyaddr((char *)&addr,4,AF_INET);
	}
	if (oem->host == NULL ) 
	{	//con->add("Client: Cannot resolve address [%s]: Error %d\n",host,WSAGetLastError());
		connectionerror = net_cantresolveaddr;
		return;
	}

	oem->conn_socket = socket(AF_INET,oem->socket_type,0); // Open a socket
	if (oem->conn_socket <0 ) 
	{	//con->add("Client: Error Opening socket: Error %d\n",WSAGetLastError());
		connectionerror = net_opensocketerror;
		return;
	}

	u_long argp = 1;
	if (ioctlsocket(oem->conn_socket,FIONBIO,&argp))
	{	connectionerror = net_ioctlfailed;
		closesocket(oem->conn_socket);
		return;
	}

	//
	// Notice that nothing in this code is specific to whether we 
	// are using UDP or TCP.
	// We achieve this by using a simple trick.
	//    When connect() is called on a datagram socket, it does not 
	//    actually establish the connection as a stream (TCP) socket
	//    would. Instead, TCP/IP establishes the remote half of the
	//    ( LocalIPAddress, LocalPort, RemoteIP, RemotePort) mapping.
	//    This enables us to use send() and recv() on datagram sockets,
	//    instead of recvfrom() and sendto()

	//
	// Copy the resolved information into the sockaddr_in structure
	//
	sockaddr_in server;
	memfill(&server,0,sizeof(server));
	memcpy(&server.sin_addr,oem->host->h_addr,oem->host->h_length);
	server.sin_family = oem->host->h_addrtype;
	server.sin_port = htons(port);	// Port Number
//dword ip = *(dword *)oem->host->h_addr;
//con->add("server IP = %i.%i.%i.%i, Port %i",(ip)&0xff,(ip>>8)&0xff,(ip>>16)&0xff,(ip>>24)&0xff,port);

	long retval = WSAEWOULDBLOCK;
	while (retval==WSAEWOULDBLOCK)
	{	checkmsg();
		retval = connect(oem->conn_socket,(struct sockaddr*)&server,sizeof(server));
		if (retval == SOCKET_ERROR) 
		{	connectionerror = net_connectfailed;
			closesocket(oem->conn_socket);
			return;
		}
	}

	// Send initial request for a connection
	char *constring = buildstr("%s%8x",requeststring,bandwidth);
	retval = WSAEWOULDBLOCK;
	while (retval==WSAEWOULDBLOCK)
	{	checkmsg();
//con->add("Send '%s' on Socket %i",requeststring,oem->conn_socket);
//		retval = send(oem->conn_socket,requeststring,txtlen(requeststring)+1,0);
		retval = send(oem->conn_socket,constring,txtlen(constring)+1,0);
		if (retval == SOCKET_ERROR) 
		{	connectionerror = net_hostlogin;
			closesocket(oem->conn_socket);
			return;
		}
	}
		
	retval = WSAEWOULDBLOCK;
	dword timeouttime = globalTimer + currentTimerHz * 2;
	while ((globalTimer<timeouttime) && (retval == WSAEWOULDBLOCK))
	{	checkmsg();
		retval = recv(oem->conn_socket,(char *)&oem->Buffer[0],sizeof(oem->Buffer),0 );
		if (retval == SOCKET_ERROR) 
		{	retval = WSAEWOULDBLOCK;
			//connectionerror = net_hostresponse;
			//closesocket(oem->conn_socket);
			//return;
		}
		if (retval == 0) 
		{	retval = WSAEWOULDBLOCK;
		}
	}
	
	if (globalTimer>=timeouttime)
	{	connectionerror = net_timeout;
		closesocket(oem->conn_socket);
		return;
	}

	//con->add("Server says: %s\n",Buffer);
	long ofs = 0;
	char sep;
	char *token;
	token = gettoken((char *)oem->Buffer,&ofs,&sep);
	connectionerror = net_invalidresponse;
	if (txtcmp(token,"ServerFull")==0) connectionerror = net_serverfull;
	if (txtcmp(token,"ConnectionFailed")==0) connectionerror = net_servererror;
	if (txtcmp(token,"ConnectionGranted")==0) 
	{	port = (word)getuinttoken((char *)oem->Buffer,&ofs,&sep);
		oem->refnum = port;
		connectionerror = 0;
		return;
	}

//	con->add("%s",oem->Buffer);
//	con->add("Token = '%s', buffer = %s, length = %i",""/*token*/,oem->Buffer,retval);
//	con->add("Expected ConnectionGranted");
	closesocket(oem->conn_socket);	// ERROR ONLY! This line is not executed during normal execution
}

bool netclient::listen(void)
{	netclientoem *oem = (netclientoem *)oemdata;
	byte *buffer = oem->Buffer;
	long retval = recv(oem->conn_socket,(char *)buffer,sizeof(oem->Buffer),0 );
	// Buffer[0] is checksum
	// Buffer[1] is Packet command
	if (retval>0 && retval!=SOCKET_ERROR)
	{	if (buffer[1]<128)
		{	receiver(this,buffer[1],oem->Buffer+2,retval);
		}
		return true;
	}
	return false;
}

bool netclient::sendOK(void)
{//	netclientoem *oem = (netclientoem *)oemdata; 
//	fd_set info;
//	info.fd_count = 1;
//	info.fd_array[0]=oem->conn_socket;
//	TIMEVAL tv = {0,0};
//	int retval = select(0,NULL,&info,NULL,&tv);
//	return (info.fd_count>0);
	return true;
}

netclient::~netclient(void)
{	if ((connectionerror & 0x80000000) == 0)
	{	netclientoem *oem = (netclientoem *)oemdata;
		closesocket(oem->conn_socket);
	}
	fcfree(oemdata);
}

// ********************************************************************************
// ***								Server Code									***
// ********************************************************************************
struct netconnectionoem
{	sockaddr_in	ClientIP;
	long bytesinqueue, bytespertick;	// Bandwidth variables
};

struct netserveroem
{	byte Buffer[maxpacketsize];
	word port;
	int retval;
	int fromlen;
	int socket_type;
	struct sockaddr_in local, from;
	SOCKET listen_socket, msgsock;
	long bytesinqueue, bytespertick;	// Bandwidth variables
	netconnection *freeconnections;
	netconnection **connectionlookup;
};

char netsendbuffer[maxpacketsize];

void netsend(SOCKET socket,sockaddr_in target,byte *buf, long size)
{	sendto(socket,(char *)buf,size,0,
		(struct sockaddr *)&target,sizeof(sockaddr_in));
}

bool netconnection::init(void (*rec)(netconnection *,byte *,long),dword bandwidth)
{	receiver = rec;
	netconnectionoem *oem = (netconnectionoem *)oemdata;
	netserveroem *serveroem = (netserveroem *)parent->oemdata;
	sprintf(netsendbuffer,"ConnectionGranted %i",connection_num);
	netsend(serveroem->msgsock,oem->ClientIP,(byte *)&netsendbuffer[0],txtlen(netsendbuffer)+1);
	oem->bytespertick = (bandwidth<<8)/currentTimerHz;
	oem->bytesinqueue = 0;
	userdata = 0;
	userptr = NULL;
	return true;
}

byte *netconnection::getbuffer(void)
{	netserveroem *serveroem = (netserveroem *)parent->oemdata;
	return serveroem->Buffer+2;
}

bool netconnection::send(byte packettype,long size)
{	netserveroem *serveroem = (netserveroem *)parent->oemdata;
	netconnectionoem *oem = (netconnectionoem *)oemdata;
	byte *buf = serveroem->Buffer;
	buf[0] = 0;	// Checksum - ignored for now
	buf[1] = packettype;
	long retval = sendto(serveroem->listen_socket,(char *)buf,size+2,0,
						(struct sockaddr *)&oem->ClientIP,sizeof(sockaddr_in));
	return (retval==size);
}

netserver::netserver(bool reliable, char *_requeststring, long _maxconnections, long portnumber,dword bandwidth,
					 void (*newconnection)(netconnection *), void (*defaultreceiver)(netconnection *,byte *,long))
{	reliable = false;
	if (_maxconnections<1)
	{	connectionerror = net_toofewconnections;
		return;
	}

	if (!serverok)
	{	connectionerror = net_tcpipna;
		return;
	}
	netserveroem *oem = (netserveroem *)fcalloc(sizeof(netserveroem),"Network Server Buffers");
	oemdata = (void *)oem;
	if (reliable) oem->socket_type = SOCK_STREAM;	// TCP
			else  oem->socket_type = SOCK_DGRAM;	// UDP
	requeststring = _requeststring;
	requesttxtlen = txtlen(requeststring);
	maxconnections = _maxconnections;
	oem->port = (word)portnumber;

	numconnections = 0;
	oem->freeconnections = NULL;
	firstconnection = NULL;

	oem->connectionlookup = (netconnection **)malloc(maxconnections * ptrsize);
	for (long i=0; i<maxconnections; i++)
	{	netconnection *nc = oem->freeconnections;
		oem->freeconnections = new netconnection();
		oem->freeconnections->connection_num = (word)i;
		oem->freeconnections->parent = this;
		oem->connectionlookup[i] = oem->freeconnections;
		oem->freeconnections->next = nc;
		oem->freeconnections->last = NULL;
		if (nc) nc->last = oem->freeconnections;
	}

	oem->listen_socket = socket(AF_INET, oem->socket_type,0); 
	if (oem->listen_socket == INVALID_SOCKET)
	{	//fprintf(stderr,"socket() failed with error %d\n",WSAGetLastError());
		connectionerror = net_opensocketerror;
		return;
	}

	// bind() associates a local address and port combination with the
	// socket just created. This is most useful when the application is a 
	// server that has a well-known port that clients know about in advance.
	oem->local.sin_family = AF_INET;
	oem->local.sin_addr.s_addr = INADDR_ANY;
	oem->local.sin_port = htons(oem->port);
	if (bind(oem->listen_socket,(struct sockaddr*)&oem->local,sizeof(oem->local) ) != 0) 
	{	//fprintf(stderr,"bind() failed with error %d\n",WSAGetLastError());
		connectionerror = net_bindfailed;
		return;
	}

	// So far, everything we did was applicable to TCP as well as UDP.
	// However, there are certain steps that do not work when the server is
	// using UDP.
	u_long argp = 1;
	if (ioctlsocket(oem->listen_socket,FIONBIO,&argp))
	{	connectionerror = net_ioctlfailed;
		return;
	}

	// We cannot listen() on a UDP socket.
	if (oem->socket_type != SOCK_DGRAM) 
	{	if (::listen(oem->listen_socket,5) == SOCKET_ERROR) 
		{	//fprintf(stderr,"listen() failed with error %d\n",WSAGetLastError());
			connectionerror = net_listenfailed;
			return;
		}
	}
	//printf("'Listening' on port %d, protocol %s\n",port,(socket_type == SOCK_STREAM)?"TCP":"UDP");
	default_receiver = defaultreceiver;
	new_connection = newconnection;
	connectionerror = 0;
	acceptconnections = true;
}

bool netserver::listen(void)
{	long retval;
	if (connectionerror) return false;
	netserveroem *oem = (netserveroem *) oemdata; 
	oem->fromlen =sizeof(oem->from);
//	SOCKET msgsock;
	
	// accept() doesn't make sense on UDP, since we do not listen()
	if (oem->socket_type != SOCK_DGRAM) 
	{	oem->msgsock = accept(oem->listen_socket,(struct sockaddr*)&oem->from, &oem->fromlen);
		if (oem->msgsock == INVALID_SOCKET) 
		{	fprintf(stderr,"accept() error %d\n",WSAGetLastError());
			connectionerror = net_acceptfailed;
			return false;
		}
		//printf("accepted connection from %s, port %d\n", inet_ntoa(from.sin_addr), htons(from.sin_port)) ;
	}
	else
		oem->msgsock = oem->listen_socket;

	// In the case of SOCK_STREAM, the server can do recv() and 
	// send() on the accepted socket and then close it.
	// However, for SOCK_DGRAM (UDP), the server will do
	// recvfrom() and sendto()  in a loop.
	if (oem->socket_type != SOCK_DGRAM)
	{	// TCP
		retval = recv(oem->msgsock,(char *)oem->Buffer,sizeof(oem->Buffer),0 );
	}
	else 
	{	// UDP
		retval = recvfrom(oem->msgsock,(char *)oem->Buffer,sizeof(oem->Buffer),0,(struct sockaddr *)&oem->from,&oem->fromlen);
		//printf("Received datagram from %s\n",inet_ntoa(from.sin_addr));
	}

	if (retval == SOCKET_ERROR) 
	{	//fprintf(stderr,"recv() failed: error %d\n",WSAGetLastError());
		//closesocket(oem->msgsock);
		//con->add("recv() failed: error :%d",WSAGetLastError());
		return false;
	}

	if (retval == 0) 
	{	//printf("Client closed connection\n");
		//closesocket(oem->msgsock);
		return false;
	}
		
	char response[256];
	if (acceptconnections && (retval == requesttxtlen+9))
	{	if (strncmp((char *)oem->Buffer,requeststring,requesttxtlen)==0)
		//if (strncmp((char *)oem->Buffer,requeststring,requesttxtlen-9)==0)
		{	// We have a new net connection!
			netconnection *nc = oem->freeconnections;
			if (nc)
			{	oem->freeconnections = nc->next;
				if (oem->freeconnections) 
				{	oem->freeconnections->last = NULL;
				}
				nc->next = firstconnection;
				nc->last = NULL;				
				if (firstconnection)
				{	firstconnection->last = nc;
				}
				firstconnection = nc;
				numconnections++;
				netconnectionoem *conoem = (netconnectionoem *)fcalloc(sizeof(netconnectionoem),"Network Server Connection");
				firstconnection->oemdata = (void *)conoem;
				conoem->ClientIP = oem->from;
				// Calculate bandwidth
				char *bwptr = (char *)(oem->Buffer + requesttxtlen);
				long i=0;
				while (bwptr[i]!=0 && bwptr[i]==' ')
				{	i++;
				}
				bwptr += i;
				dword bw = 0;
				while (*bwptr)
				{	bw <<= 4;
					char tmp = (toupper)(*bwptr++);
					switch(tmp)
					{	case '0':	bw+=0; break;
						case '1':	bw+=1; break;
						case '2':	bw+=2; break;
						case '3':	bw+=3; break;
						case '4':	bw+=4; break;
						case '5':	bw+=5; break;
						case '6':	bw+=6; break;
						case '7':	bw+=7; break;
						case '8':	bw+=8; break;
						case '9':	bw+=9; break;
						case 'A':	bw+=10; break;
						case 'B':	bw+=11; break;
						case 'C':	bw+=12; break;
						case 'D':	bw+=13; break;
						case 'E':	bw+=14; break;
						case 'F':	bw+=15; break;
					}
				}
//				con->add("Bandwidth = %i",bw*8);
				if (nc->init(default_receiver,bw))
				{	if (new_connection)
						new_connection(nc);
					return true;
				}
				else
					strcpy(response,"ConnectionFailed");
			}
			else
				strcpy(response,"ServerFull");
			netsend(oem->msgsock,oem->from,(byte *)response,txtlen(response));
			return true;
		}
	}

	// It was not a new connection request - must be a data packet from an existing connection
	if (default_receiver)
	{	default_receiver(NULL,oem->Buffer,retval);
	}
	sprintf(response,"Error: %s",oem->Buffer);
	netsend(oem->msgsock,oem->from,(byte *)response,txtlen(response)+1);
	return true;
}

bool netserver::sendOK(void)
{//	netserveroem *oem = (netserveroem *) oemdata; 
//	fd_set info;
//	info.fd_count = 1;
//	info.fd_array[0]=oem->listen_socket;
//	TIMEVAL tv = {0,0};
//	int retval = select(0,NULL,&info,NULL,&tv);
//	return (info.fd_count>0);
	return true;
}
