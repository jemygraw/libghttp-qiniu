//
// Created by jemy on 19/12/2017.
//

#ifndef LIBGHTTP_QINIU_GHTTP_QINIU_H
#define LIBGHTTP_QINIU_GHTTP_QINIU_H

#include <stdio.h>
#include <time.h>
#include <ghttp.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <sys/socket.h>

// multi-data form body size, default is 512KB
static size_t QN_MULTIDATA_FORM_SIZE = 512 * 1024;

// chunk upload chunk size, default is 1MB
static size_t QN_CHUNK_SIZE = 1024 * 1024;

// qiniu storage upload host
static char *QN_UPLOAD_HOST = "http://upload.qiniup.com";

// qiniu arm sdk user agent (do not change)
static char *QN_REPORT_HOST = "http://uplog.qbox.me/log/3";
static char *QN_USER_AGENT = "qiniu-arm-sdk(1.0.0;)";
static int X_REQID_LEN = 17;
// qiniu key-value map
typedef struct __qn_map
{
    char *key;
    char *value;
} qn_map;

// qiniu upload response
typedef struct __qn_put_ret
{
    char *resp_body;
    int resp_body_len;
    int status_code;
    const char *error;
} qn_putret;

// qiniu chunk part
typedef struct __qn_chunk_part
{
    const char *error;
    char *etag;
    int part_number;
} qn_chunkpart;

// qiniu chunk recorder body
typedef struct __qn_chunk_recorder
{
    const char *error;
    char *upload_id;
    time_t last_modified;
    long file_size;
    long expire_at;
    int part_count;
    qn_chunkpart *parts;
} qn_chunkrecorder;

// qiniu complete chunk upload body
typedef struct __qn_chunk_payload
{
    // payload bytes, free after use
    char *bytes;
    // payload error, free after use
    const char *error;
} qn_chunkpayload;

// qiniu init chunk response
typedef struct __qn_init_chunk_ret
{
    char *upload_id;
    long int expire_at;
    const char *error;
} qn_initchunkret;

// qiniu put extra
typedef struct __qn_put_extra
{
    // customer defined extra params
    // key should startswith x:
    qn_map *custom_vars;
    int custom_vars_count;
    // qiniu defined metadata params
    // key should startswith x-qn-meta-
    qn_map *metadata;
    int metadata_count;
    // mine type of file
    const char *mime_type;
    // recorder file key of resume upload
    const char *recorder_key;
} qn_putextra;

void qn_debug(const char *format, ...);

// create a duplicate string
char *qn_strdup(const char *src);

// create file base name string
char *qn_file_basename(const char *file_path);

// create random string for form boundary
char *qn_random_str(int len);

// qiniu form body concat function
char *qn_memconcat(char *dst_buffer, const char *src_buffer, size_t src_buffer_len);

// qiniu free put result
void qn_free_putret(qn_putret *put_ret);

// qiniu form upload body assembler
char *qn_addformfield(char *dst_buffer, char *form_boundary, size_t form_boundary_len,
                      const char *field_name, char *field_value, size_t field_value_len,
                      const char *field_mime_type, size_t *form_data_len);

// qiniu form upload file
int qn_upload_file(const char *local_path, const char *upload_token, const char *file_key,
                   qn_putextra *putextra, qn_putret *put_ret);

// qiniu report upload status
void qn_upload_report(const char *upload_token, int status_code, char *req_id, char *remote_host, char *remote_ip,
                      int remote_port, long duration, long upload_time, long bytes_sent, char *upload_type,
                      long file_size);
// qiniu chunk upload file (v2)
int qn_chunk_upload_file(const char *local_path, const char *bucket_name, const char *upload_token, const char *file_key,
                         qn_putextra *putextra, qn_putret *put_ret);

#endif // LIBGHTTP_QINIU_GHTTP_QINIU_H
