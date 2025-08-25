// filepath: src/todo.c
// Build: gcc -std=c99 -O2 -Wall -Wextra -o todo src/todo.c
// Usage: ./todo [tasks.json]   or   ./todo --test
//
// Pseudocode plan:
// 1) Define Task and TaskList. Provide init, push, delete, find, next_id.
// 2) Implement minimal JSON tokenizer+parser for [ {"id":..,"title":"..","due":"YYYY-MM-DD","priority":..,"done":..}, ... ].
//    - parse strings with escapes, ints, booleans, arrays, and objects.
//    - writer emits pretty JSON, escaping necessary characters.
// 3) IO: load_tasks(file)->TaskList; save_tasks(file, TaskList).
// 4) Validation: non-empty title, date YYYY-MM-DD and valid calendar day, priority in [1,5].
// 5) Sorting & filtering: by due date asc or priority desc; optional filter by due-before and min-priority and completion.
// 6) Pretty table output with fixed widths and truncation.
// 7) Menu loop: add, list, update, delete, save, quit. Autosave on quit.
// 8) Tests: serialization round-trip with edge cases (escapes), date compare, simple assertions.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define TITLE_MAX 128
#define DATE_LEN 10

typedef struct {
    int id;
    char title[TITLE_MAX + 1];
    char due[DATE_LEN + 1]; // YYYY-MM-DD or empty
    int priority;           // 1..5
    int done;               // 0/1
} Task;

typedef struct {
    Task *items;
    size_t len;
    size_t cap;
} TaskList;

/*---------------- Utility ----------------*/
static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) die("out of memory");
    return p;
}

static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) die("out of memory");
    return q;
}

/*---------------- TaskList ----------------*/
static void list_init(TaskList *L) {
    L->items = NULL; L->len = 0; L->cap = 0;
}

static void list_free(TaskList *L) {
    free(L->items); L->items = NULL; L->len = L->cap = 0;
}

static void list_reserve(TaskList *L, size_t want) {
    if (want <= L->cap) return;
    size_t nc = L->cap ? L->cap * 2 : 8;
    if (nc < want) nc = want;
    L->items = (Task*)xrealloc(L->items, nc * sizeof(Task));
    L->cap = nc;
}

static void list_push(TaskList *L, Task t) {
    list_reserve(L, L->len + 1);
    L->items[L->len++] = t;
}

static int list_find_index_by_id(const TaskList *L, int id) {
    for (size_t i = 0; i < L->len; ++i) if (L->items[i].id == id) return (int)i;
    return -1;
}

static int list_next_id(const TaskList *L) {
    int maxid = 0;
    for (size_t i = 0; i < L->len; ++i) if (L->items[i].id > maxid) maxid = L->items[i].id;
    return maxid + 1;
}

static void list_delete_at(TaskList *L, size_t idx) {
    if (idx >= L->len) return;
    L->items[idx] = L->items[L->len - 1];
    L->len--;
}

/*---------------- Date helpers ----------------*/
static int is_leap(int y){ return (y%4==0 && y%100!=0) || (y%400==0); }

static int valid_date(const char *s) {
    if (!s || strlen(s)==0) return 1; // allow empty
    if (strlen(s) != 10) return 0;
    if (!(isdigit((unsigned char)s[0])&&isdigit((unsigned char)s[1])&&isdigit((unsigned char)s[2])&&isdigit((unsigned char)s[3])&&s[4]=='-'&&isdigit((unsigned char)s[5])&&isdigit((unsigned char)s[6])&&s[7]=='-'&&isdigit((unsigned char)s[8])&&isdigit((unsigned char)s[9]))) return 0;
    int y = (s[0]-'0')*1000 + (s[1]-'0')*100 + (s[2]-'0')*10 + (s[3]-'0');
    int m = (s[5]-'0')*10 + (s[6]-'0');
    int d = (s[8]-'0')*10 + (s[9]-'0');
    if (y < 1900 || y > 2100) return 0;
    if (m < 1 || m > 12) return 0;
    int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (m==2 && is_leap(y)) {
        if (d<1 || d>29) return 0;
    } else {
        if (d<1 || d>mdays[m]) return 0;
    }
    return 1;
}

static int date_to_int(const char *s) {
    if (!s || !*s) return 99991231;
    return (s[0]-'0')*10000000 + (s[1]-'0')*1000000 + (s[2]-'0')*100000 + (s[3]-'0')*10000 + (s[5]-'0')*1000 + (s[6]-'0')*100 + (s[8]-'0')*10 + (s[9]-'0');
}

/*---------------- JSON writer ----------------*/
static void json_escape_string(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
            case '\\': fputs("\\\\", f); break;
            case '"': fputs("\\\"", f); break;
            case '\n': fputs("\\n", f); break;
            case '\r': fputs("\\r", f); break;
            case '\t': fputs("\\t", f); break;
            default:
                if (c < 0x20) {
                    fprintf(f, "\\u%04x", c);
                } else {
                    fputc(c, f);
                }
        }
    }
    fputc('"', f);
}

static int save_tasks(const char *path, const TaskList *L) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen"); return 0; }
    fputs("[\n", f);
    for (size_t i = 0; i < L->len; ++i) {
        const Task *t = &L->items[i];
        fputs("  { ", f);
        fprintf(f, "\"id\": %d, ", t->id);
        fputs("\"title\": ", f); json_escape_string(f, t->title); fputs(", ", f);
        fputs("\"due\": ", f); json_escape_string(f, t->due); fputs(", ", f);
        fprintf(f, "\"priority\": %d, ", t->priority);
        fprintf(f, "\"done\": %s ", t->done ? "true" : "false");
        fputs("}", f);
        if (i + 1 < L->len) fputs(",", f);
        fputc('\n', f);
    }
    fputs("]\n", f);
    fclose(f);
    return 1;
}

/*---------------- Minimal JSON reader ----------------*/

typedef struct { const char *s; size_t i, n; } Json;

static void j_init(Json *j, const char *s, size_t n){ j->s=s; j->i=0; j->n=n; }
static int j_eof(Json *j){ return j->i >= j->n; }
static char j_peek(Json *j){ return j->i<j->n? j->s[j->i] : '\0'; }
static char j_get(Json *j){ return j->i<j->n? j->s[j->i++] : '\0'; }
static void j_skip_ws(Json *j){ while(!j_eof(j) && isspace((unsigned char)j->s[j->i])) j->i++; }

static int j_expect(Json *j, char c){ j_skip_ws(j); if (j_peek(j)!=c) return 0; j_get(j); return 1; }

static int j_match_lit(Json *j, const char *lit){ j_skip_ws(j); size_t k=0; while(lit[k]){ if (j_get(j)!=lit[k]) return 0; k++; } return 1; }

static int j_parse_string(Json *j, char *out, size_t outsz){
    j_skip_ws(j);
    if (j_get(j)!='"') return 0;
    size_t o=0;
    while(!j_eof(j)){
        char c = j_get(j);
        if (c=='"') { if (o<outsz) out[o]='\0'; else out[outsz-1]='\0'; return 1; }
        if (c=='\\'){
            char e = j_get(j);
            switch(e){
                case '"': c='"'; break;
                case '\\': c='\\'; break;
                case '/': c='/'; break;
                case 'b': c='\b'; break;
                case 'f': c='\f'; break;
                case 'n': c='\n'; break;
                case 'r': c='\r'; break;
                case 't': c='\t'; break;
                case 'u': {
                    // Skip 4 hex digits, store '?'
                    for (int k=0;k<4;k++) j_get(j);
                    c='?';
                } break;
                default: return 0;
            }
        }
        if (o+1<outsz) out[o++]=c;
    }
    return 0;
}

static int j_parse_int(Json *j, int *out){
    j_skip_ws(j);
    int sign=1; if (j_peek(j)=='-'){ sign=-1; j_get(j);} 
    if (!isdigit((unsigned char)j_peek(j))) return 0;
    long v=0;
    while(isdigit((unsigned char)j_peek(j))){ v = v*10 + (j_get(j)-'0'); if (v>2147483647L) return 0; }
    *out = (int)(sign*v);
    return 1;
}

static int j_parse_bool(Json *j, int *out){
    j_skip_ws(j);
    if (j_peek(j)=='t'){ if (!j_match_lit(j,"true")) return 0; *out=1; return 1; }
    if (j_peek(j)=='f'){ if (!j_match_lit(j,"false")) return 0; *out=0; return 1; }
    return 0;
}

static void j_skip_value(Json *j);

static int j_skip_string(Json *j){ char tmp[2]; return j_parse_string(j,tmp,2); }

static void j_skip_array(Json *j){
    if (!j_expect(j,'[')) return;
    j_skip_ws(j);
    if (j_peek(j)==']'){ j_get(j); return; }
    for(;;){ j_skip_value(j); j_skip_ws(j); if (j_peek(j)==','){ j_get(j); continue; } else { j_expect(j,']'); break; } }
}

static void j_skip_object(Json *j){
    if (!j_expect(j,'{')) return;
    j_skip_ws(j);
    if (j_peek(j)=='}'){ j_get(j); return; }
    for(;;){ j_skip_string(j); j_skip_ws(j); j_expect(j,':'); j_skip_value(j); j_skip_ws(j); if (j_peek(j)==','){ j_get(j); continue; } else { j_expect(j,'}'); break; } }
}

static void j_skip_value(Json *j){
    j_skip_ws(j);
    char c = j_peek(j);
    if (c=='"'){ j_skip_string(j); return; }
    if (c=='{' ){ j_skip_object(j); return; }
    if (c=='[' ){ j_skip_array(j); return; }
    if (c=='t' || c=='f'){ int b; j_parse_bool(j,&b); return; }
    if (c=='n'){ j_match_lit(j,"null"); return; }
    if (c=='-' || isdigit((unsigned char)c)){ int x; j_parse_int(j,&x); return; }
    // fallback
    j_get(j);
}

static int load_tasks(const char *path, TaskList *L) {
    FILE *f = fopen(path, "rb");
    if (!f) return 1; // no file -> empty list is fine
    if (fseek(f, 0, SEEK_END)!=0){ fclose(f); return 0; }
    long n = ftell(f); if (n < 0){ fclose(f); return 0; }
    rewind(f);
    char *buf = (char*)xmalloc((size_t)n + 1);
    size_t rd = fread(buf, 1, (size_t)n, f); buf[rd] = '\0'; fclose(f);

    Json j; j_init(&j, buf, rd);
    if (!j_expect(&j,'[')) { free(buf); return 0; }
    j_skip_ws(&j);
    if (j_peek(&j)==']'){ j_get(&j); free(buf); return 1; }

    for(;;){
        if (!j_expect(&j,'{')) { free(buf); return 0; }
        Task t; memset(&t,0,sizeof(t));
        // default empty due -> ""
        for(;;){
            char key[32]; if (!j_parse_string(&j, key, sizeof key)) { free(buf); return 0; }
            if (!j_expect(&j,':')) { free(buf); return 0; }
            if (strcmp(key,"id")==0){ if (!j_parse_int(&j,&t.id)) { free(buf); return 0; } }
            else if (strcmp(key,"title")==0){ if (!j_parse_string(&j,t.title,sizeof t.title)) { free(buf); return 0; } }
            else if (strcmp(key,"due")==0){ if (!j_parse_string(&j,t.due,sizeof t.due)) { free(buf); return 0; } }
            else if (strcmp(key,"priority")==0){ if (!j_parse_int(&j,&t.priority)) { free(buf); return 0; } }
            else if (strcmp(key,"done")==0){ if (!j_parse_bool(&j,&t.done)) { free(buf); return 0; } }
            else { j_skip_value(&j); }
            j_skip_ws(&j);
            char c = j_get(&j);
            if (c==',') { j_skip_ws(&j); continue; }
            if (c=='}') break;
            free(buf); return 0;
        }
        if (!t.priority) t.priority = 3;
        if (!valid_date(t.due)) { free(buf); return 0; }
        list_push(L, t);
        j_skip_ws(&j);
        char c = j_get(&j);
        if (c==','){ j_skip_ws(&j); continue; }
        if (c==']') break;
        free(buf); return 0;
    }
    free(buf);
    return 1;
}

/*---------------- Input helpers ----------------*/
static void read_line(const char *prompt, char *buf, size_t n) {
    if (prompt) printf("%s", prompt);
    if (!fgets(buf, (int)n, stdin)) { buf[0]='\0'; return; }
    size_t len = strlen(buf);
    if (len && buf[len-1]=='\n') buf[len-1]='\0';
}

static int read_int_range(const char *prompt, int lo, int hi, int allow_empty, int *out) {
    char buf[64];
    for(;;){
        read_line(prompt, buf, sizeof buf);
        if (allow_empty && buf[0]=='\0') return 1;
        char *end; long v = strtol(buf, &end, 10);
        if (end!=buf && *end=='\0' && v>=lo && v<=hi){ *out=(int)v; return 1; }
        printf("Invalid. Enter %d..%d.\n", lo, hi);
    }
}

static int read_date(const char *prompt, int allow_empty, char out[DATE_LEN+1]){
    char buf[64];
    for(;;){
        read_line(prompt, buf, sizeof buf);
        if (allow_empty && buf[0]=='\0'){ out[0]='\0'; return 1; }
        if (valid_date(buf)){ strncpy(out, buf, DATE_LEN); out[DATE_LEN]='\0'; return 1; }
        printf("Invalid date. Use YYYY-MM-DD or empty.\n");
    }
}

/*---------------- Sorting & filtering ----------------*/
static int cmp_due_asc(const void *a, const void *b){
    const Task *x=a, *y=b;
    int dx = date_to_int(x->due), dy = date_to_int(y->due);
    if (dx!=dy) return dx<dy? -1: 1;
    if (x->priority!=y->priority) return y->priority - x->priority;
    return x->id - y->id;
}

static int cmp_priority_desc(const void *a, const void *b){
    const Task *x=a, *y=b;
    if (x->priority!=y->priority) return y->priority - x->priority;
    int dx = date_to_int(x->due), dy = date_to_int(y->due);
    if (dx!=dy) return dx<dy? -1: 1;
    return x->id - y->id;
}

/*---------------- Table output ----------------*/
static void print_rule(void){ printf("+------+--------------------------------+------------+----------+-------+\n"); }

static void print_header(void){
    print_rule();
    printf("| %-4s | %-30s | %-10s | %-8s | %-5s |\n", "ID", "Title", "Due", "Priority", "Done");
    print_rule();
}

static void print_task_row(const Task *t){
    char title[31];
    size_t n = strlen(t->title);
    if (n>30){ memcpy(title, t->title, 27); title[27]='.'; title[28]='.'; title[29]='.'; title[30]='\0'; }
    else { strncpy(title, t->title, sizeof title); title[30]='\0'; }
    printf("| %4d | %-30s | %-10s | %8d | %5s |\n", t->id, title, t->due[0]? t->due : "", t->priority, t->done? "yes":"no");
}

static void list_tasks(const TaskList *L){
    if (L->len==0){ printf("No tasks.\n"); return; }
    printf("Sort by: 1) due  2) priority  [1]: ");
    int choice=1; char ln[16]; if (fgets(ln, sizeof ln, stdin)){ if (ln[0]=='2') choice=2; }

    // Build a copy for sorting
    TaskList tmp; list_init(&tmp); list_reserve(&tmp, L->len); tmp.len = L->len; memcpy(tmp.items = (Task*)xrealloc(tmp.items, L->len*sizeof(Task)), L->items, L->len*sizeof(Task));
    qsort(tmp.items, tmp.len, sizeof(Task), choice==2? cmp_priority_desc : cmp_due_asc);

    // Filters
    char due_before[DATE_LEN+1] = ""; int have_due=0;
    printf("Filter due before (YYYY-MM-DD) or empty: ");
    if (fgets(ln, sizeof ln, stdin)){
        if (ln[0]!='\n'){ ln[strcspn(ln,"\n")]='\0'; if (valid_date(ln)){ strncpy(due_before, ln, DATE_LEN); due_before[DATE_LEN]='\0'; have_due= (due_before[0]!='\0'); } }
    }
    int minp=0; printf("Min priority [1-5] or 0 for none: ");
    if (fgets(ln, sizeof ln, stdin)){ int v=atoi(ln); if (v>=1&&v<=5) minp=v; }
    int only_open=0; printf("Only pending? 1=yes 0=no [0]: ");
    if (fgets(ln, sizeof ln, stdin)){ if (ln[0]=='1') only_open=1; }

    print_header();
    for (size_t i=0;i<tmp.len;i++){
        const Task *t=&tmp.items[i];
        if (have_due && date_to_int(t->due) >= date_to_int(due_before)) continue;
        if (minp && t->priority < minp) continue;
        if (only_open && t->done) continue;
        print_task_row(t);
    }
    print_rule();
    list_free(&tmp);
}

/*---------------- CRUD ----------------*/
static void add_task(TaskList *L){
    Task t; memset(&t,0,sizeof t);
    t.id = list_next_id(L);
    read_line("Title: ", t.title, sizeof t.title);
    while (t.title[0]=='\0'){ printf("Title required.\n"); read_line("Title: ", t.title, sizeof t.title); }
    read_date("Due (YYYY-MM-DD or empty): ", 1, t.due);
    read_int_range("Priority [1-5] (default 3): ", 1, 5, 1, &t.priority); if (!t.priority) t.priority=3;
    t.done=0;
    list_push(L, t);
    printf("Added id %d.\n", t.id);
}

static void update_task(TaskList *L){
    if (L->len==0){ printf("No tasks.\n"); return; }
    int id; if (!read_int_range("ID to update: ", 1, 100000000, 0, &id)) return;
    int idx = list_find_index_by_id(L, id);
    if (idx<0){ printf("Not found.\n"); return; }
    Task *t = &L->items[idx];
    char buf[256];
    printf("Title [%s]: ", t->title); read_line(NULL, buf, sizeof buf); if (buf[0]){ strncpy(t->title, buf, sizeof t->title); t->title[TITLE_MAX]='\0'; }
    printf("Due [%s]: ", t->due[0]? t->due : ""); read_line(NULL, buf, sizeof buf); if (buf[0]){ if (valid_date(buf)) { strncpy(t->due, buf, DATE_LEN); t->due[DATE_LEN]='\0'; } else printf("Ignored invalid date.\n"); }
    printf("Priority [%d]: ", t->priority); read_line(NULL, buf, sizeof buf); if (buf[0]){ int v=atoi(buf); if (v>=1&&v<=5) t->priority=v; else printf("Ignored invalid priority.\n"); }
    printf("Mark done? 1=yes 0=no [%d]: ", t->done); read_line(NULL, buf, sizeof buf); if (buf[0]){ if (buf[0]=='1') t->done=1; else if (buf[0]=='0') t->done=0; }
    printf("Updated.\n");
}

static void delete_task(TaskList *L){
    if (L->len==0){ printf("No tasks.\n"); return; }
    int id; if (!read_int_range("ID to delete: ", 1, 100000000, 0, &id)) return;
    int idx = list_find_index_by_id(L, id);
    if (idx<0){ printf("Not found.\n"); return; }
    list_delete_at(L, (size_t)idx);
    printf("Deleted.\n");
}

/*---------------- Menu ----------------*/
static void menu_loop(TaskList *L, const char *path){
    for(;;){
        printf("\n[Menu] 1)add 2)list 3)update 4)delete 5)save 6)quit\n> ");
        char line[16]; if (!fgets(line, sizeof line, stdin)) break;
        int choice = atoi(line);
        switch(choice){
            case 1: add_task(L); break;
            case 2: list_tasks(L); break;
            case 3: update_task(L); break;
            case 4: delete_task(L); break;
            case 5: if (save_tasks(path,L)) printf("Saved.\n"); else printf("Save failed.\n"); break;
            case 6: if (save_tasks(path,L)) printf("Saved. Bye.\n"); else printf("Save failed. Bye.\n"); return;
            default: printf("Choose 1-6.\n");
        }
    }
}

/*---------------- Tests ----------------*/
static int tasks_equal(const Task *a, const Task *b){
    return a->id==b->id && a->priority==b->priority && a->done==b->done && strcmp(a->title,b->title)==0 && strcmp(a->due,b->due)==0;
}

static int run_tests(void){
    TaskList A; list_init(&A);
    Task t1 = { .id=1, .title="Write \"docs\" \\ core", .due="2025-08-26", .priority=5, .done=0 };
    Task t2 = { .id=2, .title="Refactor", .due="", .priority=2, .done=1 };
    list_push(&A, t1); list_push(&A, t2);
    const char *path = "tasks_test.json";
    if (!save_tasks(path, &A)) { list_free(&A); return 0; }

    TaskList B; list_init(&B);
    if (!load_tasks(path, &B)) { list_free(&A); list_free(&B); return 0; }
    int ok = (A.len==B.len);
    if (ok){ for (size_t i=0;i<A.len;i++){ if (!tasks_equal(&A.items[i], &B.items[i])) { ok=0; break; } } }

    if (ok) printf("Test: round-trip OK (%zu items)\n", A.len);
    else printf("Test: round-trip FAILED\n");

    list_free(&A); list_free(&B);
    return ok;
}

/*---------------- Main ----------------*/
int main(int argc, char **argv){
    if (argc>=2 && strcmp(argv[1],"--test")==0){
        return run_tests()? 0: 1;
    }
    const char *path = (argc>=2 ? argv[1] : "tasks.json");
    TaskList L; list_init(&L);
    if (!load_tasks(path, &L)) {
        fprintf(stderr, "Failed to load %s\n", path);
        list_free(&L);
        return 1;
    }
    printf("[Start] Loaded %zu tasks from %s\n", L.len, path);
    menu_loop(&L, path);
    list_free(&L);
    printf("[End]\n");
    return 0;
}
