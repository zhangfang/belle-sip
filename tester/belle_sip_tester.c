/*
	belle-sip - SIP (RFC3261) library.
    Copyright (C) 2010  Belledonne Communications SARL

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "belle_sip_tester.h"

#include <stdio.h>
#include "CUnit/Basic.h"
#if HAVE_CU_CURSES
#include "CUnit/CUCurses.h"
#endif
#include <belle-sip/belle-sip.h>

extern const char *test_domain;
extern const char *auth_domain;


static test_suite_t **test_suite = NULL;
static int nb_test_suites = 0;


#if HAVE_CU_CURSES
	static unsigned char curses = 0;
#endif


static void add_test_suite(test_suite_t *suite) {
	if (test_suite == NULL) {
		test_suite = (test_suite_t **)malloc(10 * sizeof(test_suite_t *));
	}
	test_suite[nb_test_suites] = suite;
	nb_test_suites++;
	if ((nb_test_suites % 10) == 0) {
		test_suite = (test_suite_t **)realloc(test_suite, (nb_test_suites + 10) * sizeof(test_suite_t *));
	}
}

static int run_test_suite(test_suite_t *suite) {
	int i;

	CU_pSuite pSuite = CU_add_suite(suite->name, suite->init_func, suite->cleanup_func);

	for (i = 0; i < suite->nb_tests; i++) {
		if (NULL == CU_add_test(pSuite, suite->tests[i].name, suite->tests[i].func)) {
			return CU_get_error();
		}
	}

	return 0;
}

static int test_suite_index(const char *suite_name) {
	int i;

	for (i = 0; i < belle_sip_tester_nb_test_suites(); i++) {
		if ((strcmp(suite_name, test_suite[i]->name) == 0) && (strlen(suite_name) == strlen(test_suite[i]->name))) {
			return i;
		}
	}

	return -1;
}

int belle_sip_tester_nb_test_suites(void) {
	return nb_test_suites;
}

int belle_sip_tester_nb_tests(const char *suite_name) {
	int i = test_suite_index(suite_name);
	if (i < 0) return 0;
	return test_suite[i]->nb_tests;
}

const char * belle_sip_tester_test_suite_name(int suite_index) {
	if (suite_index >= belle_sip_tester_nb_test_suites()) return NULL;
	return test_suite[suite_index]->name;
}

const char * belle_sip_tester_test_name(const char *suite_name, int test_index) {
	int suite_index = test_suite_index(suite_name);
	if ((suite_index < 0) || (suite_index >= belle_sip_tester_nb_test_suites())) return NULL;
	if (test_index >= test_suite[suite_index]->nb_tests) return NULL;
	return test_suite[suite_index]->tests[test_index].name;
}

static int _belle_sip_tester_ipv6_available(void){
	struct addrinfo *ai=belle_sip_ip_address_to_addrinfo(AF_INET6,"2a01:e00::2",53);
	if (ai){
		struct sockaddr_storage ss;
		struct addrinfo src;
		socklen_t slen=sizeof(ss);
		char localip[128];
		int port=0;
		belle_sip_get_src_addr_for(ai->ai_addr,ai->ai_addrlen,(struct sockaddr*) &ss,&slen,4444);
		src.ai_addr=(struct sockaddr*) &ss;
		src.ai_addrlen=slen;
		belle_sip_addrinfo_to_ip(&src,localip, sizeof(localip),&port);
		freeaddrinfo(ai);
		return strcmp(localip,"::1")!=0;
	}
	return FALSE;
}

static int ipv6_available=0;

int belle_sip_tester_ipv6_available(void){
	return ipv6_available;
}

void belle_sip_tester_init(void) {
	belle_sip_object_enable_marshal_check(TRUE);
	ipv6_available=_belle_sip_tester_ipv6_available();
	add_test_suite(&cast_test_suite);
	add_test_suite(&sip_uri_test_suite);
	add_test_suite(&generic_uri_test_suite);
	add_test_suite(&headers_test_suite);
	add_test_suite(&core_test_suite);
	add_test_suite(&sdp_test_suite);
	add_test_suite(&resolver_test_suite);
	add_test_suite(&message_test_suite);
	add_test_suite(&authentication_helper_test_suite);
	add_test_suite(&register_test_suite);
	add_test_suite(&dialog_test_suite);
	add_test_suite(&refresher_test_suite);
	add_test_suite(&http_test_suite);
}



void belle_sip_tester_uninit(void) {
	if (test_suite != NULL) {
		free(test_suite);
		test_suite = NULL;
		nb_test_suites = 0;
	}
}
int belle_sip_tester_run_tests(const char *suite_name, const char *test_name) {
	int i,ret;
	belle_sip_object_pool_t *pool;
	
	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	for (i = 0; i < belle_sip_tester_nb_test_suites(); i++) {
		run_test_suite(test_suite[i]);
	}
	pool=belle_sip_object_pool_push();
	
#if HAVE_CU_GET_SUITE
	if (suite_name){
		CU_pSuite suite;
		CU_basic_set_mode(CU_BRM_VERBOSE);
		suite=CU_get_suite(suite_name);
		if (suite==NULL){
			fprintf(stderr,"There is no suite named '%s'",suite_name);
			exit(-1);
		}
		if (test_name) {
			CU_pTest test=CU_get_test_by_name(test_name, suite);
			if (test==NULL){
				fprintf(stderr,"There is no test named '%s'",suite_name);
				exit(-1);
			}
			CU_basic_run_test(suite, test);
		} else
			CU_basic_run_suite(suite);
	} else
#endif
	{
#if HAVE_CU_CURSES
		if (curses) {
			/* Run tests using the CUnit curses interface */
			CU_curses_run_tests();
		}
		else
#endif
		{
			/* Run all tests using the CUnit Basic interface */
			CU_basic_set_mode(CU_BRM_VERBOSE);
			CU_basic_run_tests();
		}
	}

	belle_sip_object_unref(pool);
	ret=CU_get_number_of_tests_failed()!=0;
	CU_cleanup_registry();
	return ret;
}


#ifndef WINAPI_FAMILY_PHONE_APP
int main (int argc, char *argv[]) {
	int i;
	int ret;
	char *suite_name=NULL;
	char *test_name=NULL;
	const char *env_domain=getenv("TEST_DOMAIN");


	if (env_domain)
		test_domain=env_domain;

	for(i=1;i<argc;++i){
		if (strcmp(argv[i],"--help")==0){
				fprintf(stderr,"%s \t--help\n"
						"\t\t\t--verbose\n"
						"\t\t\t--domain <test sip domain>\n"
						"\t\t\t--auth-domain <test auth domain>\n"
#if HAVE_CU_GET_SUITE
						"\t\t\t--suite <suite name>\n"
						"\t\t\t--test <test name>\n"
#endif
#if HAVE_CU_CURSES
						"\t\t\t--curses\n"
#endif
						, argv[0]);
				return 0;
		}else if (strcmp(argv[i],"--verbose")==0){
			belle_sip_set_log_level(BELLE_SIP_LOG_DEBUG);
		}else if (strcmp(argv[i],"--domain")==0){
			i++;
			test_domain=argv[i];
		}
		else if (strcmp(argv[i],"--auth-domain")==0){
					i++;
					auth_domain=argv[i];
		}
#if HAVE_CU_GET_SUITE
		else if (strcmp(argv[i],"--test")==0){
			i++;
			test_name=argv[i];
		}else if (strcmp(argv[i],"--suite")==0){
			i++;
			suite_name=argv[i];
		}
#endif
#if HAVE_CU_CURSES
		else if (strcmp(argv[i], "--curses") == 0) {
			i++;
			curses = 1;
		}
#endif
	}

	belle_sip_tester_init();
	ret = belle_sip_tester_run_tests(suite_name, test_name);
	belle_sip_tester_uninit();
	return ret;
}
#endif
