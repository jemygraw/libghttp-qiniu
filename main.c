#include <stdio.h>
#include <stdlib.h>
#include "ghttp-qiniu/ghttp-qiniu.h"

int main() {
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

    char *local_path = "/Users/jemy/Documents/qiniu.png";
    char *upload_token = "<upload token get from remote api server>";
    char *file_key = "qiniu-test2.png";
    char *mime_type = "image/png";

    qn_putret put_ret;

    //upload with file key, mime type, extra params
    int ret = qn_upload_file(local_path, upload_token, file_key, mime_type, params, 2, &put_ret);
    printf("upload result: %d\n", ret);
    printf("upload status: %d\n", put_ret.status_code);
    printf("upload response len: %d\n", put_ret.resp_body_len);
    printf("upload response: %s\n", put_ret.resp_body);
    //free
    free(put_ret.resp_body);

    //upload without file key, use hash as file key
    ret = qn_upload_file(local_path, upload_token, NULL, NULL, NULL, 0, &put_ret);
    printf("upload result: %d\n", ret);
    printf("upload status: %d\n", put_ret.status_code);
    printf("upload response len: %d\n", put_ret.resp_body_len);
    printf("upload response: %s\n", put_ret.resp_body);
    //free
    free(put_ret.resp_body);

    //free extra params
    free(params);
    return 0;
}