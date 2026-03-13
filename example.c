#define LEXIN_NOLOG
#define LEXIN_IMPLEMENTATION
#include "lexin.h"

char* read_entrie_file
(FILE* file,uint32_t * count);

int main(void)
{
    lexin_t lex = {0};
    lex.ops = "~?^|!#&[]%.,-<>={}+-\\/*\'\"():;";
    lex.opc = strlen(lex.ops);
    char* arr[] = {
        "return","goto","if","int64_t","char","sizeof",
        "define","include","unsigned","uint32_t",
        "typedef","struct","for","endif","ifndef","ifdef",
        "while","do","enum","bool","const","switch","case",
        "default","else","break","continue"};
    uint32_t a;
    lex.keys = arr;
    lex.keyc = 27;
    lex.sl_com = "//";
    lex.ml_com_start = "/*";
    lex.ml_com_end = "*/";
    lex.file_name = "lexin.h";
    FILE* fptr = fopen(lex.file_name,"r");
    if(!fptr) return 1;
    lex.ctx = read_entrie_file(fptr,&a);
    fclose(fptr);
    bool res = !lexin_consume_context(&lex);
    free(lex.ctx);      
    printf("token count: %d\n",lex.tokens.count);
    printf("string count: %d\n",lex.strs.count);
    #if 0
    uint32_t i = 0;
    for(i = 0;i < lex.tokens.count;++i){
        print_token(&lex,lex.tokens.data[i],i);
    }
    #endif
    lexin_free(&lex);
    return res;
}

char* read_entrie_file
(FILE* file,uint32_t* count)
{
    int64_t size = 0,current_pos = 0;
    current_pos = ftell(file);
    if(current_pos == -1) return 0;
    if(fseek(file, 0, SEEK_END) != 0) return 0;
    size = ftell(file);
    if(size == -1) return 0;
    if(fseek(file, current_pos, SEEK_SET) != 0) return 0;
    char* buffer = (char*)calloc(size+1,sizeof(char));
    if(!buffer) return 0;
    *count = fread(buffer,sizeof(char),size,file);
    if(*count != size) {free(buffer);return 0;}
    return buffer;
}
