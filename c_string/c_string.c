#include "c_string.h"

//=================================== FUNCTIONS ==============================================


void str_obj_set(void* src_str, c_obj_t* dst_obj){
    
    dst_obj->value = calloc(strlen((char*)src_str) + 1, sizeof(char));
    strcpy(dst_obj->value, (char*)src_str);
    dst_obj->obj_type = _STR_ID;
}

void* str_obj_get(c_obj_t src_obj, void* dst_str){
    size_t src_len = strlen((char*)(src_obj.value));

    (*(char**)(dst_str)) = realloc((*(char**)(dst_str)), src_len+1);

    strcpy((*(char**)(dst_str)), (char*)(src_obj.value));

    return NULL;
}

int32_t str_obj_cmp(void* a, void* b, ...){
    return strcmp((char*)(a), (char*)(b));
}

int32_t str_obj_print(void* src, ...){
    return printf("%s", (char*)(src));
}

size_t str_count(const char* src_str, const char* value){
    size_t val_len = strlen(value), count = 0;
    
    for(size_t i = 0; (i + val_len) <= strlen(src_str); i++){
    	if(!strncmp(&src_str[i], value, val_len)){
    		i += val_len - 1;
    		count++;
		}
	}

    return count;
}

size_t str_split(const char* src_str, const char* value, char*** dest_list){
    size_t val_len = strlen(value);
    size_t count = 0, last_pos = 0, i;
    
    (*dest_list) = NULL;
    
    for(i = 0; (i + val_len) <= strlen(src_str); i++){
    	if(!strncmp(&src_str[i], value, val_len)){
            (*dest_list) = (char**)realloc((*dest_list), (count + 1)*sizeof(char*));
            (*dest_list)[count] = (char*)calloc(i - last_pos + 1, sizeof(char));
            strncpy((*dest_list)[count], &src_str[last_pos], i - last_pos);
            last_pos = i + val_len;
    		i += val_len - 1;
            count++;
		}
	}

    if(last_pos < strlen(src_str) - 1){
        (*dest_list) = (char**)realloc((*dest_list), (count + 1)*sizeof(char*));
        (*dest_list)[count] = (char*)calloc(i - last_pos + 1, sizeof(char));
        strcpy((*dest_list)[count], &src_str[last_pos]);
        count++;
    }

    return count;
}

size_t str_join(char** dest_str, char* join_str, char** str_list, size_t item_count){
    
    size_t count = 0, i;
    (*dest_str) = (char*)calloc(1, sizeof(char));

    for(i = 0; i < item_count - 1; i++){
        count += strlen(str_list[i]);
        (*dest_str) = (char*)realloc((*dest_str), count*sizeof(char));
        strcat((*dest_str), str_list[i]);
        strcat((*dest_str), join_str);
    }
    
    count += strlen(str_list[i]);
    (*dest_str) = (char*)realloc((*dest_str), count*sizeof(char));
    strcat((*dest_str), str_list[i]);

    return count;
}

size_t str_vjoin(char** dest_str, char* join_str, ...){
    size_t output_len = 0;
    (*dest_str) = NULL;
    
    va_list args;
    va_start(args, join_str);

    char* tmp = va_arg(args, char*);
    
    while(strcmp(tmp, END_LIST) != 0){
        output_len += strlen(tmp) + strlen(join_str);
        (*dest_str) = (char*)realloc((*dest_str), output_len);
        strcat((*dest_str), tmp);
        strcat((*dest_str), join_str);
        tmp = NULL;
        tmp = va_arg(args, char*);
    };

    output_len -= strlen(join_str);
    (*dest_str) = (char*)realloc((*dest_str), output_len);
    (*dest_str)[output_len] = '\0';

    va_end(args);

    return output_len;
}

size_t str_search(const char* src_str, const char* value, size_t** dest_pos, size_t occurrence_num){
    
    char* buffer = (char*)calloc(strlen(src_str), sizeof(char));
    strcpy(buffer, src_str);

    if(*dest_pos != NULL){
        free(*dest_pos);
        *dest_pos = NULL;
    }

    size_t count = 0;

    if(!occurrence_num){

        buffer = strstr(buffer, value);
        while(buffer != NULL){
            count++;
            (*dest_pos) = (size_t*)realloc((*dest_pos), count);
            (*dest_pos)[count-1] = (size_t)(strlen(src_str) - strlen(buffer));
            buffer = strstr(&buffer[1], value);
        }

        return count;
    }else{

        buffer = strstr(buffer, value);
        while((buffer != NULL) && (count < occurrence_num)){
            count++;
            (*dest_pos) = (size_t*)realloc((*dest_pos), count);
            (*dest_pos)[count - 1] = (size_t)(strlen(src_str) - strlen(buffer));
            buffer = strstr(&buffer[1], value);
        }

        return count;
    }

};

size_t str_replace(char** dest_str, const char* src_str, char* old_value, char* new_value){
    size_t old_val_len = strlen(old_value);
    size_t new_val_len = strlen(new_value);
    size_t dest_len = strlen(src_str);

    size_t dif = new_val_len - old_val_len;
    size_t last_pos_dest = 0, last_pos_src = 0, i;

    (*dest_str) = (char*)calloc(dest_len, sizeof(char));

    for(i = 0; (i + old_val_len) <= strlen(src_str); i++){
    	if(!strncmp(&src_str[i], old_value, old_val_len)){
            dest_len += dif;
            (*dest_str) = (char*)realloc((*dest_str), dest_len*sizeof(char));
            strncat((*dest_str), &src_str[last_pos_src], i - last_pos_src);
            strcat((*dest_str), new_value);
            last_pos_dest = i + old_val_len + dif;
            last_pos_src = i + old_val_len;
    		i += old_val_len - 1;
		}
	}

    if(last_pos_src < strlen(src_str) - 1) strcat(&(*dest_str)[last_pos_dest], &src_str[last_pos_src]);

    return dest_len;
}

size_t str_from_file(char** dest_str, char* file_path){
    FILE *f = fopen(file_path, "r");

    if (f == NULL) {
        perror("\nErro ao abrir o arquivo.\n");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    size_t f_len = ftell(f);
    rewind(f);

    (*dest_str) = (char*)calloc(f_len, sizeof(char));

    fread((*dest_str), sizeof(char), f_len, f);
    (*dest_str)[f_len] = '\0';

    fclose(f);

    return f_len;
}

size_t str_to_file(const char* src_str, char* file_path, char* mode){
    FILE *f = fopen(file_path, mode);

    if (f == NULL) {
        perror("\nErro ao abrir o arquivo.\n");
        return 0;
    }

    fwrite(src_str, sizeof(char), strlen(src_str), f);
    fflush(f);


    rewind(f);
    fseek(f, 0, SEEK_END);
    size_t f_len = ftell(f);

    fclose(f);

    return f_len;
}

str_attributes_t str_att = {{str_obj_set, str_obj_get, str_obj_cmp, str_obj_print}, strlen, str_count, str_split, str_join, str_vjoin, str_search, str_replace, str_from_file, str_to_file};

void str_map_add(att_map_t* attribute_map){
    att_map_t *current = attribute_map;

    if(current->att == NULL){
        current->att = calloc(1, sizeof(common_attributes_t));
        memcpy(current->att, &str_att, 32);
        current->type_id = _STR_ID;
        //current->att = &str_att;
        current->next = NULL;   
    }else{
        while (current->next != NULL) current = current->next;
        current->next = (att_map_t*)calloc(1, sizeof(att_map_t));
        current = current->next;
        current->att = calloc(1, sizeof(common_attributes_t));
        memcpy(current->att, &str_att, 32);
        current->type_id = _STR_ID;
        //current->att = &str_att;
        current->next = NULL;
    }
      
}