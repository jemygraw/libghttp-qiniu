

#include "ghttp-qiniu.h"

int main(int argc, char **argv)
{
    char *bucket_name = "xdb-backup";
    char *local_path = "sample.mp4";
    char *upload_token = "1nSNkZ-ZvkpdQcBWAn8fZjJIFeLBd4WCNwVO3-id:Yhj5hehO29mM4_nPshthc_mI3F0=:eyJzY29wZSI6InhkYi1iYWNrdXAiLCJkZWFkbGluZSI6MTc0NDYxNjI0Nn0=";
    char *file_key = NULL;

    qn_putret put_ret = {
        .error = NULL,
        .resp_body = NULL,
    };

    int ret = qn_chunk_upload_file(local_path, bucket_name, upload_token, file_key, NULL, &put_ret);
    printf("==> upload result: %d, error=%s\n", ret, put_ret.error);
    printf("==> status code:%d\n", put_ret.status_code);
    printf("==> resp body: %s\n", put_ret.resp_body);
    qn_free_putret(&put_ret);
}