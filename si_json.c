/*
 * JSON formatter
 */

#include <json-c/json.h>
#include <string.h>

#include <stdio.h>

#include "si_base.h"

const char *stat_errdesc[] = {
	"OK",
	"Race not found",
	"Bad authentication",
	"No connection to server",
	"Error in program",
	"Unknown state"
};

const char *si_stat_errdesc(int err){
	return stat_errdesc[err];
}

char *si_init_json(void){
	json_object *jo, *ji;
	const char *result;
	char *jsonstr;

	jo = json_object_new_object();
	ji = json_object_new_string("init");

	json_object_object_add(jo, "init", ji);
	result = json_object_to_json_string(jo);

	if((jsonstr = (char *) malloc(strlen(result)+1)) == NULL){
		json_object_put(jo);
		return NULL;
	}
	strcpy(jsonstr, result);
	json_object_put(jo);
	return jsonstr;
}

char *si_data_json(struct s_sidata *card){
	json_object *jo, *ji, *jj, *ja;
	int i;
	const char *result;
	char *jsonstr;

	jo = json_object_new_object();
	
	ji = json_object_new_int(card->cardnum);
	json_object_object_add(jo, "si_number", ji);

	ji = json_object_new_int(card->cardtype);
	json_object_object_add(jo, "si_type", ji);

	if(card->lname){
		ji = json_object_new_string(card->lname);
		json_object_object_add(jo, "si_lname", ji);
	}
	if(card->fname){
		ji = json_object_new_string(card->fname);
		json_object_object_add(jo, "si_fname", ji);
	}
	ji = json_object_new_int(card->start.time);
	json_object_object_add(jo, "tm_start", ji);
	ji = json_object_new_int(card->check.time);
	json_object_object_add(jo, "tm_check", ji);
	ji = json_object_new_int(card->clear.time);
	json_object_object_add(jo, "tm_clear", ji);
	ji = json_object_new_int(card->finish.time);
	json_object_object_add(jo, "tm_finish", ji);

	ja = json_object_new_array();
	for(i = 0; i < card->npunch; i++){
		jj = json_object_new_object();
		ji = json_object_new_int(card->punches[i].cn);
		json_object_object_add(jj, "cn", ji);
		ji = json_object_new_int(card->punches[i].time);
		json_object_object_add(jj, "time", ji);
		ji = json_object_new_int(card->punches[i].timestat);
		json_object_object_add(jj, "tst", ji);
		json_object_array_add(ja, jj);
	}
	json_object_object_add(jo, "punches", ja);

	result = json_object_to_json_string(jo);

	if((jsonstr = (char *) malloc(strlen(result)+1)) == NULL){
		json_object_put(jo);
		return NULL;
	}
	strcpy(jsonstr, result);
	json_object_put(jo);
	return jsonstr;
}

int si_getstatus_json(char *jsonstr){
	json_object *ji;
    enum json_type type;
	
	ji = json_tokener_parse(jsonstr);

	if(! json_object_is_type(ji, json_type_object)){
		json_object_put(ji);
		return STAT_ERROR;
	}

	json_object_object_foreach(ji, key, val) { /*Passing through every array element*/
		type = json_object_get_type(val);
		if(strcmp(key, "status") == 0 && type == json_type_string){
			if(strcmp(json_object_get_string(val), "OK") == 0){
				return STAT_OK;
			}
		}
		if(strcmp(key, "error") == 0 && type == json_type_string){
			if(strcmp(json_object_get_string(val), "NoRace") == 0){
				return STAT_NORACE;
			}else if(strcmp(json_object_get_string(val), "NoAuth") == 0){
				return STAT_NOAUTH;
			}
		}
	}
	return STAT_UNKNOWN;
}

char *si_getstring_json(char *jsonstr, char *retkey){
	json_object *ji;
    enum json_type type;
	char *str;
	
	ji = json_tokener_parse(jsonstr);

	if(! json_object_is_type(ji, json_type_object)){
		json_object_put(ji);
		return NULL;
	}
	json_object_object_foreach(ji, key, val) { /*Passing through every array element*/
		type = json_object_get_type(val);
		if(strcmp(key, retkey) == 0 && type == json_type_string) {
			if((str = (char *) malloc((strlen(json_object_get_string(val)) + 1) * sizeof(char))) == NULL) {
				json_object_put(ji);
				return NULL;
			}else{
				strcpy(str, json_object_get_string(val));
				json_object_put(ji);
				return str;
			}
		}
	}
	return NULL;
}
