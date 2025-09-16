// ====================================================================
//  $File: main.c $
//  $Date: 16-09-2025 @ 10-39-45 $
//  $Revision: 0.0.0 $
//  $Creator: aeternatrix $
//  $Notice: (C) Copyright 2025 by aeternatrix. All Rights Reserved. $
// ====================================================================

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define PAGE 4096

extern int sprintf(char *__restrict __s, const char *__restrict __format, ...) __attribute__ ((__nothrow__));
extern int sscanf(const char *__restrict __s, const char *__restrict __format, ...) __attribute__ ((__nothrow__));
extern char* getenv(const char *__name) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__(1)));

enum { FILE, DIRECTORY } entry_type;

typedef struct content {
    char line[1024];
    char opt[32];
    int is_template;
    struct content* next;
} content;

typedef struct {
    char path[PAGE];
    int is_directory;
    
    content* contents;
    int options;
} entry;

static void
add_content(entry* e, char* buf, char* options, int is_template) {
    content* new = calloc(1, sizeof(content));
    new->is_template = is_template;
    new->next = 0;
    if (buf[0] == '\n' || is_template) sprintf(new->line, "%s", buf);
    else sprintf(new->line, "%s\n", buf);
    if (e->contents) {
        content* scanner = e->contents;
        while (scanner->next) scanner = scanner->next;
        scanner->next = new;
    } else {
        e->contents = new;
    }
    if (options) sprintf(new->opt, "%s", options);
}

static void
free_entry(entry* e) {
    if (e->contents) {
        content* scanner = e->contents;
        while (scanner->next) {
            content* tmp = scanner->next;
            free(scanner);
            scanner = tmp;
        }
        free(scanner);
    }
}

static int
is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

static void
cdoo(char* path) {
    char* last_slash = 0;
    for (char*c=path;*c;c++) if (*c=='/') last_slash = c;
    if (last_slash) *last_slash = 0;
    else path[0] = 0;
}

static char*
entry_name(const char* path, char* rt) {
    const char* last_slash = 0;
    for (const char*c=path;*c;c++) if (*c=='/') last_slash = c;
    if (last_slash) sprintf(rt, "%s", ++last_slash);
    return rt;
}

static char*
file_ext(const char* path, char* rt) {
    const char* last_dot = 0;
    for (const char*c=path;*c;c++) if (*c=='.') last_dot = c;
    if (last_dot) sprintf(rt, "%s", ++last_dot);
    return rt;
}

static int
str_len(const char* str) {
    const char* c = str;
    for(;*c;c++);
    return c-str;
}

static int
streq(const char* a, const char* b) {
    int len_a = str_len(a);
    int len_b = str_len(b);
    if (len_a != len_b) return 0;
    for (int i=0;i<len_a;i++) if (a[i] != b[i]) return 0;
    return 1;
}

static int
streq_for_n(const char* a, const char* b, int n) {
    for (int i=0;i<n && a[i] && b[i];i++) if (a[i] != b[i]) return 0;
    return 1;
}

static int
read_options(char* options, int default_opts) {
    int rt = default_opts;
    int toggle = 3;
    int a[3][3] = {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
    };

    int on = 1;

    for (char*c = options; *c; c++) {
        if (*c == 'u') { toggle = 0; }
        if (*c == 'g') { toggle = 1; }
        if (*c == 'o') { toggle = 2; }
        if (*c == 'a') { toggle = 3; }
        if (*c == '-') on = -1;
        if (*c == '+') on =  1;
        if (*c == 'r') { if (toggle == 3) { a[0][0] = 1 * on; a[1][0] = 1 * on; a[2][0] = 1 * on; } else { a[toggle][0] = 1 * on; } }
        if (*c == 'w') { if (toggle == 3) { a[0][1] = 1 * on; a[1][1] = 1 * on; a[2][1] = 1 * on; } else { a[toggle][1] = 1 * on; } }
        if (*c == 'x') { if (toggle == 3) { a[0][2] = 1 * on; a[1][2] = 1 * on; a[2][2] = 1 * on; } else { a[toggle][2] = 1 * on; } }
    }

    if (a[0][0] == 1)  rt |= S_IRUSR; if (a[0][0] == -1) rt &= ~S_IRUSR;
    if (a[0][1] == 1)  rt |= S_IWUSR; if (a[0][1] == -1) rt &= ~S_IWUSR;
    if (a[0][2] == 1)  rt |= S_IXUSR; if (a[0][2] == -1) rt &= ~S_IXUSR;

    if (a[1][0] == 1)  rt |= S_IRGRP; if (a[1][0] == -1) rt &= ~S_IRGRP;
    if (a[1][1] == 1)  rt |= S_IWGRP; if (a[1][1] == -1) rt &= ~S_IWGRP;
    if (a[1][2] == 1)  rt |= S_IXGRP; if (a[1][2] == -1) rt &= ~S_IXGRP;
    
    if (a[2][0] == 1)  rt |= S_IROTH; if (a[2][0] == -1) rt &= ~S_IROTH;
    if (a[2][1] == 1)  rt |= S_IWOTH; if (a[2][1] == -1) rt &= ~S_IWOTH;
    if (a[2][2] == 1)  rt |= S_IXOTH; if (a[2][2] == -1) rt &= ~S_IXOTH;

    return rt;
}

static char*
comment_header(char* filename, char* cmt, char* header, char* out) {
    const char* h = getenv("HOME");
    char user[1024] = {};
    entry_name(h, user);

    time_t raw;
    struct tm *ti;
    time(&raw);
    ti = localtime(&raw);
    char time_precise[1024] = {};
    char time_year[16] = {};

    strftime(time_precise, 1024, "%d-%m-%Y @ %H-%M-%S", ti);
    strftime(time_year, 1024, "%Y", ti);

    int ptr = 0;
    int adv = 0;

    for (char*r=cmt; *r; r++) out[ptr++] = *r;

    enum replacement { NO_REPLACE, FILENAME, DATE, ME, YEAR, COMMENT } replace = NO_REPLACE;

    for (char*c = header; *c; c++) {
        replace = NO_REPLACE;
        adv = 0;
        if (streq_for_n(c, "%FILE%", sizeof("%FILE%"))) { replace = FILENAME; adv = sizeof("%FILE%") - 2; }
        if (streq_for_n(c, "%DATE%", sizeof("%DATE%"))) { replace =     DATE; adv = sizeof("%DATE%") - 2; }
        if (streq_for_n(c,   "%ME%", sizeof(  "%ME%"))) { replace =       ME; adv = sizeof(  "%ME%") - 2; }
        if (streq_for_n(c, "%YEAR%", sizeof("%YEAR%"))) { replace =     YEAR; adv = sizeof("%YEAR%") - 2; }
        if (*c == '\n' && c[1]) { out[ptr++] =*c; replace = COMMENT; adv = 0; }

        if (replace) {
            switch (replace) {
                case FILENAME:  for (char*r=filename; *r; r++)      out[ptr++] = *r; break;
                case DATE:      for (char*r=time_precise; *r; r++)  out[ptr++] = *r; break;
                case ME:        for (char*r=user; *r; r++)          out[ptr++] = *r; break;
                case YEAR:      for (char*r=time_year; *r; r++)     out[ptr++] = *r; break;
                case COMMENT:   for (char*r=cmt; *r; r++)           out[ptr++] = *r; break;
                case NO_REPLACE:
                  break;
                }
            c += adv;
        } else {
            out[ptr++] = *c;
        }
    }
    out[ptr] = '\n';

    return out;
}

void
run_structure(const char* structure_file, const char* template_directory, const char* root) {
    char* buf = calloc(PAGE, sizeof(char));
    int fd = open(structure_file, O_RDONLY);
    read(fd, buf, PAGE);
    close(fd);

    char* cp = buf;
    cp+=2; //-- Skip ".\n"
    
    int n = 0;
    char* path = calloc(PAGE, sizeof(char));
    char* working_path = calloc(PAGE, sizeof(char));
    char name[32] = {};
    char options[32] = {};
    char ext[10] = {};
    char line[1024] = {};

    int occ = 1;
    int depth_last = 0;
    int depth_curr = 0;

    int df_perm_flags = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    entry* ce = 0;
    entry* entries = calloc(20, sizeof(entry));
    int entry_ptr = 0;

    sprintf(working_path, "%s", root);
    while(sscanf(cp, "%[^\n]%n\n", line, &n) == 1) {
        cp += n+1;
        if (line[0] == '-') goto body;
        char *c = line;
        for (int count = 0; *c; c++, count++) {
            if (*c == '+') depth_curr = count;
            if (is_alpha(*c)) {
                if (depth_curr < depth_last) {
                    cdoo(working_path);
                    cdoo(working_path);
                    if (!working_path[0] || streq(working_path, ".\0")) sprintf(working_path, "%s", root);
                } else if (depth_curr == depth_last) {
                    cdoo(working_path);
                    if (!working_path[0] || streq(working_path, ".\0")) sprintf(working_path, "%s", root);
                }
                depth_last = depth_curr;


                if (sscanf(c, "%[^.].%s %s", name, ext, options) == 3) {
                    sprintf(path, "%s/%s.%s", working_path, name, ext);
                    sprintf(working_path, "%s", path);
                    {
                        sprintf(entries[entry_ptr].path, "%s", path);
                        entries[entry_ptr].options = read_options(options, df_perm_flags);
                        entry_ptr++;
                    }
                } else if (sscanf(c, "%s %s", name, options) == 2) {
                    sprintf(path, "%s/%s", working_path, name);
                    sprintf(working_path, "%s", path);
                    {
                        sprintf(entries[entry_ptr].path, "%s", path);
                        entries[entry_ptr].options = read_options(options, df_perm_flags);
                        entry_ptr++;
                    }
                } else if (sscanf(c, "%[^.].%s", name, ext) == 2) {
                    sprintf(path, "%s/%s.%s", working_path, name, ext);
                    sprintf(working_path, "%s", path);
                    {
                        sprintf(entries[entry_ptr].path, "%s", path);
                        entries[entry_ptr].options = df_perm_flags;
                        entry_ptr++;
                    }
                } else if (sscanf(c, "%s", name) == 1) {
                    sprintf(path, "%s/%s", working_path, name);
                    sprintf(working_path, "%s", path);
                    {
                        sprintf(entries[entry_ptr].path, "%s", path);
                        entries[entry_ptr].is_directory = 1;
                        entry_ptr++;
                    }
                }

                break;
            }
        }
    }
    while(sscanf(cp, "%[^\n]%n\n", line, &n) == 1 || *cp == '\n') {
        if (*cp == '\n') {n = 0; sprintf(line, "\n"); }
        cp+=n+1;
body:
        occ = 1;
        if (sscanf(line, "--- %s %d", name, &occ) == 2) {
            for (entry* scanner = entries; scanner->path[0]; scanner++) {
                char buf[1024] = {};
                if (streq(name, entry_name(scanner->path, buf))) {
                    occ--;
                    if (!occ) { ce = scanner; break; }
                }
            }
        } else if (sscanf(line, "--- %s", name) == 1) {
            for (entry* scanner = entries; scanner->path[0]; scanner++) {
                char buf[1024] = {};
                if (streq(name, entry_name(scanner->path, buf))) {
                    ce = scanner;
                    break;
                }
            }
        } else if (sscanf(line, "//-- %s %s", name, options) == 2) {
            add_content(ce, name, options, 1);
        } else if (sscanf(line, "//-- %s", name) == 1) {
            add_content(ce, name, 0, 1);
        } else {
            add_content(ce, line, 0, 0);
        }
    }

    for (entry* e = entries; e->path[0]; e++) {
        if (e->is_directory) {
            mkdir(e->path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
        } else {
            int write_to = open(e->path, O_WRONLY | O_CREAT | O_TRUNC, e->options);
            content* c = e->contents;
            while(c) {
                if (c->is_template){
                    sprintf(path, "%s/%s", template_directory, c->line);
                    int template_fd = open(path, O_RDONLY);

                    char* template_content = calloc(PAGE, sizeof(char));
                    read(template_fd, template_content, PAGE);
                    close(template_fd);

                    int sz = str_len(template_content);
                    
                    if (streq (c->line, "comment_header")) {
                        char cmt[33] = {};
                        sprintf(cmt, "%s ", c->opt);
                        entry_name(e->path,     name);

                        char* rep = calloc(PAGE, sizeof(char));
                        comment_header(name, cmt, template_content, rep);
                        
                        sz = str_len(rep);
                        write(write_to, rep, sz);
                        free(rep);
                    } else write(write_to, template_content, sz);
                    free(template_content);
                } else {
                    int sz = str_len(c->line);
                    write(write_to, c->line, sz);
                }
                c = c->next;
            }
            close(write_to);
        }
        free_entry(e);
    }
    free(buf);
    free(path);
    free(working_path);
    free(entries);
}

