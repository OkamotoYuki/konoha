/****************************************************************************
 * Copyright (c) 2012, the Konoha project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

/* ************************************************************************ */
#ifndef DSE_H_
#define DSE_H_

#include <event.h>
#include <evhttp.h>
#include <event2/buffer.h>
#include <sys/queue.h>
#include <actor/actor.h>
#include <jansson.h>
#include <konoha2/konoha2.h>
#include "dse_util.h"
struct dDserv {
	struct event_base *base;
	struct evhttp *httpd;
};


struct dReq {
	int method;
	int context;
	int taskid;
	char logpoolip[16];
	char *scriptfilepath;
};

struct dRes {
	int taskid;
	int status;
	char *status_symbol;
	char *status_detail;
};

enum {
	E_METHOD_EVAL,
	E_METHOD_TYCHECK,
	E_STATUS_OK,
};

#define DSE_FILEPATH_SIZE 128

static struct dReq *newDReq()
{
	struct dReq *ret = (struct dReq *)malloc(sizeof(struct dReq));
	ret->method = 0;
	ret->context = 0;
	ret->taskid = 0;
	memset(ret->logpoolip, 16, 0);
	ret->scriptfilepath = (char*)dse_malloc(DSE_FILEPATH_SIZE);
	memset(ret->scriptfilepath, 128, 0);
	return ret;
}

static void deleteDReq(struct dReq *req)
{
	if (req == NULL) return;
	dse_free(req->scriptfilepath, DSE_FILEPATH_SIZE);
	dse_free(req, sizeof(struct dReq));
}

static struct dRes *newDRes()
{
	struct dRes *ret = (struct dRes *)dse_malloc(sizeof(struct dRes));
	ret->taskid = 0;
	ret->status = 0;
	ret->status_detail = NULL;
	ret->status_symbol = NULL;
	return ret;
}

static void deleteDRes (struct dRes *res)
{
	// check if satus_* is set
	if (res == NULL) return;
	dse_free(res, sizeof(struct dRes));
}


/* ************************************************************************ */
#define JSON_INITGET(O, K) \
		json_t *K = json_object_get(O, #K)

static struct dReq *dse_parseJson(const char *input)
{
	json_t *root;
	json_error_t error;

	root = json_loads(input, 0, &error);
	if(!root) {
		D_("error occurs");
		return NULL;
	}
	JSON_INITGET(root, taskid);
	JSON_INITGET(root, type);
	JSON_INITGET(root, context);
	JSON_INITGET(root, method);
	JSON_INITGET(root, logpool);
	JSON_INITGET(root, script);

	struct dReq *ret = newDReq();
	if(strncmp(json_string_value(method), "eval", 4) == 0) {
		ret->method = E_METHOD_EVAL;
	}
	else if(strncmp(json_string_value(method), "tycheck", 7) == 0) {
		ret->method = E_METHOD_TYCHECK;
	}
	else{
		D_("error");
		return NULL;
	}
	// store file
	char *filename = ret->scriptfilepath;
	const char *str_context = json_string_value(context);
	size_t context_len = strlen(str_context);
	strncpy(filename, str_context, context_len);
	strncat(filename, ".k", context_len + 2);
	FILE *fp = fopen(filename, "w");
	const char *str_script= json_string_value(script);
	size_t script_len = strlen(str_script);
	fwrite(str_script, script_len, 1, fp);
	fflush(fp);
	fclose(fp);
	json_decref(root);
	return ret;
}

static void eval_actor_init(Actor *a) { /* do nothing */ }
static void eval_actor_exit(Actor *a) { /* do nothing */ }

static int eval_actor_act(Actor *a, Message *message)
{
    if (JSON_type(message) == JSON_String) {
		char *filepath = JSONString_get(a->self, 6);
		filepath[5] = '\0';
		konoha_t konoha = konoha_open();
		int ret = konoha_load(konoha, filepath);
		konoha_close(konoha);
	}
    return 0;
}

static const struct actor_api eval_actor_api = {
    eval_actor_act,
    eval_actor_init,
    eval_actor_exit
};

static void eval_actor(struct dReq *req)
{
    if (ActorStage_init(4, 4)) {
        fprintf(stderr, "ActorStage initialization failed.\n");
        return 1;
    }

	JSON message = JSONInt_new(1);
	Actor *a = Actor_new(message, &eval_actor_api);
	Actor_act(a);
	Actor_send(a, JSONString_new(req->scriptfilepath, strlen(req->scriptfilepath)));
}

static struct dRes *dse_dispatch(struct dReq *req)
{
//	konoha_t konoha = konoha_open();
	int ret;
	D_("scriptpath:%s", req->scriptfilepath);
	struct dRes *dres = newDRes();
	switch (req->method){
	case E_METHOD_EVAL: case E_METHOD_TYCHECK:
//		ret = konoha_load(konoha, req->scriptfilepath);
		eval_actor(req);
		if(ret == 1) {
			// ok;
			dres->status = E_STATUS_OK;
		}
		break;
	//case E_METHOD_TYCHECK:
	//	break;
	default:
		D_("there's no such method");
		break;
	}
	return dres;
}


static void dse_send_reply(struct evhttp_request *req, struct dRes *dres)
{
	struct evbuffer *buf = evbuffer_new();
	switch(dres->status){
	case E_STATUS_OK:
		evhttp_send_reply(req, HTTP_OK, "OK", buf);
		break;
	default:
		break;
	}
}

/* ************************************************************************ */

void dump_http_header(struct evhttp_request *req, struct evbuffer *evb, void *ctx)
{
	struct evkeyvalq *head = evhttp_request_get_input_headers(req);
	struct evkeyval *entry;
	TAILQ_FOREACH(entry, head, next) {
		evbuffer_add_printf(evb, "%s:%s<br>", entry->key, entry->value);
	}
	evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "http://localhost:8080/");
	evhttp_add_header(req->output_headers, "Access-Control-Max-Age", "6238800");
	evhttp_add_header(req->output_headers, "Access-Control-Allow-Method", "POST");

	evhttp_send_reply(req, HTTP_OK, "OK", evb);
}

// request handler for DSE Protocol
void dse_req_handler (struct evhttp_request *req, void *arg)
{
	struct evbuffer *body = evhttp_request_get_input_buffer(req);
	size_t len = evbuffer_get_length(body);
	struct dReq *dreq = NULL;
	struct dRes *dres = NULL;
	if (req->type == EVHTTP_REQ_GET) {
		evhttp_send_error(req, HTTP_BADREQUEST, "DSE server doesn't accept GET request");
	} else if(req->type == EVHTTP_REQ_POST) {
		// now, we fetch key values
		unsigned char *requestLine;
		requestLine = evbuffer_pullup(body, -1);
		requestLine[len] = '\0';
		//now, parse json.
		dreq = dse_parseJson((const char*)requestLine);
		dres = dse_dispatch(dreq);
		dse_send_reply(req, dres);
		//evbuffer_add_printf(buf, "Reqested POSPOS: %s\n", evhttp_request_uri(req));
		//evhttp_send_reply(req, HTTP_OK, "OK", buf);

	}
	else{
		evhttp_send_error(req, HTTP_BADREQUEST, "Available POST only");
	}
	deleteDReq(dreq);
	deleteDRes(dres);
}
static struct dDserv *dserv_new(void)
{
	struct dDserv *dserv = dse_malloc(sizeof(struct dDserv));
	dserv->base = event_base_new();
	dserv->httpd = evhttp_new(dserv->base);
	return dserv;
}

static int dserv_start(struct dDserv *dserv, const char *addr, int ip)
{
	if (evhttp_bind_socket(dserv->httpd, addr, ip) < 0){
		perror("evhttp_bind_socket");
		exit(EXIT_FAILURE);
	}
	evhttp_set_gencb(dserv->httpd, dse_req_handler, NULL);
	event_base_dispatch(dserv->base);
	return 0;
}


static void dserv_close(struct dDserv *dserv)
{
	evhttp_free(dserv->httpd);
	event_base_free(dserv->base);
	dse_free(dserv, sizeof(struct dDserv));
}

#endif /* DSE_H_ */
