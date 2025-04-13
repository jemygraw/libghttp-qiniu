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
    char *etag;
    int part_number;
} qn_chunkpart;

// qiniu recorder body
typedef struct __qn_recorder_body
{
    long int last_modified;
    long int file_size;
    qn_chunkpart parts[];
} qn_recorderbody;

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
    qn_map *extra_params;
    int extra_params_count;
    // mine type of file
    const char *mime_type;
    // recorder file key of resume upload
    const char *recorder_key;
} qn_putextra;

// create a duplicate string
char *qn_strdup(const char *src);

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
