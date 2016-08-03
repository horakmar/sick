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

/* Make init JSON string */
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

/* Make punch details JSON - private helper */
json_object * make_json_punch(struct s_punch *punch){
	json_object *jp, *ji;

	jp = json_object_new_object();
	ji = json_object_new_int(punch->cn);
	json_object_object_add(jp, "cn", ji);
	ji = json_object_new_int(punch->time);
	json_object_object_add(jp, "time", ji);
	ji = json_object_new_int(punch->timestat);
	json_object_object_add(jp, "tst", ji);
	return jp;
}

/* Make data JSON string from punch data */
char *si_data_json(struct s_sidata *card){
	json_object *jo, *ji, *jp, *ja;
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

	jp = make_json_punch(&card->clear);
	json_object_object_add(jo, "clear", jp);
	jp = make_json_punch(&card->check);
	json_object_object_add(jo, "check", jp);
	jp = make_json_punch(&card->start);
	json_object_object_add(jo, "start", jp);
	jp = make_json_punch(&card->finish);
	json_object_object_add(jo, "finish", jp);

	ja = json_object_new_array();
	for(i = 0; i < card->npunch; i++){
		jp = make_json_punch(&card->punches[i]);
		json_object_array_add(ja, jp);
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

/* Get punch details - private helper */
void get_json_punch(struct s_punch *punch, json_object *jp){
	enum json_type type;

	json_object_object_foreach(jp, keyp, valp) {
		type = json_object_get_type(valp);
		if(strcmp(keyp, "time") == 0 && type == json_type_int){
			punch->time = json_object_get_int(valp);
		}
		if(strcmp(keyp, "tst") == 0 && type == json_type_int){
			punch->timestat = json_object_get_int(valp);
		}
		if(strcmp(keyp, "cn") == 0 && type == json_type_int){
			punch->cn = json_object_get_int(valp);
		}
	}
	json_object_put(jp);

}

/* Parse JSON string and return punch data */
struct s_sidata *si_json_data(struct s_sidata *card, char *jsonstr){
	json_object *ja, *ji, *jp;
    enum json_type type;
	int i;

	ji = json_tokener_parse(jsonstr);
	if(! json_object_is_type(ji, json_type_object)){
		json_object_put(ji);
		return NULL;
	}

	json_object_object_foreach(ji, key, val) {
		type = json_object_get_type(val);

		if(strcmp(key, "si_number") == 0 && type == json_type_int){
			card->cardnum = json_object_get_int(val);
		}
		if(strcmp(key, "si_type") == 0 && type == json_type_int){
			card->cardtype = json_object_get_int(val);
		}
		if(strcmp(key, "si_lname") == 0 && type == json_type_string){
			strcpy(card->lname, json_object_get_string(val));
		}
		if(strcmp(key, "si_fname") == 0 && type == json_type_string){
			strcpy(card->fname, json_object_get_string(val));
		}
		if(strcmp(key, "tm_start") == 0 && type == json_type_int){
			card->start.time = json_object_get_int(val);
		}
		if(strcmp(key, "tm_check") == 0 && type == json_type_int){
			card->check.time = json_object_get_int(val);
		}
		if(strcmp(key, "tm_clear") == 0 && type == json_type_int){
			card->clear.time = json_object_get_int(val);
			card->clear.timestat = H24;
		}
		if(strcmp(key, "clear") == 0 && type == json_type_object){
			json_object_object_get_ex(ji, key, &jp);
			get_json_punch(&card->clear, jp);
		}
		if(strcmp(key, "check") == 0 && type == json_type_object){
			json_object_object_get_ex(ji, key, &jp);
			get_json_punch(&card->check, jp);
		}
		if(strcmp(key, "start") == 0 && type == json_type_object){
			json_object_object_get_ex(ji, key, &jp);
			get_json_punch(&card->start, jp);
		}
		if(strcmp(key, "finish") == 0 && type == json_type_object){
			json_object_object_get_ex(ji, key, &jp);
			get_json_punch(&card->finish, jp);
		}
		if(strcmp(key, "tm_finish") == 0 && type == json_type_int){
			card->finish.time = json_object_get_int(val);
		}

		if(strcmp(key, "punches") == 0 && type == json_type_array){
			json_object_object_get_ex(ji, key, &ja);
			card->npunch = json_object_array_length(ja);
			for(i = 0; i < card->npunch; i++) {
				jp = json_object_array_get_idx(ja, i);
				get_json_punch(&card->punches[i], jp);
			}
		}
	}
	return card;
}



/* Parse status response from server and return status */
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

/* Parse JSON string and return string for specific key */
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
