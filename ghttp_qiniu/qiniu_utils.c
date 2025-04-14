#include "ghttp_qiniu.h"

// print the debug message when set QINIU_DEBUG=1
// export QINIU_DEBUG=1 in your shell and default to 0
void qn_debug(const char *format, ...)
{
    const char *debug_on = getenv("QINIU_DEBUG");
    if (!debug_on || strcmp(debug_on, "1") == -1)
    {
        return;
    }
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

// create a duplicate string for save memory cleanup
char *qn_strdup(const char *src)
{
    char *dst = NULL;
    if (src == NULL)
    {
        return NULL;
    }
    dst = (char *)calloc(strlen(src) + 1, sizeof(char));
    if (dst == NULL)
    {
        return NULL;
    }
    strncpy(dst, src, strlen(src));
    return dst;
}

// create a file base name string
char *qn_file_basename(const char *file_path)
{
    char *file_name = NULL;
    if (file_path == NULL)
    {
        return NULL;
    }
    file_name = strrchr(file_path, '/');
    if (file_name == NULL)
    {
        return qn_strdup(file_path);
    }
    return qn_strdup(file_name + 1);
}

// create a fixed length of random string
char *qn_random_str(int len)
{
    int i = 0, val = 0;
    const char *base = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int base_len = (int)strlen(base);

    char *random_str = (char *)malloc(sizeof(char) * (len + 1));
    srand((unsigned int)time(NULL));
    for (; i < len; i++)
    {
        val = 1 + (int)((float)(base_len - 1) * rand() / (RAND_MAX + 1.0));
        random_str[i] = base[val];
    }
    random_str[len] = 0;
    return random_str;
}

/*
 * form body concatenation function
 *
 * @param dst_buffer     a buffer used to make concatenation of strings
 * @param src_buffer     a buffer to be concated to the dst_buffer
 * @param src_buffer_len src_buffer length
 *
 * @return the end pointer of the dst_buffer, used for the next concatenation
 * */
char *qn_memconcat(char *dst_buffer, const char *src_buffer, size_t src_buffer_len)
{
    memcpy(dst_buffer, src_buffer, src_buffer_len);
    char *p_end = dst_buffer + src_buffer_len;
    return p_end;
}

// for qn_putret object memory cleanup
void qn_free_putret(qn_putret *put_ret)
{
    if (put_ret == NULL)
    {
        return;
    }
    if (put_ret->resp_body)
    {
        free(put_ret->resp_body);
    }
    if (put_ret->error)
    {
        free((void *)put_ret->error);
    }
}