/*
 * Hospital Management System  (PBL project) - C version
 * =====================================================
 * Everything is built on data structures that we implement ourselves:
 *
 *   - Hash table   : patient records and resource records (chaining + resize)
 *   - Min-heap     : emergency queue (most severe first, ties by arrival order)
 *   - Graph + BFS  : "alternative resource" relationships; BFS finds the
 *                    NEAREST available substitute
 *   - Linked list  : treatment history log
 *   - Queue (array): used by BFS
 *
 * Severity levels (1 = most urgent):
 *   1 Critical | 2 Emergency | 3 Urgent | 4 Less urgent | 5 Non-urgent
 *
 * No database / files are used; all data lives in memory.
 *
 * Compile:  gcc -std=c99 -Wall -Wextra -o hospital hospital_management.c
 * Run:      ./hospital        (Windows: hospital.exe)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

/* ===================================================================
 *  SIZES
 * =================================================================== */
#define NAME_LEN      64
#define SYMPTOM_LEN   128
#define PHONE_LEN     16
#define RES_NAME_LEN  32
#define MAX_PRES      16      /* max resources held by one patient   */
#define MAX_NODES     64      /* max resources in the graph          */
#define MAX_ADJ       16      /* max alternatives per resource       */

/* ===================================================================
 *  DATA STRUCTURE 1: HASH TABLE (string key -> void* value)
 * =================================================================== */
typedef struct HNode {
    char key[NAME_LEN];
    void *value;
    struct HNode *next;
} HNode;

typedef struct {
    HNode **buckets;
    int size;
    int count;
} HashTable;

static unsigned long ht_hash(const char *key, int size)
{
    unsigned long long h = 0;
    for (; *key; key++)
        h = (h * 31ULL + (unsigned char)*key) % 1000000007ULL;
    return (unsigned long)(h % (unsigned long long)size);
}

static HashTable *ht_create(int size)
{
    HashTable *t = malloc(sizeof(HashTable));
    t->size = size;
    t->count = 0;
    t->buckets = calloc((size_t)size, sizeof(HNode *));
    return t;
}

static void ht_resize(HashTable *t)
{
    int new_size = t->size * 2 + 1;
    HNode **nb = calloc((size_t)new_size, sizeof(HNode *));
    for (int i = 0; i < t->size; i++) {
        HNode *n = t->buckets[i];
        while (n) {
            HNode *next = n->next;
            unsigned long idx = ht_hash(n->key, new_size);
            n->next = nb[idx];
            nb[idx] = n;
            n = next;
        }
    }
    free(t->buckets);
    t->buckets = nb;
    t->size = new_size;
}

static void ht_put(HashTable *t, const char *key, void *value)
{
    unsigned long idx = ht_hash(key, t->size);
    for (HNode *n = t->buckets[idx]; n; n = n->next) {
        if (strcmp(n->key, key) == 0) {
            n->value = value;
            return;
        }
    }
    HNode *n = malloc(sizeof(HNode));
    strncpy(n->key, key, NAME_LEN - 1);
    n->key[NAME_LEN - 1] = '\0';
    n->value = value;
    n->next = t->buckets[idx];
    t->buckets[idx] = n;
    t->count++;
    if (t->count > t->size * 2)
        ht_resize(t);
}

static void *ht_get(HashTable *t, const char *key)
{
    unsigned long idx = ht_hash(key, t->size);
    for (HNode *n = t->buckets[idx]; n; n = n->next)
        if (strcmp(n->key, key) == 0)
            return n->value;
    return NULL;
}

/* fills out[] (must hold t->count entries); returns number of values */
static int ht_values(HashTable *t, void **out)
{
    int k = 0;
    for (int i = 0; i < t->size; i++)
        for (HNode *n = t->buckets[i]; n; n = n->next)
            out[k++] = n->value;
    return k;
}

/* ===================================================================
 *  DATA STRUCTURE 2: MIN-HEAP (emergency / priority queue)
 *  Ordered by (severity, arrival number, patient id)
 * =================================================================== */
typedef struct {
    int severity;
    int arrival;
    char pid[8];
} HeapItem;

typedef struct {
    HeapItem *data;
    int size;
    int cap;
} MinHeap;

static int item_less(const HeapItem *a, const HeapItem *b)
{
    if (a->severity != b->severity) return a->severity < b->severity;
    if (a->arrival != b->arrival)   return a->arrival < b->arrival;
    return strcmp(a->pid, b->pid) < 0;
}

static void heap_init(MinHeap *h)
{
    h->cap = 16;
    h->size = 0;
    h->data = malloc((size_t)h->cap * sizeof(HeapItem));
}

static void heap_swap(MinHeap *h, int i, int j)
{
    HeapItem tmp = h->data[i];
    h->data[i] = h->data[j];
    h->data[j] = tmp;
}

static void heap_sift_up(MinHeap *h, int i)
{
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (item_less(&h->data[i], &h->data[parent])) {
            heap_swap(h, i, parent);
            i = parent;
        } else {
            break;
        }
    }
}

static void heap_sift_down(MinHeap *h, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, s = i;
        if (l < h->size && item_less(&h->data[l], &h->data[s])) s = l;
        if (r < h->size && item_less(&h->data[r], &h->data[s])) s = r;
        if (s == i) break;
        heap_swap(h, i, s);
        i = s;
    }
}

static void heap_push(MinHeap *h, HeapItem item)
{
    if (h->size == h->cap) {
        h->cap *= 2;
        h->data = realloc(h->data, (size_t)h->cap * sizeof(HeapItem));
    }
    h->data[h->size] = item;
    heap_sift_up(h, h->size);
    h->size++;
}

static int heap_pop(MinHeap *h, HeapItem *out)
{
    if (h->size == 0) return 0;
    *out = h->data[0];
    h->size--;
    if (h->size > 0) {
        h->data[0] = h->data[h->size];
        heap_sift_down(h, 0);
    }
    return 1;
}

/* remove the entry of a given patient (used on update / discharge) */
static int heap_remove_patient(MinHeap *h, const char *pid)
{
    for (int i = 0; i < h->size; i++) {
        if (strcmp(h->data[i].pid, pid) == 0) {
            h->size--;
            if (i < h->size) {
                h->data[i] = h->data[h->size];
                heap_sift_up(h, i);
                heap_sift_down(h, i);
            }
            return 1;
        }
    }
    return 0;
}

/* items in priority order WITHOUT disturbing the real heap.
 * out[] must hold h->size entries; returns the count. */
static int heap_sorted(const MinHeap *h, HeapItem *out)
{
    MinHeap tmp;
    tmp.cap = h->size > 0 ? h->size : 1;
    tmp.size = h->size;
    tmp.data = malloc((size_t)tmp.cap * sizeof(HeapItem));
    memcpy(tmp.data, h->data, (size_t)h->size * sizeof(HeapItem));
    int n = 0;
    HeapItem it;
    while (heap_pop(&tmp, &it))
        out[n++] = it;
    free(tmp.data);
    return n;
}

/* ===================================================================
 *  DATA STRUCTURE 3: SINGLY LINKED LIST (history log, O(1) append)
 * =================================================================== */
typedef struct LNode {
    char text[192];
    struct LNode *next;
} LNode;

typedef struct {
    LNode *head;
    LNode *tail;
    int length;
} LinkedList;

static void list_append(LinkedList *l, const char *text)
{
    LNode *n = malloc(sizeof(LNode));
    strncpy(n->text, text, sizeof(n->text) - 1);
    n->text[sizeof(n->text) - 1] = '\0';
    n->next = NULL;
    if (l->tail == NULL) l->head = l->tail = n;
    else { l->tail->next = n; l->tail = n; }
    l->length++;
}

/* ===================================================================
 *  DATA STRUCTURE 4: GRAPH (adjacency lists) + BFS
 *  An edge means "can substitute for each other".
 * =================================================================== */
typedef struct {
    char name[RES_NAME_LEN];
    int adj[MAX_ADJ];
    int deg;
} GNode;

typedef struct {
    GNode nodes[MAX_NODES];
    int n;
} Graph;

static int g_find(const Graph *g, const char *name)
{
    for (int i = 0; i < g->n; i++)
        if (strcmp(g->nodes[i].name, name) == 0)
            return i;
    return -1;
}

static int g_add_node(Graph *g, const char *name)
{
    int i = g_find(g, name);
    if (i >= 0) return i;
    if (g->n >= MAX_NODES) return -1;
    strncpy(g->nodes[g->n].name, name, RES_NAME_LEN - 1);
    g->nodes[g->n].name[RES_NAME_LEN - 1] = '\0';
    g->nodes[g->n].deg = 0;
    return g->n++;
}

static void g_link(Graph *g, int a, int b)
{
    for (int i = 0; i < g->nodes[a].deg; i++)
        if (g->nodes[a].adj[i] == b) return;
    if (g->nodes[a].deg < MAX_ADJ)
        g->nodes[a].adj[g->nodes[a].deg++] = b;
}

static void g_add_edge(Graph *g, const char *a, const char *b)
{
    int ia = g_add_node(g, a), ib = g_add_node(g, b);
    if (ia < 0 || ib < 0) return;
    g_link(g, ia, ib);
    g_link(g, ib, ia);
}

/* resources reachable from `start`, nearest first (start excluded).
 * order[] must hold MAX_NODES ints; returns the count. */
static int g_bfs_order(const Graph *g, const char *start, int *order)
{
    int s = g_find(g, start);
    if (s < 0) return 0;
    int visited[MAX_NODES] = {0};
    int queue[MAX_NODES];
    int qh = 0, qt = 0, count = 0;
    visited[s] = 1;
    queue[qt++] = s;
    while (qh < qt) {
        int cur = queue[qh++];
        for (int i = 0; i < g->nodes[cur].deg; i++) {
            int nxt = g->nodes[cur].adj[i];
            if (!visited[nxt]) {
                visited[nxt] = 1;
                order[count++] = nxt;
                queue[qt++] = nxt;
            }
        }
    }
    return count;
}

/* ===================================================================
 *  DOMAIN TYPES + GLOBAL HOSPITAL STATE
 * =================================================================== */
enum { WAITING = 0, IN_TREATMENT = 1, DISCHARGED = 2 };

static const char *STATUS_NAMES[] = {"Waiting", "In Treatment", "Discharged"};
static const char *SEVERITY_NAMES[] = {"", "Critical", "Emergency", "Urgent",
                                       "Less urgent", "Non-urgent"};

/* resources a patient needs, by severity */
static const char *REQ[6][3] = {
    {NULL, NULL, NULL},
    {"ICU Bed", "Ventilator", "Doctor"},
    {"HDU Bed", "Oxygen Cylinder", "Doctor"},
    {"General Bed", "Doctor", NULL},
    {"Doctor", NULL, NULL},
    {"Doctor", NULL, NULL}
};

typedef struct {
    char pid[8];
    char name[NAME_LEN];
    int age;
    char gender[8];
    char phone[PHONE_LEN];
    char symptoms[SYMPTOM_LEN];
    int severity;
    int arrival_no;
    char arrival_time[24];
    int status;
    char resources[MAX_PRES][RES_NAME_LEN];
    int res_count;
} Patient;

typedef struct {
    char name[RES_NAME_LEN];
    int total;
    int available;
} Resource;

static HashTable *patients;
static HashTable *resources;
static MinHeap    queue;
static Graph      alt;
static LinkedList history;
static int next_id = 1;
static int arrival_counter = 0;

/* ===================================================================
 *  HELPERS
 * =================================================================== */
static int ci_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static void history_log(const char *fmt, ...)
{
    char msg[160], line[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    time_t now = time(NULL);
    char ts[16];
    strftime(ts, sizeof ts, "%H:%M:%S", localtime(&now));
    snprintf(line, sizeof line, "[%s] %s", ts, msg);
    list_append(&history, line);
}

static Patient *get_patient(const char *pid)
{
    char key[16];
    size_t i;
    for (i = 0; pid[i] && i < sizeof key - 1; i++)
        key[i] = (char)toupper((unsigned char)pid[i]);
    key[i] = '\0';
    return ht_get(patients, key);
}

static int cmp_patient_arrival(const void *a, const void *b)
{
    const Patient *pa = *(Patient *const *)a, *pb = *(Patient *const *)b;
    return pa->arrival_no - pb->arrival_no;
}

static int cmp_resource_name(const void *a, const void *b)
{
    const Resource *ra = *(Resource *const *)a, *rb = *(Resource *const *)b;
    return strcmp(ra->name, rb->name);
}

/* ===================================================================
 *  RESOURCE LOGIC
 * =================================================================== */
static Resource *add_resource(const char *name, int qty)
{
    Resource *r = malloc(sizeof(Resource));
    strncpy(r->name, name, RES_NAME_LEN - 1);
    r->name[RES_NAME_LEN - 1] = '\0';
    r->total = qty;
    r->available = qty;
    ht_put(resources, r->name, r);
    g_add_node(&alt, r->name);
    return r;
}

static void setup_resources(void)
{
    static const struct { const char *name; int qty; } stock[] = {
        {"ICU Bed", 2}, {"HDU Bed", 2}, {"General Bed", 5},
        {"Ventilator", 1}, {"BiPAP Machine", 1}, {"Oxygen Cylinder", 3},
        {"Doctor", 2}, {"Junior Doctor", 2},
        {"Operation Theatre", 1}, {"Minor OT", 1}, {"Ambulance", 2}
    };
    for (size_t i = 0; i < sizeof stock / sizeof stock[0]; i++)
        add_resource(stock[i].name, stock[i].qty);

    /* substitution graph */
    g_add_edge(&alt, "ICU Bed", "HDU Bed");
    g_add_edge(&alt, "HDU Bed", "General Bed");
    g_add_edge(&alt, "Ventilator", "BiPAP Machine");
    g_add_edge(&alt, "BiPAP Machine", "Oxygen Cylinder");
    g_add_edge(&alt, "Doctor", "Junior Doctor");
    g_add_edge(&alt, "Operation Theatre", "Minor OT");
}

/* nearest available substitute (BFS over the graph) */
static int find_alternative(const char *name, char *out)
{
    int order[MAX_NODES];
    int n = g_bfs_order(&alt, name, order);
    for (int i = 0; i < n; i++) {
        const char *cand = alt.nodes[order[i]].name;
        Resource *r = ht_get(resources, cand);
        if (r && r->available > 0) {
            strcpy(out, cand);
            return 1;
        }
    }
    return 0;
}

/* allocate a resource, falling back to an alternative.
 * returns 1 on success; out = resource actually used; *sub = 1 if substituted */
static int allocate_resource(const char *name, char *out, int *sub)
{
    Resource *r = ht_get(resources, name);
    if (!r) return 0;
    if (r->available > 0) {
        r->available--;
        strcpy(out, name);
        *sub = 0;
        return 1;
    }
    char altname[RES_NAME_LEN];
    if (find_alternative(name, altname)) {
        Resource *a = ht_get(resources, altname);
        a->available--;
        strcpy(out, altname);
        *sub = 1;
        return 1;
    }
    return 0;
}

static void release_resources(Patient *p)
{
    for (int i = 0; i < p->res_count; i++) {
        Resource *r = ht_get(resources, p->resources[i]);
        if (r && r->available < r->total)
            r->available++;
    }
    p->res_count = 0;
}

/* ===================================================================
 *  PATIENT LOGIC
 * =================================================================== */
static Patient *register_patient(const char *name, int age, const char *gender,
                                 const char *phone, const char *symptoms, int severity)
{
    Patient *p = calloc(1, sizeof(Patient));
    snprintf(p->pid, sizeof p->pid, "P%03d", next_id++);
    snprintf(p->name, sizeof p->name, "%s", name);
    p->age = age;
    snprintf(p->gender, sizeof p->gender, "%s", gender);
    snprintf(p->phone, sizeof p->phone, "%s", phone);
    snprintf(p->symptoms, sizeof p->symptoms, "%s", symptoms);
    p->severity = severity;
    p->arrival_no = ++arrival_counter;
    time_t now = time(NULL);
    strftime(p->arrival_time, sizeof p->arrival_time, "%d-%b %H:%M:%S", localtime(&now));
    p->status = WAITING;
    p->res_count = 0;
    ht_put(patients, p->pid, p);

    HeapItem it;
    it.severity = p->severity;
    it.arrival = p->arrival_no;
    snprintf(it.pid, sizeof it.pid, "%s", p->pid);
    heap_push(&queue, it);
    return p;
}

/* try to allocate everything the patient needs; prints a report */
static int try_treat(Patient *p)
{
    char used[3][RES_NAME_LEN];
    int nused = 0;

    for (int i = 0; i < 3 && REQ[p->severity][i]; i++) {
        const char *need = REQ[p->severity][i];
        char got[RES_NAME_LEN];
        int sub = 0;
        if (!allocate_resource(need, got, &sub)) {
            printf("  [X] %s: UNAVAILABLE and no alternative found\n", need);
            for (int j = 0; j < nused; j++) {          /* roll back */
                Resource *r = ht_get(resources, used[j]);
                if (r) r->available++;
            }
            return 0;
        }
        strcpy(used[nused++], got);
        if (sub) printf("  [~] %s unavailable -> using alternative: %s\n", need, got);
        else     printf("  [OK] %s allocated\n", need);
    }

    char list[128] = "";
    p->res_count = 0;
    for (int j = 0; j < nused; j++) {
        strcpy(p->resources[p->res_count++], used[j]);
        if (j > 0) strncat(list, ", ", sizeof list - strlen(list) - 1);
        strncat(list, used[j], sizeof list - strlen(list) - 1);
    }
    p->status = IN_TREATMENT;
    history_log("%s %s (severity %d) treated using %s", p->pid, p->name, p->severity, list);
    return 1;
}

/* ===================================================================
 *  CONSOLE INPUT HELPERS
 * =================================================================== */
static void read_line(const char *prompt, char *out, size_t outsize)
{
    char tmp[512];
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(tmp, sizeof tmp, stdin)) {
        printf("\nGoodbye!\n");
        exit(0);
    }
    /* if the line was longer than the buffer, discard the rest */
    if (!strchr(tmp, '\n')) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }
    size_t len = strlen(tmp);
    while (len > 0 && isspace((unsigned char)tmp[len - 1])) tmp[--len] = '\0';
    char *start = tmp;
    while (*start && isspace((unsigned char)*start)) start++;
    snprintf(out, outsize, "%s", start);
}

/* returns 1 if a value was entered (non-blank), 0 if blank was allowed */
static int ask_text(const char *prompt, char *out, size_t size, int allow_blank)
{
    for (;;) {
        read_line(prompt, out, size);
        if (out[0] != '\0') return 1;
        if (allow_blank) return 0;
        printf("  Value cannot be empty.\n");
    }
}

static int ask_int(const char *prompt, int lo, int hi, int allow_blank, int *out)
{
    char buf[64];
    for (;;) {
        read_line(prompt, buf, sizeof buf);
        if (allow_blank && buf[0] == '\0') return 0;
        int ok = buf[0] != '\0' && strlen(buf) < 9;
        for (char *c = buf; *c; c++)
            if (!isdigit((unsigned char)*c)) ok = 0;
        if (ok) {
            long v = atol(buf);
            if (v >= lo && v <= hi) { *out = (int)v; return 1; }
        }
        printf("  Enter a number between %d and %d.\n", lo, hi);
    }
}

static int ask_severity(int allow_blank, int *out)
{
    printf("  Severity: 1=Critical 2=Emergency 3=Urgent 4=Less urgent 5=Non-urgent\n");
    return ask_int(allow_blank ? "  New severity (1-5, Enter to skip): "
                               : "  Severity (1-5): ", 1, 5, allow_blank, out);
}

static void ask_gender(char *out, size_t size)
{
    char buf[16];
    for (;;) {
        read_line("  Gender (M/F/O): ", buf, sizeof buf);
        char g = (char)toupper((unsigned char)buf[0]);
        if (buf[1] == '\0' && (g == 'M' || g == 'F' || g == 'O')) {
            snprintf(out, size, "%s", g == 'M' ? "Male" : g == 'F' ? "Female" : "Other");
            return;
        }
        printf("  Enter M, F or O.\n");
    }
}

static int ask_phone(char *out, size_t size, int allow_blank)
{
    for (;;) {
        read_line(allow_blank ? "  Phone (10 digits) (Enter to skip): "
                              : "  Phone (10 digits): ", out, size);
        if (allow_blank && out[0] == '\0') return 0;
        int ok = strlen(out) == 10;
        for (char *c = out; *c; c++)
            if (!isdigit((unsigned char)*c)) ok = 0;
        if (ok) return 1;
        printf("  Phone must be exactly 10 digits.\n");
    }
}

static Patient *find_patient_or_warn(void)
{
    char pid[32];
    ask_text("  Patient ID (e.g. P001): ", pid, sizeof pid, 0);
    Patient *p = get_patient(pid);
    if (!p) printf("  Patient not found.\n");
    return p;
}

static Resource *pick_resource(void)
{
    char name[RES_NAME_LEN];
    ask_text("  Resource name: ", name, sizeof name, 0);
    void **vals = malloc((size_t)resources->count * sizeof(void *));
    int n = ht_values(resources, vals);
    Resource *found = NULL;
    for (int i = 0; i < n; i++) {
        Resource *r = vals[i];
        if (ci_equal(r->name, name)) { found = r; break; }
    }
    free(vals);
    if (!found)
        printf("  Resource not found. Use 'View resources' to see exact names.\n");
    return found;
}

/* ===================================================================
 *  MENU ACTIONS
 * =================================================================== */
static void print_patient_header(void)
{
    printf("%-6s%-19s%-5s%-8s%-13s%-14s\n", "ID", "Name", "Age", "Gender", "Severity", "Status");
    printf("-----------------------------------------------------------------\n");
}

static void print_patient_detail(const Patient *p)
{
    printf("\n  ID           : %s\n", p->pid);
    printf("  Name         : %s\n", p->name);
    printf("  Age / Gender : %d / %s\n", p->age, p->gender);
    printf("  Phone        : %s\n", p->phone);
    printf("  Symptoms     : %s\n", p->symptoms);
    printf("  Severity     : %d (%s)\n", p->severity, SEVERITY_NAMES[p->severity]);
    printf("  Arrived      : %s\n", p->arrival_time);
    printf("  Status       : %s\n", STATUS_NAMES[p->status]);
    printf("  Resources    : ");
    if (p->res_count == 0) printf("-");
    for (int i = 0; i < p->res_count; i++)
        printf("%s%s", i ? ", " : "", p->resources[i]);
    printf("\n");
}

static void register_action(void)
{
    char name[NAME_LEN], gender[8], phone[PHONE_LEN], symptoms[SYMPTOM_LEN];
    int age, severity;
    printf("\n--- Register Patient ---\n");
    ask_text("  Name: ", name, sizeof name, 0);
    ask_int("  Age: ", 0, 120, 0, &age);
    ask_gender(gender, sizeof gender);
    ask_phone(phone, sizeof phone, 0);
    ask_text("  Symptoms / complaint: ", symptoms, sizeof symptoms, 0);
    ask_severity(0, &severity);

    Patient *p = register_patient(name, age, gender, phone, symptoms, severity);

    HeapItem *items = malloc((size_t)queue.size * sizeof(HeapItem));
    int n = heap_sorted(&queue, items);
    int pos = 0;
    for (int i = 0; i < n; i++)
        if (strcmp(items[i].pid, p->pid) == 0) { pos = i + 1; break; }
    free(items);
    printf("\n  Registered %s with ID %s. Queue position: %d of %d\n",
           p->name, p->pid, pos, queue.size);
}

static void view_all(void)
{
    printf("\n--- All Patients ---\n");
    if (patients->count == 0) {
        printf("  No patients registered yet.\n");
        return;
    }
    Patient **arr = malloc((size_t)patients->count * sizeof(Patient *));
    int n = ht_values(patients, (void **)arr);
    qsort(arr, (size_t)n, sizeof(Patient *), cmp_patient_arrival);
    print_patient_header();
    for (int i = 0; i < n; i++) {
        Patient *p = arr[i];
        printf("%-6s%-19.18s%-5d%-8s%-13s%-14s\n", p->pid, p->name, p->age, p->gender,
               SEVERITY_NAMES[p->severity], STATUS_NAMES[p->status]);
    }
    free(arr);
}

static void search_action(void)
{
    Patient *p = find_patient_or_warn();
    if (p) print_patient_detail(p);
}

static void update_action(void)
{
    printf("\n--- Update Patient (press Enter to keep old value) ---\n");
    Patient *p = find_patient_or_warn();
    if (!p) return;
    if (p->status == DISCHARGED) {
        printf("  Patient is already discharged.\n");
        return;
    }
    char name[NAME_LEN], phone[PHONE_LEN], symptoms[SYMPTOM_LEN];
    int age = 0, severity = 0;

    int has_name = ask_text("  New name: ", name, sizeof name, 1);
    int has_age = ask_int("  New age (Enter to skip): ", 0, 120, 1, &age);
    int has_phone = ask_phone(phone, sizeof phone, 1);
    int has_sym = ask_text("  New symptoms: ", symptoms, sizeof symptoms, 1);
    int has_sev = ask_severity(1, &severity);

    if (has_name)  snprintf(p->name, sizeof p->name, "%s", name);
    if (has_age)   p->age = age;
    if (has_phone) snprintf(p->phone, sizeof p->phone, "%s", phone);
    if (has_sym)   snprintf(p->symptoms, sizeof p->symptoms, "%s", symptoms);
    if (has_sev) {
        p->severity = severity;
        if (p->status == WAITING) {            /* re-prioritise the queue */
            heap_remove_patient(&queue, p->pid);
            HeapItem it;
            it.severity = p->severity;
            it.arrival = p->arrival_no;
            snprintf(it.pid, sizeof it.pid, "%s", p->pid);
            heap_push(&queue, it);
        }
    }
    printf("  Patient updated.%s\n",
           (has_sev && p->status == WAITING) ? " Queue re-prioritised." : "");
}

static void discharge_action(void)
{
    Patient *p = find_patient_or_warn();
    if (!p) return;
    if (p->status == DISCHARGED) {
        printf("  Patient already discharged.\n");
        return;
    }
    if (p->status == WAITING)
        heap_remove_patient(&queue, p->pid);
    release_resources(p);
    p->status = DISCHARGED;
    history_log("%s %s discharged", p->pid, p->name);
    printf("  %s discharged. Resources released.\n", p->name);
}

static void view_queue(void)
{
    printf("\n--- Emergency Queue (highest priority first) ---\n");
    if (queue.size == 0) {
        printf("  Queue is empty.\n");
        return;
    }
    HeapItem *items = malloc((size_t)queue.size * sizeof(HeapItem));
    int n = heap_sorted(&queue, items);
    printf("%-5s%-6s%-19s%-13s%s\n", "Pos", "ID", "Name", "Severity", "Symptoms");
    printf("-----------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        Patient *p = get_patient(items[i].pid);
        printf("%-5d%-6s%-19.18s%-13s%.25s\n", i + 1, p->pid, p->name,
               SEVERITY_NAMES[items[i].severity], p->symptoms);
    }
    free(items);
}

static void treat_next(void)
{
    printf("\n--- Treat Next Patient ---\n");
    if (queue.size == 0) {
        printf("  No patients waiting.\n");
        return;
    }
    HeapItem *items = malloc((size_t)queue.size * sizeof(HeapItem));
    int n = heap_sorted(&queue, items);
    char skipped[256] = "";

    for (int i = 0; i < n; i++) {
        Patient *p = get_patient(items[i].pid);
        printf("  Next in line: %s %s - %s\n", p->pid, p->name, SEVERITY_NAMES[items[i].severity]);
        if (try_treat(p)) {
            heap_remove_patient(&queue, p->pid);
            printf("  >> %s is now IN TREATMENT.\n", p->name);
            if (skipped[0])
                printf("  (Higher-priority patients still waiting for resources: %s)\n", skipped);
            free(items);
            return;
        }
        printf("  %s must keep waiting - required resources not available.\n", p->name);
        if (skipped[0]) strncat(skipped, ", ", sizeof skipped - strlen(skipped) - 1);
        strncat(skipped, p->pid, sizeof skipped - strlen(skipped) - 1);

        char ans[16];
        read_line("  Treat the next patient in line instead? (y/n): ", ans, sizeof ans);
        if (tolower((unsigned char)ans[0]) != 'y' || ans[1] != '\0') {
            free(items);
            return;
        }
    }
    free(items);
    printf("  No waiting patient can be treated with the current resources.\n");
}

static void view_resources(void)
{
    printf("\n--- Resources ---\n");
    Resource **arr = malloc((size_t)resources->count * sizeof(Resource *));
    int n = ht_values(resources, (void **)arr);
    qsort(arr, (size_t)n, sizeof(Resource *), cmp_resource_name);
    printf("%-18s%-8s%-11s%-8s\n", "Resource", "Total", "Available", "In use");
    printf("---------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        Resource *r = arr[i];
        printf("%-18s%-8d%-11d%-8d%s\n", r->name, r->total, r->available,
               r->total - r->available, r->available == 0 ? "  <-- NONE LEFT" : "");
    }
    free(arr);
}

static void update_availability(void)
{
    printf("\n--- Update Availability ---\n");
    Resource *r = pick_resource();
    if (!r) return;
    printf("  %s: %d/%d available\n", r->name, r->available, r->total);
    char prompt[96];
    int v;
    snprintf(prompt, sizeof prompt, "  Set available count (0-%d): ", r->total);
    ask_int(prompt, 0, r->total, 0, &v);
    r->available = v;
    printf("  Updated. %s: %d/%d available\n", r->name, r->available, r->total);
}

static void add_stock(void)
{
    char name[RES_NAME_LEN];
    int qty;
    printf("\n--- Add Stock / New Resource ---\n");
    ask_text("  Resource name: ", name, sizeof name, 0);

    Resource *r = NULL;
    void **vals = malloc((size_t)resources->count * sizeof(void *));
    int n = ht_values(resources, vals);
    for (int i = 0; i < n; i++)
        if (ci_equal(((Resource *)vals[i])->name, name)) { r = vals[i]; break; }
    free(vals);

    ask_int("  Quantity to add (1-100): ", 1, 100, 0, &qty);
    if (!r) r = add_resource(name, 0);
    r->total += qty;
    r->available += qty;
    printf("  %s: now %d/%d available\n", r->name, r->available, r->total);
}

static void show_alternatives(void)
{
    printf("\n--- Find Alternatives ---\n");
    Resource *r = pick_resource();
    if (!r) return;
    int order[MAX_NODES];
    int n = g_bfs_order(&alt, r->name, order);
    if (n == 0) {
        printf("  %s has no registered alternatives.\n", r->name);
        return;
    }
    printf("  Alternatives for %s (nearest first):\n", r->name);
    for (int i = 0; i < n; i++) {
        Resource *a = ht_get(resources, alt.nodes[order[i]].name);
        printf("    - %s: %d/%d available\n", a->name, a->available, a->total);
    }
    char best[RES_NAME_LEN];
    if (find_alternative(r->name, best))
        printf("  Best available alternative: %s\n", best);
    else
        printf("  Best available alternative: NONE available right now\n");
}

static void manual_allocate(void)
{
    printf("\n--- Allocate Resource to Patient ---\n");
    Patient *p = find_patient_or_warn();
    if (!p) return;
    if (p->status == DISCHARGED) {
        printf("  Patient is discharged.\n");
        return;
    }
    if (p->res_count >= MAX_PRES) {
        printf("  Patient already holds the maximum number of resources.\n");
        return;
    }
    Resource *r = pick_resource();
    if (!r) return;
    char got[RES_NAME_LEN];
    int sub = 0;
    if (!allocate_resource(r->name, got, &sub)) {
        printf("  %s is unavailable and no alternative exists.\n", r->name);
        return;
    }
    strcpy(p->resources[p->res_count++], got);
    printf("  %s%s allocated to %s.\n", sub ? "Alternative " : "", got, p->name);
}

static void view_history(void)
{
    printf("\n--- Treatment History ---\n");
    if (history.length == 0)
        printf("  No activity yet.\n");
    for (LNode *n = history.head; n; n = n->next)
        printf("  %s\n", n->text);
}

static void load_demo(void)
{
    static const struct {
        const char *name; int age; const char *gender;
        const char *phone; const char *symptoms; int severity;
    } demo[] = {
        {"Ravi Kumar", 45, "Male", "9876543210", "Chest pain, breathlessness", 1},
        {"Anita Sharma", 30, "Female", "9123456780", "Fractured arm", 3},
        {"Gurpreet Singh", 60, "Male", "9988776655", "Severe bleeding after accident", 1},
        {"Meena Devi", 25, "Female", "9090909090", "Mild fever", 5},
        {"Arjun Verma", 52, "Male", "9811122233", "High fever, difficulty breathing", 2},
        {"Simran Kaur", 8, "Female", "9700011122", "Cut on hand", 4}
    };
    int n = (int)(sizeof demo / sizeof demo[0]);
    for (int i = 0; i < n; i++)
        register_patient(demo[i].name, demo[i].age, demo[i].gender,
                         demo[i].phone, demo[i].symptoms, demo[i].severity);
    printf("  Loaded %d demo patients.\n", n);
}

static void print_menu(void)
{
    printf("\n================ HOSPITAL MANAGEMENT SYSTEM ================\n");
    printf(" PATIENTS                         EMERGENCY QUEUE\n");
    printf("  1. Register patient              6. View emergency queue\n");
    printf("  2. View all patients             7. Treat next patient\n");
    printf("  3. Search patient by ID\n");
    printf("  4. Update patient (incl. severity)   RESOURCES\n");
    printf("  5. Discharge patient             8. View resources\n");
    printf("                                   9. Update availability\n");
    printf(" OTHER                            10. Add stock / new resource\n");
    printf(" 13. Treatment history            11. Find alternatives\n");
    printf(" 14. Load demo patients           12. Allocate resource to patient\n");
    printf("  0. Exit\n");
    printf("============================================================\n");
}

/* ===================================================================
 *  MAIN
 * =================================================================== */
int main(void)
{
    patients  = ht_create(11);
    resources = ht_create(11);
    heap_init(&queue);
    alt.n = 0;
    history.head = history.tail = NULL;
    history.length = 0;
    setup_resources();

    for (;;) {
        print_menu();
        int choice;
        ask_int("Enter choice: ", 0, 14, 0, &choice);
        switch (choice) {
            case 0:  printf("Goodbye!\n"); return 0;
            case 1:  register_action();      break;
            case 2:  view_all();             break;
            case 3:  search_action();        break;
            case 4:  update_action();        break;
            case 5:  discharge_action();     break;
            case 6:  view_queue();           break;
            case 7:  treat_next();           break;
            case 8:  view_resources();       break;
            case 9:  update_availability();  break;
            case 10: add_stock();            break;
            case 11: show_alternatives();    break;
            case 12: manual_allocate();      break;
            case 13: view_history();         break;
            case 14: load_demo();            break;
        }
    }
}