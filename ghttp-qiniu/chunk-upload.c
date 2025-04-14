#include "ghttp-qiniu.h"
#include <ghttp.h>
#include <ghttp/http_base64.h>
#include <cjson/cJSON.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

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

void qn_free_chunkpart(qn_chunkpart *part)
{
    if (part->etag)
    {
        free((void *)part->etag);
    }
    if (part->error)
    {
        free((void *)part->error);
    }
}

void qn_free_chunkrecorder(qn_chunkrecorder *recorder)
{
    if (recorder->error)
    {
        free((void *)recorder->error);
    }
    if (recorder->upload_id)
    {
        free(recorder->upload_id);
    }
    if (recorder->parts)
    {
        for (int i = 0; i < recorder->part_count; i++)
        {
            qn_free_chunkpart(&recorder->parts[i]);
        }
        free(recorder->parts);
    }
}

void qn_free_chunkpayload(qn_chunkpayload *payload)
{
    if (payload->bytes)
    {
        free(payload->bytes);
    }
    if (payload->error)
    {
        free((void *)payload->error);
    }
}

int qn_get_file_parts(long int file_size)
{
    int parts = file_size / QN_CHUNK_SIZE;
    if (file_size % QN_CHUNK_SIZE != 0)
    {
        parts++;
    }
    return parts;
}
// get the default recorder key, identified by pair(local_path, bucket_name, file_key)
// free the return value after used.
char *qn_get_default_recorder_key(const char *local_path, const char *bucket_name, const char *file_key)
{
    int src_str_len = strlen(local_path) + strlen(bucket_name);
    if (file_key)
    {
        src_str_len += strlen(file_key);
    }
    char *src_str = (char *)calloc(src_str_len + 1, sizeof(char));
    char *src_src_p = src_str;
    src_src_p = qn_memconcat(src_src_p, local_path, strlen(local_path));
    src_src_p = qn_memconcat(src_src_p, bucket_name, strlen(bucket_name));
    if (file_key)
    {
        src_src_p = qn_memconcat(src_src_p, file_key, strlen(file_key));
    }
    char *recorder_key = http_base64_encode(src_str);
    free(src_str);
    return recorder_key;
}

// restore the chunk recorder from file and check whether the upload id expired or file changed.
// if upload id expired or file content changed, return -1; otherwise, fill the recorder and return 0.
int qn_restore_chunk_recorder(FILE *fp, const char *local_path, qn_chunkrecorder *recorder)
{
    int ret = 0;
    char *file_bytes = NULL;
    cJSON *recorder_json = NULL;
    // read file content
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    rewind(fp);
    file_bytes = (char *)calloc(file_size, sizeof(char));
    fread(file_bytes, file_size, 1, fp);
    // parse json
    recorder_json = cJSON_Parse(file_bytes);
    if (recorder_json == NULL)
    {
        recorder->error = qn_strdup("parse recorder json error");
        ret = -1;
        goto cleanup;
    }
    // fill the recorder
    recorder->expire_at = (long)cJSON_GetNumberValue(cJSON_GetObjectItem(recorder_json, "expire_at"));
    // check whether the upload id expired
    time_t current_time = time(NULL);
    if (recorder->expire_at < current_time)
    {
        qn_debug("[Qiniu] upload id %s expired at %ld\n", recorder->upload_id, recorder->expire_at);
        recorder->upload_id = NULL;
        ret = -1;
        goto cleanup;
    }
    // check whether file content changed
    recorder->last_modified = (time_t)cJSON_GetNumberValue(cJSON_GetObjectItem(recorder_json, "last_modified"));
    recorder->file_size = (long)cJSON_GetNumberValue(cJSON_GetObjectItem(recorder_json, "file_size"));
    struct stat file_stat;
    stat(local_path, &file_stat);
    qn_debug("[Qiniu] local file last modified: %ld, file size: %ld\n", file_stat.st_mtime, file_stat.st_size);
    if (!(file_stat.st_size == recorder->file_size && file_stat.st_mtime == recorder->last_modified))
    {
        qn_debug("[Qiniu] local file %s content changed\n", local_path);
        recorder->upload_id = NULL;
        ret = -1;
        goto cleanup;
    }
    recorder->upload_id = qn_strdup(cJSON_GetStringValue(cJSON_GetObjectItem(recorder_json, "upload_id")));
    // restore the existing uploaded chunks
    recorder->part_count = (int)cJSON_GetArraySize(cJSON_GetObjectItem(recorder_json, "parts"));
    recorder->parts = (qn_chunkpart *)calloc(recorder->part_count, sizeof(qn_chunkpart));
    for (int i = 0; i < recorder->part_count; i++)
    {
        cJSON *part_obj = cJSON_GetArrayItem(cJSON_GetObjectItem(recorder_json, "parts"), i);
        recorder->parts[i].part_number = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(part_obj, "partNumber"));
        recorder->parts[i].etag = qn_strdup(cJSON_GetStringValue(cJSON_GetObjectItem(part_obj, "etag")));
    }
    qn_debug("[Qiniu] read existing chunk upload recorder, upload id: %s, expire at: %ld\n", recorder->upload_id,
             recorder->expire_at);
    qn_debug("[Qiniu] last modified: %ld, file size: %d, part count: %d\n", recorder->last_modified, recorder->file_size,
             recorder->part_count);
cleanup:
    if (file_bytes)
    {
        free(file_bytes);
    }
    if (recorder_json)
    {
        cJSON_Delete(recorder_json);
    }
    return ret;
}

int qn_flush_chunk_recorder(const char *recorder_key, qn_chunkrecorder *recorder)
{
    cJSON *json = cJSON_CreateObject();
    // make up the recorder json
    cJSON_AddStringToObject(json, "upload_id", recorder->upload_id);
    cJSON_AddNumberToObject(json, "last_modified", recorder->last_modified);
    cJSON_AddNumberToObject(json, "expire_at", recorder->expire_at);
    cJSON_AddNumberToObject(json, "file_size", recorder->file_size);
    cJSON_AddNumberToObject(json, "part_count", recorder->part_count);
    cJSON *parts_obj = cJSON_CreateArray();
    for (int i = 0; i < recorder->part_count; i++)
    {
        cJSON *part_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(part_obj, "partNumber", recorder->parts[i].part_number);
        cJSON_AddStringToObject(part_obj, "etag", recorder->parts[i].etag);
        cJSON_AddItemToArray(parts_obj, part_obj);
    }
    cJSON_AddItemToObject(json, "parts", parts_obj);
    const char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    // flush to local file
    FILE *fp = fopen(recorder_key, "w+");
    if (fp)
    {
        fwrite(json_str, strlen(json_str), 1, fp);
        fclose(fp);
    }
    free((void *)json_str);
    return 0;
}

int qn_make_finish_chunk_payload(const char *local_path, qn_chunkrecorder *recorder, qn_putextra *put_extra, qn_chunkpayload *payload)
{
    int ret = 0;
    // get file base name
    const char *fname = qn_file_basename(local_path);

    // create req json
    cJSON *req_json = cJSON_CreateObject();
    cJSON_AddStringToObject(req_json, "fname", fname);
    if (put_extra)
    {
        if (put_extra->mime_type)
        {
            cJSON_AddStringToObject(req_json, "mimeType", put_extra->mime_type);
        }
        if (put_extra->metadata)
        {
            cJSON *metadata = cJSON_CreateObject();
            for (int i = 0; i < put_extra->metadata_count; i++)
            {
                cJSON_AddStringToObject(metadata, put_extra->metadata[i].key, put_extra->metadata[i].value);
            }
            cJSON_AddItemToObject(req_json, "metadata", metadata);
        }
        if (put_extra->custom_vars)
        {
            cJSON *custom_vars = cJSON_CreateObject();
            for (int i = 0; i < put_extra->custom_vars_count; i++)
            {
                cJSON_AddStringToObject(custom_vars, put_extra->custom_vars[i].key, put_extra->custom_vars[i].value);
            }
            cJSON_AddItemToObject(req_json, "customVars", custom_vars);
        }
    }
    cJSON *parts_obj = cJSON_CreateArray();
    for (int i = 0; i < recorder->part_count; i++)
    {
        cJSON *part_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(part_obj, "partNumber", recorder->parts[i].part_number);
        cJSON_AddStringToObject(part_obj, "etag", recorder->parts[i].etag);
        cJSON_AddItemToArray(parts_obj, part_obj);
    }
    cJSON_AddItemToObject(req_json, "parts", parts_obj);
    payload->bytes = cJSON_PrintUnformatted(req_json);
cleanup:
    free((void *)fname);
    cJSON_Delete(req_json);
    return ret;
}

int qn_init_chunk_recorder(const char *recorder_key, const char *local_path, qn_chunkrecorder *recorder)
{
    int ret = 0;
    cJSON *recorder_json = NULL;
    struct stat local_file_stat;
    // read recorder file
    FILE *fp = fopen(recorder_key, "r");
    if (fp)
    {
        // read existing recorder
        qn_debug("[Qiniu] reading existing recorder: %s\n", recorder_key);
        ret = qn_restore_chunk_recorder(fp, local_path, recorder);
        if (ret == 0)
        {
            qn_debug("[Qiniu] reuse existing recorder: %s\n", recorder_key);
            goto cleanup;
        }
    }
    qn_debug("[Qiniu] create a new recorder: %s\n", recorder_key);
    // create a new recorder when not found or expired
    if (stat(local_path, &local_file_stat) != 0)
    {
        recorder->error = qn_strdup("stat local path error");
        ret = -1;
        goto cleanup;
    }
    // get file last modified & length
    time_t last_modified = local_file_stat.st_mtime;
    long int file_size = local_file_stat.st_size;
    recorder->last_modified = last_modified;
    recorder->file_size = file_size;
    // calc the total parts of the file
    int part_count = qn_get_file_parts(file_size);
    qn_chunkpart *parts = (qn_chunkpart *)calloc(part_count, sizeof(qn_chunkpart));
    for (int i = 0; i < part_count; i++)
    {
        parts[i].part_number = i + 1;
        parts[i].etag = NULL;
    }
    recorder->part_count = part_count;
    recorder->parts = parts;
cleanup:
    if (fp)
    {
        fclose(fp);
    }
    return ret;
}

// init the chunk upload
// https://developer.qiniu.com/kodo/6365/initialize-multipartupload
int qn_init_chunk_upload(const char *bucket_name, const char *upload_token, const char *file_key, qn_initchunkret *init_ret)
{
    int ret = 0;
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
    }
    size_t post_uri_len = strlen(QN_UPLOAD_HOST) + strlen(bucket_name) + strlen(object_name) + 26;
    post_uri = (char *)calloc(post_uri_len + 1, sizeof(char));
    snprintf(post_uri, post_uri_len + 1, "%s/buckets/%s/objects/%s/uploads", QN_UPLOAD_HOST, bucket_name, object_name);
    qn_debug("[Qiniu] init chunk upload uri: %s\n", post_uri);
    // make auth token
    size_t auth_token_len = strlen(upload_token) + strlen("Uptoken ");
    auth_token = (char *)calloc(auth_token_len + 1, sizeof(char));
    snprintf(auth_token, auth_token_len + 1, "UpToken %s", upload_token);
    // fire request
    request = ghttp_request_new();
    if (request == NULL)
    {
        init_ret->error = qn_strdup("new request error");
        ret = -1;
        goto cleanup;
    }
    ghttp_set_uri(request, post_uri);
    ghttp_set_header(request, "User-Agent", QN_USER_AGENT);
    ghttp_set_header(request, "Authorization", auth_token);
    ghttp_set_type(request, ghttp_type_post);
    ghttp_prepare(request);
    ghttp_status status = ghttp_process(request);
    init_ret->status_code = ghttp_status_code(request);
    if (status == ghttp_error)
    {
        init_ret->error = ghttp_get_error(request);
        ret = -1;
        goto cleanup;
    }
    // parse body
    int resp_body_len = ghttp_get_body_len(request);
    resp_body = (char *)calloc(resp_body_len + 1, sizeof(char));
    char *resp_body_end = qn_memconcat(resp_body, ghttp_get_body(request), resp_body_len);
    *resp_body_end = 0; // end it
    // parse resp json
    resp_json = cJSON_Parse(resp_body);
    if (resp_json == NULL)
    {
        init_ret->error = qn_strdup("parse resp body error");
        ret = -1;
        goto cleanup;
    }
    cJSON *err = cJSON_GetObjectItem(resp_json, "error");
    if (err)
    {
        const char *error = cJSON_GetStringValue(err);
        init_ret->error = qn_strdup(error);
        ret = -1;
        goto cleanup;
    }
    // get upload id and expired at
    cJSON *upload_id = cJSON_GetObjectItem(resp_json, "uploadId");
    cJSON *expire_at = cJSON_GetObjectItem(resp_json, "expireAt");
    const char *upload_id_str = cJSON_GetStringValue(upload_id);
    double expire_at_val = cJSON_GetNumberValue(expire_at);
    //  copy to init ret
    init_ret->upload_id = qn_strdup(upload_id_str);
    init_ret->expire_at = expire_at_val;
cleanup:
    free(post_uri);
    free(auth_token);
    if (file_key)
    {
        free(object_name);
    }
    if (request)
    {
        ghttp_request_destroy(request);
    }
    if (resp_json)
    {
        cJSON_Delete(resp_json);
    }
    if (resp_body)
    {
        free(resp_body);
    }
    return ret;
}

// upload the chunk bytes
// https://developer.qiniu.com/kodo/manual/12060/upload
int qn_upload_chunk_bytes(const char *bucket_name, const char *upload_token, const char *file_key, const char *upload_id,
                          int part_number, char *chunk_bytes, size_t chunk_size, qn_chunkpart *part)
{
    int ret = 0;
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
    }
    size_t post_uri_len = strlen(QN_UPLOAD_HOST) + strlen(bucket_name) + strlen(object_name) + strlen(upload_id) + 36;
    post_uri = (char *)calloc(post_uri_len, sizeof(char));
    sprintf(post_uri, "%s/buckets/%s/objects/%s/uploads/%s/%d", QN_UPLOAD_HOST, bucket_name, object_name, upload_id, part_number);
    qn_debug("[Qiniu] upload chunk bytes uri: %s\n", post_uri);
    // make auth token
    size_t auth_token_len = strlen(upload_token) + strlen("Uptoken ");
    auth_token = (char *)calloc(auth_token_len + 1, sizeof(char));
    snprintf(auth_token, auth_token_len + 1, "UpToken %s", upload_token);
    // make content length
    char content_length[7];
    sprintf(content_length, "%d", (int)chunk_size);
    // fire request
    request = ghttp_request_new();
    if (request == NULL)
    {
        part->error = qn_strdup("new request error");
        ret = -1;
        goto cleanup;
    }
    ghttp_set_uri(request, post_uri);
    ghttp_set_header(request, "User-Agent", QN_USER_AGENT);
    ghttp_set_header(request, "Authorization", auth_token);
    ghttp_set_header(request, "Content-Length", content_length);
    ghttp_set_type(request, ghttp_type_put);
    ghttp_set_body(request, chunk_bytes, chunk_size);
    ghttp_prepare(request);
    ghttp_status status = ghttp_process(request);
    part->status_code = ghttp_status_code(request);
    if (status == ghttp_error)
    {
        part->error = ghttp_get_error(request);
        ghttp_request_destroy(request);
        ret = -1;
        goto cleanup;
    }
    // parse body
    int resp_body_len = ghttp_get_body_len(request);
    resp_body = (char *)calloc(resp_body_len + 1, sizeof(char));
    char *resp_body_end = qn_memconcat(resp_body, ghttp_get_body(request), resp_body_len);
    *resp_body_end = 0; // end it
    // parse resp json
    resp_json = cJSON_Parse(resp_body);
    if (resp_json == NULL)
    {
        part->error = qn_strdup("parse resp body error");
        ret = -1;
        goto cleanup;
    }
    cJSON *err = cJSON_GetObjectItem(resp_json, "error");
    if (err)
    {
        const char *error = cJSON_GetStringValue(err);
        part->error = qn_strdup(error);
        ret = -1;
        goto cleanup;
    }
    // get etag
    cJSON *etag = cJSON_GetObjectItem(resp_json, "etag");
    const char *etag_str = cJSON_GetStringValue(etag);
    part->etag = qn_strdup(etag_str);
cleanup:
    free(post_uri);
    free(auth_token);
    if (file_key)
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
    return ret;
}

// complete the chunk upload
// https://developer.qiniu.com/kodo/6368/complete-multipart-upload
int qn_finish_chunk_upload(const char *local_path, const char *bucket_name, const char *upload_token, const char *file_key,
                           qn_chunkrecorder *recorder, qn_putextra *put_extra, qn_putret *put_ret)
{
    int ret = 0;
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
    }
    const char *upload_id = recorder->upload_id;
    size_t post_uri_len = strlen(QN_UPLOAD_HOST) + strlen(bucket_name) + strlen(object_name) + strlen(upload_id) + 27;
    post_uri = (char *)calloc(post_uri_len + 1, sizeof(char));
    snprintf(post_uri, post_uri_len + 1, "%s/buckets/%s/objects/%s/uploads/%s", QN_UPLOAD_HOST, bucket_name, object_name,
             upload_id);
    qn_debug("[Qiniu] finish chunk upload uri: %s\n", post_uri);
    // make auth token
    size_t auth_token_len = strlen(upload_token) + strlen("Uptoken ");
    auth_token = (char *)calloc(auth_token_len + 1, sizeof(char));
    snprintf(auth_token, auth_token_len + 1, "UpToken %s", upload_token);
    // make request body
    qn_chunkpayload req_body = {
        .error = NULL,
    };
    ret = qn_make_finish_chunk_payload(local_path, recorder, put_extra, &req_body);
    if (ret == -1)
    {
        put_ret->error = qn_strdup(req_body.error);
        goto cleanup;
    }
    // fire request
    request = ghttp_request_new();
    if (request == NULL)
    {
        put_ret->error = qn_strdup("new request error");
        ret = -1;
        goto cleanup;
    }
    ghttp_set_uri(request, post_uri);
    ghttp_set_header(request, "User-Agent", QN_USER_AGENT);
    ghttp_set_header(request, "Authorization", auth_token);
    ghttp_set_header(request, "Content-Type", "application/json");
    ghttp_set_type(request, ghttp_type_post);
    ghttp_set_body(request, req_body.bytes, strlen(req_body.bytes));
    ghttp_prepare(request);
    ghttp_status status = ghttp_process(request);
    put_ret->status_code = ghttp_status_code(request);
    if (status == ghttp_error)
    {
        put_ret->error = ghttp_get_error(request);
        ghttp_request_destroy(request);
        ret = -1;
        goto cleanup;
    }
    // parse body
    int resp_body_len = ghttp_get_body_len(request);
    resp_body = (char *)calloc(resp_body_len + 1, sizeof(char));
    char *resp_body_end = qn_memconcat(resp_body, ghttp_get_body(request), resp_body_len);
    *resp_body_end = 0; // end it
    // parse resp json
    resp_json = cJSON_Parse(resp_body);
    if (resp_json == NULL)
    {
        put_ret->error = qn_strdup("parse resp body error");
        ret = -1;
        goto cleanup;
    }
    cJSON *err = cJSON_GetObjectItem(resp_json, "error");
    if (err)
    {
        const char *error = cJSON_GetStringValue(err);
        put_ret->error = qn_strdup(error);
        ret = -1;
        goto cleanup;
    }
    // set put ret
    put_ret->resp_body = resp_body;
    put_ret->resp_body_len = resp_body_len;
cleanup:
    free(post_uri);
    free(auth_token);
    if (file_key)
    {
        free(object_name);
    }
    if (request)
    {
        ghttp_request_destroy(request);
    }
    if (resp_json)
    {
        cJSON_Delete(resp_json);
    }
    qn_free_chunkpayload(&req_body);
    return ret;
}

int qn_chunk_upload_file(const char *local_path, const char *bucket_name, const char *upload_token, const char *file_key,
                         qn_putextra *put_extra, qn_putret *put_ret)
{
    int ret = 0;
    FILE *local_file = NULL;
    const char *recorder_key;
    char *chunk_bytes = NULL;
    // init chunk upload
    qn_initchunkret initret = {
        .upload_id = NULL,
        .expire_at = 0,
        .error = NULL,
    };
    // resume upload recorder
    qn_chunkrecorder recorder = {
        .upload_id = NULL,
    };
    // get the recorder key
    if (put_extra && put_extra->recorder_key)
    {
        recorder_key = qn_strdup(put_extra->recorder_key);
        qn_debug("[Qiniu] use user specified recorder key: %s\n", recorder_key);
    }
    else
    {
        recorder_key = qn_get_default_recorder_key(local_path, bucket_name, file_key);
        qn_debug("[Qiniu] use auto generated recorder key: %s\n", recorder_key);
    }
    // init chunk recorder
    ret = qn_init_chunk_recorder(recorder_key, local_path, &recorder);
    if (ret == -1)
    {
        qn_debug("[Qiniu] init chunk recorder error: %s\n", recorder.error);
        put_ret->error = qn_strdup(recorder.error);
        goto cleanup;
    }
    if (!recorder.upload_id)
    {
        // init the chunk upload to get a new upload id
        ret = qn_init_chunk_upload(bucket_name, upload_token, file_key, &initret);
        if (ret == -1)
        {
            qn_debug("[Qiniu] init chunk upload error: %s\n", initret.error);
            put_ret->error = qn_strdup(initret.error);
            put_ret->status_code = initret.status_code;
            goto cleanup;
        }
        qn_debug("[Qiniu] init chunk upload success, upload_id: %s, expire_at: %ld\n", initret.upload_id, initret.expire_at);
        recorder.upload_id = qn_strdup(initret.upload_id);
        recorder.expire_at = initret.expire_at;
        // flush the recorder content to local file
        qn_flush_chunk_recorder(recorder_key, &recorder);
    }
    // open local file
    local_file = fopen(local_path, "rb");
    if (!local_file)
    {
        put_ret->error = qn_strdup("open local file error");
        ret = -1;
        goto cleanup;
    }
    // upload the file chunks
    chunk_bytes = (char *)calloc(QN_CHUNK_SIZE, sizeof(char));
    for (int i = 0; i < recorder.part_count; i++)
    {
        qn_chunkpart part = recorder.parts[i];
        if (part.etag == NULL)
        {
            fseek(local_file, i * QN_CHUNK_SIZE, SEEK_SET);
            size_t read_size = fread(chunk_bytes, 1, QN_CHUNK_SIZE, local_file);
            qn_debug("[Qiniu] uploading part %d/%d, size: %ld\n", part.part_number, recorder.part_count, read_size);
            ret = qn_upload_chunk_bytes(bucket_name, upload_token, file_key, recorder.upload_id, part.part_number,
                                        chunk_bytes, read_size, &part);
            qn_debug("[Qiniu] upload chunk part %d/%d, etag: %s, error: %s\n", part.part_number, recorder.part_count,
                     part.etag, part.error);
            if (ret == -1)
            {
                put_ret->error = qn_strdup(part.error);
                put_ret->status_code = part.status_code;
                if (strcmp(put_ret->error, "no such uploadId") == 0)
                {
                    // remove the recorder file when invalid
                    remove(recorder_key);
                }
                ret = -1;
                goto cleanup;
            }
            recorder.parts[i].etag = part.etag;
            qn_flush_chunk_recorder(recorder_key, &recorder);
        }
        else
        {
            qn_debug("[Qiniu] chunk part %d/%d has been uploaded, etag: %s\n", part.part_number, recorder.part_count, part.etag);
        }
    }
    // complete the chunk upload
    ret = qn_finish_chunk_upload(local_path, bucket_name, upload_token, file_key, &recorder, put_extra, put_ret);
    if (ret == -1)
    {
        qn_debug("[Qiniu] finish chunk upload error: %s\n", put_ret->error);
        if (strcmp(put_ret->error, "no such uploadId") == 0)
        {
            // remove the recorder file when invalid
            remove(recorder_key);
            goto cleanup;
        }
    }
    // remove the recorder file
    remove(recorder_key);
cleanup:
    free((void *)recorder_key);
    qn_free_initchunkret(&initret);
    qn_free_chunkrecorder(&recorder);
    if (local_file)
    {
        fclose(local_file);
    }
    if (chunk_bytes)
    {
        free(chunk_bytes);
    }
    return ret;
}