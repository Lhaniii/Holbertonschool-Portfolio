#include <microhttpd.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8888

// Declaration of the functions used in the code
int verify_user(MYSQL *conn, const char *email, const char *password);
int register_user(MYSQL *conn, const char *username, const char *email, const char *password, const char *role);

struct connection_info {
    char *post_data;
};

// Function to decode URL-encoded characters like %40 (for @)
void url_decode(char *src, char *dest) {
    char hex[3];
    while (*src) {
        if (*src == '%') {
            hex[0] = *(src + 1);
            hex[1] = *(src + 2);
            hex[2] = '\0';
            *dest++ = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            *dest++ = ' ';
            src++;
        } else {
            *dest++ = *src++;
        }
    }
    *dest = '\0';
}

// Function to handle HTTP requests
static enum MHD_Result answer_to_connection(void *cls, struct MHD_Connection *connection, 
                                            const char *url, const char *method,
                                            const char *version, const char *upload_data,
                                            size_t *upload_data_size, void **con_cls) {

    // Retrieve the MySQL connection from cls
    MYSQL *conn = (MYSQL *)cls;

    if (*con_cls == NULL) {
        struct connection_info *con_info = malloc(sizeof(struct connection_info));
        con_info->post_data = NULL;
        *con_cls = con_info;
        return MHD_YES;
    }

    struct connection_info *con_info = *con_cls;

    if (strcmp(method, "POST") == 0) {
        printf("POST request received\n");

        if (*upload_data_size != 0) {
            con_info->post_data = realloc(con_info->post_data, *upload_data_size + 1);
            memcpy(con_info->post_data, upload_data, *upload_data_size);
            con_info->post_data[*upload_data_size] = '\0'; // Null terminate the string
            *upload_data_size = 0;
            return MHD_YES;
        }

        // Route to handle user login
        if (strncmp(url, "/login", 6) == 0) {
            printf("POST data received: %s\n", con_info->post_data);

            // Now parse the POST data (email and password)
            char *email = strstr(con_info->post_data, "email=");
            char *password = strstr(con_info->post_data, "password=");
            
            if (email && password) {
                email += 6;  // Move past 'email='
                password += 9;  // Move past 'password='

                // Ensure that both email and password are terminated properly
                char *email_end = strchr(email, '&');
                if (email_end) *email_end = '\0';

                // Decoded email and password
                char decoded_email[256];
                char decoded_password[256];
                url_decode(email, decoded_email);
                url_decode(password, decoded_password);

                printf("Decoded Email: %s, Decoded Password: %s\n", decoded_email, decoded_password);

                if (verify_user(conn, decoded_email, decoded_password) == 0) {
                    // User login successful
                    const char *response_text = "Login successful!";
                    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_text),
                                                                                    (void *)response_text,
                                                                                    MHD_RESPMEM_PERSISTENT);
                    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                    MHD_destroy_response(response);
                    free(con_info->post_data);
                    return ret;
                } else {
                    // Login failed
                    const char *response_text = "Invalid email or password!";
                    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_text),
                                                                                    (void *)response_text,
                                                                                    MHD_RESPMEM_PERSISTENT);
                    int ret = MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
                    MHD_destroy_response(response);
                    free(con_info->post_data);
                    return ret;
                }
            } else {
                printf("Email or password missing\n");
            }
        }

        // Route to handle user registration (similar to login)
        if (strncmp(url, "/register", 9) == 0) {
            const char *username = strstr(con_info->post_data, "username=");
            const char *email = strstr(con_info->post_data, "email=");
            const char *password = strstr(con_info->post_data, "password=");
            const char *role = strstr(con_info->post_data, "role=");

            if (username && email && password && role) {
                // Strip out the field values as needed
                // Similar parsing to login
                // Continue registration logic here...
            }
        }

        // If no matching route
        const char *error_message = "Unsupported POST route";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_message),
                                                                        (void *) error_message,
                                                                        MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        free(con_info->post_data);
        return ret;
    }

    // If the request is not a POST request
    const char *error_message = "Unsupported method";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_message),
                                                                    (void *) error_message,
                                                                    MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_IMPLEMENTED, response);
    MHD_destroy_response(response);
    return ret;
}

int main() {
    // Step 1: Initialize MySQL connection
    MYSQL *conn;
    conn = mysql_init(NULL);

    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }

    if (mysql_real_connect(conn, "localhost", "root", "password", "devlink", 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    printf("Connected to MySQL database!\n");

    // Step 2: Start the HTTP server using libmicrohttpd
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, 
                              &answer_to_connection, conn, MHD_OPTION_END);

    if (NULL == daemon) {
        printf("Failed to start HTTP server\n");
        return 1;
    }

    printf("HTTP server running on port %d...\n", PORT);

    // Step 3: Keep the server running
    getchar();  // Wait for the user to press a key to stop the server

    // Step 4: Stop the HTTP server and close MySQL connection
    MHD_stop_daemon(daemon);
    mysql_close(conn);

    return 0;
}

// Function for registering a user in the database
int register_user(MYSQL *conn, const char *username, const char *email, const char *password, const char *role) {
    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO users (username, email, password, role) VALUES ('%s', '%s', '%s', '%s')",
             username, email, password, role);

    // Insert into the users table
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to insert data into users: %s\n", mysql_error(conn));
        return 1; // Error
    }

    // Get the last inserted user ID
    int user_id = mysql_insert_id(conn);

    // If the role is 'freelance', insert into the freelances table with email_contact
    if (strcmp(role, "freelance") == 0) {
        snprintf(query, sizeof(query),
                 "INSERT INTO freelances (user_id, email_contact) VALUES (%d, '%s')", user_id, email);

        if (mysql_query(conn, query)) {
            fprintf(stderr, "Failed to insert data into freelances: %s\n", mysql_error(conn));
            return 1; // Error
        }

        printf("Freelance '%s' successfully registered with email '%s'!\n", username, email);
    }
    // If the role is 'client', insert into the clients table
    else if (strcmp(role, "client") == 0) {
        snprintf(query, sizeof(query),
                 "INSERT INTO clients (user_id) VALUES (%d)", user_id);

        if (mysql_query(conn, query)) {
            fprintf(stderr, "Failed to insert data into clients: %s\n", mysql_error(conn));
            return 1; // Error
        }

        printf("Client '%s' successfully registered!\n", username);
    }

    printf("User '%s' successfully registered!\n", username);
    return 0; // Success
}

// Function to verify user credentials
int verify_user(MYSQL *conn, const char *email, const char *password) {
    MYSQL_RES *res;
    MYSQL_ROW row;

    // Query to select the user with the provided email and password
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT id FROM users WHERE email='%s' AND password='%s'", email, password);

    // Execute the query
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to execute query: %s\n", mysql_error(conn));
        return 1; // Error
    }

    res = mysql_store_result(conn);

    // Check if a user was found
    if ((row = mysql_fetch_row(res)) != NULL) {
        printf("Login successful! User ID: %s\n", row[0]);
        mysql_free_result(res);
        return 0; // Success
    } else {
        printf("Invalid email or password.\n");
        mysql_free_result(res);
        return 1; // Invalid credentials
    }
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  