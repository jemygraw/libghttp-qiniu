#include <stdio.h>
#include <stdlib.h>
#include <ghttp-qiniu.h>

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

    char *local_path = "examples/chunk-upload.c";
    char *upload_token = "xxx";
    char *file_key = "qiniu/chunk-upload.c";
    char *mime_type = "text/plain";

    qn_putret put_ret={
        .error=NULL,
        .resp_body=NULL,
    };

    qn_putextra putextra = {
        .mime_type = mime_type,
        .extra_params = params,
        .extra_params_count = 2,
    };

    // upload with file key, mime type, extra params
    printf("upload with all params\n");
    int ret = qn_upload_file(local_path, upload_token, file_key, &putextra, &put_ret);
    printf("upload result: %d\n", ret);
    if (ret == 0)
    {
        printf("upload status: %d\n", put_ret.status_code);
        printf("upload response len: %d\n", put_ret.resp_body_len);
        printf("upload response: %s\n", put_ret.resp_body);
        // free
        qn_free_putret(&put_ret);
    }
    else
    {
        printf("upload error: %s\n", put_ret.error);
    }
    exit(1);
    printf("\n--------\n");
    // upload without file key, use hash as file key
    for (int i = 0; i < 10; i++)
    {
        ret = qn_upload_file(local_path, upload_token, NULL, NULL, &put_ret);
        printf("upload result: %d\n", ret);
        if (ret == 0)
        {
            printf("upload status: %d\n", put_ret.status_code);
            printf("upload response len: %d\n", put_ret.resp_body_len);
            printf("upload response: %s\n", put_ret.resp_body);
            // free
            qn_free_putret(&put_ret);
        }
        else
        {
            printf("upload error: %s\n", put_ret.error);
        }
        printf("\n");
    }

    // free extra params
    free(params);
    return 0;
}