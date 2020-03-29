#include "http.h"
#include "altcp.h"
#include "cmsis_os.h"
#include <string.h>
#include "lwip_fs.h"

static err_t http_poll(void *arg, struct altcp_pcb *pcb);

static http_request_s *http_request_new(http_s *server){

	http_request_s *req = pvPortMalloc(sizeof(http_request_s));

	req->server 	 = server;
	req->res_buf 	 = NULL;
	req->res_buf_len = 0;

	return req;

}

static http_request_s *http_request_free(http_request_s *req){

	req->server 	 = NULL;
	req->res_buf 	 = NULL;
	req->res_buf_len = 0;

	vPortFree(req);

	return req;

}

static void http_close(struct altcp_pcb *pcb, http_request_s *req){

	altcp_arg(pcb, NULL);
	altcp_recv(pcb, NULL);
	altcp_sent(pcb, NULL);
	altcp_poll(pcb, NULL, 0);
	altcp_err(pcb,  NULL);

	if(req != NULL){
		http_request_free(req);
	}

	if(altcp_close(pcb) != ERR_OK){
		altcp_poll(pcb, http_poll, 4);
	}

}

static err_t http_send(struct altcp_pcb *pcb, http_request_s *req){

	err_t err = ERR_OK;

	uint16_t max_len, chunk_len = LWIP_MIN(req->res_buf_len, 0xffff);

	if(chunk_len <= 0){
		return ERR_OK;
	}

	max_len = altcp_sndbuf(pcb);
	if (max_len < chunk_len) {
		chunk_len = max_len;
	}

	max_len = HTTPD_MAX_WRITE_LEN(pcb);
	if (chunk_len > max_len) {
		chunk_len = max_len;
	}

	do {
		err = altcp_write(pcb, req->res_buf, chunk_len, 0);
		if (err == ERR_MEM) {
			if ((altcp_sndbuf(pcb) == 0) || (altcp_sndqueuelen(pcb) >= TCP_SND_QUEUELEN)) {
				chunk_len = 1;
			} else {
				chunk_len /= 2;
			}
		}
	} while ((err == ERR_MEM) && (chunk_len > 1));

	if (err == ERR_OK) {
		req->res_buf_len -= chunk_len;
		req->res_buf += chunk_len;
	}

	if(req->res_buf_len == 0){
		http_close(pcb, req);
	}

	return err;

}

static err_t http_sent(void *arg, struct altcp_pcb *pcb, uint16_t len){
	http_request_s *req = arg;
	if(req != NULL){
		http_send(pcb, req);
	}
	return ERR_OK;
}

static err_t http_poll(void *arg, struct altcp_pcb *pcb){

	http_request_s *req = arg;

	  if (req == NULL) {
	    http_close(pcb, NULL);
	    return ERR_OK;
	  } else {
	    req->retries++;
	    if (req->retries == HTTPD_MAX_RETRIES) {
	      http_close(pcb, req);
	      return ERR_OK;
	    }
	    if (req->res_buf != NULL) {
	      if (http_send(pcb, req)) {
	        altcp_output(pcb);
	      }
	    }
	  }

	  return ERR_OK;
}

static void http_error(void *arg, err_t err){
	http_request_s *req = arg;
	if (req != NULL) {
		http_request_free(req);
	}
}


static err_t http_parse_request(http_request_s *req, struct pbuf *p){

	if(req->res_buf != NULL){
	    return ERR_USE;
	}

	struct pbuf *raw_req = p;
	char *req_buf = p->payload;
	uint16_t req_buf_len = p->len;

	pbuf_ref(p);

	if(lwip_strnstr(req_buf , HTTP_REQUEST_HEADER_DELIMITOR, req_buf_len) != NULL){

		http_method_e method = HTTP_GET;
		uint8_t len 		 = 0;

		for(method=HTTP_GET; method<HTTP_DELETE; method++){
			if(!strncmp(req_buf, http_methods[method].str, http_methods[method].len)){
				req_buf += http_methods[method].len + 1;
				break;
			}
		}

		do{
			if(req_buf[len] == 0){ break; }
		}while(req_buf[len++] != ' ');

		uint8_t param_count = 0;
		http_param_s *params = pvPortMalloc(sizeof(http_param_s));
		char *value_stop;
		char *params_str = lwip_strnstr(req_buf, HTTP_REQUEST_PARAMS_DELIMITOR, len);

		if(params_str !=NULL){

			len = strlen(req_buf) - strlen(params_str);
			params_str++;

			do{
				char *value_start = lwip_strnstr(params_str, "=", len) + 1;
				value_stop = lwip_strnstr(params_str, "&", len);
				params = pvPortRealloc(params, sizeof(http_param_s) * ++param_count);
				uint8_t arg_len = &params_str - &value_start - 1;
				uint8_t val_len = &value_stop - &value_start;
				params[param_count-1].arg = pvPortMalloc(arg_len * sizeof(char));
				params[param_count-1].val = pvPortMalloc(val_len * sizeof(char));
				memmove(params[param_count-1].arg, params_str, arg_len);
				memmove(params[param_count-1].val, value_start, val_len);
				params_str = value_stop + 1;
			}while(value_stop != NULL);

		}

		char uri[len-1];
		memmove(uri, req_buf, len-1);
		uri[len-1] = 0;

		req_buf += len;


		char *header_field_values[HTTP_REQ_HEADER_CONTENT_LOCATION];

		for(uint8_t i=HTTP_REQ_HEADER_CONTENT_LENGTH; i< HTTP_REQ_HEADER_CONTENT_LOCATION; i++){
			char *header_field_start = lwip_strnstr(req_buf, http_req_header_field_str[i], strlen(req_buf));
			if(header_field_start != NULL){
				header_field_start += strlen(http_req_header_field_str[i]);
				char *header_field_stop = lwip_strnstr(header_field_start, HTTP_REQUEST_HEADER_SEPARATOR, strlen(header_field_start));
				uint8_t header_field_value_length = header_field_stop - header_field_start;
				header_field_values[i] = pvPortMalloc(header_field_value_length);
				memmove(header_field_values[i], header_field_start, header_field_value_length);
			}else{
				header_field_values[i] = NULL;
			}
		}

		char *content = lwip_strnstr(req_buf, HTTP_REQUEST_HEADER_DELIMITOR, strlen(req_buf));
		content += strlen(HTTP_REQUEST_HEADER_DELIMITOR);

		if(req->server->router != NULL){
			req->res_buf_len = req->server->router(uri, header_field_values, content, params, &req->res_buf);
		}

		//Freeing dynamically allocated header data
		for(uint8_t i=0; i<param_count; i++){
			vPortFree(params[i].arg);
			vPortFree(params[i].val);
		}

		vPortFree(params);
		params = NULL;

		for(uint8_t i=HTTP_REQ_HEADER_CONTENT_LENGTH; i< HTTP_REQ_HEADER_CONTENT_LOCATION; i++){
			if(header_field_values[i] != NULL){
				vPortFree(header_field_values[i]);
				header_field_values[i] = NULL;
			}
		}

		pbuf_free(raw_req);

	}

	return ERR_OK;

}

static err_t http_receive(void *arg, struct altcp_pcb *pcb,struct pbuf *p, err_t err){

	http_request_s *req = arg;

	if(err != ERR_OK || p == NULL || req == NULL){
		if(p != NULL){							//An error has occured
			altcp_recved(pcb, p->tot_len);		//signaling request reception
			pbuf_free(p);						//Freeing buffer on receive error
		}
		http_close(pcb, req);					//Received NULL data -> FIN, close connection
	}else{
		altcp_recved(pcb, p->tot_len);			//Signaling request reception
		if(req->res_buf == NULL){
			err = http_parse_request(req, p);
			pbuf_free(p);
			if(err == ERR_OK){
				http_send(pcb, req);
			}
		}else{
			pbuf_free(p);
		}

	}

	return ERR_OK;

}

static err_t http_accept(void *arg, struct altcp_pcb *pcb, err_t err){

	http_s *this = arg;

	if(err != ERR_OK || pcb == NULL){
		err = ERR_VAL;
	}else{

		//IF IT DOESN'T WORK MIGHT BE CAUSE OF THE ARG PROBLEM
		http_request_s *req = http_request_new(this);

		if(req == NULL){
			return ERR_MEM;
		}

		altcp_arg(pcb, req);
		altcp_recv(pcb, http_receive);
		altcp_err(pcb, http_error);
		altcp_sent(pcb, http_sent);
		altcp_poll(pcb, http_poll, 4);
	}

	return err;

}

static void http_prepare_pcb(http_s *this){

	struct altcp_pcb *pcb;

	pcb = altcp_tcp_new_ip_type(IPADDR_TYPE_ANY);

	altcp_bind(pcb, IP_ANY_TYPE, this->port);

	pcb = altcp_listen(pcb);

	this->pcb = pcb;

	altcp_setprio(pcb, 1);
	altcp_arg(pcb, this);
	altcp_accept(pcb, http_accept);

}


http_s *http_init(uint16_t port, router_fn router){

	http_s *this = pvPortMalloc(sizeof(http_s));

	this->router = router;
	this->port = port;

	http_prepare_pcb(this);

	return this;

}
