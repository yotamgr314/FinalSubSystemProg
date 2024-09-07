#include "httpd.h"



int main(int c, char** v)
{
    serve_forever("8005");
    return 0;
}

