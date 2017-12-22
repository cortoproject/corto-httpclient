/* This is a managed file. Do not delete this comment. */

#include <corto/httpclient/httpclient.h>

#include <curl/curl.h>

static int client_Url_onData(
    char *data,
    size_t size,
    size_t nmemb,
    httpclient_Url *this)
{
    corto_int32 resultLength = this->response ? strlen(this->response) : 0;
    corto_int32 length = resultLength + size * nmemb;

    corto_string str = malloc(length + 1);

    if (this->response) {
        memcpy(str, this->response, resultLength);
    }

    memcpy(&str[resultLength], data, size * nmemb);

    str[length] = '\0';

    corto_dealloc(this->response);
    this->response = str;

    return size * nmemb;
}

corto_string httpclient_Url_request(
    httpclient_Url* this)
{
    CURL *conn;
    CURLcode res;

    corto_set_str(&this->response, NULL);

    conn = curl_easy_init();
    if (conn) {
        res = curl_easy_setopt(conn, CURLOPT_URL, this->address);
        if (res != CURLE_OK) {
            corto_throw("url/request set URL failed: %s\n",
                curl_easy_strerror(res));
            goto error;
        }

        res = curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, client_Url_onData);
        if (res != CURLE_OK) {
            corto_throw("url/request set callback failed: %s\n",
                curl_easy_strerror(res));
            goto error;
        }

       res = curl_easy_setopt(conn, CURLOPT_WRITEDATA, this);
       if (res != CURLE_OK) {
           corto_throw("url/request set writedata failed: %s\n",
               curl_easy_strerror(res));
           goto error;
       }

        res = curl_easy_perform(conn);
        if(res != CURLE_OK) {
            corto_throw("url/request failed: %s\n",
                curl_easy_strerror(res));
            goto error;
        }

        curl_easy_cleanup(conn);
    }

    return corto_strdup(this->response);
error:
    return NULL;
}
