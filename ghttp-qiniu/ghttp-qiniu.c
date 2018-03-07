//
// Created by jemy on 19/12/2017.
//

#include <arpa/inet.h>
#include "ghttp-qiniu.h"

// create a fixed length of random string
char *qn_random_str(int len) {
    int i = 0, val = 0;
    const char *base = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int base_len = (int) strlen(base);

    char *random_str = (char *) malloc(sizeof(char) * (len + 1));
    srand((unsigned int) time(NULL));
    for (; i < len; i++) {
        val = 1 + (int) ((float) (base_len - 1) * rand() / (RAND_MAX + 1.0));
        random_str[i] = base[val];
    }
    random_str[len] = 0;
    return random_str;
}

/*
 * form body concatenation function
 *
 * @param dst_buffer     a buffer used to make concatenation of strings
 * @param src_buffer     a buffer to be concated to the dst_buffer
 * @param src_buffer_len src_buffer length
 *
 * @return the end pointer of the dst_buffer, used for the next concatenation
 * */
char *qn_memconcat(char *dst_buffer, const char *src_buffer, size_t src_buffer_len) {
    memcpy(dst_buffer, src_buffer, src_buffer_len);
    char *p_end = dst_buffer + src_buffer_len;
    return p_end;
}

/*
 * assemble the multi-form body
 *
 * @param dst_buffer         a buffer used to make concatenation of strings
 * @param form_boundary      form boundary string
 * @param form_boundary_len  form boundary length
 * @param field_name         form field name
 * @param field_value        form field value
 * @param field_value_len    form field value length
 * @param field_mime_type    form field mime type (can be NULL)
 * @param form_data_len      form data length, used to accumulate the form body length
 *
 * @return the end pointer of the dst_buffer, used for the next concatenation
 * **/
char *qn_addformfield(char *dst_buffer, char *form_boundary, size_t form_boundary_len,
                      const char *field_name,
                      char *field_value, size_t field_value_len,
                      const char *field_mime_type, size_t *form_data_len) {
    size_t delta_len = 0;
    size_t field_name_len = strlen(field_name);
    size_t field_mime_len = 0;
    if (field_mime_type) {
        field_mime_len = strlen(field_mime_type);
    }

    char *dst_buffer_p = dst_buffer;
    dst_buffer_p = qn_memconcat(dst_buffer_p, form_boundary, form_boundary_len);
    dst_buffer_p = qn_memconcat(dst_buffer_p, "\r\n", 2);
    delta_len += 2;
    dst_buffer_p = qn_memconcat(dst_buffer_p, "Content-Disposition: form-data; name=\"", 38);
    delta_len += 38;
    dst_buffer_p = qn_memconcat(dst_buffer_p, field_name, field_name_len);
    dst_buffer_p = qn_memconcat(dst_buffer_p, "\"", 1);
    delta_len += 1;
    if (field_mime_type) {
        dst_buffer_p = qn_memconcat(dst_buffer_p, "\r\nContent-Type: ", 16);
        dst_buffer_p = qn_memconcat(dst_buffer_p, field_mime_type, field_mime_len);
        delta_len += 16;
    }

    dst_buffer_p = qn_memconcat(dst_buffer_p, "\r\n\r\n", 4);
    dst_buffer_p = qn_memconcat(dst_buffer_p, field_value, field_value_len);
    dst_buffer_p = qn_memconcat(dst_buffer_p, "\r\n--", 4);
    delta_len += 8;

    delta_len += form_boundary_len + field_name_len + field_mime_len + field_value_len;
    *form_data_len += delta_len;
    return dst_buffer_p;
}

/**
 * form upload file to qiniu storage
 *
 * @param local_path         local file path
 * @param upload_token       upload token get from remote server
 * @param file_key           the file name in the storage bucket, must be unique for each file
 * @param mime_type          the optional mime type to specify when upload the file, (can be NULL)
 * @param extra_params       the extra params which may contain some service-specific values
 * @param extra_params_count the count of the extra params
 * @param put_ret            the file upload response from qiniu storage server
 *
 * @return 0 on success, -1 on failure
 * */
int qn_upload_file(const char *local_path, const char *upload_token, const char *file_key, const char *mime_type,
                   qn_map *extra_params, int extra_params_count, qn_putret *put_ret) {

    //define reporting fields
    int status_code = 0;
    char req_id[X_REQID_LEN];
    char *p = strrchr(QN_UPLOAD_HOST, '/');
    char *remote_host = p + 1;
    //printf("upload host: %s\n",host);
    char remote_ip[INET6_ADDRSTRLEN];
    int remote_port = 80;
    long duration = 0;
    long start_time = time(NULL);
    long upload_time = start_time;
    long bytes_sent = 0;
    char *upload_type = "armsdk";
    long file_size = 0;

    memset(req_id, X_REQID_LEN, 0);
    memset(remote_ip, INET6_ADDRSTRLEN, 0);
    //end

    int i;
    FILE *fp = NULL;

    const char *form_prefix = "----QNSDKFormBoundary";
    char *random_suffix = qn_random_str(16);
    size_t form_boundary_len = strlen(form_prefix) + 16;
    char *form_boundary = (char *) calloc(form_boundary_len + 1, sizeof(char));
    strcat(form_boundary, form_prefix);
    strcat(form_boundary, random_suffix);
    free(random_suffix);

    //init the form data
    size_t form_data_len = 0;
    char *form_data = (char *) malloc(sizeof(char) * QN_MULTIDATA_FORM_SIZE);
    char *form_data_p = form_data;

    //add init tag
    form_data_p = qn_memconcat(form_data_p, "--", 2);
    form_data_len += 2;

    //add file key
    if (file_key != NULL) {
        form_data_p = qn_addformfield(form_data_p, form_boundary, form_boundary_len, "key", (char *) file_key,
                                      strlen(file_key),
                                      NULL, &form_data_len);
    }

    //add upload_token
    form_data_p = qn_addformfield(form_data_p, form_boundary, form_boundary_len, "token", (char *) upload_token,
                                  strlen(upload_token), NULL, &form_data_len);

    if (extra_params_count > 0) {
        qn_map *p = extra_params;

        for (i = 0; i < extra_params_count; i++) {
            char *param_key = p->key;
            if (strncmp(param_key, "x:", 2) == 0) {
                char *param_value = p->value;
                form_data_p = qn_addformfield(form_data_p, form_boundary, form_boundary_len, param_key, param_value,
                                              strlen(param_value), NULL, &form_data_len);
            }
            p++;
        }
    }

    //add file
    fp = fopen(local_path, "rb+");
    if (fp == NULL) {
        put_ret->error = "open local file error";
        free(form_data);
        free(form_boundary);

        status_code = -3;
        duration = (time(NULL) - start_time) * 1000;
        qn_upload_report(upload_token, status_code, req_id, remote_host, remote_ip, remote_port, duration, upload_time,
                         bytes_sent, upload_type, file_size);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_len = ftell(fp);
    file_size = file_len;
    rewind(fp);

    char *file_body = (char *) malloc(sizeof(char) * file_len);
    size_t read_num = fread(file_body, sizeof(char), file_len, fp);
    if (read_num != file_len) {
        put_ret->error = "read file size unmatch";
        free(form_data);
        free(form_boundary);
        free(file_body);

        status_code = -3;
        duration = (time(NULL) - start_time) * 1000;

        qn_upload_report(upload_token, status_code, req_id, remote_host, remote_ip, remote_port, duration, upload_time,
                         bytes_sent, upload_type, file_size);
        return -1;
    }
    fclose(fp);

    form_data_p = qn_addformfield(form_data_p, form_boundary, form_boundary_len, "file", file_body, file_len,
                                  mime_type, &form_data_len);
    free(file_body);
    form_data_p = qn_memconcat(form_data_p, form_boundary, form_boundary_len);
    qn_memconcat(form_data_p, "--\r\n", 4);
    form_data_len += form_boundary_len + 4;

    //send the request
    size_t form_content_type_len = 31 + form_boundary_len;
    char *form_content_type = (char *) malloc(sizeof(char) * form_content_type_len);
    snprintf(form_content_type, form_content_type_len, "multipart/form-data; boundary=%s", form_boundary);
    free(form_boundary);

    ghttp_request *request = NULL;
    request = ghttp_request_new();
    if (request == NULL) {
        put_ret->error = "new request error";
        free(form_data);
        free(form_content_type);

        status_code = -1004;
        duration = (time(NULL) - start_time) * 1000;

        qn_upload_report(upload_token, status_code, req_id, remote_host, remote_ip, remote_port, duration, upload_time,
                         bytes_sent, upload_type, file_size);

        return -1;
    }

//    fp = fopen("test.req.txt", "wb+");
//    printf("write: %d\n", fwrite(form_data, 1, form_data_len, fp));
//    fclose(fp);

    ghttp_set_uri(request, QN_UPLOAD_HOST);
    ghttp_set_header(request, "Content-Type", form_content_type);
    ghttp_set_header(request, "User-Agent", QN_USER_AGENT);
    ghttp_set_type(request, ghttp_type_post);
    ghttp_set_body(request, form_data, form_data_len);
    ghttp_prepare(request);
    ghttp_status status = ghttp_process(request);
    if (status == ghttp_error) {
        put_ret->error = ghttp_get_error(request);
        ghttp_request_destroy(request);
        free(form_content_type);
        free(form_data);

        status_code = -1;
        duration = (time(NULL) - start_time) * 1000;

        qn_upload_report(upload_token, status_code, req_id, remote_host, remote_ip, remote_port, duration, upload_time,
                         bytes_sent, upload_type, file_size);

        return -1;
    }

    //free
    free(form_content_type);
    free(form_data);

    int resp_body_len = ghttp_get_body_len(request);
    char *resp_body = (char *) malloc(sizeof(char) * (resp_body_len + 1));
    char *resp_body_end = qn_memconcat(resp_body, ghttp_get_body(request), resp_body_len);
    *resp_body_end = 0; //end it

    put_ret->resp_body = resp_body;
    put_ret->resp_body_len = resp_body_len;
    put_ret->status_code = ghttp_status_code(request);

    //get reqid
    strcpy(req_id, ghttp_get_header(request, "X-Reqid"));

    //get remote ip
    int socket = ghttp_get_socket(request);

    struct sockaddr_storage addr;
    socklen_t socket_len = sizeof(addr);
    getpeername(socket, (struct sockaddr *) &addr, &socket_len);
    if (addr.ss_family == AF_INET) {
        //ipv4
        struct sockaddr_in *s = (struct sockaddr_in *) &addr;
        remote_port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, remote_ip, sizeof(remote_ip));
    } else {
        //ipv6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *) &addr;
        remote_port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, remote_ip, sizeof(remote_ip));
    }

    //bytes_sent
    bytes_sent = file_size;
    //destroy
    ghttp_request_destroy(request);

    status_code = put_ret->status_code;
    duration = (time(NULL) - start_time) * 1000;

    qn_upload_report(upload_token, status_code, req_id, remote_host, remote_ip, remote_port, duration, upload_time,
                     bytes_sent, upload_type, file_size);

    return 0;
}


/**
 * report the upload status (failed, success or cancelled) to remote cloud
 * @param upload_token upload token
 * @param status_code  upload request status code
 * @param reqid        upload response header X-Reqid
 * @param host         upload remote server host
 * @param remote_ip    upload remote server ip
 * @param port         upload remote server port
 * @param duration     upload last time in milliseconds
 * @param up_time      upload timestamp in seconds
 * @param bytes_sent   upload total bytes sent
 * @param up_type      upload sdk name
 * @param file_size    upload file size
 */
void qn_upload_report(const char *upload_token, int status_code, char *req_id, char *remote_host, char *remote_ip,
                      int remote_port, long duration, long upload_time, long bytes_sent, char *upload_type,
                      long file_size) {

    if (!req_id) {
        req_id = "";
    }

    if (!remote_ip) {
        remote_ip = "";
    }

    size_t post_body_len = snprintf(NULL, 0, "%d", status_code) + snprintf(NULL, 0, "%s", req_id) +
                           snprintf(NULL, 0, "%s", remote_host) + snprintf(NULL, 0, "%s", remote_ip) +
                           snprintf(NULL, 0, "%d", remote_port) + snprintf(NULL, 0, "%ld", duration) +
                           snprintf(NULL, 0, "%ld", upload_time) + snprintf(NULL, 0, "%ld", bytes_sent) +
                           snprintf(NULL, 0, "%s", upload_type) + snprintf(NULL, 0, "%ld", file_size) + 9;
    char *post_body = (char *) malloc(sizeof(char) * (post_body_len + 1));
    if (!post_body) {
        //fail to alloc memory
        return;
    }
    sprintf(post_body, "%d,%s,%s,%s,%d,%ld,%ld,%ld,%s,%ld", status_code, req_id, remote_host, remote_ip, remote_port,
            duration, upload_time, bytes_sent, upload_type, file_size);
    post_body[post_body_len] = 0;

//    printf("%s\n", post_body);

//    printf("upload_token: %s\n", upload_token);
//    printf("status_code: %d\n", status_code);
//    printf("req_id: %s\n", req_id);
//    printf("remote_host: %s\n", remote_host);
//    printf("remote_ip: %s\n", remote_ip);
//    printf("remote_port: %d\n", remote_port);
//    printf("upload_type: %s\n", upload_type);
//    printf("duration: %ld\n", duration);
//    printf("upload_time: %ld\n", upload_time);
//    printf("bytes_sent: %ld\n", bytes_sent);
//    printf("file_size: %ld\n", file_size);

    ghttp_request *request = NULL;
    request = ghttp_request_new();
    if (!request) {
        //new request error
        free(post_body);
        return;
    }

    size_t auth_header_len = strlen("UpToken ") + strlen(upload_token);
    char *auth_header = (char *) malloc(sizeof(char) * (auth_header_len + 1));
    if (!auth_header) {
        //new header error
        free(post_body);
        return;
    }
    sprintf(auth_header, "UpToken %s", upload_token);

    ghttp_set_uri(request, QN_REPORT_HOST);
    ghttp_set_header(request, "Content-Type", "text/plain; charset=utf-8");
    ghttp_set_header(request, "Authorization", auth_header);
    ghttp_set_header(request, "User-Agent", QN_USER_AGENT);
    ghttp_set_type(request, ghttp_type_post);
    ghttp_set_body(request, post_body, post_body_len);
    ghttp_prepare(request);
    ghttp_status status = ghttp_process(request);
    if (status != ghttp_error && ghttp_status_code(request) == 200) {
        printf("report success\n");
    } else {
        printf("report error\n");
    }
    ghttp_request_destroy(request);
    free(auth_header);
    free(post_body);
}

