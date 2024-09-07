#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct { char *name, *value; } header_t;
static header_t reqhdr[17] = { {"\0", "\0"} };

char    *method,    // "GET" or "POST"
        *uri,       // "/index.html" things before '?'
        *qs,        // "a=1&b=2"     things after  '?'
        *prot;      // "HTTP/1.1"

char    *payload;     // for POST
int      payload_size;



// get request header
char *request_header(const char* name)
{
    header_t *h = reqhdr;
    while(h->name) {
        if (strcmp(h->name, name) == 0) return h->value;
        h++;
    }
    return NULL;
}
void analyze_http(char* buf ,int rcvd){

	buf[rcvd] = '\0';
	method = strtok(buf,  " \t\r\n");
        uri    = strtok(NULL, " \t");
        prot   = strtok(NULL, " \t\r\n"); 

        fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", method, uri);
        
        if (qs = strchr(uri, '?'))
        {
            *qs++ = '\0'; //split URI
        } else {
            qs = uri - 1; //use an empty string
        }

        header_t *h = reqhdr;
        char *t, *t2;

        while(h < reqhdr+16) {

            char *k,*v,*t;
            k = strtok(NULL, "\r\n: \t"); if (!k) break;
            v = strtok(NULL, "\r\n");     while(*v && *v==' ') v++;
            h->name  = k;
            h->value = v;
            h++;
            fprintf(stderr, "[H] %s: %s\n", k, v);
            t = v + 1 + strlen(v);
            if (t[1] == '\r' && t[2] == '\n')
		break;
        }

       
        t2 = request_header("Content-Length"); // and the related header if there is  
        payload_size = t2 ? atol(t2) : (rcvd-(t-buf));


	payload = buf+ rcvd-payload_size +1;
       if (payload_size <100)
            fprintf(stderr, "[H] %d %s:\n", payload_size  ,payload );

}