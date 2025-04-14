

#include "ghttp_qiniu.h"
#include <pthread.h>

void *upload_file(void *arg)
{
    char *bucket_name = "xdb-backup";
    char *local_path = "files/sample.mp4";
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
    pthread_exit((void *)ret);
}

int main(int argc, char **argv)
{
    pthread_t thread_id;
    int arg = 123;
    void *thread_result;
    if (pthread_create(&thread_id, NULL, upload_file, (void *)&arg) != 0)
    {
        perror("create thread failed");
        return 1;
    }

    // 等待线程结束
    if (pthread_join(thread_id, &thread_result) != 0)
    {
        perror("wait thread failed");
        return 1;
    }

    printf("thread result: %ld\n", (long)thread_result);
}