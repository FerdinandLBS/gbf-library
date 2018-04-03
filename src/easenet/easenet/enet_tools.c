#include "enet_types.h"
#include "enet_sock.h"
#include "enet_sys.h"

#include <stdio.h>
#include <stdlbs.h>

int ENET_CALLCONV enet_name_to_ip(char* name)
{
    int ip = 0;
    struct hostent* host;
    int adapter = 0;         

    if (name == 0)
        return 0;

    enet_socket_init();

    host = gethostbyname(name);
    if (!host)
        return 0;

    while(host->h_addr_list[adapter]) 
    {
        if (host->h_length > sizeof(int)) {
            continue;
        }
        memcpy(&ip, host->h_addr_list[adapter], host->h_length);
        adapter++; 
    } 

    return ip;
}

static lbs_status_t insert_segment(char** info, unsigned* buff_size, unsigned* current_size, const char* segment)
{
	unsigned inc;
	
	if (segment == 0) return LBS_CODE_OK;
	
	inc = strlen(segment);
	if (inc == 0) return LBS_CODE_OK;

	/* realloc new buffer if overflow */
	if (inc + *current_size >= *buff_size) {
		char* temp;
		
		*buff_size += (inc+1);
		temp = realloc(*info, *buff_size);
		if (temp == 0)
			return LBS_CODE_NO_MEMORY;

		*info = temp;
	}

	strcat(*info, segment);
	*current_size += inc;

	return LBS_CODE_OK;
}

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
	rc = insert_segment(info, &buff_size, size, method);
	if (rc) goto bail;

	if (url == 0 || (url[0] != '/' && url[0] != '\\'))
		uri_head = " /";
	else
		uri_head = " ";
	rc = insert_segment(info, &buff_size, size, uri_head);
	if (rc) goto bail;
	
	/* copy url */
	rc = insert_segment(info, &buff_size, size, url);
	if (rc) goto bail;
	
	rc = insert_segment(info, &buff_size, size, " HTTP/1.1\r\n");
	if (rc) goto bail;

	if (content || content[0] != 0) {
		char content_len[128];
		unsigned length = lbs_strlen(content);

		rc = insert_segment(info, &buff_size, size, "Content-Type: text/html; charset=UTF-8\r\n");
		if (rc) goto bail;

		sprintf(content_len, "Content-Length: %d\r\n\r\n", length);
		rc = insert_segment(info, &buff_size, size, content_len);
		if (rc) goto bail;
		
		rc = insert_segment(info, &buff_size, size, content);
		if (rc) goto bail;
	} else {
		rc = insert_segment(info, &buff_size, size, "\r\n");
		if (rc) goto bail;
	}

bail:
	return rc;
}

static lbs_status_t send_http_message(char** response, unsigned* size, const char* message, unsigned in_size, int fd)
{
	FD_SET read_set;
    struct timeval tv = {2, 0};
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

	while(1) {
		st = select(0, &read_set, 0, 0, &tv);
		if (st == 0) {
			*p = 0;
			break;
		}
		if (st < 0) {
			free(*response);
			*size = 0;
			return enet_error_code_convert(enet_get_sys_error());
		}
		st = recv(fd, p, *size-read_size, 0);
		if (st == 0) {
			*p = 0;
			return LBS_CODE_OK;
		}
		if (st < 0) {
			if (read_size > 0) {
				*p = 0;
				return LBS_CODE_OK;
			}
			free(*response);
			*size = 0;
			return enet_error_code_convert(enet_get_sys_error());
		}
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
		p = *response + read_size;
	}
	
	return LBS_CODE_OK;
}

lbs_status_t ENET_CALLCONV enet_http_get(int ip, int port, const char* url, const char* param, char** output)
{
	char* info = 0;
	unsigned size, out_size = 1024;
	ENET_TCP_CLIENT client = {0};
	lbs_status_t rc;
	SOCKET fd;

	if (ip == 0 || port == 0 || output == 0)
		return LBS_CODE_INV_PARAM;

	rc = construct_http_message(&info, &size, url, param, 0, "GET");
	if (rc != LBS_CODE_OK)
		goto bail;

	if (port != -1) {
		rc = enet_tcp_client_create_socket(port, ip, &client);
		if (rc != LBS_CODE_OK)
			goto bail;

		if (-1 == connect(client.fd, (struct sockaddr*)(&(client.addr)), sizeof(struct sockaddr))) {
			rc = enet_error_code_convert(enet_get_sys_error());
			goto bail;
		}
		fd = client.fd;
	} else {
		fd = ip;
	}

	rc = send_http_message(output, &out_size, info, size, fd);
	if (rc != LBS_CODE_OK)
		goto bail;

bail:
	free(info);
	if (client.fd && port != -1)
		enet_tcp_client_close_socket(&client);

	return rc;
}

lbs_status_t ENET_CALLCONV enet_http_post(int ip, int port, const char* url, const char* param, const char* content, char** output)
{
	char* info = 0;
	unsigned size, out_size = 1024;
	ENET_TCP_CLIENT client = {0};
	lbs_status_t rc;
	SOCKET fd;

	if (ip == 0 || port == 0 || output == 0 || url == 0)
		return LBS_CODE_INV_PARAM;

	rc = construct_http_message(&info, &size, url, param, content, "POST");
	if (rc != LBS_CODE_OK)
		goto bail;

	if (port != -1) {
		rc = enet_tcp_client_create_socket(port, ip, &client);
		if (rc != LBS_CODE_OK)
			goto bail;

		if (-1 == connect(client.fd, (struct sockaddr*)(&(client.addr)), sizeof(struct sockaddr))) {
			rc = enet_error_code_convert(enet_get_sys_error());
			goto bail;
		}
		fd = client.fd;
	} else {
		fd = ip;
	}

	rc = send_http_message(output, &out_size, info, size, fd);
	if (rc != LBS_CODE_OK)
		goto bail;

bail:
	free(info);
	if (client.fd && port != -1)
		enet_tcp_client_close_socket(&client);

	return rc;
}