#include <stdio.h>
#include <stdlib.h>
#include "ghttp_qiniu.h"

int main(int argc, char **argv)
{
    // set custom vars
    qn_map *custom_vars = (qn_map *)malloc(sizeof(qn_map) * 2);
    qn_map *p = custom_vars;
    // the name must starts with x:
    p->key = "x:name";
    p->value = "qiniu";
    p++;
    p->key = "x:age";
    p->value = "28";

    // set metadata
    qn_map *metadata = (qn_map *)malloc(sizeof(qn_map) * 1);
    qn_map *q = metadata;
    // the name must starts with x-qn-meta-
    q->key = "x-qn-meta-org-name";
    q->value = "qiniu cloud";
    char *local_path = "files/sample.png";
    char *upload_token = "1nSNkZ-ZvkpdQcBWAn8fZjJIFeLBd4WCNwVO3-id:xgNcYVYqZWpi_c_2Jtp84WFAbeE=:eyJzY29wZSI6InhkYi1iYWNrdXAiLCJkZWFkbGluZSI6MTc0NDYyNzAyNn0=";
    char *file_key = "sample.png";
    char *mime_type = "image/png";

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

    // upload with file key, mime type, extra custom_vars
    int ret = qn_upload_file(local_path, upload_token, file_key, &putextra, &put_ret);
    printf("==> upload result: %d, error=%s\n", ret, put_ret.error);
    if (ret == 0)
    {
        printf("==> status code:%d \n", put_ret.status_code);
        printf("==> resp body: %s\n", put_ret.resp_body);
    }
    else
    {
        printf("==> upload error: %s\n", put_ret.error);
    }
    // TODO
    // parse put_ret->resp_body to json
    //
    // free putret
    qn_free_putret(&put_ret);

    // free extra params
    free(custom_vars);
    free(metadata);
    return 0;
}