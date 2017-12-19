//
// Created by jemy on 19/12/2017.
//

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
    size_t field_name_len = strlen(field_name);
    size_t field_mime_len = 0;
    if (field_mime_type) {
        field_mime_len = strlen(field_mime_type);
    }

    char *dst_buffer_p = dst_buffer;
    dst_buffer_p = qn_memconcat(dst_buffer_p, form_boundary, form_boundary_len);
    dst_buffer_p = qn_memconcat(dst_buffer_p, "\r\n", strlen("\r\n"));
    dst_buffer_p = qn_memconcat(dst_buffer_p, "Content-Disposition: form-data; name=\"", 38);
    dst_buffer_p = qn_memconcat(dst_buffer_p, field_name, field_name_len);
    dst_buffer_p = qn_memconcat(dst_buffer_p, "\"", 1);
    if (field_mime_type) {
        dst_buffer_p = qn_memconcat(dst_buffer_p, "\r\nContent-Type: ", 16);
        dst_buffer_p = qn_memconcat(dst_buffer_p, field_mime_type, field_mime_len);
    }

    dst_buffer_p = qn_memconcat(dst_buffer_p, "\r\n\r\n", 4);
    dst_buffer_p = qn_memconcat(dst_buffer_p, field_value, field_value_len);
    dst_buffer_p = qn_memconcat(dst_buffer_p, "\r\n--", 4);
    *form_data_len += form_boundary_len + field_name_len + field_mime_len + field_value_len + 65;
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
    int i;
    FILE *fp = NULL;

    const char *form_prefix = "----QNSDKFormBoundary";
    char *random_suffix = qn_random_str(16);
    size_t form_boundary_len = strlen(form_prefix) + 16;
    char *form_boundary = (char *) malloc(sizeof(char) * (form_boundary_len + 1));
    strcat(form_boundary, form_prefix);
    strcat(form_boundary, random_suffix);

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

    //add extra_params
    if (extra_params_count > 0) {
        qn_map *p = extra_params;

        for (i = 0; i < extra_params_count; i++) {
            char *param_key = p->key;
            if (strncmp(param_key, "x:", 2) != 0) {
                continue;
            }
            char *param_value = p->value;
            form_data_p = qn_addformfield(form_data_p, form_boundary, form_boundary_len, param_key, param_value,
                                          strlen(param_value),
                                          NULL, &form_data_len);
            p++;
        }
    }

    //add file
    fp = fopen(local_path, "r+");
    if (fp == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_len = ftell(fp);
    rewind(fp);

    char *file_body = (char *) malloc(sizeof(char) * file_len);
    size_t read_num = fread(file_body, sizeof(char), file_len, fp);
    if (read_num != file_len) {
        return -1;
    }
    fclose(fp);


    form_data_p = qn_addformfield(form_data_p, form_boundary, form_boundary_len, "file", file_body, file_len,
                                  mime_type, &form_data_len);

    form_data_p = qn_memconcat(form_data_p, form_boundary, form_boundary_len);
    form_data_p = qn_memconcat(form_data_p, "--\r\n", 4);
    form_data_len += form_boundary_len + 4;

    //send the request
    size_t form_content_type_len = 31 + strlen(form_boundary);
    char *form_content_type = (char *) malloc(sizeof(char) * form_content_type_len);
    snprintf(form_content_type, form_content_type_len, "multipart/form-data; boundary=%s", form_boundary);

    ghttp_request *request = NULL;
    request = ghttp_request_new();
    if (request == NULL) {
        return -1;
    }
    ghttp_set_uri(request, QN_UPLOAD_HOST);
    ghttp_set_header(request, "Content-Type", form_content_type);
    ghttp_set_header(request, "User-Agent", QN_USER_AGENT);
    ghttp_set_type(request, ghttp_type_post);
    ghttp_set_body(request, form_data, form_data_len);
    ghttp_prepare(request);
    ghttp_process(request);

    int resp_body_len = ghttp_get_body_len(request);
    char *resp_body = (char *) malloc(sizeof(char) * (resp_body_len + 1));
    char *resp_body_end = qn_memconcat(resp_body, ghttp_get_body(request), resp_body_len);
    resp_body_end = NULL; //end it

    put_ret->resp_body = resp_body;
    put_ret->resp_body_len = resp_body_len;
    put_ret->status_code = ghttp_status_code(request);

    //destroy
    ghttp_request_destroy(request);

    //free
    free(random_suffix);
    free(form_boundary);
    free(form_content_type);
    free(form_data);
    return 0;
}
