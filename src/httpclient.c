/* This is a managed file. Do not delete this comment. */

#include <corto/httpclient/httpclient.h>
#include <curl/curl.h>
#define INITIAL_BODY_BUFFER_SIZE (512)
struct url_data {
    size_t size;
    char* buffer;
};
size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data) {
    size_t index = data->size;
    size_t n = (size * nmemb);
    char* tmp;
    data->size += (size * nmemb);
    tmp = corto_realloc(data->buffer, data->size + 1);
    if (tmp) {
        data->buffer = tmp;
    } else {
        if (data->buffer) {
            corto_dealloc(data->buffer);
        }

        goto error;
    }

    memcpy((data->buffer + index), ptr, n);
    data->buffer[data->size] = '\0';
    return size * nmemb;
error:
    return 0;
}

httpclient_Result httpclient_get(
    corto_string url,
    corto_string fields)
{
    httpclient_Result result = {0, NULL};
    CURL *curl = curl_easy_init();
    if (!curl) {
        corto_throw("could not init curl");
        goto error;
    }

    struct url_data data = {0, NULL};
    data.buffer = corto_alloc(INITIAL_BODY_BUFFER_SIZE);
    if (!data.buffer) {
        goto error;
    }

    data.buffer[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        corto_throw("curl_easy_perform() failed: %s", curl_easy_strerror(res));
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status);
    result.response = data.buffer;
    curl_easy_cleanup(curl);
    return result;
error:
    return (httpclient_Result){0, NULL};
}

httpclient_Result httpclient_post(
    corto_string url,
    corto_string fields)
{
    httpclient_Result result = {0, NULL};
    CURL* curl = curl_easy_init();
    if (!curl) {
        corto_throw("Could not init curl");
        goto error;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (fields) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(fields));
    }

    struct url_data data = {0, NULL};
    data.buffer = corto_alloc(INITIAL_BODY_BUFFER_SIZE);
    if (!data.buffer) {
        goto error;
    }

    data.buffer[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        corto_throw("curl_easy_perform() failed: %s", curl_easy_strerror(res));
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status);
    result.response = data.buffer;
    curl_easy_cleanup(curl);
    return result;
error:
    return (httpclient_Result){0, NULL};
}

int cortomain(int argc, char *argv[]) {

    /* Insert implementation */

    return 0;
}
