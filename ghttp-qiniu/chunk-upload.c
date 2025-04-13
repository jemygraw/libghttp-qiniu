#include "ghttp-qiniu.h"
#include <ghttp.h>
#include <ghttp/http_base64.h>
#include <cjson/cJSON.h>
#include <string.h>

void qn_free_initchunkret(qn_initchunkret *init_ret)
{
    if (init_ret->upload_id)
    {
        free(init_ret->upload_id);
    }
    if (init_ret->error)
    {
        free((void *)init_ret->error);
    }
}

// init the chunk upload
// https://developer.qiniu.com/kodo/6365/initialize-multipartupload
int qn_init_chunk_upload(const char *bucket_name, const char *upload_token, const char *file_key, qn_initchunkret *init_ret)
{
    int exit_code = 0;
    int is_object_name_set = 0;
    char *resp_body = NULL;
    ghttp_request *request = NULL;
    cJSON *resp_json = NULL;
    char *object_name = "~";
    char *auth_token = NULL;
    char *post_uri = NULL;
    // make post url
    if (file_key)
    {
        object_name = http_base64_encode(file_key);
        is_object_name_set = 1;
    }
    size_t post_uri_len = strlen(QN_UPLOAD_HOST) + strlen(bucket_name) + strlen(object_name) + 27;
    post_uri = (char *)calloc(post_uri_len, sizeof(char));
    sprintf(post_uri, "%s/buckets/%s/objects/%s/uploads", QN_UPLOAD_HOST, bucket_name, object_name);
    // printf("==> %s, %d\n", post_uri,strlen(post_uri));
    // make auth token
    size_t auth_token_len = strlen(upload_token) + strlen("Uptoken ") + 1;
    auth_token = (char *)calloc(auth_token_len, sizeof(char));
    sprintf(auth_token, "UpToken %s", upload_token);
    // fire request
    request = ghttp_request_new();
    if (request == NULL)
    {
        init_ret->error = "new request error";
        exit_code = -1;
        goto cleanup;
    }
    ghttp_set_uri(request, post_uri);
    ghttp_set_header(request, "User-Agent", QN_USER_AGENT);
    ghttp_set_header(request, "Authorization", auth_token);
    ghttp_set_type(request, ghttp_type_post);
    ghttp_prepare(request);
    ghttp_status status = ghttp_process(request);
    if (status == ghttp_error)
    {
        init_ret->error = ghttp_get_error(request);
        ghttp_request_destroy(request);
        exit_code = -1;
        goto cleanup;
    }
    // parse body
    int resp_body_len = ghttp_get_body_len(request);
    resp_body = (char *)calloc(resp_body_len + 1, sizeof(char));
    char *resp_body_end = qn_memconcat(resp_body, ghttp_get_body(request), resp_body_len);
    *resp_body_end = 0; // end it
    // printf("==>resp: %s\n", resp_body);
    //  parse resp_json
    resp_json = cJSON_Parse(resp_body);
    if (resp_json == NULL)
    {
        init_ret->error = "parse resp body error";
        exit_code = -1;
        goto cleanup;
    }
    cJSON *err = cJSON_GetObjectItem(resp_json, "error");
    if (err)
    {
        const char *error = cJSON_GetStringValue(err);
        // printf("==>error: %s\n", error);
        init_ret->error = qn_strdup(error);
        exit_code = -1;
        goto cleanup;
    }
    // get upload id and expired at
    cJSON *upload_id = cJSON_GetObjectItem(resp_json, "uploadId");
    cJSON *expire_at = cJSON_GetObjectItem(resp_json, "expireAt");
    const char *upload_id_str = cJSON_GetStringValue(upload_id);
    double expire_at_val = cJSON_GetNumberValue(expire_at);
    // printf("==>upload_id: %s, expire_at: %ld\n", upload_id_str, (long int)expire_at_val);
    //  copy to init ret
    init_ret->upload_id = qn_strdup(upload_id_str);
    init_ret->expire_at = expire_at_val;
// cleanup
cleanup:
    free(post_uri);
    free(auth_token);
    if (!is_object_name_set)
    {
        free(object_name);
    }
    if (request)
    {
        ghttp_request_destroy(request);
    }
    if (resp_body)
    {
        free(resp_body);
    }
    if (resp_json)
    {
        cJSON_Delete(resp_json);
    }
    return exit_code;
}

int qn_chunk_upload_file(const char *local_path, const char *bucket_name, const char *upload_token, const char *file_key,
                         qn_putextra *putextra, qn_putret *put_ret)
{
    int exit_code = 0;
    // init chunk upload
    qn_initchunkret initret = {
        .upload_id = NULL,
        .expire_at = 0,
        .error = NULL,
    };
    // TODO read resume upload record
    int ret = qn_init_chunk_upload(bucket_name, upload_token, file_key, &initret);
    if (ret == -1)
    {
        put_ret->error = qn_strdup(initret.error);
        exit_code = -1;
        goto cleanup;
    }
    printf("upload_id: %s, expire_at: %ld\n", initret.upload_id, initret.expire_at);

cleanup:
    qn_free_initchunkret(&initret);
    return exit_code;
}