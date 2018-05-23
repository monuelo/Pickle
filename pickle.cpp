#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define __USE_GNU

/*** Includes ***/
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <fcntl.h>

using namespace std;

/*** prototypes ***/

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(const char *prompt);

//*** Defines ***/
#define PICKLE_VERSION "0.0.1"
#define PICKLE_TAB_STOP 8
#define CTRL_KEY(k) ((k) & 0x1f)
#define PICKLE_QUIT_TIMES 2

typedef struct erow {
  int size;
  int rsize;
  char *chars;
  char *render;
} erow;


enum keys {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  DEL_KEY,
  ARROW_DOWN,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

struct pickleConfig {
  int trash;
  int cx, cy, rx;
  int rowoff, coloff;
  int screenrows, screencols, numrows;
  erow *row;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct termios orig_termios;
};

struct pickleConfig P;

/*** Error ***/
void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

// Disable Raw Mode
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &P.orig_termios);
}

// Input Flags
void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &P.orig_termios) == -1){
    die("tcgetattr");
  }
  atexit(disableRawMode);

  #define CTRL_KEY(k) ((k) & 0x1f)

  struct termios raw = P.orig_termios;

  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

struct appendBuffer{
  char *b;
  int len;
};

#define APPENDBUFFER_INIT {NULL, 0}

void abAppend(struct appendBuffer *ab, const char *s, int len){
  char *buff = (char*)realloc(ab -> b, ab -> len + len);

  if (buff == NULL) return;
  memcpy(&buff[ab -> len], s, len);
  ab -> b = buff;
  ab -> len += len;
}

void abFree(struct appendBuffer *ab){
  free(ab -> b);
}

void welcomeScreenDraw(struct appendBuffer *ab, const char message[]) {
  char ch[80];

  int lenght = snprintf(ch, sizeof(ch), message, PICKLE_VERSION);

  if (lenght > P.screencols) lenght = P.screencols;

  int padding = (P.screencols - lenght) / 2;

  if (padding) {
    abAppend(ab, "-", 1);
    padding--;
  }

  while (padding--) abAppend(ab, " ", 1);
  abAppend(ab, ch, lenght);
}

int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (PICKLE_TAB_STOP - 1) - (rx % PICKLE_TAB_STOP);
    rx++;
  }
  return rx;
}

int editorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row -> size; cx++){
    if (row - > chars[cx] == '\t'){
      cur_rx += (PICKLE_TAB_STOP - 1) - (cur_rx % PICKLE_TAB_STOP);
    }
    cur_rx++;
    if (cur_rx > rx) return cx;
  }
  return cx;
}

void editorScroll() {
  P.rx = 0;
  if (P.cy < P.numrows) {
    P.rx = editorRowCxToRx(&P.row[P.cy], P.cx);
  }
  if (P.cy < P.rowoff) {
    P.rowoff = P.cy;
  }
  if (P.cy >= P.rowoff + P.screenrows) {
    P.rowoff = P.cy - P.screenrows + 1;
  }
  if (P.rx < P.coloff) {
    P.coloff = P.rx;
  }
  if (P.rx >= P.coloff + P.screencols) {
    P.coloff = P.rx - P.screencols + 1;
  }
}

void editorDrawRows(struct appendBuffer *ab) {
  int y;

  for (y = 0; y < P.screenrows; y++) {
    int filerow = y + P.rowoff;
    if (filerow >= P.numrows){
      if (P.numrows == 0 && y == P.screenrows / 3) {
        welcomeScreenDraw(ab, "Pickle editor -- version %s");
      } else if (P.numrows == 0 && y == ((P.screenrows)/3)+1){
        welcomeScreenDraw(ab, "Press 'Ctrl+Q' to Quit");
      }else{
        abAppend(ab, "-", 1);
      }
    } else {
      int len = P.row[filerow].rsize - P.coloff;
      if (len < 0) len = 0;
      if (len > P.screencols) len = P.screencols;
      abAppend(ab, &P.row[filerow].render[P.coloff], len);
    }
    abAppend(ab, "\x1b[K", 3);
      abAppend(ab, "\r\n", 2);
    }
}

void editorDrawStatusBar(struct appendBuffer *ab){
  abAppend(ab, "\x1b[7m", 4);

  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
    P.filename ? P.filename : "[No Name]", P.numrows,
    P.trash ? "(modified)" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", P.cy + 1, P.numrows);
  if (len > P.screencols) len = P.screencols;
    abAppend(ab, status, len);
  while (len < P.screencols){
    if(P.screencols - len == rlen){
      abAppend(ab, rstatus, rlen);
      break;
    }
      abAppend(ab, " ", 1);
      len++;
  }
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct appendBuffer *ab) {
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(P.statusmsg);
  if (msglen > P.screencols) msglen = P.screencols;
  if (msglen && time(NULL) - P.statusmsg_time < 5)
    abAppend(ab, P.statusmsg, msglen);
}

void editorSetStatusMessage(const char *fmt, ...){
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(P.statusmsg, sizeof(P.statusmsg), fmt, ap);
  va_end(ap);
  P.statusmsg_time = time(NULL);
}

// Clear Screen
void editorRefreshScreen() {
  editorScroll();
  struct appendBuffer ab = APPENDBUFFER_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (P.cy - P.rowoff) + 1, (P.rx - P.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

// Read Bytes
int editorReadKey() {
  int nread;
  char c;

  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  if (c == '\x1b') {
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}

char *editorPrompt(const char *prompt) {
  size_t bufsize = 128;
  char *buff = (char*)malloc(bufsize);
  size_t buflen = 0;
  buff[0] = '\0';
  while (1) {
    editorSetStatusMessage(prompt, buff);
    editorRefreshScreen();
    int c = editorReadKey();
    
    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen != 0)
        buff[--buflen] = '\0';
    } else if (c == '\x1b'){
      editorSetStatusMessage("");
      free(buff);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        editorSetStatusMessage("");
        return buff;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buff = (char*)realloc(buff, bufsize);
      }
      buff[buflen++] = c;
      buff[buflen] = '\0';
    }
  }
}

// Get Cursor Position
int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  printf("\r\n&buf[1]: '%s'\r\n", &buf[1]);
  editorReadKey();

  return -1;
}

// Get Terminal Window Size
int getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12 != 12)){
          return -1;
        }
        return getCursorPosition(rows, cols);
    } else {
      *cols = ws.ws_col;
      *rows = ws.ws_row;
      return 0;
    }
}

void editorMoveCursor(int key){
  erow *row = (P.cy >= P.numrows) ? NULL : &P.row[P.cy];

  switch (key){
    case ARROW_LEFT:
      if(P.cx != 0){
        P.cx--;
      } else if (P.cy > 0){
        P.cy--;
        P.cx = P.row[P.cy].size;
      }
      break;

    case ARROW_RIGHT:
      if(row && P.cx < row -> size){
        P.cx++;
      }
      break;

    case ARROW_UP:
      if(P.cy != 0){
        P.cy--;
      }
      break;

    case ARROW_DOWN:
      if (P.cy < P.numrows) {
        P.cy++;
      }
      break;
  }

  row = (P.cy >= P.numrows) ? NULL : &P.row[P.cy];
  int rowlenght = row ? row -> size : 0;

  if (P.cx > rowlenght) {
    P.cx = rowlenght;
  }
}


/*** row ***/

void editorUpdateRow(erow *row) {
  int tabs = 0;
  for (int i = 0; i < row -> size; i++)
    if (row -> chars[i] == '\t') tabs++;

  free(row->render);
  row->render = (char*)malloc(row->size + tabs*(PICKLE_TAB_STOP - 1) + 1);

  int index = 0;
  int j;
  for (j = 0; j < row -> size; j++) {
    if (row -> chars[j] == '\t') {
      row -> render[index++] = ' ';
      while (index % PICKLE_TAB_STOP != 0) row -> render[index++] = ' ';
    } else {
      row -> render[index++] = row -> chars[j];
    }
  }
  row -> render[index] = '\0';
  row -> rsize = index;
}

void editorInsertRow(int at, const char *s, size_t len) {
  if (at < 0 || at > P.numrows){
    return;
  }
  P.row = (erow*) realloc(P.row, sizeof(erow) * (P.numrows + 1));
  
  memmove(&P.row[at + 1], &P.row[at], sizeof(erow) * (P.numrows - at));

  P.row[at].size = len;
  P.row[at].chars = (char*)malloc(len + 1);
  memcpy(P.row[at].chars, s, len);
  P.row[at].chars[len] = '\0';

  P.row[at].rsize = 0;
  P.row[at].render = NULL;
  editorUpdateRow(&P.row[at]);

  P.numrows++;
  P.trash++;
}

void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row -> size) at = row -> size;
  row -> chars = (char*)realloc(row -> chars, row -> size + 2);
  memmove(&row -> chars[at + 1], &row -> chars[at], row -> size - at + 1);
  row -> size++;
  row -> chars[at] = c;
  editorUpdateRow(row);
  P.trash++;
}

void editorRowDeleteChar(erow *row, int at) {
  if (at < 0 || at > row -> size){
    return;
  }
  memmove(&row -> chars[at], &row -> chars[at+1], row -> size - at);
  row -> size--;
  editorUpdateRow(row);
  P.trash++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
  row -> chars = (char*) realloc(row ->chars, row -> size + len + 1);
  memcpy(&row -> chars[row -> size], s, len);
  row -> size += len;
  row -> chars[row -> size] = '\0';
  editorUpdateRow(row);
  P.trash++;
}

void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
}

void editorDelRow(int at) {
  if (at < 0 || at >= P.numrows){
    return;
  }
  editorFreeRow(&P.row[at]);
  memmove(&P.row[at], &P.row[at + 1], sizeof(erow) * (P.numrows - at - 1));
  P.numrows--;
  P.trash++;
}

void editorDelChar(){
  if (P.cy == P.numrows){
    return;
  }
  if(P.cx == 0 && P.cy == 0){
    return;
  }
  erow *row = &P.row[P.cy];
  if (P.cx > 0) {
    editorRowDeleteChar(row, P.cx - 1);
    P.cx--;
  } else {
    P.cx = P.row[P.cy -1].size;
    editorRowAppendString(&P.row[P.cy - 1], row -> chars, row -> size);
    editorDelRow(P.cy);
    P.cy--;
  }
}

/*** operations ***/

void editorInsertChar(int c){
  if (P.cy == P.numrows) {
    editorInsertRow(P.numrows, "",0);
  }
  editorRowInsertChar(&P.row[P.cy], P.cx, c);
  P.cx++;
}

void editorInsertNewline() {
  if (P.cx == 0) {
    editorInsertRow(P.cy, "", 0);
  } else {
    erow *row = &P.row[P.cy];
    editorInsertRow(P.cy + 1, &row->chars[P.cx], row -> size - P.cx);
    row = &P.row[P.cy];
    row -> size = P.cx;
    row -> chars[row -> size] = '\0';
    editorUpdateRow(row);
  }
  P.cy++;
  P.cx = 0;
}

char *editorRowsToString(int *buflen) {
  int len = 0;
  int i;
  for (i = 0; i < P.numrows; i++)
    len += P.row[i].size + 1;
  *buflen = len;
  char *buff = (char*) malloc(len);
  char *p = buff;
  for (i = 0; i < P.numrows; i++) {
    memcpy(p, P.row[i].chars, P.row[i].size);
    p += P.row[i].size;
    *p = '\n';
    p++;
  }
  return buff;
}

void editorOpen(char *filename) {
  free(P.filename);
  P.filename = strdup(filename);
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    die("fopen");
  }

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while((linelen = getline(&line, &linecap, fp)) != -1)
  if (linelen != -1){
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen -1] == '\r')){
      linelen--;
    }
    editorInsertRow(P.numrows,line, linelen);
  }
  free(line);
  fclose(fp);
  P.trash = 0;
}

void saveFile() {
  if (P.filename == NULL){
    P.filename = editorPrompt("Save as: %s (Press 'ESC' to cancel)");
    if (P.filename == NULL){
      editorSetStatusMessage("Save aborted");
    }
    return;
  }

  int lenght;
  char *buff = editorRowsToString(&lenght);

  int filed = open(P.filename, O_RDWR | O_CREAT, 0644);
  if (filed != -1) {
    if (ftruncate(filed, lenght) != -1){
      if (write(filed, buff, lenght) == lenght) {
        close(filed);
        free(buff);
        P.trash = 0;
        editorSetStatusMessage("%d bytes written to disk", lenght);
        return;
      }
    }
    close(filed);
  }
  free(buff);
  editorSetStatusMessage("Can't save file. Error: %s", strerror(errno));
}

void editorFind() {
  char *query = editorPrompt("Search: %s (ESC to cancel)");
  if (query == NULL){
    return;
  }

  int i;
  for (i = 0; i < P.numrows; i++){
    erow *row = &P.row[i];
    char *match = strstr(row -> render, query);
  }
  if (match) {
    P.cy = i;
    P.cx = editorRowRxToCx(match - row -> render);
    P.rowoff = P.numrows;
    break;
  }

  free(query);
}


// Config Keypress
void editorProcessKeypress() {
  static int quit_times = PICKLE_QUIT_TIMES;
  int c = editorReadKey();
  switch (c) {
    case '\r':
      editorInsertNewline();
      break;
    case CTRL_KEY('q'):
      if(P.trash && quit_times > 0){
        editorSetStatusMessage("Warning! File has unsaved changes -- Press Ctrl+Q %d more times to Quit", quit_times);
        quit_times--;
        return;
      }
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;

    case CTRL_KEY('s'):
      saveFile();
    case HOME_KEY:
      P.cx = 0;
      break;
    case END_KEY:
      if (P.cy < P.numrows){
        P.cx = P.row[P.cy].size;
      }
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
        editorDelChar();
      break;
    case PAGE_UP:
    case PAGE_DOWN:
      {
        if (c == PAGE_UP){
          P.cy = P.rowoff;
        } else if (c == PAGE_DOWN){
          P.cy = P.rowoff + P.screenrows - 1;
        }


        int times = P.screenrows;
        while (times--)
          editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
        break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;

    case CTRL_KEY('l'):
    case '\x1b':
      break;

    default:
      editorInsertChar(c);
      break;
  }
}

// Init the Editor with previous configs
void init(){
    P.cx = 0;
    P.cy = 0;
    P.rx = 0;
    P.trash = 0;
    P.rowoff = 0;
    P.coloff = 0;
    P.numrows = 0;
    P.row = NULL;
    P.filename = NULL;
    P.statusmsg[0] = '\0';
    P.statusmsg_time = 0;
    if(getWindowSize(&P.screenrows, &P.screencols) == -1){
        die("getWindowSize");
    }
    P.screenrows -=2;
}

int main(int argc, char *argv[]) {
  enableRawMode();
  init();
  if (argc >= 2){
    editorOpen(argv[1]);
  }
  editorSetStatusMessage("HELP: Ctrl+S to save | Ctrl+Q to quit");
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}