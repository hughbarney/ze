/* ze2.c,  Ze Emacs, Public Domain, Hugh Barney, 2024, Derived from: Anthony's Editor January 93 */
#include <stdlib.h>
#include <assert.h>
#include <curses.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#define E_NAME          "ze2"
#define E_VERSION       "v0.1"
#define E_LABEL         "Ze:"
#define MSGLINE         (LINES-1)
#define CHUNK           8096L
#define K_BUFFER_LENGTH 256
#define MAX_FNAME       256
#define TEMPBUF         512
#define MIN_GAP_EXPAND  512
#define STRBUF_M        64

typedef unsigned char char_t;
typedef long point_t;

typedef struct keymap_t {
    char *key_desc;                 /* name of bound function */
    char *key_bytes;        /* the string of bytes when this key is pressed */
    void (*func)(void);
} keymap_t;

typedef struct buffer_t
{
    point_t b_mark;           /* the mark */
    point_t b_point;          /* the point */
    point_t b_page;           /* start of page */
    point_t b_epage;          /* end of page */
    point_t b_reframe;        /* force a reframe of the display */
    char_t *b_buf;            /* start of buffer */
    char_t *b_ebuf;           /* end of buffer */
    char_t *b_gap;            /* start of gap */
    char_t *b_egap;           /* end of gap */
    char w_top;               /* Origin 0 top row of window */
    char w_rows;              /* no. of rows of text in window */
    int b_row;                /* cursor row */
    int b_col;                /* cursor col */
    char b_fname[MAX_FNAME + 1]; /* filename */
    char b_modified;          /* was modified */
} buffer_t;

/*
 * Some compilers define size_t as a unsigned 16 bit number while point_t and
 * off_t might be defined as a signed 32 bit number. malloc(), realloc(),
 * fread(), and fwrite() take size_t parameters, which means there will be some
 * size limits because size_t is too small of a type.
 */
#define MAX_SIZE_T      ((unsigned long) (size_t) ~0)

int done;
char_t *input;
int msgflag;
char msgline[TEMPBUF];
keymap_t *key_return;
keymap_t *key_map;
buffer_t *curbp;

buffer_t* new_buffer()
{
    buffer_t *bp = (buffer_t *)malloc(sizeof(buffer_t));
    assert(bp != NULL);
    bp->b_point = 0;
    bp->b_page = 0;
    bp->b_epage = 0;
    bp->b_reframe = 0;
    bp->b_modified = 0;
    bp->b_buf = NULL;
    bp->b_ebuf = NULL;
    bp->b_gap = NULL;
    bp->b_egap = NULL;
    bp->b_fname[0] = '\0';
    bp->w_top = 0;    
    bp->w_rows = LINES - 2;
    return bp;
}

void fatal(char *msg)
{
    noraw();
    endwin();
    printf("\n" E_NAME " " E_VERSION ": %s\n", msg);
    exit(1);
}

int msg(char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    (void)vsprintf(msgline, msg, args);
    va_end(args);
    msgflag = TRUE;
    return FALSE;
}

/* Given a buffer offset, convert it to a pointer into the buffer */
char_t * ptr(buffer_t *bp, register point_t offset)
{
    if (offset < 0) return (bp->b_buf);
    return (bp->b_buf+offset + (bp->b_buf + offset < bp->b_gap ? 0 : bp->b_egap-bp->b_gap));
}

/* Given a pointer into the buffer, convert it to a buffer offset */
point_t pos(buffer_t *bp, register char_t *cp)
{
    assert(bp->b_buf <= cp && cp <= bp->b_ebuf);
    return (cp - bp->b_buf - (cp < bp->b_egap ? 0 : bp->b_egap - bp->b_gap));
}

/* Enlarge gap by n chars, position of gap cannot change */
int growgap(buffer_t *bp, point_t n)
{
    char_t *new;
    point_t buflen, newlen, xgap, xegap;
    assert(bp->b_buf <= bp->b_gap);
    assert(bp->b_gap <= bp->b_egap);
    assert(bp->b_egap <= bp->b_ebuf);
    xgap = bp->b_gap - bp->b_buf;
    xegap = bp->b_egap - bp->b_buf;
    buflen = bp->b_ebuf - bp->b_buf;
    
    /* reduce number of reallocs by growing by a minimum amount */
    n = (n < MIN_GAP_EXPAND ? MIN_GAP_EXPAND : n);
    newlen = buflen + n * sizeof (char_t);

    if (buflen == 0) {
        if (newlen < 0 || MAX_SIZE_T < newlen) fatal("Failed to allocate required memory");
        new = (char_t*) malloc((size_t) newlen);
        if (new == NULL) fatal("Failed to allocate required memory");
    } else {
        if (newlen < 0 || MAX_SIZE_T < newlen) return msg("Failed to allocate required memory");
        new = (char_t*) realloc(bp->b_buf, (size_t) newlen);
        if (new == NULL) return msg("Failed to allocate required memory");
    }

    /* Relocate pointers in new buffer and append the new extension to the end of the gap */
    bp->b_buf = new;
    bp->b_gap = bp->b_buf + xgap;      
    bp->b_ebuf = bp->b_buf + buflen;
    bp->b_egap = bp->b_buf + newlen;
    while (xegap < buflen--)
        *--bp->b_egap = *--bp->b_ebuf;
    bp->b_ebuf = bp->b_buf + newlen;

    assert(bp->b_buf < bp->b_ebuf);          /* Buffer must exist. */
    assert(bp->b_buf <= bp->b_gap);
    assert(bp->b_gap < bp->b_egap);          /* Gap must grow only. */
    assert(bp->b_egap <= bp->b_ebuf);
    return (TRUE);
}

point_t movegap(buffer_t *bp, point_t offset)
{
    char_t *p = ptr(bp, offset);
    while (p < bp->b_gap)
        *--bp->b_egap = *--bp->b_gap;
    while (bp->b_egap < p)
        *bp->b_gap++ = *bp->b_egap++;
    assert(bp->b_gap <= bp->b_egap);
    assert(bp->b_buf <= bp->b_gap);
    assert(bp->b_egap <= bp->b_ebuf);
    return (pos(bp, bp->b_egap));
}

void save()
{
    FILE *fp;
    point_t length;
    fp = fopen(curbp->b_fname, "w");
    if (fp == NULL) msg("Failed to open file \"%s\".", curbp->b_fname);
    (void) movegap(curbp, (point_t) 0);
    length = (point_t) (curbp->b_ebuf - curbp->b_egap);
    if (fwrite(curbp->b_egap, sizeof (char), (size_t) length, fp) != length) 
        msg("Failed to write file \"%s\".", curbp->b_fname);
    fclose(fp);
    curbp->b_modified = 0;
    msg("File \"%s\" %ld bytes saved.", curbp->b_fname, pos(curbp, curbp->b_ebuf));
}

/* reads file into buffer at point */
int insert_file(char *fn)
{
    FILE *fp;
    size_t len;
    struct stat sb;

    if (stat(fn, &sb) < 0) return msg("Failed to find file \"%s\".", fn);
    if (MAX_SIZE_T < sb.st_size) return msg("File \"%s\" is too big to load.", fn);
    if (curbp->b_egap - curbp->b_gap < sb.st_size * sizeof (char_t) && !growgap(curbp, sb.st_size))
        return (FALSE);
    if ((fp = fopen(fn, "r")) == NULL) return msg("Failed to open file \"%s\".", fn);
    curbp->b_point = movegap(curbp, curbp->b_point);
    curbp->b_gap += len = fread(curbp->b_gap, sizeof (char), (size_t) sb.st_size, fp);
    if (fclose(fp) != 0) return msg("Failed to close file \"%s\".", fn);
    msg("File \"%s\" %ld bytes read.", fn, len);
    return (TRUE);
}

char_t *get_key(keymap_t *keys, keymap_t **key_return)
{
    keymap_t *k;
    int submatch;
    static char_t buffer[K_BUFFER_LENGTH];
    static char_t *record = buffer;
    *key_return = NULL;

    /* if recorded bytes remain, return next recorded byte. */
    if (*record != '\0') {
        *key_return = NULL;
        return record++;
    }
    
    record = buffer; /* reset record buffer. */
    do {
        assert(K_BUFFER_LENGTH > record - buffer);
        *record++ = (unsigned)getch(); /* read and record one byte. */
        *record = '\0';

        /* if recorded bytes match any multi-byte sequence... */
        for (k = keys, submatch = 0; k->key_bytes != NULL; ++k) {
            char_t *p, *q;
            for (p = buffer, q = (char_t *)k->key_bytes; *p == *q; ++p, ++q) {
                    /* an exact match */
                if (*q == '\0' && *p == '\0') {
                        record = buffer;
                    *record = '\0';
                    *key_return = k;
                    return record; /* empty string */
                }
            }
            /* record bytes match part of a command sequence */
            if (*p == '\0' && *q != '\0') submatch = 1;
        }
    } while (submatch);
    /* nothing matched, return recorded bytes. */
    record = buffer;
    return (record++);
}

/*

In order to mananage lines longer than the column width of the screen we 
need to break lines up into segments.

Imagine a physical line of say 320 characters of

--------------long line---------------------------------
0                                                     320
|                                                      |


On a 80 character width screen we want these to wrap and display
as if 4 lines as below.

----segment 1---------
0                   80
----segment 2---------
----segment 3---------
----segment 4---------

In the Editor we also want to treat long lines as if they were
seperate lines to enable cursor movement.  For example we want
end_of_line() to take us to column 80 on segment 1 and then down() to
take us to segments 2.

The functions segstart and segnext enable a long line to be split up.

*/



/* forward scan for start of logical line segment containing the finish point */
point_t segstart(buffer_t *bp, point_t start, point_t finish)
{
    char_t *p;
    int col = 0;
    point_t pt = start;

    while (pt < finish) {
        p = ptr(bp, pt);
        if (*p == '\n') {
            col = 0;
            start = pt + 1;
        } else if (COLS <= col) {
            col = 0;
            start = pt;
        }
        ++pt;
        col += *p == '\t' ? 8 - (col & 7) : 1;
    }
    return (col < COLS ? start : finish);
}

/* Forward scan for start of logical line segment following 'finish' */
point_t segnext(buffer_t *bp, point_t start, point_t finish)
{
    char_t *p;
    int c = 0;

    point_t pt = segstart(bp, start, finish);
    for (;;) {
        p = ptr(bp, pt);
        if (bp->b_ebuf <= p || COLS <= c) break;
        ++pt;
        if (*p == '\n') break;
        c += *p == '\t' ? 8 - (c & 7) : 1;
    }
    return (p < bp->b_ebuf ? pt : pos(bp, bp->b_ebuf));
}

/* reverse scan for start of line containing point pt */
point_t start_of_line_point(buffer_t *bp, register point_t pt)
{
    register char_t *p;
    do
        p = ptr(bp, --pt);
    while (bp->b_buf < p && *p != '\n');
    return (bp->b_buf < p ? ++pt : 0);
}

/* move up one screen line */
point_t previous_line_point(buffer_t *bp, point_t pt)
{
    point_t pl_start = start_of_line_point(bp, pt);        // get physical line start
    point_t ls_start = segstart(bp, pl_start, pt);         // line segment start

    // is start of line segment ahead of the physical start of the line
    if (pl_start < ls_start)                
        pt = segstart(bp, pl_start, ls_start-1);           // get prev segment of current physical line
    else
        pt = segstart(bp,start_of_line_point(bp, pl_start - 1), pl_start - 1);   // get last segment of previous line physical line
    return (pt);
}

/* Move down one screen line */
point_t next_line_point(buffer_t *bp, point_t pt) {
    return (segnext(bp, start_of_line_point(bp,pt), pt)); 
}

/* return the point of a display column on the line containing pt */
point_t column_to_point(buffer_t *bp, point_t pt, int column)
{
    int c = 0;
    char_t *p;
    while ((p = ptr(bp, pt)) < bp->b_ebuf && *p != '\n' && c < column) {
        c += *p == '\t' ? 8 - (c & 7) : 1;
        ++pt;
    }
    return (pt);
}

void modeline(buffer_t *bp)
{
    int i;
    char temp[TEMPBUF];
    char mch;
    
    standout();
    move(bp->w_top + bp->w_rows, 0);
    mch = bp->b_modified ? '*' : '=';
    sprintf(temp, "=%c " E_LABEL " == %s ", mch, bp->b_fname);
    addstr(temp);
    for (i = strlen(temp) + 1; i <= COLS; i++)
        addch('=');
    standend();
}

void dispmsg()
{
    move(MSGLINE, 0);
    if (msgflag) {
        addstr(msgline);
        msgflag = FALSE;
    }
    clrtoeol();
}

void display()
{
    char_t *p;
    int i, j, k;
    buffer_t *bp = curbp;
    
    /* find start of screen, handle scroll up off page or top of file  */
    /* point is always within b_page and b_epage */
    if (bp->b_point < bp->b_page)
        bp->b_page = segstart(bp, start_of_line_point(bp,bp->b_point), bp->b_point);

    /* reframe when scrolled off bottom */
    if (bp->b_reframe == 1 || (bp->b_epage <= bp->b_point && curbp->b_point != pos(curbp, curbp->b_ebuf))) {
        bp->b_reframe = 0;
        /* Find end of screen plus one. */
        bp->b_page = next_line_point(bp, bp->b_point);


        /* if we scoll to EOF we show 1 blank line at bottom of screen */
        if (pos(bp, bp->b_ebuf) <= bp->b_page) {
            bp->b_page = pos(bp, bp->b_ebuf);
            i = bp->w_rows - 1;
        } else {
            i = bp->w_rows - 0;
        }
        /* Scan backwards the required number of lines. */
        while (0 < i--)
            bp->b_page = previous_line_point(bp, bp->b_page);
    }

    move(bp->w_top, 0); /* start from top of window */
    i = bp->w_top;
    j = 0;
    bp->b_epage = bp->b_page;
    
    /* paint screen from top of page until we hit maxline */ 
    while (1) {
        /* reached point - store the cursor position */
        if (bp->b_point == bp->b_epage) {
            bp->b_row = i;
            bp->b_col = j;
        }
        p = ptr(bp, bp->b_epage);
        if (bp->w_top + bp->w_rows <= i || bp->b_ebuf <= p) /* maxline */
            break;
        if (*p != '\r') {
            if (isprint(*p) || *p == '\t' || *p == '\n') {
                j += *p == '\t' ? 8-(j&7) : 1;
                addch(*p);
            } else {
                const char *ctrl = unctrl(*p);
                j += (int) strlen(ctrl);
                addstr(ctrl);
            }
        }
        if (*p == '\n' || COLS <= j) {
            j -= COLS;
            if (j < 0) j = 0;
            ++i;
        }
        ++bp->b_epage;
    }

    /* replacement for clrtobot() to bottom of window */
    for (k=i; k < bp->w_top + bp->w_rows; k++) {
        move(k, j); /* clear from very last char not start of line */
        clrtoeol();
        j = 0; /* thereafter start of line */
    }

    modeline(bp);
    dispmsg();
    move(bp->b_row, bp->b_col); /* set cursor */
    refresh();
}

void top() { curbp->b_point = 0; }
void bottom() { curbp->b_point = pos(curbp, curbp->b_ebuf); if (curbp->b_epage < pos(curbp, curbp->b_ebuf)) curbp->b_reframe = 1;}
void left() { if (0 < curbp->b_point) --curbp->b_point; }
void right() { if (curbp->b_point < pos(curbp, curbp->b_ebuf)) ++curbp->b_point; }
void up() { curbp->b_point = column_to_point(curbp, previous_line_point(curbp, curbp->b_point),curbp->b_col); }
void down() { curbp->b_point = column_to_point(curbp, next_line_point(curbp, curbp->b_point),curbp->b_col); }
void beginning_of_line() { curbp->b_point = start_of_line_point(curbp,curbp->b_point); }
void end_of_line() { curbp->b_point = next_line_point(curbp, curbp->b_point); left(); }
void quit() { done = 1; }

void pgdown()
{
    curbp->b_page = curbp->b_point = previous_line_point(curbp, curbp->b_epage);
    while (0 < curbp->b_row--)
        down();
    curbp->b_epage = pos(curbp, curbp->b_ebuf);
}

void pgup()
{
    int i = curbp->w_rows;
    while (0 < --i) {
        curbp->b_page = previous_line_point(curbp, curbp->b_page);
        up();
    }
}

void insert()
{
    assert(curbp->b_gap <= curbp->b_egap);
    if (curbp->b_gap == curbp->b_egap && !growgap(curbp, CHUNK)) return;
    curbp->b_point = movegap(curbp, curbp->b_point);
    *curbp->b_gap++ = *input == '\r' ? '\n' : *input;
    curbp->b_point = pos(curbp, curbp->b_egap);
    curbp->b_modified = 1;
}

void backsp()
{
    curbp->b_point = movegap(curbp, curbp->b_point);
    if (curbp->b_buf < curbp->b_gap) {
        --curbp->b_gap;
        curbp->b_modified = 1;
    }
    curbp->b_point = pos(curbp, curbp->b_egap);
}

void delete()
{
    curbp->b_point = movegap(curbp, curbp->b_point);
    if (curbp->b_egap < curbp->b_ebuf) {
        curbp->b_point = pos(curbp, ++curbp->b_egap);
        curbp->b_modified = 1;
    }
}

/* the key bindings:  desc, keys, func */
keymap_t keymap[] = {
    {"C-a beginning-of-line    ", "\x01", beginning_of_line },
    {"C-b                      ", "\x02", left },
    {"C-d forward-delete-char  ", "\x04", delete },
    {"C-e end-of-line          ", "\x05", end_of_line },
    {"C-f                      ", "\x06", right },
    {"C-n                      ", "\x0E", down },
    {"C-p                      ", "\x10", up },
    {"C-h backspace            ", "\x08", backsp },
    {"C-v                      ", "\x16", pgdown },
    {"esc v                    ", "\x1B\x76", pgup },
    {"esc < beg-of-buf         ", "\x1B\x3C", top },
    {"esc > end-of-buf         ", "\x1B\x3E", bottom },
    {"up previous-line         ", "\x1B\x5B\x41", up },
    {"down next-line           ", "\x1B\x5B\x42", down },
    {"left backward-character  ", "\x1B\x5B\x44", left },
    {"right forward-character  ", "\x1B\x5B\x43", right },
    {"home beginning-of-line   ", "\x1B\x4F\x48", beginning_of_line },
    {"end end-of-line          ", "\x1B\x4F\x46", end_of_line },
    {"DEL forward-delete-char  ", "\x1B\x5B\x33\x7E", delete },
    {"backspace delete-left    ", "\x7f", backsp },
    {"PgUp                     ", "\x1B\x5B\x35\x7E",pgup },
    {"PgDn                     ", "\x1B\x5B\x36\x7E", pgdown },
    {"C-x C-s save-buffer      ", "\x18\x13", save },  
    {"C-x C-c exit             ", "\x18\x03", quit },
    {"K_ERROR                  ", NULL, NULL }
};

int main(int argc, char **argv)
{
    if (argc != 2) fatal("usage: " E_NAME " filename\n");
    initscr();    
    raw();
    noecho();
    curbp = new_buffer();
    (void)insert_file(argv[1]);
    strncpy(curbp->b_fname, argv[1], MAX_FNAME);  /* save filename regardless */
    curbp->b_fname[MAX_FNAME] = '\0'; /* force truncation */
    if (!growgap(curbp, CHUNK)) fatal("Failed to allocate required memory.\n");
    key_map = keymap;

    while (!done) {
        display();
        input = get_key(key_map, &key_return);
        if (key_return != NULL) {
            (key_return->func)();
        } else {
            if (*input > 31 || *input == 10 || *input == 9) /* allow TAB, NEWLINE and other control char is Not Bound */
                insert();
            else
                msg("Not bound");
        }
    }

    if (curbp != NULL) free(curbp);
    noraw();
    endwin();
    return 0;
}
