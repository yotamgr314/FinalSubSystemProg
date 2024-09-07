#include "httpd.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE 1024

// Function to decode URL-encoded strings
char *urldecode(char *src) {
    char *dest = src;
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dest++ = 16*a + b;
            src += 3;
        } else if (*src == '+') {
            *dest++ = ' ';
            src++;
        } else {
            *dest++ = *src++;
        }
    }
    *dest = '\0';
    return src;
}

// Function to extract values from form data (without modifying original string)
int extract_value(const char *data, const char *key, char *value, size_t value_size) {
    const char *start = strstr(data, key);
    if (!start) return 0;  // Key not found

    start += strlen(key);  // Move past the key
    const char *end = strchr(start, '&');  // Find the end of this key-value pair
    if (!end) end = start + strlen(start);  // If no '&' found, use the remaining string

    size_t len = end - start;
    if (len >= value_size) return 0;  // Value too long

    strncpy(value, start, len);
    value[len] = '\0';
    urldecode(value);
    return 1;
}

// Function to serve image files
void serve_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }
    
    // Print image headers
    printf("HTTP/1.1 200 OK\r\n");
    printf("Content-Type: image/jpeg\r\n");
    printf("Connection: close\r\n\r\n");
    
    // Send the image data
    char buffer[BUF_SIZE];
    size_t n;
    while ((n = fread(buffer, 1, BUF_SIZE, file)) > 0) {
        fwrite(buffer, 1, n, stdout);
    }
    
    fclose(file);
}

void route() {
    ROUTE_START()

    // Serve the lion images
    ROUTE_GET("/lion_awake.jpg") {
        serve_file("/mnt/c/FinalSubSystemProg/data/lion_awake.jpg");
    }

    ROUTE_GET("/lion_sleeping.jpg") {
        serve_file("/mnt/c/FinalSubSystemProg/data/lion_sleeping.jpg");
    }

    // Home page with lion sleeping background
    ROUTE_GET("/")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("<html><body style='background-image: url(\"/lion_sleeping.jpg\"); background-size: cover;'>");
        printf("<h2>Welcome to the Lion Site</h2>");
        printf("<form action='/login' method='POST'>");
        printf("Username: <input type='text' name='username'><br>");
        printf("Password: <input type='password' name='password'><br>");
        printf("<button type='submit'>Log in</button></form><br>");
        printf("<form action='/register' method='POST'>");
        printf("Username: <input type='text' name='username'><br>");
        printf("Password: <input type='password' name='password'><br>");
        printf("<button type='submit'>Register</button></form>");
        printf("</body></html>");
    }

    // Register a new user
    ROUTE_POST("/register")
    {
        if (payload_size > 0) {
            char username[BUF_SIZE];
            char password[BUF_SIZE];

            if (extract_value(payload, "username=", username, sizeof(username)) &&
                extract_value(payload, "password=", password, sizeof(password))) {

                FILE *file = fopen("password.txt", "a");
                if (file != NULL) {
                    fprintf(file, "%s:%s\n", username, password);
                    fclose(file);
                    printf("HTTP/1.1 200 OK\r\n\r\n");
                    printf("User registered successfully! Please log in.");
                } else {
                    printf("HTTP/1.1 500 Internal Server Error\r\n\r\n");
                    printf("Failed to register the user.");
                }
            } else {
                printf("HTTP/1.1 400 Bad Request\r\n\r\n");
                printf("Username or password missing.");
            }
        } else {
            printf("HTTP/1.1 400 Bad Request\r\n\r\n");
            printf("No data received.");
        }
    }

    // Login an existing user
    ROUTE_POST("/login")
    {
        char username[BUF_SIZE];
        char password[BUF_SIZE];

        if (extract_value(payload, "username=", username, sizeof(username)) &&
            extract_value(payload, "password=", password, sizeof(password))) {

            FILE *file = fopen("password.txt", "r");
            if (file == NULL) {
                printf("HTTP/1.1 500 Internal Server Error\r\n\r\n");
                printf("Failed to open the password file.");
                return;
            }

            char line[BUF_SIZE];
            int found = 0;
            while (fgets(line, sizeof(line), file)) {
                char stored_username[BUF_SIZE], stored_password[BUF_SIZE];
                sscanf(line, "%[^:]:%s", stored_username, stored_password);
                
                if (strcmp(stored_username, username) == 0 && strcmp(stored_password, password) == 0) {
                    found = 1;
                    break;
                }
            }
            fclose(file);

            if (found) {
                // Load the user profile
                char profile_filename[BUF_SIZE];
                snprintf(profile_filename, sizeof(profile_filename), "%s.data", username);

                file = fopen(profile_filename, "r");
                if (file) {
                    char profile_text[BUF_SIZE];
                    fread(profile_text, sizeof(char), BUF_SIZE, file);
                    fclose(file);
                    printf("HTTP/1.1 200 OK\r\n\r\n");
                    printf("<html><body style='background-image: url(\"/lion_awake.jpg\"); background-size: cover;'>");
                    printf("<h2>Welcome, %s</h2>", username);
                    printf("<form action='/update_profile' method='POST'>");
                    printf("Your profile:<br><textarea name='profile'>%s</textarea><br>", profile_text);
                    printf("<input type='hidden' name='username' value='%s'>", username);
                    printf("<button type='submit'>Save Profile</button></form>");
                    printf("</body></html>");
                } else {
                    printf("HTTP/1.1 200 OK\r\n\r\n");
                    printf("No profile found.");
                }
            } else {
                printf("HTTP/1.1 401 Unauthorized\r\n\r\n");
                printf("Incorrect username or password.");
            }
        } else {
            printf("HTTP/1.1 400 Bad Request\r\n\r\n");
            printf("Username or password missing.");
        }
    }

    // Update the user profile
    ROUTE_POST("/update_profile")
    {
        char username[BUF_SIZE], profile[BUF_SIZE];
        if (extract_value(payload, "username=", username, sizeof(username)) &&
            extract_value(payload, "profile=", profile, sizeof(profile))) {

            char profile_filename[BUF_SIZE];
            snprintf(profile_filename, sizeof(profile_filename), "%s.data", username);
            
            FILE *file = fopen(profile_filename, "w");
            fprintf(file, "%s", profile);
            fclose(file);

            printf("HTTP/1.1 200 OK\r\n\r\n");
            printf("Profile updated successfully.");
        } else {
            printf("HTTP/1.1 400 Bad Request\r\n\r\n");
            printf("Failed to update profile.");
        }
    }

    ROUTE_END()
}
