/* edu_compiler_integrated.c
 *
 * Integrated EduMIPS64 compiler using:
 *   /mnt/data/Registers.c
 *   /mnt/data/R-type.c
 *   /mnt/data/I-type.c
 *
 * Uses R-type ops:
 *   DADDU (for +)
 *   DSUBU (for -)
 *
 * Produces runnable.s
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "MIPS64.h"

/* ---------- Macros requested by user ---------- */
#define ESCAPE_SEQUENCE(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')
#define APHABETIC_CHARACTER(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_')
#define DIGIT_CHARACTER(c) ((c) >= '0' && (c) <= '9')
#define ALPHANUMERIC_CHARACTER(c) (APHABETIC_CHARACTER(c) || DIGIT_CHARACTER(c))

/* ---------- Symbol table ---------- */
typedef struct Var {
    char name[64];
    int offset; 
    struct Var *next;
} Var;

/* trim */
static void trim(char *s) {
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    char *end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end)) { *end = '\0'; end--; }
}

/* symbol table helpers */
static Var *find_var(Var *head, const char *name) {
    for (Var *v = head; v; v = v->next)
        if (strcmp(v->name, name) == 0) return v;
    return NULL;
}
static Var *add_var(Var **head, const char *name, int offset) {
    Var *v = malloc(sizeof(Var));
    if (!v) return NULL;
    strncpy(v->name, name, sizeof(v->name)-1);
    v->name[sizeof(v->name)-1] = '\0';
    v->offset = offset;
    v->next = *head;
    *head = v;
    return v;
}

/* token helpers */
static int is_integer_token(const char *tok) {
    if (!tok || *tok == '\0') return 0;
    const char *p = tok;
    if (*p == '+' || *p == '-') p++;
    if (!DIGIT_CHARACTER((unsigned char)*p)) return 0;
    for (; *p; p++) if (!DIGIT_CHARACTER((unsigned char)*p)) return 0;
    return 1;
}

static int parse_token(const char *tok, long *val, char *varname) {
    if (!tok || *tok == '\0') return -1;

    if (is_integer_token(tok)) {
        *val = strtol(tok, NULL, 10);
        return 1;
    }

    if (!APHABETIC_CHARACTER((unsigned char)tok[0])) return -1;
    for (size_t i = 1; tok[i]; ++i)
        if (!ALPHANUMERIC_CHARACTER((unsigned char)tok[i])) return -1;

    strncpy(varname, tok, 63);
    varname[63] = '\0';
    return 0;
}

/* optional opcode check */
static void check_opcode(const char *mnemonic) {
    char out[OPCODE_STR_LEN];
    if (get_r_opcode(mnemonic, out)) return;
    if (get_i_opcode(mnemonic, out)) return;

    fprintf(stderr, "Warning: opcode '%s' not found in tables\n", mnemonic);
}

/* Emit expr → R8, with R9 temp.
   Use:
     DADDU for +
     DSUBU for -
*/
static int emit_eval_expr(FILE *out, const char *expr, Var *vars_head) {
    if (!expr) return -1;

    const char *p = expr;
    char token[128];
    char varname[64];
    long val;
    int first = 1;
    char last_op = 0;

    while (*p) {
        while (*p && ESCAPE_SEQUENCE(*p)) p++;
        if (!*p) break;

        const char *start = p;
        while (*p && *p != '+' && *p != '-') p++;

        size_t len = p - start;
        if (len == 0 || len >= sizeof(token)) return -1;
        strncpy(token, start, len);
        token[len] = '\0';
        trim(token);

        int t = parse_token(token, &val, varname);
        if (t == -1) return -1;

        if (first) {
            if (t == 1) {
                check_opcode("DADDIU");
                fprintf(out, "    ADDI  R8, R0, %ld\n", val);
            } else {
                Var *v = find_var(vars_head, varname);
                if (!v) return -1;
                check_opcode("LD");
                fprintf(out, "    LD    R8, %d(R29)\n", v->offset);
            }
            first = 0;
        }
        else {
            if (t == 1) {
                fprintf(out, "    ADDI  R9, R0, %ld\n", val);
            } else {
                Var *v = find_var(vars_head, varname);
                if (!v) return -1;
                fprintf(out, "    LD    R9, %d(R29)\n", v->offset);
            }

            if (last_op == '+') {
                check_opcode("DADDU");
                fprintf(out, "    DADDU R8, R8, R9\n");
            } else if (last_op == '-') {
                check_opcode("DSUBU");
                fprintf(out, "    DSUBU R8, R8, R9\n");
            } else return -1;
        }

        if (*p == '+' || *p == '-') last_op = *p++;
        else last_op = 0;
    }

    return 0;
}

/* main compile function */
void compile_to_assemble(const char *source, const char *file_out) {
    FILE *out = fopen(file_out, "w");
    if (!out) { fprintf(stderr, "ERROR opening %s\n", file_out); return; }

    Var *vars = NULL;
    int next_offset = 8;
    char *buf = strdup(source ? source : "");
    if (!buf) return;

    char *stmts[1024];
    int sc = 0;

    char *t = strtok(buf, ";");
    while (t && sc < 1024) {
        char *s = strdup(t);
        trim(s);
        if (s[0] != '\0') stmts[sc++] = s;
        else free(s);
        t = strtok(NULL, ";");
    }

    /* first pass */
    for (int i = 0; i < sc; ++i) {
        char tmp[512];
        strncpy(tmp, stmts[i], sizeof(tmp)-1);
        tmp[sizeof(tmp)-1] = '\0';
        trim(tmp);

        if (strncmp(tmp, "int ", 4) == 0) {
            char *rest = tmp + 4;
            trim(rest);

            char *eq = strchr(rest, '=');
            char name[64];

            if (eq) {
                size_t n = eq - rest;
                strncpy(name, rest, n);
                name[n] = '\0';
                trim(name);
            } else {
                strncpy(name, rest, sizeof(name)-1);
                name[sizeof(name)-1] = '\0';
                trim(name);
            }

            if (!find_var(vars, name))
                add_var(&vars, name, next_offset), next_offset += 8;
        }
    }

    int stack_size = next_offset + 8;
    if (stack_size % 16) stack_size += 16 - (stack_size % 16);

    /* prologue */
    fprintf(out, ".code\n");

    /* second pass */
    for (int i = 0; i < sc; ++i) {
        char stmt[1024];
        strncpy(stmt, stmts[i], sizeof(stmt)-1);
        stmt[sizeof(stmt)-1] = '\0';
        trim(stmt);
        if (!stmt[0]) continue;

        if (strncmp(stmt, "int ", 4) == 0) {
            char *rest = stmt + 4;
            trim(rest);

            char *eq = strchr(rest, '=');
            if (eq) {
                char name[64];
                size_t n = eq - rest;
                strncpy(name, rest, n);
                name[n] = '\0';
                trim(name);

                char *expr = eq + 1;
                trim(expr);

                if (emit_eval_expr(out, expr, vars) != 0)
                    fprintf(stderr, "expr error: %s\n", expr);

                Var *v = find_var(vars, name);
                fprintf(out, "    SD   R8, %d(R29)\n", v->offset);
            }
            else {
                Var *v = find_var(vars, rest);
                fprintf(out, "    ADDI R8, R0, 0\n");
                fprintf(out, "    SD   R8, %d(R29)\n", v->offset);
            }

            continue;
        }

        char *eq = strchr(stmt, '=');
        if (eq) {
            char name[64];
            size_t n = eq - stmt;
            strncpy(name, stmt, n);
            name[n] = '\0';
            trim(name);

            char *expr = eq + 1;
            trim(expr);

            if (emit_eval_expr(out, expr, vars) != 0)
                fprintf(stderr, "expr error: %s\n", expr);

            Var *v = find_var(vars, name);
            fprintf(out, "    SD   R8, %d(R29)\n", v->offset);
            continue;
        }

        fprintf(out, "    # unsupported: %s\n", stmt);
    }

    /* epilogue */
    fprintf(out, "    LD   R31, 0(R29)\n");
    fprintf(out, "    ADDI R29, R29, %d\n", stack_size);
    fprintf(out, "    JR   R31\n");
    fprintf(out, "    HALT\n");

    for (int i = 0; i < sc; ++i) free(stmts[i]);
    free(buf);
    fclose(out);
}

/* ---------------- MAIN ---------------- */
int main(void) {
    const char *src =
        "int a = 5;"
        "int b;"
        "b = a + 3;"
        "a = b - 2;"
        "int c = a + b - 1;";

    compile_to_assemble(src, "runnable.s");

    printf("Generated runnable.s\n");
    return 0;
}
