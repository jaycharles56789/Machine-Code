#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MIPS64.h"

// Remove spaces from input
void remove_spaces(char *str) {
    int i, j = 0;
    for(i=0; str[i] != '\0'; i++){
        if(str[i] != ' ' && str[i] != '\t') str[j++] = str[i];
    }
    str[j] = '\0';
}

// Map variable name to register
void map_register(const char *var, char *reg) {
    if(var[0]=='a') strcpy(reg, "$t0");
    else if(var[0]=='b') strcpy(reg, "$t1");
    else if(var[0]=='c') strcpy(reg, "$t2");
    else strcpy(reg, "$t3");
}

// Get 5-bit binary code for register
void get_register_binary(const char *reg, char *code) {
    if(strcmp(reg, "$t0")==0) strcpy(code,"01000");
    else if(strcmp(reg, "$t1")==0) strcpy(code,"01001");
    else if(strcmp(reg, "$t2")==0) strcpy(code,"01010");
    else strcpy(code,"01011");
}

// Convert integer to 16-bit binary
void int_to_binary16(int val, char *bin16){
    int i;
    for(i=15;i>=0;i--){
        bin16[i]=(val&1)?'1':'0';
        val>>=1;
    }
    bin16[16]='\0';
}

// Map instruction to opcode and funct
void get_opcode_funct(const char *instr, char *opcode, char *funct){
    if(strcmp(instr,"DADDIU")==0){ strcpy(opcode,"011001"); strcpy(funct,""); }
    else if(strcmp(instr,"DADDU")==0){ strcpy(opcode,"000000"); strcpy(funct,"000000"); }
    else if(strcmp(instr,"DSUBU")==0){ strcpy(opcode,"000000"); strcpy(funct,"000011"); }
    else if(strcmp(instr,"DMUL")==0){ strcpy(opcode,"000000"); strcpy(funct,"000010"); }
    else if(strcmp(instr,"DDIV")==0){ strcpy(opcode,"000000"); strcpy(funct,"000110"); }
}

// Convert 32-bit binary to hex
void binary_to_hex(const char *bin,char *hex){
    unsigned int val=0;
    for(int i=0;i<32;i++){
        val=(val<<1)+(bin[i]=='1'?1:0);
    }
    sprintf(hex,"0x%08X",val);
}

char* source_file() {
    FILE *source_code = fopen("input.txt","r");
    if(source_code == NULL) {
        fprintf(stderr, "ERROR: File cannot be opened.\n");
        return NULL;
    }
    char *line_of_code = (char *)malloc(1024 * sizeof(char));
    if(line_of_code == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(source_code);
        return NULL;
    }
    char lines[1024];
    line_of_code[0] = '\0';
    while(fgets(lines, sizeof(lines), source_code) != NULL) {
        strcat(line_of_code, lines);
    }
    fclose(source_code);
    return line_of_code;
}

int main(){
    char input[128], clean_input[128];
    char var1[32], var2[32], var3[32];
    char reg1[5], reg2[5], reg3[5];
    int value=0;
    char assembly[256];
    char opcode[7], funct[7];
    char rs_code[6], rt_code[6], rd_code[6], imm_bin[17];
    char machine_code[33], hexcode[11];
    char op;

    char *source = source_file();
    if(source == NULL) {
        return 1;
    }
    printf("input source code:\n%s\n", source);
    return 0;
    
    printf("Enter a C-based assignment or arithmetic operation:\n");
    fgets(input,sizeof(input),stdin);
    input[strcspn(input,"\n")]='\0';
    strcpy(clean_input,input);
    remove_spaces(clean_input);

    assembly[0]='\0';
    machine_code[0]='\0';

    if(strncmp(clean_input,"int",3)==0){
        char *equalSign=strchr(clean_input,'=');
        if(equalSign!=NULL){
            int len=equalSign-(clean_input+3);
            strncpy(var1,clean_input+3,len); var1[len]='\0';
            value=atoi(equalSign+1);

            map_register(var1,reg2);
            strcpy(reg1,"$zero");

            sprintf(assembly,"DADDIU %s, %s, %d",reg2,reg1,value);

            get_register_binary(reg1, rs_code);
            get_register_binary(reg2, rt_code);
            get_register_binary(reg2, rd_code);
            int_to_binary16(value,imm_bin);
            get_opcode_funct("DADDIU",opcode,funct);
        }
    }else{
        char *equalSign=strchr(clean_input,'=');
        if(equalSign!=NULL){
            strncpy(var1,clean_input,equalSign-clean_input); var1[equalSign-clean_input]='\0';
            char *expr=equalSign+1;

            if(strchr(expr,'+')!=NULL) op='+';
            else if(strchr(expr,'-')!=NULL) op='-';
            else if(strchr(expr,'*')!=NULL) op='*';
            else if(strchr(expr,'/')!=NULL) op='/';
            else op='?';

            char *token=strtok(expr,"+-*/");
            strcpy(var2,token);
            token=strtok(NULL,"+-*/");
            if(token!=NULL) strcpy(var3,token);

            map_register(var1,reg1);
            map_register(var2,reg2);
            map_register(var3,reg3);

            get_register_binary(reg1,rd_code);
            get_register_binary(reg2,rs_code);
            get_register_binary(reg3,rt_code);

            if(op=='+'){ strcpy(assembly,"DADDU "); get_opcode_funct("DADDU",opcode,funct); }
            else if(op=='-'){ strcpy(assembly,"DSUBU "); get_opcode_funct("DSUBU",opcode,funct); }
            else if(op=='*'){ strcpy(assembly,"DMUL "); get_opcode_funct("DMUL",opcode,funct); }
            else if(op=='/'){ strcpy(assembly,"DDIV "); get_opcode_funct("DDIV",opcode,funct); }

            strcat(assembly,reg1); strcat(assembly,", "); strcat(assembly,reg2); strcat(assembly,", "); strcat(assembly,reg3);
            strcpy(imm_bin,"0000000000000000"); // no immediate for R-type
        }
    }

    // Build full 32-bit machine code
    if(strlen(funct)==0) sprintf(machine_code,"%s%s%s%s",opcode,rs_code,rt_code,imm_bin); // I-type
    else sprintf(machine_code,"%s%s%s%s00000%s",opcode,rs_code,rt_code,rd_code,funct); // R-type

    binary_to_hex(machine_code,hexcode);

    printf("\nInput string:\n%s\n",input);
    printf("\nAssembly equivalent:\n%s\n",assembly);
    printf("\nMachine code equivalent (components):\n");
    printf("Opcode: %s\n",opcode);
    printf("RS: %s\n",rs_code);
    printf("RT: %s\n",rt_code);
    printf("RD: %s\n",rd_code);
    if(strlen(funct)>0) printf("Funct: %s\n",funct);
    else printf("Immediate: %s\n",imm_bin);

    printf("\nHex value:\n%s\n",hexcode);

    return 0;
}
