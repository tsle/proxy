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
#include "token.h"

#define MAXWORDS 100

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void logit(char *logstring, int blocked, int not_found, int size);
char** read_disallowedwords();
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t MAXLEN);
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n);
void Rio_writen_w(int fd, void *usrbuf, size_t n);
int Open_clientfd_w(char *hostname, int port);
int block(char* buf, char** dwords);
char** read_disallowedwords();
void free_disallowedwords(char** dwords);
char* cpystr(char* str);

void echo(int connfd, struct sockaddr_in *clientaddr, char** dwords) {

  size_t n;
  int port;
  rio_t rio, rio_s;
  char method[16], URI[MAXLINE], version[16];
  char buf[MAXLINE], request[MAXLINE], target_addr[MAXLINE], path[MAXLINE];
  int serverfd;
  
  Rio_readinitb(&rio, connfd);

  if((n = Rio_readlineb_w(&rio, buf, MAXLINE)) > 0) {
    sscanf(buf, "%s %s %s", method, URI, version);
    //if(strstr(buf, "HTTP/1.1"))
    //strncpy(strstr(buf, "HTTP/1.1"), "HTTP/1.0", 8);

    int ret = parse_uri(URI, target_addr, path, &port);
    // if there is no error in parsing uri
    if(ret >= 0) {   
      serverfd = Open_clientfd_w(target_addr, port);
      Rio_readinitb(&rio_s, serverfd);
      Rio_writen_w(serverfd, buf, n);     
      // write headers
      while((n = Rio_readlineb_w(&rio, buf, MAXLINE)) > 0) {
	// avoid connection keep alive header
	/*char* connection = strstr(buf, "Connection:");
	if(connection) {
	  strncpy(buf, "Connection: close\r\n", strlen("Connection: close\r\n")+1);
	  Rio_writen_w(serverfd, "Proxy-Connection: close\r\n", 
		       strlen("Proxy-Connection: close\r\n"));
	}
	if (strstr(buf, "Cookie:") != NULL) continue;
	if (strstr(buf, "User-Agent:") != NULL) continue;
	if (strstr(buf, "Accept:") != NULL) continue;
	if (strstr(buf, "Accept-") != NULL) continue;
	*/
	Rio_writen_w(serverfd, buf, n);
	if(strcmp(buf, "\r\n") == 0  || strstr(buf, "Host:") != NULL) {
	  Rio_writen_w(serverfd, "Connection: close\r\n", strlen("Connection: close\r\n"));
	  Rio_writen_w(serverfd, "Proxy-Connection: close\r\n", 
		       strlen("Proxy-Connection: close\r\n"));
	  Rio_writen_w(serverfd, "\r\n\r\n", strlen("\r\n\r\n"));
	  break;
	}
      }
      // read from server and write to client      
      int t_size = 0, blocked = 0, not_found=0;
      struct token** head = (struct token**) malloc(sizeof(struct token*));
      struct token* hd = (struct token*) malloc(sizeof(struct token));
      t_size = Rio_readnb_w(&rio_s, buf, MAXLINE);
      hd->text = cpystr(buf);
      hd->size = t_size; 
      hd->next = NULL;
      *head = hd;
      blocked = block(hd->text, dwords);
      not_found = (strstr(buf, "404 Not Found") != NULL);

      while((n = Rio_readnb_w(&rio_s, buf, MAXLINE)) > 0) {
	struct token* tk = (struct token*) malloc(sizeof(struct token));
	tk->text = cpystr(buf);
	tk->size = n;
	t_size += n;
	addToken(head, tk);
	if (!blocked) blocked = block(buf, dwords);
      }

      if (!blocked) {
	struct token* temp = hd;
	while(temp != NULL) {
	  Rio_writen_w(connfd, temp->text, temp->size);
	  temp = temp->next;
	}
      } else {
	Rio_writen_w(connfd, "BLOCKED \n", strlen("BLOCKED"));
      }

      freeTokenList(head);
      Close(serverfd);

      char logstring[MAXLINE]; 
      printf("%d\n", t_size);
      format_log_entry(logstring, clientaddr, URI, t_size);
      logit(logstring, blocked, not_found, t_size);
    }
  }
  printf(": %d\n", n);
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

  if (fclose(file)) {
    fprintf(stderr, "Error closing file\n");
  };
  dwords[i] = NULL;
  return dwords;
}

void free_disallowedwords(char** dwords) {
  unsigned int i=0;
  for(; dwords[i] != NULL; i++) {
    free(dwords[i]);
  } 
  free(dwords);
}

int block(char* buf, char** dwords) {
  char* text = malloc(strlen(buf) + 1);
  strcpy(text, buf);

  unsigned int i=0;
  for(; dwords[i] != NULL; i++) {
    if (strstr(text, dwords[i]) != NULL) return 1;
  }
  free(text);

  return 0;
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
    clientlen = sizeof(clientaddr);    
    while(1) {
      connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
      /* Determine the domain name and IP address of the client */
      hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
			 sizeof(clientaddr.sin_addr.s_addr), AF_INET);
      haddrp = inet_ntoa(clientaddr.sin_addr);
      echo(connfd, &clientaddr, dis_words);
      Close(connfd);      
    }
    free_disallowedwords(dis_words);
    printf("I've left the unleavable loop!\n");
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

void logit(char* logstring, int blocked, int not_found, int size) {
  FILE* file = fopen("log_file.txt", "a");
  if (blocked) {
    sprintf(logstring, "%s %d (BLOCKED: Page has disallowed words)\n", logstring, size);
  } else if (not_found) {
    sprintf(logstring, "%s (NOTFOUND)\n", logstring);
  } else {
    sprintf(logstring, "%s %d\n", logstring, size);
  }
  
  printf("%s\n", logstring); 
  fprintf(file, "%s\n", logstring);
  if(fclose(file) != 0) {
    fprintf(stderr, "Error closing log file\n.");
  }
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

int Open_clientfd_w(char* hostname, int port) {
  int rc;

  if ((rc = open_clientfd(hostname, port)) < 0) {
    fprintf(stderr, "Open_clientfd_w error: %s\n", strerror(errno));
  }
  return rc;
}

char* cpystr(char* str) {
  int size = strlen(str);
  char *cpy = (char*) malloc((size+1)*sizeof(char));
  strcpy(cpy, str);
  return cpy;
}
