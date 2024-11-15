#include <microhttpd.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8888

// Structure to hold POST data
struct connection_info {
    char *post_data;
};

// Function to decode URL-encoded characters (e.g., %40 for @)
void url_decode(const char *src, char *dest) {
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

// Function to generate HTML for freelancers
static int generate_freelance_html(MYSQL *conn, char *response_buffer) {
    MYSQL_RES *res;
    MYSQL_ROW row;

    printf("Generating freelancer HTML...\n");

    if (mysql_query(conn, "SELECT name, age, skills, email FROM freelances")) {
        fprintf(stderr, "Failed to fetch freelancers: %s\n", mysql_error(conn));
        strcat(response_buffer, "<p>Error fetching freelancers data.</p>");
        return 1;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "Failed to store result: %s\n", mysql_error(conn));
        strcat(response_buffer, "<p>Error storing freelancers data.</p>");
        return 1;
    }

    printf("Number of freelancers found: %lu\n", (unsigned long)mysql_num_rows(res));
    strcat(response_buffer, "<h2>Freelancers List</h2><div class='freelancer-list'>");

    if (mysql_num_rows(res) == 0) {
        strcat(response_buffer, "<p>No freelancers available.</p>");
    } else {
        while ((row = mysql_fetch_row(res)) != NULL) {
            printf("Fetched freelancer: \nName: %s\nAge: %s\nSkills: %s\nEmail: %s\n",
                   row[0] ? row[0] : "NULL", row[1] ? row[1] : "NULL",
                   row[2] ? row[2] : "NULL", row[3] ? row[3] : "NULL");

            char freelancer_entry[512];
            snprintf(freelancer_entry, sizeof(freelancer_entry),
                     "<div class='freelancer'><h3>%s</h3><p>Age: %s</p><p>Skills: %s</p><p>Email: %s</p></div>",
                     row[0] ? row[0] : "Unknown", row[1] ? row[1] : "Not provided",
                     row[2] ? row[2] : "Not provided", row[3] ? row[3] : "Not provided");
            strcat(response_buffer, freelancer_entry);
        }
    }
    strcat(response_buffer, "</div>");
    mysql_free_result(res);
    return 0;
}

// Function to generate HTML for clients with detailed logs
static int generate_client_html(MYSQL *conn, char *response_buffer) {
    MYSQL_RES *res;
    MYSQL_ROW row;

    printf("Generating client HTML...\n");

    if (mysql_query(conn, "SELECT name, email FROM clients")) {
        fprintf(stderr, "Failed to fetch clients: %s\n", mysql_error(conn));
        strcat(response_buffer, "<p>Error fetching clients data.</p>");
        return 1;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "Failed to store result: %s\n", mysql_error(conn));
        strcat(response_buffer, "<p>Error storing clients data.</p>");
        return 1;
    }

    printf("Number of clients found: %lu\n", (unsigned long)mysql_num_rows(res));
    strcat(response_buffer, "<h2>Clients List</h2><div class='client-list'>");

    if (mysql_num_rows(res) == 0) {
        strcat(response_buffer, "<p>No clients available.</p>");
    } else {
        while ((row = mysql_fetch_row(res)) != NULL) {
            printf("Fetched client: \nName: %s\nEmail: %s\n",
                   row[0] ? row[0] : "NULL", row[1] ? row[1] : "NULL");

            char client_entry[512];
            snprintf(client_entry, sizeof(client_entry),
                     "<div class='client'><h3>%s</h3><p>Email: %s</p></div>",
                     row[0] ? row[0] : "Unknown", row[1] ? row[1] : "Not provided");
            strcat(response_buffer, client_entry);
        }
    }
    strcat(response_buffer, "</div>");
    mysql_free_result(res);
    return 0;
}

// Handle HTTP requests
static enum MHD_Result answer_to_connection(void *cls, struct MHD_Connection *connection,
                                            const char *url, const char *method,
                                            const char *version, const char *upload_data,
                                            size_t *upload_data_size, void **con_cls) {
    MYSQL *conn = (MYSQL *)cls;
    if (*con_cls == NULL) {
        struct connection_info *con_info = malloc(sizeof(struct connection_info));
        con_info->post_data = NULL;
        *con_cls = con_info;
        return MHD_YES;
    }
    struct connection_info *con_info = *con_cls;

    printf("Received request: URL=%s, METHOD=%s\n", url, method);

    if (strcmp(method, "POST") == 0) {
        if (*upload_data_size != 0) {
            con_info->post_data = realloc(con_info->post_data, *upload_data_size + 1);
            memcpy(con_info->post_data, upload_data, *upload_data_size);
            con_info->post_data[*upload_data_size] = '\0';
            *upload_data_size = 0;
            printf("Received POST data: %s\n", con_info->post_data);
            return MHD_YES;
        }

        // Registration logic for both clients and freelancers
        if (strcmp(url, "/register") == 0) {
            char username[256], email[256], password[256], role[256] = "", age[256] = "", skills[256] = "";

            // Parsing fields carefully, considering role and skills conditional requirements
            sscanf(con_info->post_data, "username=%255[^&]&email=%255[^&]&password=%255[^&]&role=%255[^&]&age=%255[^&]&skills=%255[^&]",
                   username, email, password, role, age, skills);

            // Decode URL-encoded fields
            url_decode(username, username);
            url_decode(email, email);
            url_decode(password, password);
            url_decode(role, role);
            url_decode(age, age);

            if (strcmp(role, "freelance") == 0) {
                url_decode(skills, skills); // Decode skills only for freelancers
            }

            printf("Registration Data:\nUsername: %s\nEmail: %s\nPassword: %s\nRole: %s\nAge: %s\nSkills: %s\n",
                   username, email, password, role, age, skills);

            // Insert into the users table
            char query[2048];
            snprintf(query, sizeof(query), "INSERT INTO users (username, email, password, role) VALUES ('%s', '%s', '%s', '%s')",
                     username, email, password, role);

            if (mysql_query(conn, query)) {
                fprintf(stderr, "User insertion failed: %s\n", mysql_error(conn));
            } else {
                int user_id = mysql_insert_id(conn);
                if (strcmp(role, "freelance") == 0) {
                    snprintf(query, sizeof(query), "INSERT INTO freelances (user_id, name, age, skills, email) VALUES (%d, '%s', %s, '%s', '%s')",
                             user_id, username, age, skills, email);
                } else if (strcmp(role, "client") == 0) {
                    snprintf(query, sizeof(query), "INSERT INTO clients (user_id, name, email, password, age) VALUES (%d, '%s', '%s', '%s', %s)",
                             user_id, username, email, password, age);
                }
                if (mysql_query(conn, query)) {
                    fprintf(stderr, "Role-specific insertion failed: %s\n", mysql_error(conn));
                }
            }

            const char *success_message = "Registration successful! Redirecting to login...";
            struct MHD_Response *response = MHD_create_response_from_buffer(
                strlen(success_message), (void *)success_message,
                               MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return MHD_YES;
        }
    }

    // Handle GET requests for freelancers
    if (strcmp(url, "/freelancers") == 0 && strcmp(method, "GET") == 0) {
        char response_buffer[4096] = {0};
        printf("Generating freelancers data...\n");
        if (generate_freelance_html(conn, response_buffer) != 0) {
            struct MHD_Response *response = MHD_create_response_from_buffer(
                strlen("Failed to fetch freelancers"), (void *)"Failed to fetch freelancers", MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            return ret;
        }
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_buffer), (void *)response_buffer, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Handle GET requests for clients with additional logs
    if (strcmp(url, "/clients") == 0 && strcmp(method, "GET") == 0) {
        char response_buffer[4096] = {0};
        printf("Generating clients data...\n");
        if (generate_client_html(conn, response_buffer) != 0) {
            struct MHD_Response *response = MHD_create_response_from_buffer(
                strlen("Failed to fetch clients"), (void *)"Failed to fetch clients", MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            return ret;
        }
        printf("Generated HTML Response for clients: \n%s\n", response_buffer);
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_buffer), (void *)response_buffer, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    const char *error_message = "404 Not Found";
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(error_message), (void *)error_message, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

int main() {
    MYSQL *conn = mysql_init(NULL);
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

    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &answer_to_connection, conn, MHD_OPTION_END);
    if (daemon == NULL) {
        mysql_close(conn);
        return 1;
    }

    printf("HTTP server running on port %d...\n", PORT);
    getchar();
    MHD_stop_daemon(daemon);
    mysql_close(conn);
    return 0;
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
