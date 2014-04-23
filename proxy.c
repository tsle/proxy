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

#define MAXWORDS 100

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
char** read_disallowedwords();
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t MAXLEN);
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n);
void Rio_writen_w(int fd, void *usrbuf, size_t n);

void echo(int connfd) {
  size_t n,m;
  char buf[MAXLINE] = "\0", request[MAXLINE] = "\0", 
    target_addr[MAXLINE] = "\0", path[MAXLINE] = "\0";
  int port;
  rio_t rio;
  char method[MAXLINE], URI[MAXLINE], version[MAXLINE];
  
  Rio_readinitb(&rio, connfd);
  while((n = Rio_readlineb_w(&rio, buf, MAXLINE)) != 0) {
    sscanf(buf, "%s %s %s", method, URI, version);
    if(strcmp(method,"GET") != 0) continue;
    //strncpy(strstr(buf, "HTTP/1.1"), "HTTP/1.0", 8);

    int ret = parse_uri(URI, target_addr, path, &port);

    printf("hostname: %s, path: %s, port: %d\n", target_addr, path, port); 

    // printf("%s\n", target_addr);
    // if there is no error in parsing uri
    if(ret >= 0) {
      int serverfd = Open_clientfd(target_addr, port);      
      rio_t rio_s;
      char buffer[MAXBUF];
      Rio_readinitb(&rio_s, serverfd);
      Rio_writen_w(serverfd, buf, n);      
      Rio_writen_w(serverfd, "\r\n", strlen("\r\n"));
      // read from server and write to client      
      while((m = Rio_readnb_w(&rio_s, buffer, MAXBUF)) > 0) {
	Rio_writen_w(connfd, buffer, m);
      }
      Close(serverfd);
    }
    
    buf[0] = '\0', request[0] = '\0', target_addr[0] = '\0';
    path[0] = '\0', method[0]  = '\0', URI[0] = '\0';
  }
}

char** read_disallowedwords() {
  char** dwords = malloc(MAXWORDS * sizeof(char*));
  FILE* file = fopen("DisallowedWords","r");
  // if could not open file
  if(file == NULL) {
    fprintf(stderr, "Can't open file DisallowedWords.\n");
    exit(1);
  }
  // read from file
  char word[200];
  int i=0;
  while( fgets(word, MAXLINE, file) != NULL) {
    char* p = strchr(word,'\n');
    if(p) *p = '\0';
    dwords[i] = malloc(sizeof(char) * strlen(word));
    strcpy(dwords[i++],word);
  }
  dwords[i] = NULL;
  return dwords;
}



/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    char** dis_words = read_disallowedwords();
    // print out disallowed words
    int i=0;
    while(dis_words[i] != NULL) {
      printf("%s\n", dis_words[i++]);
    }

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
      pathname[0] = '\0';
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

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t MAXLEN) 
{
  ssize_t rc;

  if ((rc = rio_readlineb(rp, usrbuf, MAXLEN)) < 0){
    fprintf(stderr, "Rio_readlineb_w error: %s\n", strerror(errno));
    return 0;
  }
  return rc;
} 

ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n)
{
  ssize_t rc;

  if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
    fprintf(stderr, "Rio_readnb_w error: %s\n", strerror(errno));
    return -1;
  }
  return rc;
}

void Rio_writen_w(int fd, void *usrbuf, size_t n) 
{
  if (rio_writen(fd, usrbuf, n) != n)
    fprintf(stderr, "Rio_writen_w error: %s\n", strerror(errno));
}


