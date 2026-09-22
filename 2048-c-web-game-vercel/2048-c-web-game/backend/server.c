/*
 * Minimal HTTP server for the C 2048 game engine.
 *
 * Windows build:
 *   gcc server.c game.c -o 2048_server.exe -lws2_32
 *
 * The server serves:
 *   /                 -> ../frontend/index.html
 *   /style.css        -> ../frontend/style.css
 *   /script.js        -> ../frontend/script.js
 *
 * API:
 *   POST /api/new-game
 *   POST /api/move
 *
 * No game rules are implemented in JavaScript. All game state and rules
 * live in game.c.
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define CLOSE_SOCKET close
#endif

#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 16384
#define BODY_SIZE 8192

static void send_all(socket_t client, const char *data, int length)
{
    int sent = 0;

    while (sent < length) {
        int result = send(client, data + sent, length - sent, 0);

        if (result <= 0) {
            return;
        }

        sent += result;
    }
}

static void send_response(
    socket_t client,
    const char *status_line,
    const char *content_type,
    const char *body
)
{
    char header[1024];
    int body_length = (int)strlen(body);

    int header_length = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-store\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        status_line,
        content_type,
        body_length
    );

    send_all(client, header, header_length);
    send_all(client, body, body_length);
}

static const char *statusToString(GameStatus status)
{
    switch (status) {
        case WON:
            return "WON";
        case GAME_OVER:
            return "GAME_OVER";
        default:
            return "PLAYING";
    }
}

static void build_game_json(char *out, size_t out_size)
{
    int board[SIZE][SIZE];
    int position = 0;

    getBoard(board);

    position += snprintf(
        out + position,
        out_size - (size_t)position,
        "{\"board\":["
    );

    for (int r = 0; r < SIZE; r++) {
        position += snprintf(
            out + position,
            out_size - (size_t)position,
            "["
        );

        for (int c = 0; c < SIZE; c++) {
            position += snprintf(
                out + position,
                out_size - (size_t)position,
                "%d%s",
                board[r][c],
                (c == SIZE - 1) ? "" : ","
            );
        }

        position += snprintf(
            out + position,
            out_size - (size_t)position,
            "]%s",
            (r == SIZE - 1) ? "" : ","
        );
    }

    snprintf(
        out + position,
        out_size - (size_t)position,
        "],\"score\":%d,\"status\":\"%s\"}",
        getScore(),
        statusToString(getStatus())
    );
}

static int extract_direction(const char *body, char *direction, size_t size)
{
    const char *key = "\"direction\"";
    const char *start = strstr(body, key);

    if (!start) {
        return 0;
    }

    start += strlen(key);

    while (*start == ' ' || *start == '\t' || *start == ':') {
        start++;
    }

    if (*start != '"') {
        return 0;
    }

    start++;

    size_t i = 0;

    while (*start && *start != '"' && i + 1 < size) {
        direction[i++] = *start++;
    }

    direction[i] = '\0';

    return *start == '"';
}

static int handle_move(const char *body)
{
    char direction[16];

    if (!extract_direction(body, direction, sizeof(direction))) {
        return 0;
    }

    if (strcmp(direction, "left") == 0) {
        moveLeft();
        return 1;
    }

    if (strcmp(direction, "right") == 0) {
        moveRight();
        return 1;
    }

    if (strcmp(direction, "up") == 0) {
        moveUp();
        return 1;
    }

    if (strcmp(direction, "down") == 0) {
        moveDown();
        return 1;
    }

    return 0;
}

static int load_file(const char *path, char **data, long *length)
{
    FILE *file = fopen(path, "rb");

    if (!file) {
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }

    long size = ftell(file);

    if (size < 0) {
        fclose(file);
        return 0;
    }

    rewind(file);

    char *buffer = (char *)malloc((size_t)size + 1);

    if (!buffer) {
        fclose(file);
        return 0;
    }

    size_t read_count = fread(buffer, 1, (size_t)size, file);
    fclose(file);

    if (read_count != (size_t)size) {
        free(buffer);
        return 0;
    }

    buffer[size] = '\0';

    *data = buffer;
    *length = size;
    return 1;
}

static void serve_static(socket_t client, const char *url)
{
    const char *relative_path = NULL;
    const char *content_type = NULL;

    if (strcmp(url, "/") == 0 || strcmp(url, "/index.html") == 0) {
        relative_path = "../frontend/index.html";
        content_type = "text/html; charset=utf-8";
    } else if (strcmp(url, "/style.css") == 0) {
        relative_path = "../frontend/style.css";
        content_type = "text/css; charset=utf-8";
    } else if (strcmp(url, "/script.js") == 0) {
        relative_path = "../frontend/script.js";
        content_type = "application/javascript; charset=utf-8";
    } else {
        send_response(client, "404 Not Found", "text/plain; charset=utf-8",
                      "Not found");
        return;
    }

    char *file_data = NULL;
    long file_length = 0;

    if (!load_file(relative_path, &file_data, &file_length)) {
        send_response(
            client,
            "500 Internal Server Error",
            "text/plain; charset=utf-8",
            "Could not load frontend file. Run the server from the backend directory."
        );
        return;
    }

    char header[1024];

    int header_length = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-store\r\n"
        "\r\n",
        content_type,
        file_length
    );

    send_all(client, header, header_length);
    send_all(client, file_data, (int)file_length);

    free(file_data);
}

static int get_content_length(const char *request)
{
    const char *header = strstr(request, "Content-Length:");

    if (!header) {
        header = strstr(request, "content-length:");
    }

    if (!header) {
        return 0;
    }

    header += strlen("Content-Length:");

    while (*header == ' ') {
        header++;
    }

    return atoi(header);
}

/*
 * A single recv() call is not guaranteed to return the whole request:
 * slow clients, mobile networks, and proxies can split it across many
 * TCP segments. This reads until the header block ("\r\n\r\n") is fully
 * present, then keeps reading until the advertised Content-Length worth
 * of body has also arrived (bounded by the buffer size).
 *
 * Returns 1 with *out_total set to the number of bytes in buffer, or 0 if
 * the connection closed/errored before a full header block was received.
 * buffer is always null-terminated within [0, buffer_capacity).
 */
static int recv_full_request(socket_t client, char *buffer, int buffer_capacity, int *out_total)
{
    int total = 0;
    int header_end = -1;

    /* Phase 1: read until the header block is complete. */
    while (total < buffer_capacity - 1) {
        int n = recv(client, buffer + total, buffer_capacity - 1 - total, 0);

        if (n <= 0) {
            return 0;
        }

        total += n;
        buffer[total] = '\0';

        char *separator = strstr(buffer, "\r\n\r\n");

        if (separator) {
            header_end = (int)(separator - buffer) + 4;
            break;
        }
    }

    if (header_end < 0) {
        /* Headers never completed (client too slow, or header block too large). */
        buffer[total] = '\0';
        *out_total = total;
        return 0;
    }

    int content_length = get_content_length(buffer);

    if (content_length < 0) {
        content_length = 0;
    }

    /*
     * Cap how much body we'll actually try to buffer. If the client
     * claims a body larger than fits, read only up to capacity; the
     * caller still sees the true Content-Length via get_content_length()
     * and can reject the request (e.g. 413) instead of hanging forever
     * trying to fill a buffer that can never hold it all.
     */
    int needed_total = header_end + content_length;

    if (needed_total > buffer_capacity - 1) {
        needed_total = buffer_capacity - 1;
    }

    /* Phase 2: keep reading until the full body (or our cap) has arrived. */
    while (total < needed_total) {
        int n = recv(client, buffer + total, buffer_capacity - 1 - total, 0);

        if (n <= 0) {
            break;
        }

        total += n;
    }

    buffer[total] = '\0';
    *out_total = total;
    return 1;
}

static void handle_client(socket_t client)
{
    char buffer[BUFFER_SIZE + 1];
    int received = 0;

    if (!recv_full_request(client, buffer, sizeof(buffer), &received)) {
        if (received > 0) {
            send_response(
                client,
                "400 Bad Request",
                "text/plain; charset=utf-8",
                "Incomplete request"
            );
        }

        CLOSE_SOCKET(client);
        return;
    }

    char method[16] = {0};
    char path[256] = {0};

    if (sscanf(buffer, "%15s %255s", method, path) != 2) {
        send_response(
            client,
            "400 Bad Request",
            "text/plain; charset=utf-8",
            "Bad request"
        );
        CLOSE_SOCKET(client);
        return;
    }

    if (strcmp(method, "OPTIONS") == 0) {
        send_response(client, "204 No Content", "text/plain", "");
        CLOSE_SOCKET(client);
        return;
    }

    if (strcmp(method, "GET") == 0) {
        serve_static(client, path);
        CLOSE_SOCKET(client);
        return;
    }

    const char *body = strstr(buffer, "\r\n\r\n");
    int content_length = get_content_length(buffer);

    if (!body) {
        send_response(
            client,
            "400 Bad Request",
            "application/json; charset=utf-8",
            "{\"error\":\"Missing HTTP body separator\"}"
        );
        CLOSE_SOCKET(client);
        return;
    }

    body += 4;

    /*
     * For this small local game API, the browser request fits in one recv.
     * Reject unusually large requests rather than using unsafe buffers.
     */
    if (content_length < 0 || content_length > BODY_SIZE) {
        send_response(
            client,
            "413 Payload Too Large",
            "application/json; charset=utf-8",
            "{\"error\":\"Request body too large\"}"
        );
        CLOSE_SOCKET(client);
        return;
    }

    /*
     * recv_full_request() tries to read the whole advertised body, but a
     * client can still disconnect mid-send. Detect that explicitly instead
     * of silently handing truncated JSON to the parser below.
     */
    {
        int body_bytes_received = received - (int)(body - buffer);

        if (body_bytes_received < content_length) {
            send_response(
                client,
                "400 Bad Request",
                "application/json; charset=utf-8",
                "{\"error\":\"Incomplete request body\"}"
            );
            CLOSE_SOCKET(client);
            return;
        }
    }

    if (strcmp(method, "POST") != 0) {
        send_response(
            client,
            "405 Method Not Allowed",
            "application/json; charset=utf-8",
            "{\"error\":\"Method not allowed\"}"
        );
        CLOSE_SOCKET(client);
        return;
    }

    char response[4096];

    if (strcmp(path, "/api/new-game") == 0) {
        resetGame();
        build_game_json(response, sizeof(response));

        send_response(
            client,
            "200 OK",
            "application/json; charset=utf-8",
            response
        );
        CLOSE_SOCKET(client);
        return;
    }

    if (strcmp(path, "/api/move") == 0) {
        if (!handle_move(body)) {
            send_response(
                client,
                "400 Bad Request",
                "application/json; charset=utf-8",
                "{\"error\":\"Direction must be left, right, up, or down\"}"
            );
            CLOSE_SOCKET(client);
            return;
        }

        build_game_json(response, sizeof(response));

        send_response(
            client,
            "200 OK",
            "application/json; charset=utf-8",
            response
        );
        CLOSE_SOCKET(client);
        return;
    }

    send_response(
        client,
        "404 Not Found",
        "application/json; charset=utf-8",
        "{\"error\":\"API route not found\"}"
    );

    CLOSE_SOCKET(client);
}

int main(void)
{
#ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
    }
#else
    /*
     * Without this, writing to a socket after the client has already
     * closed its end raises SIGPIPE, whose default action terminates the
     * whole process. Since this server handles one client at a time in a
     * single process, that turns "a client disconnected early" into
     * "the server is down for everyone." Ignoring SIGPIPE makes send()
     * return -1/EPIPE instead, which send_all() already handles safely.
     */
    signal(SIGPIPE, SIG_IGN);
#endif

    socket_t server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "Could not create socket.\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int reuse = 1;

#ifdef _WIN32
    setsockopt(
        server_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        (const char *)&reuse,
        sizeof(reuse)
    );
#else
    setsockopt(
        server_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    );
#endif

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));

    int port = DEFAULT_PORT;
    const char *port_env = getenv("PORT");

    if (port_env && *port_env) {
        int parsed_port = atoi(port_env);
        if (parsed_port > 0 && parsed_port <= 65535) {
            port = parsed_port;
        }
    }

    address.sin_family = AF_INET;
    /* Vercel/container hosting requires the server to accept external connections. */
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((unsigned short)port);

    if (bind(
        server_socket,
        (struct sockaddr *)&address,
        sizeof(address)
    ) == SOCKET_ERROR) {
        fprintf(stderr, "Could not bind to port %d\n", port);
        CLOSE_SOCKET(server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR) {
        fprintf(stderr, "Could not listen on port %d.\n", port);
        CLOSE_SOCKET(server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    initializeGame();

    printf("2048 C server running on port %d\n", port);
    printf("Press Ctrl+C to stop the server.\n");

    while (1) {
        socket_t client = accept(server_socket, NULL, NULL);

        if (client == INVALID_SOCKET) {
            continue;
        }

        handle_client(client);
    }

    CLOSE_SOCKET(server_socket);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}