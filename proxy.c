/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS: (put your names here)
 *     Student Name1, student1@cs.uky.edu 
 *     Student Name2, student2@cs.uky.edu 
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);

void echo(int connfd) {
  size_t n,m;
  char buf[MAXLINE], request[MAXLINE], target_addr[MAXLINE], path[MAXLINE];
  int port;
  rio_t rio;
  
  Rio_readinitb(&rio, connfd);
  while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
    //    Rio_writen(connfd, buf, n);
    int ret = parse_uri(buf, target_addr, path, &port);
    // if there is no error in parsing uri
    if(ret >= 0) {
      int serverfd = Open_clientfd(target_addr, port);
      rio_t rio_s;
      char buffer[MAXLINE];
      Rio_readinitb(&rio_s, serverfd);
      // create request string
      printf("GET %s HTTP/1.1\n", path);
      sprintf(request, "GET %s HTTP/1.1\n\n", path);
      Rio_writen(serverfd, request, sizeof(request));
      // read from server and write to client
      while((m = Rio_readlineb(&rio_s, buffer, MAXLINE)) != 0) {
	Rio_writen(connfd, buffer, m);
      }
      Close(serverfd);
    }   
  }
}



/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{

    /* Check arguments */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
	exit(0);
    }
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    struct hostent* hp;
    char *haddrp;

    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);
    
    while(1) {
      clientlen = sizeof(clientaddr);
      connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
      /* Determine the domain name and IP address of the client */
      hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
			 sizeof(clientaddr.sin_addr.s_addr), AF_INET);
      haddrp = inet_ntoa(clientaddr.sin_addr);
      printf("server connected to %s (%s)\n", hp->h_name, haddrp);
      echo(connfd);
      Close(connfd);      
    }

    exit(0);
}


/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
	hostname[0] = '\0';
	return -1;
    }
       
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
	*port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
      pathname[0] = '/';
      pathname[1] = '\0';
    }
    else {
	pathbegin++;	
	strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}


