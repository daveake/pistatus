#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ifaddrs.h>

void UDPSend(char *message, int port)
{ 
	if (port > 0)
	{
		int sd, rc;
		struct sockaddr_in cliAddr, remoteServAddr;
		struct hostent *h;
		int broadcast = 1;
	  
		h = gethostbyname("255.255.255.255");
		remoteServAddr.sin_family = h->h_addrtype;
		memcpy((char *) &remoteServAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
		remoteServAddr.sin_port = htons(port);

		// socket creation
		sd = socket(AF_INET,SOCK_DGRAM,0);
		if (sd<0)
		{
			printf("Cannot open socket on port %d\n", port);
			return;
		}
		
		if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof broadcast) == -1)
		{
			printf("setsockopt (SO_BROADCAST)");
			return;
		}
	  
		// bind any port
		cliAddr.sin_family = AF_INET;
		cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		cliAddr.sin_port = htons(0);
	  
		rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
		if (rc<0)
		{
			printf("Cannot bind port %d\n", port);
			return;
		}

		// send data
		rc = sendto(sd, message, strlen(message), 0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));

		if (rc<0)
		{
			printf("Cannot send data %s to port %d \n", message, port);
			close(sd);
			return;
		}

		printf("Send %d bytes to port %d\n", rc, port);
		
		close(sd);
	}
}

char *Hostname(void)
{
	static char Buffer[80];
	
	strcpy(Buffer, "PI");
	
    gethostname(Buffer, sizeof(Buffer));

	return Buffer;
}

float Temperature(void)
{
	FILE *fp;
	float T;
			
	fp = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
	if (fp != NULL)
	{
		fscanf (fp, "%f", &T);
		fclose (fp);
		return T / 1000.0;
	}
	
	return 0;
}

int GetCPULoad(void)
{
	int FileHandler;
	char FileBuffer[1024];
	float load;

	FileHandler = open("/proc/loadavg", O_RDONLY);
	if (FileHandler < 0)
	{
		return -1;
	}

	read(FileHandler, FileBuffer, sizeof(FileBuffer) - 1);
	sscanf(FileBuffer, "%f", &load);
	close(FileHandler);
	return (int)(load * 100);
}

char *GetIPAddress(void)
{
	static char IPAddress[100];
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;
	
	IPAddress[0] = '\0';

    if (getifaddrs(&ifap) == 0)
	{
		for (ifa = ifap; ifa; ifa = ifa->ifa_next)
		{
			if (ifa->ifa_addr != NULL)
			{
				// Family is known (which it isn't for a VPN)
				if (ifa->ifa_addr->sa_family==AF_INET)
				{
					// Exclude docker bridges
					if (strstr(ifa->ifa_name, "docker") == NULL)
					{
						sa = (struct sockaddr_in *) ifa->ifa_addr;
						addr = inet_ntoa(sa->sin_addr);
						if (strcmp(addr, "127.0.0.1") != 0)
						{
							strcpy(IPAddress, addr);
						}
					}
				}
			}
        }
    }

    freeifaddrs(ifap);
	
	return IPAddress;
}

void main(int argc, char *argv[])
{
	int Port;
	char Message[200];

	Port = 0;
	if (argc > 1)
	{
		Port = atoi(argv[1]);
		sprintf(Message, "PI:SRC=%s,IP=%s,TEMP=%.1f,CPU=%d\n", Hostname(), GetIPAddress(), Temperature(), GetCPULoad());
		printf("Sending to port %d: %s\n", Port, Message);
		UDPSend(Message, Port);
	}
	else
	{
		printf("Usage: pistatus <port>\n");
	}
}

