/* A simple client program for server.c

   To compile: gcc client.c -o client

   To run: start the server, then the client */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_URL_LENGTH 1001
#define MAX_URL_NO 101
#define MAX_RESPONSE 100001
#define CONTENT_TYPE "text/html"

void *parse_header(int *status_code, char content_type[100], int *content_length, char*str);
void *parse_html(char *str, char host[MAX_URL_NO][MAX_URL_LENGTH], char page[MAX_URL_NO][MAX_URL_LENGTH], int *url_count, char *implied_host);
void *print_log(char host[MAX_URL_NO][MAX_URL_LENGTH], char page[MAX_URL_NO][MAX_URL_LENGTH], int *url_count);
void *parse_url(char *str, char temp_host[MAX_URL_LENGTH], char temp_page[MAX_URL_LENGTH]);
int compare_host(char* host1, char* host2);


int main(int argc, char ** argv)
{
    int sockfd, portno = 80, url_count = 1, url_read = 0, repeat_flag = 0;
    struct sockaddr_in serv_addr;
    struct hostent * server;
    char *reference_url;
    char hosts[MAX_URL_NO][MAX_URL_LENGTH];
    char pages[MAX_URL_NO][MAX_URL_LENGTH];
    char temp_host[MAX_URL_LENGTH];
    char temp_page[MAX_URL_LENGTH];
    char buffer[MAX_RESPONSE];
    char msg[2000];
    int status_code, content_length;
    char content_type[100];
    
    reference_url = argv[1];
    
    parse_url(reference_url, temp_host, temp_page);
    strcpy(hosts[0],temp_host);
	strcpy(pages[0],temp_page);
    while (url_read < url_count) {
        url_read +=1;
    server = gethostbyname(temp_host);
    
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    /* Building data structures for socket */

    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    bcopy(server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno);

    /* Create TCP socket -- active open
    * Preliminary steps: Setup: creation of active open socket
    */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(0);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting");
        exit(0);
    }
    sprintf(msg, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nUser-Agent: pou1\r\n\r\n", temp_page, temp_host);
    write(sockfd, msg, strlen(msg));
    bzero(buffer, MAX_RESPONSE);
    
    while(read(sockfd, buffer, MAX_RESPONSE-1) != 0){
    	//fprintf(stderr, "%s", buffer);
    	parse_header(&status_code, content_type, &content_length, buffer);
    	//add condition here, status code response, content type, content length?
    	if (status_code == 503) {
    	    repeat_flag = 1;
    	}
		parse_html(buffer, hosts, pages, &url_count, temp_host);
		bzero(buffer, MAX_RESPONSE);
	}
	if (repeat_flag == 1) {
	    shutdown(sockfd, SHUT_RDWR); 
	    close(sockfd);
	    repeat_flag = 0;
	    continue;
	}
    bzero(buffer, MAX_RESPONSE);
	shutdown(sockfd, SHUT_RDWR); 
	close(sockfd);
	bzero(temp_host, MAX_URL_LENGTH);
	bzero(temp_page, MAX_URL_LENGTH);
	if (url_read < url_count) {
		strcpy(temp_host, hosts[url_read]);
		strcpy(temp_page, pages[url_read]);
	}
	
	}
	
	print_log(hosts, pages, &url_count);
    return 0;
}


void *parse_header(int *status_code, char content_type[100], int *content_length, char*str) {
	char *status_pattern = "HTTP/1.1 ";
	char *type_pattern = "Content-Type: ";
	char *length_pattern = "Content-Length: ";
	int status_flag = 0, type_flag = 0, length_flag = 0, i = 0,j = 0;
	bzero(content_type,100);
	while(*str != '\0') {
		if (status_flag == 0) {
			//compare string with pattern, if matched, i would be the length of the pattern
			for (i = 0; i < strlen(status_pattern); i++) {
				if (*(str+i) != *(status_pattern+i)) {
					break;
				}
			}
			if (i == strlen(status_pattern)) {
				while (isdigit(*(str+i))) {
					//numeric manipulation of string to numbers
					*status_code = *status_code*10 + (*(str+i) - '0');
					i++;
				}
				status_flag = 1;
			}
		}
		if (length_flag == 0) {
			for (i = 0; i < strlen(length_pattern); i++) {
				if (*(str+i) != *(length_pattern+i)) {
					break;
				}
			}
			if (i == strlen(length_pattern)) {
				while (isdigit(*(str+i))) {
					*content_length = *content_length*10 + (*(str+i) - '0');
					i++;
				}
				length_flag = 1;
			}
		}
		if (type_flag == 0) {
			for (i = 0; i < strlen(type_pattern); i++) {
				if (*(str+i) != *(type_pattern+i)) {
					break;
				}
			}
			if (i == strlen(type_pattern)) {
				while (isalpha(*(str+i+j)) || *(str+i+j) == '/') {
					*(content_type + j) = *(str+i + j);
					j++;
				}
				type_flag = 1;
			}
		}
		*str++;
	}
}

void *parse_html(char *str, char host[MAX_URL_NO][MAX_URL_LENGTH], char page[MAX_URL_NO][MAX_URL_LENGTH], int *url_count, char* implied_host)
{
	int first_flag = 0, i, page_flag = 0, index_value = 0, k;
	char *first = "<a";
	char *second = "href=\"";
	char *second_1 = "href = \"";
	char *scheme = "http://";
	char temp_host[MAX_URL_LENGTH];
	char temp_page[MAX_URL_LENGTH];
	int status_code, content_length;
	char *content_type;
	
	bzero(temp_host,MAX_URL_LENGTH);
	bzero(temp_page,MAX_URL_LENGTH); 
	while (*str != '\0' && *url_count < MAX_URL_NO) {
		//keep looping with cotinue until match is found
		if (first_flag == 0) {
			for (i = 0; i < strlen(first); i++) {
				if (tolower(*(str+i)) != *(first+i)) {
					break;
				}
			}
			if (i != strlen(first)) {
				*str++;
				continue;
			} else {
				first_flag = 1;
				*str++;	
			}
		}
		//i is used to keep track of how long the prefix is, then skip these and read the url
		for (i = 0; i < strlen(second); i++) {
			if (tolower(*(str+i)) != (*(second+i))) {
				break;
			}
		}
		//try the other pattern, possible to put all possible patterns into a list, and go through the list one by one
		//if there are many patterns
		if (i != strlen(second)) {
			for (k = 0; k < strlen(second_1); k++) {
				if (tolower(*(str+k)) != (*(second_1+k))) {
					break;
				}
			}
			if (k == strlen(second_1)) {
				i = k;
			} else {
				*str++;
				continue;
			}
		}
		//page_flag use to determine if host is treated already (whether implied or )
		page_flag = 0;
		if (*(str+i) == '/') {
			//host is implied with // at the start of url
			if (*(str+i+1) != '/') {
				page_flag = 1;
				strcpy(temp_host, implied_host);
			} else {
				//implied protocol
				i += 2;
			}
		} else {
			//7 is length of http://
			i += 7;
		}

		temp_page[0] = '/';
		index_value = 0;
		for (int j = 0; j < MAX_URL_LENGTH; j++) {
			if (*(str+i+j) != '\"') {
				if (page_flag == 0) {
					if (*(str+i+j) != '/') {
						temp_host[j] = *(str+i+j);
					} else {
						page_flag = 1;
						index_value = j;
					}
				} else {
					temp_page[j-index_value] = *(str+i+j);
				}		
			} else {
				//printf("temp_host: %s, temp_page: %s\n",temp_host, temp_page); for some reason temp_host empty with // start
				//compare no identical with any other url in log and all but first components must be equal
				//printf("host: %s, page: %s\n",temp_host, temp_page);
				for (i = 0; i < *url_count; i++) {
					if (strcmp(temp_host,host[i]) == 0 && strcmp(temp_page,page[i]) == 0) {
						break;
					}
				}
				if (compare_host(temp_host,implied_host) == 1) {
					if (i == *url_count) {
						//printf("new added: host: %s, page: %s", temp_host, temp_page);
						strcpy(host[*url_count],temp_host);
						strcpy(page[*url_count],temp_page);
						*url_count += 1;
					}
				}
				first_flag = 0;
				page_flag = 0;
				bzero(temp_host,MAX_URL_LENGTH);
				bzero(temp_page, MAX_URL_LENGTH);
				break;
			}
		}
		*str++;
	}
}

void *print_log(char host[MAX_URL_NO][MAX_URL_LENGTH], char page[MAX_URL_NO][MAX_URL_LENGTH], int *url_count) {
	for (int i = 0; i < *url_count; i++) {
    	printf("http://");
		for (int j = 0; j < MAX_URL_LENGTH; j++) {
			if (host[i][j]) {
				printf("%c",host[i][j]);
				fflush(stdout);
			} else {
				break;
			}
		}
		for (int j = 0; j < MAX_URL_LENGTH; j++) {
			if (page[i][j]) {
				printf("%c",page[i][j]);
				fflush(stdout);
			} else {
				break;
			}
		}
		printf("\n");
	}
	
}

void *parse_url(char *str, char temp_host[MAX_URL_LENGTH], char temp_page[MAX_URL_LENGTH]){
	bzero(temp_host,MAX_URL_LENGTH);
	bzero(temp_page,MAX_URL_LENGTH); 
	int i = 7, page_flag = 0, index_value = 0, j = 0;
	temp_page[0] = '/';
	while (*(str+i+j)) {
		if (page_flag == 0) {
			if (*(str+i+j) != '/') {
				temp_host[j] = *(str+i+j);
			} else {
				page_flag = 1;
				index_value = j;
			}
		} else {
			temp_page[j-index_value] = *(str+i+j);
		}
		j++;
	}
}

int compare_host(char* host1, char* host2) {
	int host1_index = -1, host2_index = -1;
	for (int i = 0; i < MAX_URL_LENGTH; i++) {
		if (*(host1+i) == '.') {
			host1_index = i;
			break;
		}
	}
	for (int i = 0; i < MAX_URL_LENGTH; i++) {
		if (*(host2+i) == '.') {
			host2_index = i;
			break;
		}
	}
	//both only have one component
	if (host1_index == -1 && host2_index == -1) {
		return 1;
	}
	//can assume all urls have same number of components
	if (strlen(host1)-host1_index != strlen(host2)-host2_index) {
		return 0;
	}
	//equal length of all but first components, so don't need to worry about out of index issue
	for (int i = 1; i < strlen(host1) - host1_index; i++) {
		if (host1[host1_index+i]!=host2[host2_index+i]) {
			return 0;
		}
	}
	return 1;
}
