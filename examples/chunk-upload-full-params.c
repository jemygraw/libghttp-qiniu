

#include "ghttp-qiniu.h"

int main(int argc, char **argv)
{
    qn_map *custom_vars = (qn_map *)malloc(sizeof(qn_map) * 2);
    qn_map *p = custom_vars;
    // the name must starts with x:
    p->key = "x:name";
    p->value = "qiniu";
    p++;
    p->key = "x:age";
    p->value = "28";

    qn_map *metadata = (qn_map *)malloc(sizeof(qn_map) * 1);
    qn_map *q = metadata;
    // the name must starts with x-qn-meta-
    q->key = "x-qn-meta-org-name";
    q->value = "qiniu cloud";

    char *bucket_name = "xdb-backup";
    char *local_path = "sample.mp4";
    char *upload_token = "1nSNkZ-ZvkpdQcBWAn8fZjJIFeLBd4WCNwVO3-id:Yhj5hehO29mM4_nPshthc_mI3F0=:eyJzY29wZSI6InhkYi1iYWNrdXAiLCJkZWFkbGluZSI6MTc0NDYxNjI0Nn0=";
    char *file_key = "qiniu/sample.mp4";
    char *mime_type = "video/mp4";

    qn_putret put_ret = {
        .error = NULL,
        .resp_body = NULL,
    };

    qn_putextra putextra = {
        .mime_type = mime_type,
        .custom_vars = custom_vars,
        .custom_vars_count = 2,
        .metadata = metadata,
        .metadata_count = 1,
    };

    int ret = qn_chunk_upload_file(local_path, bucket_name, upload_token, file_key, &putextra, &put_ret);
    printf("==> upload result: %d, error=%s\n", ret, put_ret.error);
    printf("==> status code:%d\n", put_ret.status_code);
    printf("==> resp body: %s\n", put_ret.resp_body);
    qn_free_putret(&put_ret);
}