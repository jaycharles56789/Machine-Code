#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* A tiny symbol table entry */
typedef struct Var {
    char name[64];
    int offset; /* offset from $sp (positive) */
    struct Var *next;
} Var;

/* Helpers */
static void trim(char *s) {
    char *p = s;
    while (isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    /* trim end */
    char *end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end)) { *end = '\0'; end--; }
}

/* find var by name in list */
static Var *find_var(Var *head, const char *name) {
    for (Var *v = head; v; v = v->next) if (strcmp(v->name, name) == 0) return v;
    return NULL;
}

/* add var to list, returns pointer; caller ensures name not duplicate */
static Var *add_var(Var **head, const char *name, int offset) {
    Var *v = (Var*)malloc(sizeof(Var));
    strncpy(v->name, name, sizeof(v->name)-1); v->name[sizeof(v->name)-1] = '\0';
    v->offset = offset;
    v->next = *head;
    *head = v;
    return v;
}

/* is token integer literal? */
static int is_integer_token(const char *tok) {
    if (*tok == '\0') return 0;
    const char *p = tok;
    if (*p == '+' || *p == '-') p++;
    if (!isdigit((unsigned char)*p)) return 0;
    for (; *p; p++) if (!isdigit((unsigned char)*p)) return 0;
    return 1;
}

/* parse a simple token */
static int parse_token(const char *tok, long *val, char *varname) {
    if (is_integer_token(tok)) {
        *val = strtol(tok, NULL, 10);
        return 1;
    } else {
        strncpy(varname, tok, 63);
        varname[63] = '\0';
        return 0;
    }
}

/* Evaluate simple + and - expressions */
static int emit_eval_expr(FILE *out, const char *expr, Var *vars_head) {
    const char *p = expr;
    char token[128];
    char varname[64];
    long val;
    int first_token = 1;
    char last_op = 0;

    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        const char *start = p;
        while (*p && *p != '+' && *p != '-') p++;
        size_t len = (size_t)(p - start);
        if (len == 0) return -1;
        if (len >= sizeof(token)) return -1;
        strncpy(token, start, len);
        token[len] = '\0';
        trim(token);

        int is_int = parse_token(token, &val, varname);

        if (first_token) {
            if (is_int)
                fprintf(out, "\tli\t$t0, %ld\n", val);
            else {
                Var *v = find_var(vars_head, varname);
                if (!v) return -1;
                fprintf(out, "\tld\t$t0, %d($sp)\n", v->offset);
            }
            first_token = 0;
        } else {
            if (is_int)
                fprintf(out, "\tli\t$t1, %ld\n", val);
            else {
                Var *v = find_var(vars_head, varname);
                if (!v) return -1;
                fprintf(out, "\tld\t$t1, %d($sp)\n", v->offset);
            }

            if (last_op == '+')
                fprintf(out, "\tadd\t$t0, $t0, $t1\n");
            else if (last_op == '-')
                fprintf(out, "\tsub\t$t0, $t0, $t1\n");
            else return -1;
        }

        if (*p == '+' || *p == '-') {
            last_op = *p;
            p++;
        } else {
            last_op = 0;
        }
    }
    return 0;
}

/* Main compile function -> mips64 assembly file */
void compile_to_assemble(const char *source_code, const char *file_name) {

    FILE *output_file = fopen(file_name, "w");
    if(output_file == NULL) {
        fprintf(stderr, "ERROR: \"%s\"\n", file_name);
        return;
    }

    Var *vars = NULL;
    int next_offset = 8;

    char *buf = strdup(source_code ? source_code : "");
    if (!buf) { fclose(output_file); return; }

    char *statements[1024];
    int stmt_count = 0;

    char *saveptr = NULL;
    char *tok = strtok(buf, ";");
    while (tok && stmt_count < 1024) {
        char *s = strdup(tok);
        trim(s);
        if (s[0] != '\0') statements[stmt_count++] = s;
        else free(s);
        tok = strtok(NULL, ";");
    }

    /* ---- FIRST PASS: VAR DECLARATIONS ---- */
    for (int i = 0; i < stmt_count; ++i) {
        char tmp[512];
        strncpy(tmp, statements[i], sizeof(tmp)-1);
        tmp[sizeof(tmp)-1] = '\0';
        trim(tmp);

        if (strncmp(tmp, "int ", 4) == 0) {
            char *rest = tmp + 4;
            trim(rest);
            char varname[64];
            char *eq = strchr(rest, '=');

            if (eq) {
                size_t n = eq - rest;
                char namebuf[128];
                strncpy(namebuf, rest, n); namebuf[n] = '\0';
                trim(namebuf);
                strncpy(varname, namebuf, 63); varname[63] = '\0';
            } else {
                strncpy(varname, rest, 63); varname[63] = '\0';
                trim(varname);
            }

            if (!find_var(vars, varname)) {
                add_var(&vars, varname, next_offset);
                next_offset += 8;
            }
        }
    }

    int stack_size = next_offset + 8;
    if (stack_size % 16) stack_size += 16 - (stack_size % 16);

    /* ---- PROLOGUE ---- */
    fprintf(output_file, ".text\n");
    fprintf(output_file, ".globl main\n");
    fprintf(output_file, "main:\n");
    fprintf(output_file, "\taddi\t$sp, $sp, -%d\n", stack_size);
    fprintf(output_file, "\tsd\t$ra, 0($sp)\n");

    /* ---- SECOND PASS: EMIT CODE ---- */
    for (int i = 0; i < stmt_count; ++i) {
        char stmt[1024];
        strncpy(stmt, statements[i], sizeof(stmt)-1);
        stmt[sizeof(stmt)-1] = '\0';
        trim(stmt);

        if (strncmp(stmt, "int ", 4) == 0) {
            char *rest = stmt + 4;
            trim(rest);
            char *eq = strchr(rest, '=');

            if (eq) {
                size_t n = eq - rest;
                char namebuf[128];
                strncpy(namebuf, rest, n); namebuf[n] = '\0';
                trim(namebuf);

                char *expr = eq + 1;
                trim(expr);

                if (emit_eval_expr(output_file, expr, vars) == 0) {
                    Var *v = find_var(vars, namebuf);
                    fprintf(output_file, "\tsd\t$t0, %d($sp)\n", v->offset);
                }
            } else {
                char namebuf[128];
                strncpy(namebuf, rest, 127); namebuf[127] = '\0';
                trim(namebuf);
                Var *v = find_var(vars, namebuf);
                fprintf(output_file, "\tli\t$t0, 0\n");
                fprintf(output_file, "\tsd\t$t0, %d($sp)\n", v->offset);
            }
            continue;
        }

        char *eq = strchr(stmt, '=');
        if (eq) {
            size_t n = eq - stmt;
            char lhs[128];
            strncpy(lhs, stmt, n); lhs[n] = '\0';
            trim(lhs);

            char *expr = eq + 1;
            trim(expr);

            if (emit_eval_expr(output_file, expr, vars) == 0) {
                Var *v = find_var(vars, lhs);
                fprintf(output_file, "\tsd\t$t0, %d($sp)\n", v->offset);
            }
            continue;
        }

        fprintf(output_file, "\t# unsupported: %s\n", stmt);
    }

    /* ---- EPILOGUE ---- */
    fprintf(output_file, "\tld\t$ra, 0($sp)\n");
    fprintf(output_file, "\taddi\t$sp, $sp, %d\n", stack_size);
    fprintf(output_file, "\tjr\t$ra\n");

    /* cleanup */
    for (int i = 0; i < stmt_count; ++i) free(statements[i]);
    free(buf);

    while (vars) { Var *n = vars->next; free(vars); vars = n; }

    fclose(output_file);
}

/* ------------------------------------------------------ */
/* ---------------------- MAIN -------------------------- */
/* ------------------------------------------------------ */

int main(void) {
    const char *sample_code =
        "int x = 5; "
        "int y = x + 3 - 1; "
        "y = y + x - 2; "
        "int z; "
        "z = x + y - 4;";

    compile_to_assemble(sample_code, "output2.s");

    printf("Compilation done. Assembly written to output.s\n");

    return 0;
}
