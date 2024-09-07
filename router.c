#include "httpd.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 1024

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
        serve_file("/mnt/data/lion_awake.jpg");
    }

    ROUTE_GET("/lion_sleeping.jpg") {
        serve_file("/mnt/data/lion_sleeping.jpg");
    }

    ROUTE_GET("/")
    {
        // דף הכניסה עם רקע האריה הישן
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("<html><body style='background-image: url(\"/lion_sleeping.jpg\"); background-size: cover;'>");
        printf("<h2>Welcome to the Lion Site</h2>");
        printf("<form action='/login' method='POST'>");
        printf("Username: <input type='text' name='username'><br>");
        printf("Password: <input type='password' name='password'><br>");
        printf("<button type='submit'>Log in</button></form><br>");
        printf("<form action='/register' method='POST'>");
        printf("<button type='submit'>Register</button></form>");
        printf("</body></html>");
    }

    ROUTE_POST("/register")
    {
        // רישום משתמש חדש
        char username[BUF_SIZE], password[BUF_SIZE];
        sscanf(payload, "username=%s&password=%s", username, password);
        
        FILE *file = fopen("password.txt", "a");
        fprintf(file, "%s:%s\n", username, password);
        fclose(file);

        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("User registered successfully! Please log in.");
    }

    ROUTE_POST("/login")
    {
        // התחברות משתמש קיים
        char username[BUF_SIZE], password[BUF_SIZE], line[BUF_SIZE];
        int found = 0;
        
        sscanf(payload, "username=%s&password=%s", username, password);

        FILE *file = fopen("password.txt", "r");
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
            // טעינת פרופיל המשתמש
            char profile_filename[BUF_SIZE];
            sprintf(profile_filename, "%s.data", username);

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
    }

    ROUTE_POST("/update_profile")
    {
        // עדכון פרופיל המשתמש
        char username[BUF_SIZE], profile[BUF_SIZE];
        sscanf(payload, "username=%s&profile=%s", username, profile);
        
        char profile_filename[BUF_SIZE];
        sprintf(profile_filename, "%s.data", username);
        
        FILE *file = fopen(profile_filename, "w");
        fprintf(file, "%s", profile);
        fclose(file);

        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Profile updated successfully.");
    }

    ROUTE_END()
}
