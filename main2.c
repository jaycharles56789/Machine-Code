// Main.c
// Simple translator: reads C-like statements from a file and writes assembly (.s) with binary comments.
// Supports: int var; int var = value; var = value; var = var op var  (op = + - * /)
// Uses shunting-yard to respect operator precedence.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* --------- Lookup tables (table-driven) --------- */
/* Register binary codes (r0..r31 as names and $t0..$t3) */
typedef struct { const char *name; const char *bin; } RegEntry;
static const RegEntry reg_table[] = {
    {"r0","00000"}, {"r1","00001"}, {"r2","00010"}, {"r3","00011"},
    {"r4","00100"}, {"r5","00101"}, {"r6","00110"}, {"r7","00111"},
    {"r8","01000"}, {"r9","01001"}, {"r10","01010"}, {"r11","01011"},
    {"r12","01100"}, {"r13","01101"}, {"r14","01110"}, {"r15","01111"},
    {"r16","10000"}, {"r17","10001"}, {"r18","10010"}, {"r19","10011"},
    {"r20","10100"}, {"r21","10101"}, {"r22","10110"}, {"r23","10111"},
    {"r24","11000"}, {"r25","11001"}, {"r26","11010"}, {"r27","11011"},
    {"r28","11100"}, {"r29","11101"}, {"r30","11110"}, {"r31","11111"},
    {"$zero","00000"}, {"$t0","01000"}, {"$t1","01001"}, {"$t2","01010"}, {"$t3","01011"}
};

/* R-type and I-type instruction table */
typedef struct { const char *mnemonic; const char *opcode; const char *funct; int isI; } InstEntry;
static const InstEntry inst_table[] = {
    {"DADDIU", "011001", "", 1},   // I-type
    {"SB",     "101000", "", 1},   // I-type store byte (we will use base in rs, rt contains source)
    {"LB",     "100000", "", 1},   // I-type load byte
    {"DMULT",  "000000", "000010", 0}, // R-type use "DMULT" to emit dmult (we'll map to special style)
    {"DMUL",   "000000", "000010", 0}, // alias
    {"DDIV",   "000000", "000110", 0},
    {"DADDU",  "000000", "000000", 0},
    {"DSUBU",  "000000", "000011", 0},
    {"MFLO",   "000000", "000000", 0} // We'll generate mflo as special (treated as pseudo R-type with funct)
};

/* Utility: find register binary */
const char *reg_to_bin(const char *rname) {
    for (size_t i = 0; i < sizeof(reg_table)/sizeof(reg_table[0]); ++i) {
        if (strcmp(reg_table[i].name, rname) == 0) return reg_table[i].bin;
    }
    return "00000"; // default r0
}

/* Utility: find instruction entry */
const InstEntry *find_inst(const char *mn) {
    for (size_t i = 0; i < sizeof(inst_table)/sizeof(inst_table[0]); ++i) {
        if (strcmp(inst_table[i].mnemonic, mn) == 0) return &inst_table[i];
    }
    return NULL;
}

/* --------- Simple tokenizer and helpers --------- */
void trim(char *s) {
    // remove leading/trailing spaces and '\r' and '\n'
    int i, j = 0;
    while (isspace((unsigned char)s[j])) ++j;
    if (j) memmove(s, s+j, strlen(s+j)+1);
    i = strlen(s);
    while (i>0 && isspace((unsigned char)s[i-1])) s[--i] = '\0';
}

/* map variable name to memory label (same name) and to a register when needed
   For simplicity we map:
   a -> r1 (used as temp stores), but for loads/stores we follow sample:
   We'll use r1 for temporary store/accumulator, r2..r10 for temps
*/
const char *var_to_memlabel(const char *var) {
    return var; // use variable name as label in assembly
}

/* allocate temporary register names (r2..r10) for expression evaluation */
int next_temp = 2;
void reset_temps() { next_temp = 2; }
char *alloc_temp(char *buf) { // buf must hold e.g. "r10"
    if (next_temp > 10) next_temp = 2; // wrap (simple)
    sprintf(buf, "r%d", next_temp++);
    return buf;
}

/* ------- Shunting-yard: convert infix to RPN (token array) ------- */
#define MAXTOK 64
typedef enum { TOK_VAR, TOK_NUM, TOK_OP } TokType;
typedef struct { TokType type; char text[32]; char op; } Token;

int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

/* tokenize simple expression (no parentheses expected here) */
int tokenize_expr(const char *expr, Token tokens[], int *ntok) {
    *ntok = 0;
    int i = 0, n = strlen(expr);
    while (i < n) {
        if (isspace((unsigned char)expr[i])) { ++i; continue; }
        if (isalpha((unsigned char)expr[i])) {
            int j = 0;
            while (i < n && (isalnum((unsigned char)expr[i]) || expr[i]=='_')) {
                if (j < 30) tokens[*ntok].text[j++] = expr[i];
                ++i;
            }
            tokens[*ntok].text[j] = 0;
            tokens[*ntok].type = TOK_VAR;
            (*ntok)++;
        } else if (isdigit((unsigned char)expr[i])) {
            int j = 0;
            while (i < n && isdigit((unsigned char)expr[i])) {
                if (j < 30) tokens[*ntok].text[j++] = expr[i];
                ++i;
            }
            tokens[*ntok].text[j] = 0;
            tokens[*ntok].type = TOK_NUM;
            (*ntok)++;
        } else if (strchr("+-*/()", expr[i])) {
            tokens[*ntok].type = TOK_OP;
            tokens[*ntok].op = expr[i];
            tokens[*ntok].text[0] = 0;
            (*ntok)++;
            ++i;
        } else {
            // unknown char
            ++i;
        }
    }
    return 1;
}

/* shunting-yard -> output RPN in tokens_out */
int shunting_yard(Token tokens[], int ntok, Token out[], int *nout) {
    Token stack[MAXTOK]; int sp = 0;
    *nout = 0;
    for (int i = 0; i < ntok; ++i) {
        Token t = tokens[i];
        if (t.type == TOK_NUM || t.type == TOK_VAR) {
            out[(*nout)++] = t;
        } else if (t.type == TOK_OP) {
            if (t.op == '(') {
                stack[sp++] = t;
            } else if (t.op == ')') {
                while (sp>0 && stack[sp-1].op != '(') out[(*nout)++] = stack[--sp];
                if (sp>0 && stack[sp-1].op == '(') --sp;
            } else {
                while (sp>0 && stack[sp-1].type==TOK_OP &&
                       ((precedence(stack[sp-1].op) > precedence(t.op)) ||
                        (precedence(stack[sp-1].op) == precedence(t.op) && t.op != '^')) &&
                       stack[sp-1].op != '(') {
                    out[(*nout)++] = stack[--sp];
                }
                stack[sp++] = t;
            }
        }
    }
    while (sp>0) out[(*nout)++] = stack[--sp];
    return 1;
}

/* Evaluate RPN and emit assembly lines into out file.
   We'll return result stored register name in destbuf (e.g. "r3").
*/
void eval_rpn_and_emit(Token rpn[], int nrpn, FILE *out, char *destbuf) {
    // stack of register names or immediate markers
    char valstack[64][16];
    int vs = 0;
    reset_temps();
    for (int i = 0; i < nrpn; ++i) {
        Token t = rpn[i];
        if (t.type == TOK_NUM) {
            // load immediate into temp r
            char tmp[8]; alloc_temp(tmp);
            fprintf(out, "    daddi %s, r0, %s\n", tmp, t.text); // daddi tmp, r0, imm
            // comment binary (I-type: opcode rs rt imm)
            const InstEntry *ie = find_inst("DADDIU");
            const char *opcode = ie ? ie->opcode : "011001";
            const char *rs = reg_to_bin("r0");
            const char *rt = reg_to_bin(tmp);
            // immediate to 16-bit binary
            int imm = atoi(t.text);
            char imm16[17]; for (int k=15;k>=0;--k){ imm16[k] = (imm & 1) ? '1' : '0'; imm>>=1; } imm16[16]=0;
            fprintf(out, "# %s%s%s%s\n", opcode, rs, rt, imm16);
            strcpy(valstack[vs++], tmp);
        } else if (t.type == TOK_VAR) {
            // load memory variable into temp
            char tmp[8]; alloc_temp(tmp);
            fprintf(out, "    lb %s, %s(r0)\n", tmp, t.text);
            const InstEntry *ie = find_inst("LB");
            const char *opcode = ie ? ie->opcode : "100000";
            const char *rs = reg_to_bin("r0");
            const char *rt = reg_to_bin(tmp);
            char imm16[17]; memset(imm16,'0',16); imm16[16]=0;
            fprintf(out, "# %s%s%s%s\n", opcode, rs, rt, imm16);
            strcpy(valstack[vs++], tmp);
        } else if (t.type == TOK_OP) {
            // pop two operands (right then left)
            char right[16]; char left[16];
            if (vs < 2) { strcpy(destbuf, "r0"); return; }
            strcpy(right, valstack[--vs]);
            strcpy(left,  valstack[--vs]);
            // allocate result register
            char dst[8]; alloc_temp(dst);
            // decide instruction based on op
            if (t.op == '*') {
                // use dmult left, right ; mflo dst
                fprintf(out, "    dmult %s, %s\n", left, right);
                // DMULT binary comment (we output opcode + rs + rt + rd+shamt+funct style for clarity)
                const InstEntry *ie = find_inst("DMULT");
                const char *opcode = ie ? ie->opcode : "000000";
                const char *rs = reg_to_bin(left);
                const char *rt = reg_to_bin(right);
                // for mult, we show a 32-bit placeholder: opcode rs rt rd shamt funct
                char rd_bin[6]; strcpy(rd_bin, "00000");
                char shamt[6]; strcpy(shamt,"00000");
                fprintf(out, "# %s%s%s%s%s\n", opcode, rs, rt, rd_bin, ie->funct);
                // mflo dst
                fprintf(out, "    mflo %s\n", dst);
                // comment for mflo: use custom pseudo bits
                fprintf(out, "# 00000000000000000000000000000000\n");
            } else if (t.op == '/') {
                fprintf(out, "    ddiv %s, %s\n", left, right);
                const InstEntry *ie = find_inst("DDIV");
                const char *opcode = ie ? ie->opcode : "000000";
                const char *rs = reg_to_bin(left);
                const char *rt = reg_to_bin(right);
                fprintf(out, "# %s%s%s0000000000%s\n", opcode, rs, rt, ie->funct);
                fprintf(out, "    mflo %s\n", dst);
                fprintf(out, "# 00000000000000000000000000000000\n");
            } else if (t.op == '+') {
                // DADDU dst,left,right
                fprintf(out, "    dadd %s, %s, %s\n", dst, left, right);
                const InstEntry *ie = find_inst("DADDU");
                const char *opcode = ie ? ie->opcode : "000000";
                const char *rs = reg_to_bin(left);
                const char *rt = reg_to_bin(right);
                const char *rd = reg_to_bin(dst);
                const char *funct = ie ? ie->funct : "000000";
                // opcode rs rt rd shamt funct
                fprintf(out, "# %s%s%s%s00000%s\n", opcode, rs, rt, rd, funct);
            } else if (t.op == '-') {
                fprintf(out, "    dsub %s, %s, %s\n", dst, left, right);
                const InstEntry *ie = find_inst("DSUBU");
                const char *opcode = ie ? ie->opcode : "000000";
                const char *rs = reg_to_bin(left);
                const char *rt = reg_to_bin(right);
                const char *rd = reg_to_bin(dst);
                const char *funct = ie ? ie->funct : "000011";
                fprintf(out, "# %s%s%s%s00000%s\n", opcode, rs, rt, rd, funct);
            }
            // push dst as result
            strcpy(valstack[vs++], dst);
        }
    }
    // final result register name
    if (vs>0) strcpy(destbuf, valstack[--vs]);
    else strcpy(destbuf, "r0");
}

/* --------- Main translation flow per input line ---------- */

void translate_line(const char *line, FILE *out) {
    char s[256];
    strcpy(s, line);
    trim(s);
    if (strlen(s) == 0) return;

    // print a marker for input (optional)
    // fprintf(out, "# input: %s\n", s);

    // Declaration only: "int a;" or "int a ;"
    if (strncmp(s, "int ", 4) == 0 && strchr(s, '=') == NULL) {
        // just declare; nothing to emit in this simple assembler other than a comment
        fprintf(out, "    # declare %s\n", s+4);
        return;
    }

    // Declaration with init or assignment: "int a = 10;" or "a = 10;"
    char *eq = strchr(s, '=');
    if (eq != NULL) {
        // left side
        char left[64]; int L = eq - s;
        strncpy(left, s, L); left[L] = '\0'; trim(left);
        // right side
        char right[128]; strcpy(right, eq+1); // may include semicolon
        // remove trailing semicolon if present
        char *semi = strchr(right, ';'); if (semi) *semi = '\0';
        trim(right);

        // If left starts with "int", handle init like "int a = 10"
        if (strncmp(left, "int ", 4) == 0) {
            // remove "int "
            char varname[64]; strcpy(varname, left+4); trim(varname);
            // if RHS is numeric constant -> immediate store
            int isnum = 1;
            for (size_t i=0;i<strlen(right);++i) if (!isdigit((unsigned char)right[i]) && right[i] != '-') { isnum = 0; break; }

            if (isnum) {
                // load immediate to r1, store to memory label
                fprintf(out, "    daddi r1, r0, %s\n", right);
                // I-type: DADDIU opcode(6) rs(5) rt(5) imm(16)
                const InstEntry *ie = find_inst("DADDIU");
                const char *opcode = ie?ie->opcode:"011001";
                fprintf(out, "# %s%s%s%s\n",
                        opcode,
                        reg_to_bin("r0"),
                        reg_to_bin("r1"),
                        "0000000000000000"); // simplified imm placeholder
                fprintf(out, "    sb r1, %s(r0)\n", varname);
                // SB comment
                const InstEntry *sbi = find_inst("SB");
                const char *sbop = sbi ? sbi->opcode : "101000";
                fprintf(out, "# %s%s%s%s\n", sbop, reg_to_bin("r0"), reg_to_bin("r1"), "0000000000000000");
            } else {
                // assignment from expression (e.g., int a = b * c)
                // parse RHS expression and evaluate to register
                Token tok[MAXTOK], rpn[MAXTOK]; int nt=0, nr=0;
                tokenize_expr(right, tok, &nt);
                shunting_yard(tok, nt, rpn, &nr);
                char resultreg[16];
                eval_rpn_and_emit(rpn, nr, out, resultreg);
                // store resultreg into memory label
                fprintf(out, "    sb %s, %s(r0)\n", resultreg, varname);
                const InstEntry *sbi = find_inst("SB");
                const char *sbop = sbi ? sbi->opcode : "101000";
                fprintf(out, "# %s%s%s%s\n", sbop, reg_to_bin("r0"), reg_to_bin(resultreg), "0000000000000000");
            }
            return;
        }

        // Otherwise left is a variable name assignment, e.g., "a = ..." or "a = a + b * c"
        char varname[64]; strcpy(varname, left); trim(varname);

        // Check if RHS is numeric constant:
        int isnum = 1;
        for (size_t i=0;i<strlen(right);++i) if (!isdigit((unsigned char)right[i]) && right[i] != '-') { isnum = 0; break; }

        if (isnum) {
            // immediate assignment
            fprintf(out, "    daddi r1, r0, %s\n", right);
            const InstEntry *ie = find_inst("DADDIU");
            const char *opcode = ie?ie->opcode:"011001";
            fprintf(out, "# %s%s%s%s\n",
                    opcode, reg_to_bin("r0"), reg_to_bin("r1"), "0000000000000000");
            fprintf(out, "    sb r1, %s(r0)\n", varname);
            const InstEntry *sbi = find_inst("SB");
            const char *sbop = sbi ? sbi->opcode : "101000";
            fprintf(out, "# %s%s%s%s\n", sbop, reg_to_bin("r0"), reg_to_bin("r1"), "0000000000000000");
            return;
        } else {
            // RHS is expression: use shunting-yard + eval to emit code, then sb result to mem
            Token tok[MAXTOK], rpn[MAXTOK]; int nt=0, nr=0;
            tokenize_expr(right, tok, &nt);
            shunting_yard(tok, nt, rpn, &nr);
            char resultreg[16];
            eval_rpn_and_emit(rpn, nr, out, resultreg);
            // store resultreg to memory varname
            fprintf(out, "    sb %s, %s(r0)\n", resultreg, varname);
            const InstEntry *sbi = find_inst("SB");
            const char *sbop = sbi ? sbi->opcode : "101000";
            fprintf(out, "# %s%s%s%s\n", sbop, reg_to_bin("r0"), reg_to_bin(resultreg), "0000000000000000");
            return;
        }
    }

    // If no '=', we just ignore or comment
    fprintf(out, "    # unsupported or empty line: %s\n", s);
}

/* ---------------- main ------------------ */
int main(int argc, char **argv) {
    const char *infile = "input.txt";
    const char *outfile = "output.s";
    if (argc >= 2) infile = argv[1];
    if (argc >= 3) outfile = argv[2];

    FILE *fin = fopen(infile, "r");
    if (!fin) { printf("Cannot open input file %s\n", infile); return 1; }
    FILE *fout = fopen(outfile, "w");
    if (!fout) { printf("Cannot open output file %s\n", outfile); fclose(fin); return 1; }

    fprintf(fout, ".code\n");

    char line[512];
    while (fgets(line, sizeof(line), fin)) {
        char tmp[512]; strcpy(tmp, line);
        // trim leading/trailing
        trim(tmp);
        if (strlen(tmp) == 0) continue;
        // remove trailing semicolon only for processing (we keep memory labels)
        translate_line(tmp, fout);
    }

    fclose(fin);
    fclose(fout);
    printf("Translation complete: wrote %s\n", outfile);
    return 0;
}
