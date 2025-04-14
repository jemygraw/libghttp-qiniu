

#include "ghttp-qiniu.h"

int main(int argc, char **argv)
{
    qn_map *params = (qn_map *)malloc(sizeof(qn_map) * 2);
    qn_map *p = params;
    p->key = "x:name"; // the name must starts with x:
    p->value = "qiniu";
    p++;
    p->key = "x:age";
    p->value = "28";

    p = params;
    for (int i = 0; i < 2; i++)
    {
        printf("%s\t%s\n", p->key, p->value);
        p += 1;
    }

    char *bucket_name = "xdb-backup";
    char *local_path = "sample.mp4";
    char *upload_token = "sample-tokn";
    char *file_key = "qiniu/sample.mp4";
    char *mime_type = "video/mp4";

    qn_putret put_ret = {
        .error = NULL,
        .resp_body = NULL,
    };

    qn_putextra putextra = {
        .mime_type = mime_type,
        .custom_vars = params,
        .custom_vars_count = 2,
    };

    int ret = qn_chunk_upload_file(local_path, bucket_name, upload_token, file_key, &putextra, &put_ret);
    printf("==> upload result: %d, error=%s\n", ret, put_ret.error);
    printf("==> status code:%d\n", put_ret.status_code);
    printf("==> resp body:%s\n", put_ret.resp_body);
    qn_free_putret(&put_ret);
}