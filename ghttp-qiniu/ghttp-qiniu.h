//
// Created by jemy on 19/12/2017.
//

#ifndef LIBGHTTP_QINIU_GHTTP_QINIU_H
#define LIBGHTTP_QINIU_GHTTP_QINIU_H

#include <stdio.h>
#include <ghttp.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

// multi-data form body size, default is 512KB
static size_t QN_MULTIDATA_FORM_SIZE = 512 * 1024;

// qiniu storage upload host
static char *QN_UPLOAD_HOST = "http://upload.qiniup.com";

// qiniu arm sdk user agent (do not change)
static char *QN_REPORT_HOST = "http://uplog.qbox.me/log/3";
static char *QN_USER_AGENT = "qiniu-arm-sdk(1.0.0;)";
static int X_REQID_LEN = 17;
// qiniu key-value map
typedef struct __qn_map__ {
    char *key;
    char *value;
} qn_map;

// qiniu upload response
// you can parse the resp_body use any JSON library you like
typedef struct _qn_putret__ {
    char *resp_body;
    int resp_body_len;
    int status_code;
    const char *error;
} qn_putret;


// function declarations

// create random string for form boundary
char *qn_random_str(int len);

// qiniu form body concat function
char *qn_memconcat(char *dst_buffer, const char *src_buffer, size_t src_buffer_len);

// qiniu form upload body assembler
char *qn_addformfield(char *dst_buffer, char *form_boundary, size_t form_boundary_len,
                      const char *field_name, char *field_value, size_t field_value_len, const char *field_mime_type,
                      size_t *form_data_len);

// qiniu form upload file
int qn_upload_file(const char *local_path, const char *upload_token, const char *file_key, const char *mime_type,
                   qn_map *extra_params, int extra_params_count, qn_putret *put_ret);

// qiniu report upload status
void qn_upload_report(const char *upload_token, int status_code, char *req_id, char *remote_host, char *remote_ip,
                      int remote_port, long duration, long upload_time, long bytes_sent, char *upload_type,
                      long file_size);

#endif //LIBGHTTP_QINIU_GHTTP_QINIU_H
