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
        serve_file("/mnt/c/FinalSubSystemProg/data/lion_awake.jpg");
    }

    ROUTE_GET("/lion_sleeping.jpg") {
        serve_file("/mnt/c/FinalSubSystemProg/data/lion_sleeping.jpg");
    }

    // דף הבית עם רקע האריה הישן
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
        printf("<button type='submit'>Register</button></form>");
        printf("</body></html>");
    }

    // רישום משתמש חדש
ROUTE_POST("/register")
{
    if (payload_size > 0) {
        char *username = strtok(payload, "&");
        char *password = strtok(NULL, "&");

        // הסר את "username=" ו-"password="
        if (username && password) {
            username = username + 9;  // "username=" הוא 9 תווים
            password = password + 9;  // "password=" הוא 9 תווים

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

    // התחברות משתמש קיים
    ROUTE_POST("/login")
    {
        char *username = strtok(payload, "&");
        char *password = strtok(NULL, "&");

        username = username + 9;  // "username=" הוא 9 תווים
        password = password + 9;  // "password=" הוא 9 תווים

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
            // טעינת פרופיל המשתמש
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
    }

    // עדכון פרופיל המשתמש
    ROUTE_POST("/update_profile")
    {
        char username[BUF_SIZE], profile[BUF_SIZE];
        sscanf(payload, "username=%s&profile=%s", username, profile);
        
        char profile_filename[BUF_SIZE];
        snprintf(profile_filename, sizeof(profile_filename), "%s.data", username);
        
        FILE *file = fopen(profile_filename, "w");
        fprintf(file, "%s", profile);
        fclose(file);

        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Profile updated successfully.");
    }

    ROUTE_END()
}
