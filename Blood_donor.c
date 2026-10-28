/* BLOOD DONOR MANAGEMENT & EMERGENCY MATCHING SYSTEM (compact version)
   - Donor database: add / view / search by blood group or area
   - Blood requests with urgency (pending list works as a priority queue)
   - Matching: blood compatibility + 90-day rule + same-area first
   - Donation history log and reports
   Files used: donors.txt, requests.txt, history.txt,6 story.txt */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX 200
#define GAP 90                 /* minimum days between two donations */
#define DAY 86400L

typedef struct { int id, age, count; char name[30], sex[4], bg[5], phone[15], area[20]; long last; } Donor;
typedef struct { int id, units, urgency, done; char patient[30], bg[5], area[20]; long date; } Request;

Donor D[MAX];   int nd = 0;
Request R[MAX]; int nr = 0;
const char *BG[8] = {"O-", "O+", "A-", "A+", "B-", "B+", "AB-", "AB+"};
const char *URG[4] = {"", "HIGH", "MEDIUM", "LOW"};
char *mArea;                   /* area used by the match sorter */

/* ---------- input helpers ---------- */
void ask(const char *p, char *buf, int n) {
    char t[100];
    printf("%s", p);
    if (!fgets(t, 100, stdin)) exit(0);
    t[strcspn(t, "\n")] = 0;
    if (!t[0]) strcpy(t, "-");                 /* keeps file format safe */
    strncpy(buf, t, n - 1); buf[n - 1] = 0;
}
int askInt(const char *p) { char b[20]; ask(p, b, 20); return atoi(b); }

void askBG(char *bg) {
    while (1) {
        ask("Blood group (O-,O+,A-,A+,B-,B+,AB-,AB+): ", bg, 5);
        for (char *c = bg; *c; c++) *c = toupper(*c);
        for (int i = 0; i < 8; i++) if (!strcmp(bg, BG[i])) return;
        printf("Invalid blood group.\n");
    }
}

long askDate() {
    struct tm t = {0};
    char b[30];
    ask("Last donation date (dd mm yyyy): ", b, 30);
    if (sscanf(b, "%d %d %d", &t.tm_mday, &t.tm_mon, &t.tm_year) != 3) {
        printf("Invalid date, using today.\n");
        return (long)time(NULL);
    }
    t.tm_mon--; t.tm_year -= 1900; t.tm_hour = 12;
    return (long)mktime(&t);
}

/* ---------- small utilities ---------- */
int same(const char *a, const char *b) {       /* case-insensitive compare */
    for (; *a && tolower(*a) == tolower(*b); a++, b++);
    return tolower(*a) == tolower(*b);
}

char *fmt(long t) {                            /* date as dd-mm-yyyy */
    static char s[12];
    time_t x = t;
    if (!t) return "Never";
    strftime(s, 12, "%d-%m-%Y", localtime(&x));
    return s;
}

/* can donor blood d be given to recipient r? */
int compat(const char *d, const char *r) {
    int dl = strlen(d), rl = strlen(r);
    if (d[dl - 1] == '+' && r[rl - 1] == '-') return 0;   /* Rh+ -> Rh- not allowed */
    if (d[0] == 'O' || !strncmp(r, "AB", 2)) return 1;    /* O gives all, AB takes all */
    return dl == rl && !strncmp(d, r, dl - 1);            /* otherwise same ABO group */
}

int eligible(Donor *d) { return d->last == 0 || difftime(time(NULL), d->last) >= GAP * DAY; }

int findReq(int id)   { for (int i = 0; i < nr; i++) if (R[i].id == id) return i; return -1; }
int findDonor(int id) { for (int i = 0; i < nd; i++) if (D[i].id == id) return i; return -1; }

/* ---------- file storage ---------- */
void save() {
    FILE *f = fopen("donors.txt", "w");
    if (f) {
        for (int i = 0; i < nd; i++)
            fprintf(f, "%d|%s|%d|%s|%s|%s|%s|%ld|%d\n", D[i].id, D[i].name, D[i].age,
                    D[i].sex, D[i].bg, D[i].phone, D[i].area, D[i].last, D[i].count);
        fclose(f);
    }
    f = fopen("requests.txt", "w");
    if (f) {
        for (int i = 0; i < nr; i++)
            fprintf(f, "%d|%s|%s|%d|%s|%d|%d|%ld\n", R[i].id, R[i].patient, R[i].bg,
                    R[i].units, R[i].area, R[i].urgency, R[i].done, R[i].date);
        fclose(f);
    }
}

void load() {
    FILE *f = fopen("donors.txt", "r");
    if (f) {
        while (nd < MAX && fscanf(f, "%d|%29[^|]|%d|%3[^|]|%4[^|]|%14[^|]|%19[^|]|%ld|%d\n",
               &D[nd].id, D[nd].name, &D[nd].age, D[nd].sex, D[nd].bg, D[nd].phone,
               D[nd].area, &D[nd].last, &D[nd].count) == 9) nd++;
        fclose(f);
    }
    f = fopen("requests.txt", "r");
    if (f) {
        while (nr < MAX && fscanf(f, "%d|%29[^|]|%4[^|]|%d|%19[^|]|%d|%d|%ld\n",
               &R[nr].id, R[nr].patient, R[nr].bg, &R[nr].units, R[nr].area,
               &R[nr].urgency, &R[nr].done, &R[nr].date) == 8) nr++;
        fclose(f);
    }
}

/* ---------- donors ---------- */
void donorHead() {
    printf("\n%-4s %-16s %-4s %-4s %-5s %-13s %-12s %-11s %-4s %s\n",
           "ID", "Name", "Age", "Sex", "BGrp", "Contact", "Area", "LastDonated", "Elig", "Count");
}

void donorRow(Donor *d) {
    printf("%-4d %-16s %-4d %-4s %-5s %-13s %-12s %-11s %-4s %d\n", d->id, d->name, d->age,
           d->sex, d->bg, d->phone, d->area, fmt(d->last), eligible(d) ? "Yes" : "No", d->count);
}

void addDonor() {
    if (nd >= MAX) { printf("Donor list is full.\n"); return; }
    Donor *d = &D[nd];
    d->id = nd ? D[nd - 1].id + 1 : 1;
    ask("Name: ", d->name, 30);
    d->age = askInt("Age: ");
    ask("Gender (M/F/O): ", d->sex, 4);
    askBG(d->bg);
    ask("Contact: ", d->phone, 15);
    ask("Area: ", d->area, 20);
    d->last = 0; d->count = 0;
    if (askInt("Donated before? (1=Yes, 0=No): ")) { d->last = askDate(); d->count = 1; }
    nd++; save();
    printf("Donor added with ID %d\n", d->id);
}

void viewDonors() {
    if (!nd) { printf("No donors yet.\n"); return; }
    donorHead();
    for (int i = 0; i < nd; i++) donorRow(&D[i]);
}

void searchDonors(int byArea) {
    char key[20];
    int found = 0;
    ask(byArea ? "Area: " : "Blood group: ", key, 20);
    donorHead();
    for (int i = 0; i < nd; i++)
        if (same(byArea ? D[i].area : D[i].bg, key)) { donorRow(&D[i]); found = 1; }
    if (!found) printf("No donors found.\n");
}

/* ---------- requests ---------- */
void addRequest() {
    if (nr >= MAX) { printf("Request list is full.\n"); return; }
    Request *r = &R[nr];
    r->id = nr ? R[nr - 1].id + 1 : 1;
    ask("Patient name: ", r->patient, 30);
    askBG(r->bg);
    while ((r->units = askInt("Units needed: ")) < 1);
    ask("Area/Hospital locality: ", r->area, 20);
    while ((r->urgency = askInt("Urgency (1=High, 2=Medium, 3=Low): ")) < 1 || r->urgency > 3);
    r->done = 0; r->date = (long)time(NULL);
    nr++; save();
    printf("Request added with ID %d\n", r->id);
}

int cmpReq(const void *a, const void *b) {     /* urgency first, then oldest first */
    Request *x = &R[*(int *)a], *y = &R[*(int *)b];
    return x->urgency != y->urgency ? x->urgency - y->urgency : x->id - y->id;
}

void showRequests(int pendingOnly) {
    int idx[MAX], n = 0;
    for (int i = 0; i < nr; i++) if (!pendingOnly || !R[i].done) idx[n++] = i;
    if (!n) { printf("No requests.\n"); return; }
    if (pendingOnly) qsort(idx, n, sizeof(int), cmpReq);
    printf("\n%-4s %-16s %-5s %-6s %-12s %-8s %-10s %s\n",
           "ID", "Patient", "BGrp", "Units", "Area", "Urgency", "Status", "Date");
    for (int i = 0; i < n; i++) {
        Request *r = &R[idx[i]];
        printf("%-4d %-16s %-5s %-6d %-12s %-8s %-10s %s\n", r->id, r->patient, r->bg,
               r->units, r->area, URG[r->urgency], r->done ? "Fulfilled" : "Pending", fmt(r->date));
    }
}

/* ---------- matching ---------- */
int cmpDonor(const void *a, const void *b) {   /* same area first, then longest wait */
    Donor *x = &D[*(int *)a], *y = &D[*(int *)b];
    int sx = same(x->area, mArea), sy = same(y->area, mArea);
    if (sx != sy) return sy - sx;
    return (x->last > y->last) - (x->last < y->last);
}

void findMatches() {
    int idx[MAX], n = 0;
    showRequests(1);
    int ri = findReq(askInt("\nRequest ID: "));
    if (ri < 0) { printf("Request not found.\n"); return; }
    Request *r = &R[ri];
    mArea = r->area;
    for (int i = 0; i < nd; i++)
        if (compat(D[i].bg, r->bg) && eligible(&D[i])) idx[n++] = i;
    printf("\nMatches for request #%d (%s needs %s, area: %s)\n", r->id, r->patient, r->bg, r->area);
    if (!n) { printf("No eligible & compatible donors found.\n"); return; }
    qsort(idx, n, sizeof(int), cmpDonor);
    printf("%-4s %-16s %-5s %-13s %-12s %-9s %s\n",
           "ID", "Name", "BGrp", "Contact", "Area", "SameArea", "LastDonated");
    for (int i = 0; i < n; i++) {
        Donor *d = &D[idx[i]];
        printf("%-4d %-16s %-5s %-13s %-12s %-9s %s\n", d->id, d->name, d->bg, d->phone,
               d->area, same(d->area, r->area) ? "Yes" : "No", fmt(d->last));
    }
}

void recordDonation() {
    int ri = findReq(askInt("Request ID: "));
    if (ri < 0 || R[ri].done) { printf("Request not found or not pending.\n"); return; }
    int di = findDonor(askInt("Donor ID: "));
    if (di < 0) { printf("Donor not found.\n"); return; }
    Donor *d = &D[di];
    Request *r = &R[ri];
    if (!compat(d->bg, r->bg)) { printf("Mismatch: %s cannot donate to %s.\n", d->bg, r->bg); return; }
    if (!eligible(d)) { printf("Donor not eligible yet (wait %d days).\n", GAP); return; }

    d->last = (long)time(NULL); d->count++;
    if (--r->units <= 0) r->done = 1;
    FILE *f = fopen("history.txt", "a");
    if (f) { fprintf(f, "%d|%s|%s|%s|%s\n", r->id, d->name, r->patient, d->bg, fmt(d->last)); fclose(f); }
    save();
    printf("Donation recorded. Units still needed: %d\n", r->units);
}

/* ---------- reports ---------- */
int cmpCount(const void *a, const void *b) { return D[*(int *)b].count - D[*(int *)a].count; }

void history() {
    int id;
    char dn[30], pn[30], bg[5], dt[12];
    FILE *f = fopen("history.txt", "r");
    if (!f) { printf("No donations recorded yet.\n"); return; }
    printf("\n%-6s %-16s %-16s %-5s %s\n", "ReqID", "Donor", "Patient", "BGrp", "Date");
    while (fscanf(f, "%d|%29[^|]|%29[^|]|%4[^|]|%11[^\n]\n", &id, dn, pn, bg, dt) == 5)
        printf("%-6d %-16s %-16s %-5s %s\n", id, dn, pn, bg, dt);
    fclose(f);
}

void reports() {
    int c = askInt("1.Blood group count  2.Area-wise  3.Top 5 donors  4.History\nChoice: ");
    if (c == 1) {
        for (int i = 0; i < 8; i++) {
            int k = 0;
            for (int j = 0; j < nd; j++) if (!strcmp(D[j].bg, BG[i])) k++;
            printf("%-4s : %d donor(s)\n", BG[i], k);
        }
    } else if (c == 2) {
        for (int i = 0; i < nd; i++) {
            int first = 1, k = 0;
            for (int j = 0; j < nd; j++)
                if (same(D[j].area, D[i].area)) { k++; if (j < i) first = 0; }
            if (first) printf("%-15s : %d donor(s)\n", D[i].area, k);
        }
    } else if (c == 3) {
        int idx[MAX];
        for (int i = 0; i < nd; i++) idx[i] = i;
        qsort(idx, nd, sizeof(int), cmpCount);
        for (int i = 0; i < nd && i < 5; i++)
            printf("%d. %-16s %-5s %d donation(s)\n", i + 1, D[idx[i]].name, D[idx[i]].bg, D[idx[i]].count);
    } else if (c == 4) history();
    else printf("Invalid choice.\n");
}

/* ---------- main ---------- */
int main() {
    int c;
    load();
    printf("Loaded %d donor(s) and %d request(s).\n", nd, nr);
    do {
        printf("\n===== BLOOD DONOR MANAGEMENT & EMERGENCY MATCHING =====\n");
        printf(" 1. Add donor              6. View all requests\n");
        printf(" 2. View all donors        7. View pending (priority order)\n");
        printf(" 3. Search by blood group  8. Find matching donors\n");
        printf(" 4. Search by area         9. Record a donation\n");
        printf(" 5. Raise blood request   10. Reports\n");
        printf(" 0. Exit\n");
        c = askInt("Choice: ");
        switch (c) {
            case 1: addDonor(); break;
            case 2: viewDonors(); break;
            case 3: searchDonors(0); break;
            case 4: searchDonors(1); break;
            case 5: addRequest(); break;
            case 6: showRequests(0); break;
            case 7: showRequests(1); break;
            case 8: findMatches(); break;
            case 9: recordDonation(); break;
            case 10: reports(); break;
            case 0: printf("All data saved. Goodbye!\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (c != 0);
    return 0;
}