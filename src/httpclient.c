/* This is a managed file. Do not delete this comment. */

#include <corto/httpclient/httpclient.h>
#include <curl/curl.h>
#define INITIAL_BODY_BUFFER_SIZE (512)

corto_buffer g_logBuffer;
bool g_logSet = false;

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

static
int httpclient_log(
    CURL *handle,
    curl_infotype type,
    char *data,
    size_t size,
    void *userp);

void httpclient_log_config(CURL *curl)
{
    if (corto_verbosityGet() <= CORTO_TRACE) {
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, httpclient_log);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        g_logSet = false;
        corto_buffer_reset(&g_logBuffer);
        g_logBuffer = CORTO_BUFFER_INIT;
    }
}

void httpclient_log_print(void)
{
    if (g_logSet) {
        corto_trace("LibCurl:\n%s====> LibCurl Complete.",
            corto_buffer_str(&g_logBuffer));
        g_logSet = false;
    }
}

httpclient_Result httpclient_get(
    corto_string url,
    corto_string fields)
{
    httpclient_Result result = {0, NULL};
    CURL *curl = curl_easy_init();
    if (!curl) {
        corto_seterr("could not init curl");
        goto error;
    }

    /* Build URL with Fields concatenated as parameters */
    corto_string urlParams = NULL;
    if ((fields) && (strlen(fields) > 0)) {
        urlParams = corto_asprintf("%s&%s", url, fields);
    }

    struct url_data data = {0, NULL};
    data.buffer = corto_alloc(INITIAL_BODY_BUFFER_SIZE);
    if (!data.buffer) {
        goto error;
    }

    httpclient_log_config(curl);

    data.buffer[0] = '\0';
    if (urlParams) {
        curl_easy_setopt(curl, CURLOPT_URL, urlParams);
    }

    else {
        curl_easy_setopt(curl, CURLOPT_URL, url);
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        corto_seterr("curl_easy_perform() failed: %s", curl_easy_strerror(res));
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status);
    result.response = data.buffer;
    curl_easy_cleanup(curl);
    if (urlParams) {
        corto_dealloc(urlParams);
    }

    httpclient_log_print();

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
        corto_seterr("Could not init curl");
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

    httpclient_log_config(curl);

    data.buffer[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        corto_seterr("curl_easy_perform() failed: %s", curl_easy_strerror(res));
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status);
    result.response = data.buffer;
    curl_easy_cleanup(curl);

    httpclient_log_print();

    return result;
error:
    return (httpclient_Result){0, NULL};
}

int clientMain(int argc, char *argv[]) {

    curl_global_init(CURL_GLOBAL_DEFAULT);

    return 0;
}

int httpclientMain(int argc, char *argv[]) {

    /* Insert implementation */

    return 0;
}

corto_string httpclient_encode_fields(
    corto_string fields)
{
    corto_string encoded = curl_easy_escape(NULL, fields, 0);
    return encoded;
}

int httpclient_log(
    CURL *handle,
    curl_infotype type,
    char *data,
    size_t size,
    void *userp)
{
    g_logSet = true;
    (void)handle; /* satisfy compiler warning */
    (void)userp;

    switch (type) {
        case CURLINFO_TEXT:
            // corto_info("libcurl InfoText: %s", data);
            corto_buffer_appendstr(&g_logBuffer, "Info: ");
            break;
        default: /* in case a new one is introduced to shock us */
            corto_error("Unhandled LibCurl InfoType: \n%s", data);
            return 0;

        case CURLINFO_HEADER_OUT:
            corto_buffer_appendstr(&g_logBuffer, "libcurl => Send header");
            break;
        case CURLINFO_DATA_OUT:
            corto_buffer_appendstr(&g_logBuffer, "libcurl => Send data");
            break;
        case CURLINFO_SSL_DATA_OUT:
            corto_buffer_appendstr(&g_logBuffer, "libcurl => Send SSL data");
            break;
        case CURLINFO_HEADER_IN:
            corto_buffer_appendstr(&g_logBuffer, "libcurl => Recv header");
            break;
        case CURLINFO_DATA_IN:
            corto_buffer_appendstr(&g_logBuffer, "libcurl => Recv data");
            break;
        case CURLINFO_SSL_DATA_IN:
            corto_buffer_appendstr(&g_logBuffer, "libcurl => Recv SSL data");
            break;
    }

    /* Uncomment to debug byte size resolution.
    corto_string bytes = corto_asprintf(" [%ld bytes]", (long)size);
    corto_buffer_appendstr(&buffer, bytes);
    corto_dealloc(bytes);
    */
    corto_buffer_appendstr(&g_logBuffer, data);

    // corto_trace("%s", corto_buffer_str(&buffer));

    return 0;
}
