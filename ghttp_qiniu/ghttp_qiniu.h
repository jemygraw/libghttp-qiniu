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

// qiniu key-value map used for customer defined vars and metadata
typedef struct __qn_map
{
    char *key;
    char *value;
} qn_map;

// qiniu upload response body
typedef struct __qn_put_ret
{
    const char *error;
    char *resp_body;
    int resp_body_len;
    int status_code;
} qn_putret;

// qiniu init chunk upload response body
typedef struct __qn_init_chunk_ret
{
    int status_code;
    char *upload_id;
    long int expire_at;
    const char *error;
} qn_initchunkret;

// qiniu chunk upload part
typedef struct __qn_chunk_part
{
    const char *error;
    char *etag;
    int part_number;
    int status_code;
} qn_chunkpart;

// qiniu chunk upload recorder
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

// qiniu complete chunk upload request body
typedef struct __qn_chunk_payload
{
    // payload error
    const char *error;
    // payload bytes
    char *bytes;
} qn_chunkpayload;

// qiniu put extra
typedef struct __qn_put_extra
{
    // customer defined extra params
    // key should startswith x:
    qn_map *custom_vars;
    // customer defined vars count
    int custom_vars_count;
    // qiniu defined metadata params
    // key should startswith x-qn-meta-
    qn_map *metadata;
    // metadata count
    int metadata_count;
    // mine type of file, eg: text/plain
    const char *mime_type;
    // recorder file key of resume upload
    // default to base64(local_path+bucket_name+file_key)
    // the recorder file specified by recorder_key will be used for resume upload
    // and will be deleted after upload success or upload token is invalid or local
    // file content changed
    const char *recorder_key;
} qn_putextra;


// print the debug message when set QINIU_DEBUG=1
void qn_debug(const char *format, ...);

// create a duplicate string for save memory cleanup
char *qn_strdup(const char *src);

// create file base name string
char *qn_file_basename(const char *file_path);

// create random string for form boundary
char *qn_random_str(int len);

// qiniu form body concat function
char *qn_memconcat(char *dst_buffer, const char *src_buffer, size_t src_buffer_len);

// for qn_putret object memory cleanup
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
