

#include "ghttp-qiniu.h"

int main(int argc, char **argv) {
    qn_map *params = (qn_map *) malloc(sizeof(qn_map) * 2);
    qn_map *p = params;
    p->key = "x:name"; // the name must starts with x:
    p->value = "qiniu";
    p++;
    p->key = "x:age";
    p->value = "28";

    p = params;
    for (int i = 0; i < 2; i++) {
        printf("%s\t%s\n", p->key, p->value);
        p += 1;
    }

    char *bucket_name="test";
    char *local_path = "test.txt";
    char *upload_token = "xxx";
    char *file_key = "qiniu/test.txt";
    char *mime_type = "text/plain";

    qn_putret put_ret={
        .error=NULL,
        .resp_body=NULL,
    };

    qn_putextra putextra = {
        .mime_type=mime_type,
        .extra_params=params,
        .extra_params_count=2,
    };

    int ret = qn_chunk_upload_file(local_path,bucket_name, upload_token, file_key, &putextra, &put_ret);
    printf("%d\n",ret);
    printf("upload result: %d, error=%s\n", ret, put_ret.error);
    qn_free_putret(&put_ret);
}