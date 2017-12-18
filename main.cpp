#include <stdio.h>
#include <ghttp.h>
#include <stdlib.h>
#include <string.h>

size_t QN_MULTIDATA_FORM_SIZE = 1024 * 1024;
//1MB
size_t QN_BUFFER_SIZE = 4096;
size_t QN_FNAME_LEN = 20;
char *QN_UPLOAD_HOST = "http://upload.qiniup.com";
char *QN_USER_AGENT = "qiniu-arm-sdk(1.0.0;)";

typedef struct __qn_map__ {
    char *key;
    char *value;
} qn_map;

int qn_upload_file(const char *local_path, const char *upload_token, const char *file_key, const char *mime_type,
                   const qn_map *extra_params, const int extra_params_len);

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


/**
 * @param local_path
 * @param upload_token
 * @param file_key
 * @param mime_type
 * @param extra_params
 *
 * @return 0 on success
 * */
int qn_upload_file(const char *local_path, const char *upload_token, const char *file_key, const char *mime_type,
                   qn_map *extra_params, int extra_params_len) {
    int i;
    FILE *fp = NULL;
    char buffer[QN_BUFFER_SIZE + 1];
    size_t read_num;
    char fname[QN_FNAME_LEN + 1];

    const char *form_prefix = "----QNSDKFormBoundary";
    char *random_suffix = qn_random_str(16);
    char *form_boundary = (char *) calloc(38, sizeof(char));
    strcat(form_boundary, form_prefix);
    strcat(form_boundary, random_suffix);
    char *form_data = (char *) calloc(QN_MULTIDATA_FORM_SIZE, sizeof(char));
    //add key
    strcat(form_data, "--");

    if (file_key != NULL) {
        strcat(form_data, form_boundary);
        strcat(form_data, "\r\n");
        strcat(form_data, "Content-Disposition: form-data; name=\"key\"\r\n\r\n");
        strcat(form_data, file_key);
        strcat(form_data, "\r\n--");
    }

    //add upload_token
    strcat(form_data, form_boundary);
    strcat(form_data, "\r\n");
    strcat(form_data, "Content-Disposition: form-data; name=\"token\"\r\n\r\n");
    strcat(form_data, upload_token);
    strcat(form_data, "\r\n--");


    //check extra_params
    if (extra_params_len > 0) {
        qn_map *p = extra_params;

        for (i = 0; i < extra_params_len; i++) {
            char *param_key = p->key;
            if (strncmp(param_key, "x:", 2) != 0) {
                continue;
            }
            char *param_value = p->value;
            strcat(form_data, form_boundary);
            strcat(form_data, "\r\n");

            size_t content_disposition_len = 44 + strlen(param_key);
            char *content_disposition = (char *) malloc(sizeof(char) * content_disposition_len);
            snprintf(content_disposition, content_disposition_len,
                     "Content-Disposition: form-data; name=\"%s\"\r\n\r\n",
                     param_key);
            strcat(form_data, content_disposition);
            free(content_disposition);
            strcat(form_data, param_value);
            strcat(form_data, "\r\n--");
            p++;
        }
    }

    //add file
    strcat(form_data, form_boundary);
    strcat(form_data, "\r\n");
    char *slash_flag = strrchr(local_path, '/');
    if (slash_flag != NULL) {
        slash_flag++;
        strcpy(fname, slash_flag);
    } else {
        strcpy(fname, local_path);
    }

    size_t file_content_disposition_len = 59 + strlen(fname);
    char *file_content_disposition = (char *) malloc(sizeof(char) * file_content_disposition_len);
    snprintf(file_content_disposition, file_content_disposition_len,
             "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n", fname);
    strcat(form_data, file_content_disposition);
    if (mime_type != NULL) {
        strcat(form_data, "Content-Type: ");
        strcat(form_data, mime_type);
        strcat(form_data, "\r\n");
    }

    strcat(form_data, "\r\n");

    fp = fopen(local_path, "r+");
    if (fp == NULL) {
        return -1;
    }

    while (!feof(fp)) {
        read_num = fread(buffer, sizeof(char), QN_BUFFER_SIZE, fp);
        strncat(form_data, buffer, read_num);
    }

    fclose(fp);
    strcat(form_data, "\r\n--");
    strcat(form_data, form_boundary);
    strcat(form_data, "--");

    size_t form_content_type_len = 31 + strlen(form_boundary);
    char *form_content_type = (char *) malloc(sizeof(char) * form_content_type_len);
    snprintf(form_content_type, form_content_type_len, "multipart/form-data; boundary=%s", form_boundary);
    //send the request
    ghttp_request *request = NULL;
    request = ghttp_request_new();
    if (request == NULL) {
        return -1;
    }
    ghttp_set_uri(request, QN_UPLOAD_HOST);
    ghttp_set_header(request, "Content-Type", form_content_type);
    ghttp_set_header(request, "User-Agent", QN_USER_AGENT);
    ghttp_set_type(request, ghttp_type_post);
    ghttp_set_body(request, form_data, strlen(form_data));
    ghttp_prepare(request);
    ghttp_process(request);
    ghttp_request_destroy(request);

    //free
    free(random_suffix);
    free(file_content_disposition);
    free(form_boundary);
    free(form_content_type);
    free(form_data);


}


int main() {
    qn_map *params = (qn_map *) calloc(10, sizeof(qn_map));
    qn_map *p = params;
    p->key = "x:name";
    p->value = "qiniu";
    p++;
    p->key = "x:age";
    p->value = "28";

    p = params;
    for (int i = 0; i < 2; i++) {
        printf("%s\t%s\n", p->key, p->value);
        p += 1;
    }

    char *local_path = "/Users/jemy/Documents/qiniu.png";
    char *upload_token = "0jHFupMTJiWhntX7P7GPlE8PRImaTN39s58KKEVc:vm5EVPJm7IO6d7G4goLdTySDxs4=:eyJzY29wZSI6ImlmLXBibCIsImRlYWRsaW5lIjoxNTEzNjAzMTMzfQ==";
    char *file_key = "qiniu-test.png";
    char *mime_type = "image/png";
    int ret = qn_upload_file(local_path, upload_token, file_key, mime_type, params, 2);
    printf("upload result: %d\n", ret);
    return 0;
}