

#include "ghttp_qiniu.h"

int main(int argc, char **argv)
{
    char *bucket_name = "xdb-backup";
    char *local_path = "sample.mp4";
    char *upload_token = "xxx";
    char *file_key = NULL;

    qn_putret put_ret = {
        .error = NULL,
        .resp_body = NULL,
    };

    int ret = qn_chunk_upload_file(local_path, bucket_name, upload_token, file_key, NULL, &put_ret);
    printf("==> upload result: %d, error=%s\n", ret, put_ret.error);
    if (ret == 0)
    {
        printf("==> status code: %d\n", put_ret.status_code);
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
 }