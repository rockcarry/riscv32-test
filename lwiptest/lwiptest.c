#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "netif/ethernet.h"

extern err_t ethernetif_init(struct netif *netif);
extern void  ethernetif_exit(struct netif *netif);

#define SOCKET int

#define FFHTTPD_SERVER_PORT      80
#define FFHTTPD_MAX_CONNECTIONS  100
#define FFHTTPD_MAX_WORK_THREADS 8

static char *g_ffhttpd_head1 =
"HTTP/1.1 %s\r\n"
"Server: ffhttpd/1.0.0\r\n"
"Accept-Ranges: bytes\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\r\n"
"Connection: close\r\n\r\n";

static char *g_404_page =
"<html>\r\n"
"<head><title>404 Not Found</title></head>\r\n"
"<body>\r\n"
"<center><h1>404 Not Found</h1></center>\r\n"
"<hr><center>ffhttpd/1.0.0</center>\r\n"
"</body>\r\n"
"</html>\r\n";

static char *g_ffhttpd_head2 =
"HTTP/1.1 206 Partial Content\r\n"
"Server: ffhttpd/1.0.0\r\n"
"Content-Range: bytes %d-%d/%d\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\r\n"
"Connection: close\r\n\r\n";

static char *g_content_type_list[][2] = {
    { ".asf" , "video/x-ms-asf"                 },
    { ".avi" , "video/avi"                      },
    { ".bmp" , "application/x-bmp"              },
    { ".css" , "text/css"                       },
    { ".exe" , "application/x-msdownload"       },
    { ".gif" , "image/gif"                      },
    { ".htm" , "text/html"                      },
    { ".html", "text/html"                      },
    { ".ico" , "image/x-icon"                   },
    { ".jpeg", "image/jpeg"                     },
    { ".jpg" , "image/jpeg"                     },
    { ".mp3" , "audio/mp3"                      },
    { ".mp4" , "video/mp4"                      },
    { ".pdf" , "application/pdf"                },
    { ".png" , "image/png"                      },
    { ".ppt" , "application/x-ppt"              },
    { ".swf" , "application/x-shockwave-flash"  },
    { ".tif" , "image/tiff"                     },
    { ".tiff", "image/tiff"                     },
    { ".txt" , "text/plain"                     },
    { ".wav" , "audio/wav"                      },
    { ".wma" , "audio/x-ms-wma"                 },
    { ".wmv" , "video/x-ms-wmv"                 },
    { ".xml" , "text/xml"                       },
    { ".c"   , "text/plain"                     },
    { NULL },
};

static int stricmp(char *str1, char *str2)
{
    int ch1, ch2;
    do {
        ch1 = *str1++, ch2 = *str2++;
        if (ch1 >= 'A' && ch1 <= 'Z') ch1 += 0x20;
        if (ch2 >= 'A' && ch2 <= 'Z') ch2 += 0x20;
    } while (ch1 && ch1 == ch2);
    return ch1 - ch2;
}

static char* get_content_type(char *file)
{
    int   i;
    char *ext = file + strlen(file);
    while (ext > file && *ext != '.') ext--;
    if (ext != file) {
        for (i = 0; g_content_type_list[i][0]; i++) {
            if (stricmp(g_content_type_list[i][0], ext) == 0) {
                return g_content_type_list[i][1];
            }
        }
    }
    return "application/octet-stream";
}

static void get_file_range_size(char *file, int *start, int *end, int *size)
{
    char path[256];
    snprintf(path, sizeof(path), "/disk/%s", file);
    FILE *fp = fopen(path, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        *size = ftell(fp);
        fclose(fp);
        if (*start < 0) *start = *size + *start;
        if (*end >= *size) *end = *size - 1;
    } else {
        *start = *end = 0;
        *size  = -1;
    }
}

static void send_file_data(SOCKET fd, char *file, int start, int end)
{
    char path[256];
    snprintf(path, sizeof(path), "/disk/%s", file);
    FILE *fp = fopen(path, "rb");
    if (fp) {
        char buf[1024];
        int  len = end - start + 1, ret = 0, n;
        fseek(fp, start, SEEK_SET);
        do {
            n = len < sizeof(buf) ? len : sizeof(buf);
            n = (int)fread(buf, 1, n, fp);
            len -= n > 0 ? n : 0;
            while (n > 0) {
                ret = send(fd, buf, n, 0);
                if (ret == 0 || (ret < 0 && errno != EWOULDBLOCK && errno != EINTR)) goto done;
                n  -= ret > 0 ? ret : 0;
            }
        } while (len > 0 && !feof(fp));
done:   fclose(fp);
    }
}

static void parse_range_datasize(char *str, int *partial, int *start, int *end, int *size)
{
    char *range_start, *range_end, *temp;
    *start = 0;
    *end   = 0x7FFFFFFF;
    *size  = 0;
    if (!str) return;
    range_start = strstr(str, "range");
    if (range_start && (range_end = strstr(range_start, "\r\n"))) {
        if (strstr(range_start, ":") && strstr(range_start, "bytes") && (range_start = strstr(range_start, "="))) {
            range_start += 1;
            *start = atoi(range_start);
            if (*start < 0) {
                range_start  = strstr(range_start, "-");
                range_start += 1;
            }
            range_start = strstr(range_start, "-");
            if (range_start && range_start + 1 < range_end) {
                range_start += 1;
                *end = atoi(range_start);
            }
        }
    }
    temp = strstr(str, "content-length");
    if (temp) {
        temp += 14;
        temp  = strstr(temp, ":");
        if (temp) *size = atoi(temp+1);
    }
    *partial = !!range_start;
}

typedef struct {
    int    head;
    int    tail;
    int    size; // size == -1 means exit
    SOCKET conns[FFHTTPD_MAX_CONNECTIONS];
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    pthread_t       threads[FFHTTPD_MAX_WORK_THREADS];
} THEADPOOL;

static SOCKET threadpool_dequeue(THEADPOOL *tp)
{
    SOCKET fd = -1;
    pthread_mutex_lock(&tp->mutex);
    while (tp->size == 0) pthread_cond_wait(&tp->cond, &tp->mutex);
    if (tp->size != -1) {
        fd = tp->conns[tp->head++ % FFHTTPD_MAX_CONNECTIONS];
        tp->size--;
        pthread_cond_signal(&tp->cond);
    }
    pthread_mutex_unlock(&tp->mutex);
    return fd;
}

static void threadpool_enqueue(THEADPOOL *tp, SOCKET fd)
{
    pthread_mutex_lock(&tp->mutex);
    while (tp->size == FFHTTPD_MAX_CONNECTIONS) pthread_cond_wait(&tp->cond, &tp->mutex);
    if (tp->size != -1) {
        tp->conns[tp->tail++ % FFHTTPD_MAX_CONNECTIONS] = fd;
        tp->size++;
        pthread_cond_signal(&tp->cond);
    }
    pthread_mutex_unlock(&tp->mutex);
}

static void* handle_http_request(void *argv)
{
    THEADPOOL *tp = (THEADPOOL*)argv;
    int    length, range_start, range_end, range_size, partial = 0;
    char   recvbuf[1024], sendbuf[1024];
    char  *request_type = NULL, *request_path = NULL, *url_args = NULL, *request_head = NULL;
    SOCKET conn_fd;

    while ((conn_fd = threadpool_dequeue(tp)) != -1) {
        length = recv(conn_fd, recvbuf, sizeof(recvbuf)-1, 0);
        if (length <= 0) goto _next;
        recvbuf[length] = '\0';
        printf("%s\n", recvbuf);

        request_head = strstr(recvbuf, "\r\n");
        if (request_head) {
            request_head[0] = 0;
            request_head   += 2;
            strlwr(request_head);
        }
        request_type = recvbuf;
        request_path = strstr(recvbuf, " ");
        if (request_path) {
           *request_path++ = 0;
            request_path   = strstr(request_path, "/");
        }
        if (request_path) {
            request_path  += 1;
            url_args = strstr(request_path, " ");
            if (url_args) *url_args   = 0;
            url_args = strstr(request_path, "?");
            if (url_args) *url_args++ = 0;
            if (!request_path[0]) request_path = "index.html";
        }

        parse_range_datasize(request_head, &partial, &range_start, &range_end, &range_size);
        printf("request_type: %s, request_path: %s, url_args: %s, range_size: %d\n", request_type, request_path, url_args, range_size);

        get_file_range_size(request_path, &range_start, &range_end, &range_size);
        if (range_size == -1) {
            length = snprintf(sendbuf, sizeof(sendbuf), g_ffhttpd_head1, "404 Not Found", "text/html", strlen(g_404_page));
            printf("%s", sendbuf);
            send(conn_fd, sendbuf, length, 0);
            send(conn_fd, g_404_page, (int)strlen(g_404_page), 0);
        } else if (strcmp(request_type, "GET") == 0 || strcmp(request_type, "HEAD") == 0) {
            if (!partial) {
                length = snprintf(sendbuf, sizeof(sendbuf), g_ffhttpd_head1, "200 OK", get_content_type(request_path), range_size);
            } else {
                length = snprintf(sendbuf, sizeof(sendbuf), g_ffhttpd_head2, range_start, range_end, range_size, get_content_type(request_path), range_size ? range_end - range_start + 1 : 0);
            }
            printf("%s", sendbuf);
            send(conn_fd, sendbuf, length, 0);
            if (strcmp(request_type, "GET") == 0) {
                send_file_data(conn_fd, request_path, range_start, range_end);
            }
        }

_next:  closesocket(conn_fd);
    }
    return NULL;
}

static void threadpool_init(THEADPOOL *tp)
{
    int i;
    memset(tp, 0, sizeof(THEADPOOL));
    pthread_mutex_init(&tp->mutex, NULL);
    pthread_cond_init (&tp->cond , NULL);
    for (i = 0; i < FFHTTPD_MAX_WORK_THREADS; i++) pthread_create(&tp->threads[i], NULL, handle_http_request, tp);
}

static void threadpool_free(THEADPOOL *tp)
{
    int i;
    pthread_mutex_lock(&tp->mutex);
    tp->size = -1;
    pthread_cond_broadcast(&tp->cond);
    pthread_mutex_unlock(&tp->mutex);
    for (i = 0; i < FFHTTPD_MAX_WORK_THREADS; i++) pthread_join(tp->threads[i], NULL);
    pthread_mutex_destroy(&tp->mutex);
    pthread_cond_destroy (&tp->cond );
}

static int g_exit_server = 0;

int main(void)
{
    libpthread_init();
    struct ip4_addr ip, mask, gw;
    struct netif netif;
    struct sockaddr_in server_addr, client_addr;
    SOCKET server_fd = 0, conn_fd = 0;
    int addrlen = sizeof(client_addr);
    THEADPOOL thread_pool;

    tcpip_init(NULL, NULL);

    IP4_ADDR(&ip  , 192, 168, 137, 11);
    IP4_ADDR(&mask, 255, 255, 255, 0 );
    IP4_ADDR(&gw  , 192, 168, 137, 1 );

    netif_add(&netif, &ip, &mask, &gw, NULL, ethernetif_init, tcpip_input);
    netif_set_default(&netif);
    netif_set_up(&netif);

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(FFHTTPD_SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("failed to open socket !\n");
        goto done;
    }

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("failed to bind !\n");
        goto done;
    }

    if (listen(server_fd, FFHTTPD_MAX_CONNECTIONS) == -1) {
        printf("failed to listen !\n");
        goto done;
    }

    threadpool_init(&thread_pool);
    while (!g_exit_server) {
        conn_fd = accept(server_fd, (struct sockaddr*)&client_addr, (void*)&addrlen);
        if (conn_fd != -1) threadpool_enqueue(&thread_pool, conn_fd);
        else printf("failed to accept !\n");
    }
    threadpool_free(&thread_pool);

done:
    if (server_fd > 0) closesocket(server_fd);
    ethernetif_exit(&netif);
    tcpip_exit();
    libpthread_exit();
    return 0;
}
