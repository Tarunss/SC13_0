// Asgn 2: A simple HTTP server.
// By: Eugene Chou
//     Andrew Quinn
//     Brian Zhao

#include "asgn2_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "response.h"
#include "request.h"
#include "queue.h"
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/file.h>
#include <sys/types.h>

// void handle_connection(uintptr_t, threadpool *p);
// void handle_get(conn_t *);
// void handle_put(conn_t *);
// void handle_unsupported(conn_t *);

//structs for threadpool

//task is what our worker_execute will take
// typedef struct Task {
//     void (*worker_execute)(void *args); // function that our threads are running
//     void *args; // thread also needs argument for functions
// } Task;

typedef struct worker_thread {
    pthread_t pthread; //actual thread
    struct threadpool *threadp; // pointer to threadpool this thread is contained in
} worker_thread;

typedef struct threadpool {
    worker_thread **threads; //array of threads in threadpool
    queue_t *bbuff; // queue to push all the thread tasks into
    pthread_mutex_t lock; // this lock is for blocking threads (put requests n stuff)
} threadpool;

//function prototypes:
void handle_connection(int, threadpool *p);
void handle_get(conn_t *, const Request_t *);
void handle_put(conn_t *, threadpool *, const Request_t *);
void handle_unsupported(conn_t *);
void worker_execute(struct worker_thread *threadp);
void audit_put(const char *req, char *uri, uint16_t status, char *requestID);
void audit_get(const char *req, char *uri, uint16_t status);

// audit function
// Should just print everything passed in to stderr
void audit_put(const char *req, char *uri, uint16_t status, char *requestID) {
    fprintf(stderr, "%s,%s,%d,%s\n", req, uri, status, requestID);
}

void audit_get(const char *req, char *uri, uint16_t status) {
    fprintf(stderr, "%s,%s,%d\n", req, uri, status);
}
//functions to operate on thread pool
struct threadpool *threadp_init(int numthreads) {
    //initialize threadpool structure
    //printf("CREATING THREADPOOL\n");
    threadpool *threadp;
    //allocate memory for threadpool
    threadp = (struct threadpool *) calloc(1, sizeof(threadpool));
    if (threadp == NULL) {
        fprintf(stderr, "failed to initialize threadpool object\n");
        return NULL;
    }
    //allocate memory for bounded buffer
    threadp->bbuff = queue_new(numthreads);
    //allocate memory for threads in the threadpool
    threadp->threads = (struct worker_thread **) calloc(1, sizeof(worker_thread *));
    if (threadp->threads == NULL) {
        fprintf(stderr, "failed to initialize threads object\n");
        return NULL;
    }
    //initialize mutex
    pthread_mutex_init(&threadp->lock, NULL);
    //initialize threads in threadpool
    for (int i = 0; i < numthreads; ++i) {
        //allocate memory to each single thread in threadpool
        threadp->threads[i] = (struct worker_thread *) calloc(1, sizeof(struct worker_thread));
        if (threadp->threads[i] == NULL) {
            fprintf(stderr, "problem allocating memory to single thread\n");
            return NULL;
        }
        threadp->threads[i]->threadp = threadp;

        //pthread create
        pthread_create(&threadp->threads[i]->pthread, NULL, (void *(*) (void *) ) worker_execute,
            (threadp->threads[i]));
    }
    //printf("THREADPOOL CREATED\n");

    return threadp;
}

//destroy threadpool
void threadp_destroy(threadpool *threadp, int numthreads) {
    //free memory allocated to array of threads
    for (int i = 0; i < numthreads; ++i) {
        free(threadp->threads[i]);
    }
    free(threadp->threads);
    threadp = NULL;
    //delete the job queue
    queue_delete(&threadp->bbuff);
}

//definition of worker thread (meant to mimic handle_connection, but thread safe)
void worker_execute(struct worker_thread *wthread) {
    bool isPopped;
    uintptr_t elem;
    while (1) {
        //printf("POPPING CONNECTION\n");
        //printf("worker going to work\n");
        isPopped = queue_pop(wthread->threadp->bbuff, (void **) &elem);
        //printf("CONNECTION POPPED\n");
        //cast conn into a uintptr
        if (!isPopped) {
            fprintf(stderr, "queue_pop failed\n");
            exit(EXIT_FAILURE);
        }
        //printf("%lu\n", (*(uintptr_t *) elem));
        handle_connection((*(int *) elem), wthread->threadp);
        free((void *) elem);
        //close(*(int *) elem);
    }
}

int main(int argc, char **argv) {
    //ensure valid port number
    long numthreads = 1;
    if (argc < 2) {
        //warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    //obtain number of threads from input
    int opt = 0;
    while ((opt = getopt(argc, argv, ":t:")) != -1) {
        //norma
        if (opt == 't') {
            //our thread # is in optargs
            numthreads = atol(optarg);
        }
    }
    // for (; optind < argc; optind++) {
    //     printf("extra arguments: %s\n", argv[optind]);
    // }
    //printf("%s\n", argv[optind]);
    //char *endptr = NULL;
    size_t port = atoi(argv[optind]);
    // printf("%li\n", numthreads);
    // printf("%zu\n", port);
    signal(SIGPIPE, SIG_IGN);
    //create our thread pool
    threadpool *tpool = threadp_init(numthreads);

    //initialize socket and connection
    Listener_Socket sock;
    listener_init(&sock, port);

    while (1) {
        //printf("accepting new connections\n");
        int connfd = listener_accept(&sock);
        if (connfd <= 0) {
            //fprintf(stderr, "Invalid connfd\n");
            continue;
        }
        //printf("obtained new connection to push: %d\n", connfd);
        //make space for pushing onto heap
        int *connfd2 = (int *) calloc(1, sizeof(int));
        *connfd2 = connfd;
        //make a new connection
        //conn_t *conn = conn_new(connfd);
        //printf("PUSHING CONNECTION\n");
        queue_push(tpool->bbuff, (void *) connfd2);
        //handle_connection(connfd);
    }
    //close(connfd);

    return EXIT_SUCCESS;
}

void handle_connection(int connfd, threadpool *p) {
    //printf("got to new connection: %d\n", connfd);
    conn_t *conn = conn_new(connfd);

    const Response_t *res = conn_parse(conn);
    // const char *msg = response_get_message(res);
    // char *header = NULL;
    // char *uri = conn_get_uri(conn);
    // header = conn_get_header(conn, "Request-Id");
    // printf("%s,%s,%s\n", msg, header, uri);

    if (res != NULL) {
        //printf("res is not null\n");

        conn_send_response(conn, res);
    } else {
        //debug("%s", conn_str(conn));
        //printf("res is null\n");

        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
            //get doesn't need threadpool, only needs a reader lock
            handle_get(conn, req);
        } else if (req == &REQUEST_PUT) {
            //put needs a file creation lock, so we pass threadpool in order to lock our thread
            //printf("handling put\n");
            handle_put(conn, p, req);
        } else {
            handle_unsupported(conn);
        }
    }
    // printf("end of handle_connection\n");
    conn_delete(&conn);
    close(connfd);
}

void handle_get(conn_t *conn, const Request_t *req) {

    char *uri = conn_get_uri(conn);
    //debug("GET request not implemented. But, we want to get %s", uri);
    const Response_t *res = NULL;
    // What are the steps in here?

    // 1. Open the file.
    int fd = open(uri, O_RDONLY);
    flock(fd, LOCK_SH);
    // If  open it returns < 0, then use the result appropriately
    //   a. Cannot access -- use RESPONSE_FORBIDDEN
    //   b. Cannot find the file -- use RESPONSE_NOT_FOUND
    //   c. other error? -- use RESPONSE_INTERNAL_SERVER_ERROR
    // (hint: check errno for these cases)!
    char *header;
    const char *reqstr;
    uint16_t status;
    struct stat path;
    int exists = stat(uri, &path);
    int bytes = path.st_size;
    if (exists < 0) {
        //debug("%s: %d", uri, errno);
        if (errno == EACCES) {
            res = &RESPONSE_FORBIDDEN;
            char requestID[2] = "0";
            header = conn_get_header(conn, "Request-Id");
            if (header == NULL) {
                header = requestID;
            }
            reqstr = request_get_str(req);
            status = response_get_code(res);
            audit_put(reqstr, uri, status, header);
            close(fd);
            goto out;
        } else if (errno == ENOENT) {
            res = &RESPONSE_NOT_FOUND;
            char requestID[2] = "0";
            header = conn_get_header(conn, "Request-Id");
            if (header == NULL) {
                header = requestID;
            }
            reqstr = request_get_str(req);
            status = response_get_code(res);
            audit_put(reqstr, uri, status, header);
            close(fd);
            goto out;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            char requestID[2] = "0";
            header = conn_get_header(conn, "Request-Id");
            if (header == NULL) {
                header = requestID;
            }
            reqstr = request_get_str(req);
            status = response_get_code(res);
            audit_put(reqstr, uri, status, header);
            close(fd);
            goto out;
        }
    }
    // 2. Get the size of the file.
    // (hint: checkout the function fstat)!
    // Get the size of the file.
    // 3. Check if the file is a directory, because directories *will*
    if (S_ISDIR(path.st_mode)) {
        res = &RESPONSE_FORBIDDEN;
        // obtain strings for audit
        //header = conn_get_header(conn, "Request-Id");
        char requestID[2] = "0";
        header = conn_get_header(conn, "Request-Id");
        if (header == NULL) {
            header = requestID;
        }
        reqstr = request_get_str(req);
        status = response_get_code(res);
        audit_put(reqstr, uri, status, header);
        close(fd);
        goto out;
    }
    // open, but are not valid.
    // (hint: checkout the macro "S_IFDIR", which you can use after you call fstat!)

    // 4. Send the file
    // (hint: checkout the conn_send_file function!)
    //conn_send_file(conn, fd, bytes);
    res = &RESPONSE_OK;
    char requestID[2] = "0";
    header = conn_get_header(conn, "Request-Id");
    if (header == NULL) {
        header = requestID;
    }
    reqstr = request_get_str(req);
    status = response_get_code(res);
    audit_put(reqstr, uri, status, header);
    conn_send_file(conn, fd, bytes);
    close(fd);
    goto end;
out:
    //conn_send_file(conn, fd, bytes);
    conn_send_response(conn, res);
end:
    return;
}

void handle_unsupported(conn_t *conn) {
    // debug("handling unsupported request");

    // send responses
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}

void handle_put(conn_t *conn, threadpool *p, const Request_t *req) {
    //lock thread so that we can flock the file
    pthread_mutex_lock(&p->lock);
    //pthread_mutex_unlock(&p->lock);
    char *uri = conn_get_uri(conn);
    char *header;
    const char *reqstr;
    uint16_t status;
    const Response_t *res = NULL;
    //debug("handling put request for %s", uri);
    int fd;
    // Check if file already exists before opening it.
    bool existed = access(uri, F_OK) == 0;
    //debug("%s existed? %d", uri, existed);
    //create the file
    fd = open(uri, O_CREAT | O_WRONLY, 0600);
    //exclusive lock on fd
    flock(fd, LOCK_EX);
    pthread_mutex_unlock(&p->lock);
    //truncate the file
    ftruncate(fd, 0);
    //pthread_mutex_unlock(&p->lock);
    //unlock file creation lock
    // Open the file (don't use O_TRUNC)
    //fd = open(uri, O_CREAT | O_TRUNC | O_WRONLY, 0600);

    if (fd < 0) {
        //debug("%s: %d", uri, errno);
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            res = &RESPONSE_FORBIDDEN;
            char requestID[2] = "0";
            header = conn_get_header(conn, "Request-Id");
            if (header == NULL) {
                header = requestID;
            }
            reqstr = request_get_str(req);
            status = response_get_code(res);
            audit_put(reqstr, uri, status, header);
            close(fd);
            goto out;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            char requestID[2] = "0";
            header = conn_get_header(conn, "Request-Id");
            if (header == NULL) {
                header = requestID;
            }
            reqstr = request_get_str(req);
            status = response_get_code(res);
            audit_put(reqstr, uri, status, header);
            close(fd);
            goto out;
        }
    }

    res = conn_recv_file(conn, fd);

    if (res == NULL && existed) {
        res = &RESPONSE_OK;
    } else if (res == NULL && !existed) {
        res = &RESPONSE_CREATED;
    }
    char requestID[2] = "0";
    header = conn_get_header(conn, "Request-Id");
    if (header == NULL) {
        header = requestID;
    }
    reqstr = request_get_str(req);
    status = response_get_code(res);
    audit_put(reqstr, uri, status, header);
    close(fd);
out:
    conn_send_response(conn, res);
}
