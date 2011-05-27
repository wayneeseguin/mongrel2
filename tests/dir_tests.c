#include "minunit.h"
#include "dir.h"
#include "register.h"
#include <string.h>
#include <fcntl.h>

FILE *LOG_FILE = NULL;

char *test_Dir_find_file()
{
    bstring ctype = NULL;

    FileRecord *file = Dir_find_file(bfromcstr("tests/sample.json"),
            ctype = bfromcstr("text/plain"));

    mu_assert(file != NULL, "Failed to find the file.");

    FileRecord_destroy(file);
    bdestroy(ctype);

    return NULL;
}


char *test_Dir_resolve_file()
{
    Dir *test = Dir_create("tests/", "sample.html", "test/plain", 0);
    mu_assert(test != NULL, "Failed to make test dir.");

    FileRecord *rec = Dir_resolve_file(test, bfromcstr("/"), bfromcstr("/sample.json"));
    mu_assert(rec != NULL, "Failed to resolve file that should be there.");

    rec = Dir_resolve_file(test, bfromcstr("/"), bfromcstr("/"));
    mu_assert(rec != NULL, "Failed to find default file.");

    rec = Dir_resolve_file(test, bfromcstr("/"), bfromcstr("/../../../../../etc/passwd"));
    mu_assert(rec == NULL, "HACK! should not find this.");

    Dir_destroy(test);

    test = Dir_create("foobar/", "sample.html", "test/plan", 0);
    mu_assert(test != NULL, "Failed to make the failed dir.");

    rec = Dir_resolve_file(test, bfromcstr("/"), bfromcstr("/sample.json"));
    mu_assert(rec == NULL, "Should not get something from a bad base directory.");

    Dir_destroy(test);

    return NULL;
}

const char *REQ_PATTERN = "%s %s HTTP/1.1\r\n\r\n";

Request *fake_req(const char *method, const char *prefix, const char *path)
{
    int rc = 0;
    size_t nparsed = 0;
    Request *req = Request_create();
    Request_start(req);

    bstring p = bfromcstr(path);
    bstring rp = bformat(REQ_PATTERN, method, bdata(p));

    rc = Request_parse(req, bdata(rp), blength(rp), &nparsed);
    req->prefix = bfromcstr(prefix);
    req->pattern = bfromcstr(prefix);

    check(rc != 0, "Failed to parse request.");
    check(nparsed == blength(rp), "Failed to parse all of request.");

    return req;

error:
    return NULL;
}


char *test_Dir_serve_file()
{
    int rc = 0;
    Request *req = NULL;

    Dir *test = Dir_create("tests/", "sample.html", "test/plain", 0);

    Connection conn = {0};
    int zero_fd = open("/dev/null", O_WRONLY);
    conn.iob = IOBuf_create(1024, zero_fd, IOBUF_NULL);

    req = fake_req("GET", "/", "/sample.json");
    rc = Dir_serve_file(test, req, &conn);
    // TODO: different platforms barf on sendfile for different reasons
    // mu_assert(req->response_size > -1, "Should serve the /sample.json");

    req = fake_req("HEAD", "/", "/sample.json");
    rc = Dir_serve_file(test, req, &conn);
    mu_assert(rc == 0, "Should serve the HEAD of /sample.json");

    req = fake_req("POST", "/", "/sample.json");
    rc = Dir_serve_file(test, req, &conn);
    mu_assert(rc == -1, "POST should pass through but send an error.");

    return NULL;
}

char *test_Dir_serve_big_files(){

  struct stat less2gb;
  less2gb.st_size = 1564355066; /* ~1.5GB */

  struct stat more2gb;
  more2gb.st_size = 399560397; /* ~4GB */
  
  char *LESS_2GB = "HTTP/1.1 200 OK\r\n"
    "Date: \r\n"
    "Content-Type: \r\n"
    "Content-Length: 1564355066\r\n"
    "Last-Modified: \r\n"
    "ETag: \r\n"
    "Server: " VERSION
    "\r\n\r\n";
  
  char *MORE_2GB = "HTTP/1.1 200 OK\r\n"
    "Date: \r\n"
    "Content-Type: \r\n"
    "Content-Length: 399560397\r\n"
    "Last-Modified: \r\n"
    "ETag: \r\n"
    "Server: " VERSION
    "\r\n\r\n";

  bstring response_less_2gb = bformat(RESPONSE_FORMAT, "", "", less2gb.st_size, "", "");
  bstring response_more_2gb = bformat(RESPONSE_FORMAT, "", "", more2gb.st_size, "", "");

  mu_assert(bstrcmp(response_less_2gb, bfromcstr(LESS_2GB)) == 0, "Wrong size for <2GB files");
  mu_assert(bstrcmp(response_more_2gb,  bfromcstr(MORE_2GB)) == 0, "Wrong size for >2GB files");
  return NULL;
}

char * all_tests() {
    mu_suite_start();
    Register_init();

    mu_run_test(test_Dir_find_file);
    mu_run_test(test_Dir_serve_file);
    mu_run_test(test_Dir_resolve_file);
    mu_run_test(test_Dir_serve_big_files);

    return NULL;
}

RUN_TESTS(all_tests);

