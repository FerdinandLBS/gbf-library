#include "enet_types.h"
#include "enet_sock.h"
#include "enet_sys.h"

#include <stdio.h>
#include <stdlib.h>
#include <lbs_rwlock.h>
//#include <zlib.h>

/* opt parameter in enet_http_set */
#define ENET_HTTP_SET_HEADER 0
#define ENET_HTTP_SET_URL    1
#define ENET_HTTP_SET_BODY   2

/* opt parameter in enet_http_fetch() */
#define ENET_HTTP_GET_RAW    0
#define ENET_HTTP_GET_HEADER 1
#define ENET_HTTP_GET_CODE   2
#define ENET_HTTP_GET_STATUS 3
#define ENET_HTTP_GET_BODY   4
#define ENET_HTTP_GET_VERION 5
#define ENET_HTTP_GET_REQUEST 6

/* method parameter in enet_http_exec() */
#define ENET_HTTP_GET       0
#define ENET_HTTP_POST      1
#define ENET_HTTP_PUT       2
#define ENET_HTTP_DELETE    3
#define ENET_HTTP_RAW       -1

typedef struct __enet_http_header_st {
    char* key;
    char* content;

    struct __enet_http_header_st* prev;
    struct __enet_http_header_st* next;
}enet_http_header_t;

typedef struct __enet_http_response_st {
    char version[10];
    char status_str[10];
    int status;
    char res_des[64];
    enet_http_header_t* headers;
    void* body;
    int body_size;
}enet_http_response_t;

typedef struct __enet_http_st {
    int method;
    int addr;
    int port;
    int encoding_type;
    enet_http_header_t* headers;
    char* url;
    char* host;
    char* uri;
    char* encoding;
    void* body;
    int body_size;
    char* response;
    int res_size;
    enet_http_response_t* parsed_res;
}enet_http_obj;

typedef struct __enet_http_handle_st {
    int handle;
    enet_http_obj* obj;

    struct __enet_http_handle_st* prev;
    struct __enet_http_handle_st* next;
}enet_http_handle_t;

lbs_lock_t* g_http_handle_lock = 0;
enet_http_handle_t* g_http_handles = 0;

unsigned enet_strlen(const char* str)
{
    const char *eos = str;

    while( *eos++ ) ;

    return (eos - str - 1);
}

__inline void* enet_duplicate_mem(const void* in, unsigned size)
{
    void* out;

    out = malloc(size);
    if (out) {
        memcpy(out, in, size);
    }

    return out;
}

__inline void* enet_duplicate_str(const char* in)
{
    return enet_duplicate_mem(in, enet_strlen(in)+1);
}

lbs_status_t enet_http_init()
{
    if (g_http_handle_lock != 0)
        return LBS_CODE_OK;

    return lbs_rwlock_init(&g_http_handle_lock);   
}

void enet_http_lock()
{
    lbs_rwlock_wlock(g_http_handle_lock);
}

void enet_http_unlock()
{
    lbs_rwlock_unlock(g_http_handle_lock);
}

/* Convert string to IP */
int ENET_CALLCONV enet_name_to_ip(const char* name)
{
    int ip = 0;
    struct hostent* host;
    int adapter = 0;         

    /* as function provider, we check params strictly */
    if (name == 0 || name[0] == 0)
        return 0;

    enet_socket_init();

    host = gethostbyname(name);
    if (!host)
        return 0;

    while(host->h_addr_list[adapter]) {
        if (host->h_length > sizeof(int)) {
            continue;
        }
        // TODO: use internal function
        memcpy(&ip, host->h_addr_list[adapter], host->h_length);
        adapter++; 
    } 

    return ip;
}

/* Stream of HTTP content generator 
 *
 * seg_size is disabled if the input segment is a string.
 * Otherwise segment will be copied as binary
 */
static lbs_status_t insert_segment(char** info, unsigned* buff_size, unsigned* current_size, const char* segment, int seg_size)
{
    unsigned inc;
    unsigned req_size;

    if (segment == 0) return LBS_CODE_OK;

    if (seg_size)
        inc = seg_size;
    else
        inc = strlen(segment);
    if (inc == 0) return LBS_CODE_OK;

    req_size = inc + *current_size + 1;

    /* realloc new buffer if overflow */
    if (req_size >= *buff_size) {
        char* temp = realloc(*info, *buff_size + req_size);
        if (temp == 0)
            return LBS_CODE_NO_MEMORY;

        (*buff_size) += req_size;
        *info = temp;
    }

    if (seg_size)
        memcpy(*info+*current_size, segment, seg_size);
    else
        strcat(*info, segment);
    *current_size += inc;

    return LBS_CODE_OK;
}

/* [LEGACY] construct HTTP stream for easy HTTP */
static lbs_status_t construct_http_message(char** info, unsigned* size, const char* url, const char* param, const char* content, const char* method)
{
    unsigned buff_size = 1024; // current size
    lbs_status_t rc = LBS_CODE_OK;
    char* uri_head;

    /* Init with a fixed size */
    *info = (char*)malloc(buff_size);
    if (*info == 0)
        return LBS_CODE_NO_MEMORY;
    *size = 0;
    *info[0] = 0;

    /* copy method */
    rc = insert_segment(info, &buff_size, size, method, 0);
    if (rc) goto bail;

    if (url == 0 || (url[0] != '/' && url[0] != '\\'))
        uri_head = " /";
    else
        uri_head = " ";
    rc = insert_segment(info, &buff_size, size, uri_head, 0);
    if (rc) goto bail;

    /* copy url */
    rc = insert_segment(info, &buff_size, size, url, 0);
    if (rc) goto bail;

    rc = insert_segment(info, &buff_size, size, " HTTP/1.0\r\n", 0);
    if (rc) goto bail;

    if (content) {
        char content_len[128];
        unsigned length = enet_strlen(content);

        rc = insert_segment(info, &buff_size, size, "Content-Type: text/plain\r\n", 0);
        if (rc) goto bail;

        sprintf(content_len, "Content-Lenght: %d\r\n\r\n", length);
        rc = insert_segment(info, &buff_size, size, content_len, 0);
        if (rc) goto bail;

        rc = insert_segment(info, &buff_size, size, content, 0);
        if (rc) goto bail;
    } else {
        rc = insert_segment(info, &buff_size, size, "\r\n", 0);
        if (rc) goto bail;
    }

bail:
    return rc;
}

/* Send data through a specified socket and get response 
 * from server.
 * currently we have a fixed 5 seconds limitation of server
 * responsing. 
 */
static lbs_status_t send_http_message(char** response, unsigned* size, const char* message, unsigned in_size, int fd)
{
    FD_SET read_set;
    struct timeval tv = {5, 0};
    int st;
    unsigned read_size = 0;
    char* p;

    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);

    /* todo: add itoration here to ensure every byte is sent */
    st = send(fd, message, in_size, 0);
    if (st < 0) {
        return enet_error_code_convert(enet_get_sys_error());
    }

    *size = 1024;
    *response = (char*)malloc(*size);
    if (*response == 0)
        return LBS_CODE_NO_MEMORY;

    *response[0] = 0;
    p = *response;

    /* read response from server */
    while(1) {
        st = select(0, &read_set, 0, 0, &tv);
        if (st == 0) {
            /* socket may be closed we should stop here */
            *(*response+read_size) = 0; *size = read_size;
            return 0;
        }
        if (st < 0) {
            /* error occured, escape */
            free(*response);
            *size = 0;
            return enet_error_code_convert(enet_get_sys_error());
        }

        /* receive data */
        st = recv(fd, p, *size-read_size, 0);
        if (st == 0) {
            *(*response+read_size) = 0; *size = read_size;
            return 0;
        }
        if (st < 0) {
            if (read_size > 0) {
                *(*response+read_size) = 0; *size = read_size;
                return 0;
            }
            free(*response);
            *size = 0;
            return enet_error_code_convert(enet_get_sys_error());
        }

        /* append data to stream and expand if overflow */
        read_size += st;
        if (read_size == *size) {
            *size += 1024;
            *response = (char*)realloc(*response, *size);
            if (*response == 0) {
                return LBS_CODE_NO_MEMORY;
            }
            p = *response + read_size;
            tv.tv_sec = 0;
            tv.tv_usec = 1;
            continue;
        }
        /* pointer moves forward */
        p = *response + read_size;
    }

    return LBS_CODE_OK;
}

/* simple hash */
unsigned hash_int_shift(unsigned key) 
{ 
    key = ~key + (key << 15); // key = (key << 15) - key - 1; 
    key = key ^ (key >> 12); 
    key = key + (key << 2); 
    key = key ^ (key >> 4); 
    key = key * 2057; // key = (key + (key << 3)) + (key << 11); 
    key = key ^ (key >> 16); 
    return key; 
}

int enet_http_random_handle()
{
    return (int)hash_int_shift(GetTickCount());
}

void enet_http_destruct_headers(enet_http_header_t* head)
{
    enet_http_header_t* node = head;
    enet_http_header_t* temp;

    while (node) {
        temp = node->next;
        if (node->key) free(node->key);
        if (node->content) free(node->content);
        free(node);
        node = temp;
    }
}

void enet_http_destruct_response(enet_http_response_t* obj)
{
    if (obj->headers) {
        enet_http_destruct_headers(obj->headers);
    }
    if (obj->body)
        free(obj->body);

    free(obj);
}

void enet_http_destruct_obj(enet_http_obj* obj)
{
    if (obj->body)
        free(obj->body);
    if (obj->encoding)
        free(obj->encoding);
    if (obj->uri)
        free(obj->uri);
    if (obj->url)
        free(obj->url);
    if (obj->response)
        free(obj->response);
    if (obj->host)
        free(obj->host);
    if (obj->parsed_res)
        enet_http_destruct_response(obj->parsed_res);

    if (obj->headers) {
        enet_http_destruct_headers(obj->headers);
    }
}

/* down shift */
void lowercase(char* str, unsigned len)
{
    while (len--)
        if (str[len] >= 'A' && str[len] <= 'Z') {
            str[len] += 32;
        }
}

lbs_status_t enet_http_set_body(void* data, int size, enet_http_obj* obj)
{
    if (obj->body)
        free(obj->body);

    obj->body = malloc(size+2);
    if (obj->body == 0) {
        return LBS_CODE_NO_MEMORY;
    }

    obj->body_size = size+2;

    /* TODO: use internal function */
    memcpy(obj->body, data, size);
    ((char*)obj->body)[size] = '\r';
    ((char*)obj->body)[size+1] = '\n';
    return LBS_CODE_OK;
}

enet_http_header_t* enet_http_find_header(const char* key, enet_http_header_t* head)
{
    enet_http_header_t* node = head;

    while (node) {
        // TODO: use internal function
        if (!strcmp(node->key, key))
            break;

        node = node->next;
    }

    return node;
}

lbs_status_t enet_http_insert_header(const char* key, const char* value, enet_http_header_t** phead)
{
    lbs_status_t rc = LBS_CODE_OK;
    enet_http_header_t* root, *tail;
    enet_http_header_t* header = (enet_http_header_t*)malloc(sizeof(enet_http_header_t));

    if (header == 0)
        return LBS_CODE_NO_MEMORY;

    header->content = (char*)malloc(strlen(value)+1);
    header->key = (char*)malloc(strlen(key)+1);
    if (!header->content || !header->key) {
        rc = LBS_CODE_NO_MEMORY;
        goto bail;
    }

    // todo: use internal function
    strcpy(header->content, value);
    strcpy(header->key, key);
    lowercase(header->key, strlen(header->key));

    root = *phead;
    while (root) {
        /* Same key found. Update it */
        if (!strcmp(root->key, header->key)) {
            free(root->content);
            root->content = header->content;
            free(header->key);
            free(header);
            return LBS_CODE_OK;
        }
        if (!root->next) tail = root;
        root = root->next;
    }

    /* No same key found. Insert tail */
    if (*phead == 0) {
        *phead = header; header->next = 0;
    } else {
        tail->next = header;
        header->prev = tail;
        header->next = 0;
    }

bail:
    if (rc != LBS_CODE_OK) {
        free(header->content);
        free(header->key);
        free(header);
    }
    return rc;
}

/* update url information of HTTP object */
lbs_status_t enet_http_load_url(const char* url, enet_http_obj* obj)
{
    char* p;
    char www[512] = "";
    char port[32] = "";
    // todo: use internal function
    unsigned len = strlen(url);
    int i, j;

    if (obj->url) free(obj->url);
    if (obj->uri) free(obj->uri);

    obj->url = (char*)malloc(len+1);
    obj->uri = (char*)malloc(len+1);
    /* we could ignore the pointer. next time this function
    *  will clean it automaticly
    */
    if (obj->url == 0 || obj->uri == 0) {
        return LBS_CODE_NO_MEMORY;
    }

    // todo: use internal function
    strcpy(obj->url, url);
    lowercase(obj->url, len);
    p = obj->url;
    // escape http/https header if exists
    if (p[0] == 'h' && p[1] == 't' && p[2] == 't' && p[3] == 'p') {
        p+=4;
    }

    // escape all un-necessary chars
    i = 0;
    while (p[i] == ':' || p[i] == '/')
        i++;

    // copy 
    j = 0;
    while (p[i] != 0 && p[i] != '/' && p[i] != ':') {
        www[j] = p[i];
        j++; i++;
    }
    if (p[i] == ':') {
        i++; j = 0;
        while (p[i] != 0 && p[i] != '/') {
            port[j] = p[i];
            i++; j++;
        }
        obj->port = atol(port);
    } else {
        obj->port = 80;
    }

    obj->host = (char*)malloc(strlen(www)+2+strlen(port));
    if (obj->host == 0) {
        free(obj->url); obj->url = 0;
        free(obj->uri); obj->uri = 0;
        return LBS_CODE_NO_MEMORY;
    }
    // todo: use internal functions
    strcpy(obj->uri, &p[i]);
    strcpy(obj->host, www);
    strcat(obj->host, ":");
    strcat(obj->host, port);
    obj->addr = enet_name_to_ip(www);

    return LBS_CODE_OK;
}

void enet_http_destruct(enet_http_handle_t* obj)
{
    if (obj->obj) {
        enet_http_destruct_obj(obj->obj);
    }
    free(obj);
}

void enet_http_handle_delete(enet_http_handle_t* obj)
{
    enet_http_handle_t* prev = 0;
    enet_http_handle_t* next = 0;

    enet_http_lock();

    prev = obj->prev;
    next = obj->next;

    if (prev) {
        prev->next = next;
    } else {
        g_http_handles = next;
    }
    if (next) {
        next->prev = prev;
    }

    enet_http_unlock();
    enet_http_destruct(obj);
}

lbs_status_t enet_http_cpy_session(
                                   char* des,
                                   int des_len,
                                   const char* src,
                                   int src_len,
                                   int* offset,
                                   char needle
                                   )
{
    int j = 0;

    while (j < des_len && src[*offset] != needle) {
        char c = src[*offset];
        if (isprint(c))
            des[j] = src[(*offset)];
        else if (c != '\r' && c != '\n')
            return LBS_CODE_DATA_CORRUPT;

        j++; (*offset)++;
    }

    if (j == des_len && src[*offset] != needle) {
        while ((*offset) < src_len && src[*offset] == needle) (*offset)++;
    }
    (*offset)++;

    return LBS_CODE_OK;
}

/* parse the header part of HTTP server response.
* Always input the string start at the headers.
*/
lbs_status_t enet_http_parse_response_header(enet_http_response_t* http, const char* data)
{
    int i = 0, j = 0;
    int is_escape = 0;
    int is_operating_key = 1;
    int content_length = 0;
    lbs_status_t rc;
    char key[128];
    char content[128];

    while (1) {
        switch (data[i]) {
        case '\r':
            break;
        case '\n':
            if (is_escape)
                goto next;
            is_escape = 1;
            is_operating_key = 1;
            content[j] = 0;
            j = 0;
            rc = enet_http_insert_header(key, content, &http->headers);
            if (rc != LBS_CODE_OK)
                return rc;

            if (!strcmp(key, "Content-Length")) {
                content_length = atol(content);
            }
            break;
        case ' ':
            if (j != 0) {
                goto copy;
            }
        case ':':
            if (is_operating_key) {
                is_operating_key = 0;
                key[j] = 0;
                j = 0;
            }
            break;
        default:
copy:
            is_escape = 0;
            if (is_operating_key) 
                key[j] = data[i];
            else
                content[j] = data[i];

            j++;
        }
        i++;
    }
next:
    // offset fixing
    i++;

    http->body = malloc(content_length+1);
    if (http->body == 0) {
        return LBS_CODE_NO_MEMORY;
    }
    memcpy(http->body, &data[i], content_length);
    ((char*)http->body)[content_length] = 0;
    http->body_size = content_length;

    return LBS_CODE_OK;
}

/*
* Parse the response raw data from HTTP server.
* This operation will prepare data for enet_http_fetch()
*/
lbs_status_t enet_http_parse_response(enet_http_obj* http)
{
    lbs_status_t rc = LBS_CODE_OK;
    int i = 0;
    int j = 0;

    if (http->parsed_res) {
        enet_http_destruct_response(http->parsed_res);
    }
    http->parsed_res = (enet_http_response_t*)calloc(1, sizeof(enet_http_response_t));
    if (http->parsed_res == 0) {
        return LBS_CODE_NO_MEMORY;
    }
    rc = enet_http_cpy_session(http->parsed_res->version, 9, http->response, http->res_size, &i, ' ');
    if (rc != LBS_CODE_OK)
        goto bail;

    rc = enet_http_cpy_session(http->parsed_res->status_str, 9, http->response, http->res_size, &i, ' ');
    if (rc != LBS_CODE_OK)
        goto bail;
    http->parsed_res->status = atol(http->parsed_res->status_str);

    rc = enet_http_cpy_session(http->parsed_res->res_des, 63, http->response, http->res_size, &i, '\n');
    if (rc != LBS_CODE_OK)
        goto bail;

    rc = enet_http_parse_response_header(http->parsed_res, &http->response[i]);
    if (rc != LBS_CODE_OK)
        goto bail;

bail:
    if (rc != LBS_CODE_OK) {
        enet_http_destruct_response(http->parsed_res);
        http->parsed_res = 0;
    }
    return rc;
}

void enet_http_handle_insert(enet_http_handle_t* obj)
{
    enet_http_handle_t* node = 0;
    enet_http_handle_t* next;

    node = g_http_handles;
    if (node == 0) {
        g_http_handles = obj;
        obj->prev = 0;
        obj->next = 0;
        return;
    }

    next = node->next;
    node->next = obj;
    obj->prev = node;
    obj->next = next;
    if (next)
        next->prev = obj;
}

enet_http_handle_t* enet_http_handle_find(int handle)
{
    enet_http_handle_t* node;

    node = g_http_handles;
    while (node) {
        if (node->handle == handle) {
            break;
        }
        node = node->next;
    }

    return node;
}

void enet_http_gen_handle_value(enet_http_handle_t* obj)
{
    int handle = enet_http_random_handle();
    enet_http_handle_t* node;

    while (node = enet_http_handle_find(handle)) {
        handle = enet_http_random_handle();
    }

    obj->handle = handle;
}

enet_http_handle_t* enet_http_new_handle()
{
    enet_http_handle_t* out = 0;
    enet_http_obj* obj;

    out = (enet_http_handle_t*)calloc(1, sizeof(enet_http_handle_t));
    if (out == 0)
        return 0;

    obj = (enet_http_obj*)calloc(1, sizeof(enet_http_obj));
    if (obj == 0) {
        free(out);
        return 0;
    }
    obj->method = ENET_HTTP_RAW;

    enet_http_gen_handle_value(out);
    out->obj = obj;

    return out;
}

lbs_status_t ENET_CALLCONV enet_http_new(const char* url, int* handle)
{
    enet_http_handle_t* obj;

    if (handle == 0)
        return LBS_CODE_INV_PARAM;

    enet_http_lock();

    obj = enet_http_new_handle();
    if (obj == 0) {
        enet_http_unlock();
        return LBS_CODE_NO_MEMORY;
    }

    enet_http_handle_insert(obj);
    if (url) {
        enet_http_load_url(url, obj->obj);
    }
    *handle = obj->handle;

    enet_http_unlock();

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_http_set(int handle, int opt, void* key, void* value)
{
    enet_http_handle_t* obj;
    lbs_status_t rc = LBS_CODE_OK;

    if (key == 0 && value == 0) {
        return LBS_CODE_INV_PARAM;
    }

    enet_http_lock();
    obj = enet_http_handle_find(handle);
    if (obj == 0) {
        enet_http_unlock();
        return LBS_CODE_INV_HANDLE;
    }

    switch (opt) {
        case ENET_HTTP_SET_HEADER:
            rc = enet_http_insert_header((const char*)key, (const char*)value, &obj->obj->headers);
            break;
        case ENET_HTTP_SET_URL:
            rc = enet_http_load_url((const char*)value, obj->obj);
            break;
        case ENET_HTTP_SET_BODY:
            rc = enet_http_set_body(value, *(int*)key, obj->obj);
            break;
        default:
            rc = LBS_CODE_INV_PARAM;
    }
    enet_http_unlock();

    return rc;
}

const char* enet_http_get_method_string(int method)
{
    switch (method) {
    case ENET_HTTP_GET:
        return "GET";
    case ENET_HTTP_POST:
        return "POST";
    case ENET_HTTP_PUT:
        return "PUT";
    case ENET_HTTP_DELETE:
        return "DELETE";
    case ENET_HTTP_RAW:
        return "METHOD";
    default:
        return "GET";
    }
}

unsigned char* enet_http_gen_stream(enet_http_obj* http, int method, int* size)
{
    enet_http_header_t* header;
    lbs_status_t rc;
    int is_host_ow = 0;
    int buffer_size = 128;
    char* stream;

    /* Initialize */
    stream = (char*)malloc(buffer_size);
    if (!stream)
        return 0;
    stream[0] = 0;
    *size = 0;

    rc = insert_segment(&stream, &buffer_size, size, enet_http_get_method_string(method), 0);
    if (rc != LBS_CODE_OK) goto bail;
    rc = insert_segment(&stream, &buffer_size, size, " ", 0);
    if (rc != LBS_CODE_OK) goto bail;
    rc = insert_segment(&stream, &buffer_size, size, http->uri, 0);
    if (rc != LBS_CODE_OK) goto bail;
    rc = insert_segment(&stream, &buffer_size, size, " HTTP/1.1\r\nUser-Agent: LBS easenet API\r\n", 0);
    if (rc != LBS_CODE_OK) goto bail;

    header = http->headers;
    while (header) {
        rc = insert_segment(&stream, &buffer_size, size, header->key, 0);
        if (rc != LBS_CODE_OK) goto bail;
        rc = insert_segment(&stream, &buffer_size, size, ": ", 0);
        if (rc != LBS_CODE_OK) goto bail;
        rc = insert_segment(&stream, &buffer_size, size, header->content, 0);
        if (rc != LBS_CODE_OK) goto bail;
        rc = insert_segment(&stream, &buffer_size, size, "\r\n", 0);
        if (rc != LBS_CODE_OK) goto bail;

        lowercase(header->key, strlen(header->key));
        if (0 == strcmp(header->key, "host")) {
            is_host_ow = 1;
        }
        header = header->next;
    }

    if (!is_host_ow) {
        rc = insert_segment(&stream, &buffer_size, size, "Host: ", 0);
        if (rc != LBS_CODE_OK) goto bail;
        rc = insert_segment(&stream, &buffer_size, size, http->host, 0);
        if (rc != LBS_CODE_OK) goto bail;
        rc = insert_segment(&stream, &buffer_size, size, "\r\n", 0);
        if (rc != LBS_CODE_OK) goto bail;
    }

    rc = insert_segment(&stream, &buffer_size, size, "\r\n", 0);
    if (rc != LBS_CODE_OK) goto bail;
    if (http->body) {
        rc = insert_segment(&stream, &buffer_size, size, http->body, http->body_size);

        if (rc != LBS_CODE_OK) goto bail;
    }

    return stream;
bail:
    free(stream);
    return 0;
}

lbs_status_t ENET_CALLCONV enet_http_exec(int handle, int method)
{
    enet_http_handle_t* obj;
    lbs_status_t rc = LBS_CODE_OK;
    char* stream = 0;
    int size = 0;
    ENET_TCP_CLIENT client = {0};

    enet_http_lock();
    obj = enet_http_handle_find(handle);
    if (!obj) {
        enet_http_unlock();
        return LBS_CODE_INV_HANDLE;
    }

    rc = enet_tcp_client_create_socket(obj->obj->port, obj->obj->addr, &client);
    if (rc != LBS_CODE_OK)
        goto bail;

    if (-1 == connect(client.fd, (struct sockaddr*)(&(client.addr)), sizeof(struct sockaddr))) {
        rc = enet_error_code_convert(enet_get_sys_error());
        goto bail;
    }

    obj->obj->method = method;
    stream = enet_http_gen_stream(obj->obj, method, &size);
    if (!stream) {
        rc = LBS_CODE_NO_MEMORY;
        goto bail;
    }

    if (obj->obj->response) {
        free(obj->obj->response);
        obj->obj->response = 0;
    }

    rc = send_http_message(&obj->obj->response, &obj->obj->res_size, stream, size, client.fd);
    if (rc != LBS_CODE_OK)
        goto bail;

    rc = enet_http_parse_response(obj->obj);

bail:
    free(stream);
    enet_http_unlock();
    if (client.fd)
        enet_tcp_client_close_socket(&client);

    return rc;
}

/*
* enet_http_free() is necessary if this function is executed successfully
*
*/
lbs_status_t ENET_CALLCONV enet_http_fetch(int handle, int opt, const char* key, void** data, int* data_size)
{
    enet_http_handle_t* obj;
    enet_http_header_t* header;
    enet_http_response_t* res;
    void* out = 0;
    int size;
    lbs_status_t rc = LBS_CODE_OK;

    if (data == 0) 
        return LBS_CODE_INV_HANDLE;

    enet_http_lock();

    obj = enet_http_handle_find(handle);
    if (obj == 0) {
        rc = LBS_CODE_INV_HANDLE;
        goto bail;
    }
    res = obj->obj->parsed_res;
    if (res == 0 && opt != ENET_HTTP_GET_REQUEST) {
        rc = LBS_CODE_NOT_FOUND;
        goto bail;
    }

    switch (opt) {
    case ENET_HTTP_GET_RAW    :
        out = enet_duplicate_mem(obj->obj->response, obj->obj->res_size);
        if (!out) rc = LBS_CODE_NO_MEMORY;
        else size = obj->obj->res_size;
        break;
    case ENET_HTTP_GET_HEADER :
        header = enet_http_find_header(key, res->headers);
        if (header) {
            out = enet_duplicate_str(header->content);
            if (!out) rc = LBS_CODE_NO_MEMORY;
            else size = strlen(header->content)+1;
        } else {
            rc = LBS_CODE_NOT_FOUND;
            size = 0;
        }
        break;
    case ENET_HTTP_GET_CODE   :
        out = enet_duplicate_str(res->status_str);
        if (!out) rc = LBS_CODE_NO_MEMORY;
        else size = strlen(res->status_str)+1;
        break;
    case ENET_HTTP_GET_STATUS :
        out = enet_duplicate_str(res->res_des);
        if (!out) rc = LBS_CODE_NO_MEMORY;
        else size = strlen(res->res_des)+1;
        break;
    case ENET_HTTP_GET_BODY   :
        out = enet_duplicate_mem(res->body, res->body_size);
        if (!out) rc = LBS_CODE_NO_MEMORY;
        else size = res->body_size;
        break;
    case ENET_HTTP_GET_VERION :
        out = enet_duplicate_str(res->version);
        if (!out) rc = LBS_CODE_NO_MEMORY;
        else size = strlen(res->version)+1;
        break;
    case ENET_HTTP_GET_REQUEST:
        out = enet_http_gen_stream(obj->obj, obj->obj->method, &size);
        if (!out) rc = LBS_CODE_NO_MEMORY;
        break;
    default:
        rc = LBS_CODE_INV_PARAM;
    }

    *data = out;
    if (data_size)
        *data_size = size;
bail:
    enet_http_unlock();
    return rc;
}

lbs_status_t ENET_CALLCONV enet_http_del(int handle)
{
    enet_http_handle_t* obj;

    enet_http_lock();

    obj = enet_http_handle_find(handle);
    if (!obj) {
        enet_http_unlock();
        return LBS_CODE_INV_HANDLE;
    }

    enet_http_handle_delete(obj);

    enet_http_unlock();

    return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_http_get(int ip, int port, const char* url, const char* param, char** output)
{
    char* info = 0;
    unsigned size, out_size = 1024;
    ENET_TCP_CLIENT client = {0};
    lbs_status_t rc;

    if (ip == 0 || port == 0 || output == 0)
        return LBS_CODE_INV_PARAM;

    rc = construct_http_message(&info, &size, url, param, 0, "GET");
    if (rc != LBS_CODE_OK)
        goto bail;

    rc = enet_tcp_client_create_socket(port, ip, &client);
    if (rc != LBS_CODE_OK)
        goto bail;

    if (-1 == connect(client.fd, (struct sockaddr*)(&(client.addr)), sizeof(struct sockaddr))) {
        rc = enet_error_code_convert(enet_get_sys_error());
        goto bail;
    }

    rc = send_http_message(output, &out_size, info, size, client.fd);
    if (rc != LBS_CODE_OK)
        goto bail;

bail:
    free(info);
    if (client.fd)
        enet_tcp_client_close_socket(&client);

    return rc;
}

void ENET_CALLCONV enet_http_free(void* p)
{
    if (p == 0)
        return;

    free(p);
}

int gzDecompress(const char *src, int srcLen, const char *dst, int dstLen)
{  
/*    z_stream strm; */

    int err=-1, ret=-1;  

	/*
    strm.zalloc=NULL;  
    strm.zfree=NULL;  
    strm.opaque=NULL;  

    strm.avail_in = srcLen;  
    strm.avail_out = dstLen;  
    strm.next_in = (Bytef *)src;  
    strm.next_out = (Bytef *)dst;  

    err = inflateInit2(&strm, MAX_WBITS+16);  
    if (err == Z_OK){  
        err = inflate(&strm, Z_FINISH);  
        if (err == Z_STREAM_END){  
            ret = strm.total_out;  
        }  
        else{  
            inflateEnd(&strm);  
            return err;  
        }  
    }  
    else{  
        inflateEnd(&strm);  
        return err;  
    }  
    inflateEnd(&strm);  
	*/
    return err;  
} 

lbs_status_t ENET_CALLCONV enet_http_post(int ip, int port, const char* url, const char* param, const char* content, char** output)
{
    char* info = 0;
    char* out_buffer = 0;
    char* decomp = 0;
    unsigned size, out_size = 1024;

    ENET_TCP_CLIENT client = {0};
    lbs_status_t rc;

    if (ip == 0 || port == 0 || output == 0 || url == 0)
        return LBS_CODE_INV_PARAM;

    rc = construct_http_message(&info, &size, url, param, content, "POST");
    if (rc != LBS_CODE_OK)
        goto bail;

    rc = enet_tcp_client_create_socket(port, ip, &client);
    if (rc != LBS_CODE_OK)
        goto bail;

    if (-1 == connect(client.fd, (struct sockaddr*)(&(client.addr)), sizeof(struct sockaddr))) {
        rc = enet_error_code_convert(enet_get_sys_error());
        goto bail;
    }

    rc = send_http_message(&out_buffer, &out_size, info, size, client.fd);
    if (rc != LBS_CODE_OK)
        goto bail;

    if (out_size > 0) {/* 
        int st;
        int buf_size = out_size*3;

        decomp = (char*)malloc(buf_size);
        if (decomp == 0) {
            rc = LBS_CODE_NO_MEMORY;
            goto bail;
        }

       st = uncompress(decomp, &buf_size, out_buffer, out_size);
        if (st != Z_OK) {
            rc = LBS_CODE_DATA_CORRUPT;
            goto bail;
        }*/

		*output = enet_duplicate_mem(out_buffer, out_size);
		if (*output == 0) {
			rc = LBS_CODE_NO_MEMORY;
		}
        //*output = decomp;
    }

bail:
    free(info);
    free(out_buffer);
    if (client.fd)
        enet_tcp_client_close_socket(&client);

    return rc;
}
