#include <time.h>
#include <wchar.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "demo.h"

// (non-)ansi terminal definition-4-life
static const int MIN_SUPPORTED_ROWS = 24;
static const int MIN_SUPPORTED_COLS = 76; // allow a bit of margin, sigh

static int democount;
static demoresult* results;
static char *datadir = NOTCURSES_SHARE;

static const char DEFAULT_DEMO[] = "ixemthnbcgrwuvlsfjqo";

atomic_bool interrupted = ATOMIC_VAR_INIT(false);
// checked following demos, whether aborted, failed, or otherwise
static atomic_bool restart_demos = ATOMIC_VAR_INIT(false);

void interrupt_demo(void){
  atomic_store(&interrupted, true);
}

void interrupt_and_restart_demos(void){
  atomic_store(&restart_demos, true);
  atomic_store(&interrupted, true);
}

const demoresult* demoresult_lookup(int idx){
  if(idx < 0 || idx >= democount){
    return NULL;
  }
  return &results[idx];
}

char* find_data(const char* datum){
  char* path = malloc(strlen(datadir) + 1 + strlen(datum) + 1);
  strcpy(path, datadir);
  strcat(path, "/");
  strcat(path, datum);
  return path;
}

float delaymultiplier = 1;

// scaled in getopt() by delaymultiplier
struct timespec demodelay = {
  .tv_sec = 1,
  .tv_nsec = 0,
};

// the "jungle" demo has non-free material embedded into it, and is thus
// entirely absent (can't just be disabled). supply a stub here.
#ifdef DFSG_BUILD
int jungle_demo(struct notcurses* nc){
  (void)nc;
  return -1;
}
#endif

// Demos can be disabled either due to a DFSG build (any non-free material must
// be disabled) or a non-multimedia build (all images/videos must be disabled).
static struct {
  const char* name;
  int (*fxn)(struct notcurses*);
  bool dfsg_disabled;             // disabled for DFSG builds
  bool oiio_disabled;             // disabled for OIIO builds (implies mmeng_disabled)
  bool mmeng_disabled;            // disabled for non-multimedia builds
} demos[26] = {
  { NULL, NULL, false, false, false, },
  { "box", box_demo, false, false, false, },
  {"chunli", chunli_demo, true, false, true, },
  { NULL, NULL, false, false, false, },
  { "eagle", eagle_demo, true, false, true, },
  { "fallin'", fallin_demo, false, false, false, },
  { "grid", grid_demo, false, false, false, },
  { "highcon", highcontrast_demo, false, false, false, },
  { "intro", intro, false, false, false, },
  { "jungle", jungle_demo, true, false, false, },
  { NULL, NULL, false, false, false, },
  { "luigi", luigi_demo, true, false, true, },
  { NULL, NULL, false, false, false, },
  { "normal", normal_demo, false, false, false, },
  { "outro", outro, false, true, true, },
  { NULL, NULL, false, false, false, },
  { "qrcode", qrcode_demo, false, false, false, }, // is blank without USE_QRCODEGEN
  { "reel", reel_demo, false, false, false, },
  { "sliders", sliding_puzzle_demo, false, false, false, },
  { "trans", trans_demo, false, false, false, },
  { "uniblock", unicodeblocks_demo, false, false, false, },
  { "view", view_demo, true, true, true, },
  { "whiteout", witherworm_demo, false, false, false, },
  {"xray", xray_demo, false, true, true, },
  { NULL, NULL, false, false, false, },
  { NULL, NULL, false, false, false, },
};

static void
usage(const char* exe, int status){
  FILE* out = status == EXIT_SUCCESS ? stdout : stderr;
  fprintf(out, "usage: %s [ -hVikc ] [ -m margins ] [ -p path ] [ -l loglevel ] [ -d mult ] [ -J jsonfile ] [ -f renderfile ] demospec\n", exe);
  fprintf(out, " -h: this message\n");
  fprintf(out, " -V: print program name and version\n");
  fprintf(out, " -l: logging level (%d: silent..%d: manic)\n", NCLOGLEVEL_SILENT, NCLOGLEVEL_TRACE);
  fprintf(out, " -i: ignore failures, keep going\n");
  fprintf(out, " -k: keep screen; do not switch to alternate\n");
  fprintf(out, " -d: delay multiplier (non-negative float)\n");
  fprintf(out, " -f: render to file in addition to stdout\n");
  fprintf(out, " -J: emit JSON summary to file\n");
  fprintf(out, " -c: constant PRNG seed, useful for benchmarking\n");
<<<<<<< HEAD
  fprintf(out, " -p: data file path (default: %s)\n", NOTCURSES_SHARE);
  fprintf(out, " -m: margin, or 4 comma-separated margins\n");
  fprintf(out, "if no specification is provided, run %s\n", DEFAULT_DEMO);
  for(size_t i = 0 ; i < sizeof(demos) / sizeof(*demos) ; ++i){
    if(demos[i].name){
      fprintf(out, " %c: run %s\n", (unsigned char)i + 'a', demos[i].name);
=======
  fprintf(out, "all demos are run if no specification is provided\n");
  fprintf(out, " b: run box\n");
  fprintf(out, " e: run eagles\n");
  fprintf(out, " g: run grid\n");
  fprintf(out, " i: run intro\n");
  fprintf(out, " l: run luigi\n");
  fprintf(out, " m: run maxcolor\n");
  fprintf(out, " o: run outro\n");
  fprintf(out, " p: run panelreels\n");
  fprintf(out, " s: run shuffle\n");
  fprintf(out, " t: run thermonuclear\n");
  fprintf(out, " u: run uniblock\n");
  fprintf(out, " v: run view\n");
  fprintf(out, " w: run witherworm\n");
  fprintf(out, " x: run x-ray\n");
  exit(status);
}

static int
intro(struct notcurses* nc){
  struct ncplane* ncp;
  if((ncp = notcurses_stdplane(nc)) == NULL){
    return -1;
  }
  cell c = CELL_TRIVIAL_INITIALIZER;
  cell_set_bg_rgb(&c, 0x20, 0x20, 0x20);
  ncplane_set_default(ncp, &c);
  if(ncplane_cursor_move_yx(ncp, 0, 0)){
    return -1;
  }
  int x, y, rows, cols;
  ncplane_dim_yx(ncp, &rows, &cols);
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(cells_rounded_box(ncp, CELL_STYLE_BOLD, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  channels_set_fg_rgb(&ul.channels, 0xff, 0, 0);
  channels_set_fg_rgb(&ur.channels, 0, 0xff, 0);
  channels_set_fg_rgb(&ll.channels, 0, 0, 0xff);
  channels_set_fg_rgb(&lr.channels, 0xff, 0xff, 0xff);
  if(ncplane_box_sized(ncp, &ul, &ur, &ll, &lr, &hl, &vl, rows, cols,
                       NCBOXGRAD_TOP | NCBOXGRAD_BOTTOM |
                        NCBOXGRAD_RIGHT | NCBOXGRAD_LEFT)){
    return -1;
  }
  cell_release(ncp, &ul); cell_release(ncp, &ur);
  cell_release(ncp, &ll); cell_release(ncp, &lr);
  cell_release(ncp, &hl); cell_release(ncp, &vl);
  const char* cstr = "Δ";
  cell_load(ncp, &c, cstr);
  cell_set_fg_rgb(&c, 200, 0, 200);
  int ys = 200 / (rows - 2);
  for(y = 5 ; y < rows - 6 ; ++y){
    cell_set_bg_rgb(&c, 0, y * ys  , 0);
    for(x = 5 ; x < cols - 6 ; ++x){
      if(ncplane_cursor_move_yx(ncp, y, x)){
        return -1;
      }
      if(ncplane_putc(ncp, &c) <= 0){
        return -1;
      }
    }
  }
  cell_release(ncp, &c);
  uint64_t channels = 0;
  channels_set_fg_rgb(&channels, 90, 0, 90);
  channels_set_bg_rgb(&channels, 0, 0, 180);
  if(ncplane_cursor_move_yx(ncp, 4, 4)){
    return -1;
  }
  if(ncplane_rounded_box(ncp, 0, channels, rows - 6, cols - 6, 0)){
    return -1;
  }
  const char s1[] = " Die Welt ist alles, was der Fall ist. ";
  const char str[] = " Wovon man nicht sprechen kann, darüber muss man schweigen. ";
  if(ncplane_set_fg_rgb(ncp, 192, 192, 192)){
    return -1;
  }
  if(ncplane_set_bg_rgb(ncp, 0, 40, 0)){
    return -1;
  }
  if(ncplane_putstr_aligned(ncp, rows / 2 - 2, NCALIGN_CENTER, s1) != (int)strlen(s1)){
    return -1;
  }
  ncplane_styles_on(ncp, CELL_STYLE_ITALIC | CELL_STYLE_BOLD);
  if(ncplane_putstr_aligned(ncp, rows / 2, NCALIGN_CENTER, str) != (int)strlen(str)){
    return -1;
  }
  ncplane_styles_off(ncp, CELL_STYLE_ITALIC);
  ncplane_set_fg_rgb(ncp, 0xff, 0xff, 0xff);
  if(ncplane_putstr_aligned(ncp, rows - 3, NCALIGN_CENTER, "press q at any time to quit") < 0){
    return -1;
  }
  ncplane_styles_off(ncp, CELL_STYLE_BOLD);
  const wchar_t wstr[] = L"▏▁ ▂ ▃ ▄ ▅ ▆ ▇ █ █ ▇ ▆ ▅ ▄ ▃ ▂ ▁▕";
  if(ncplane_putwstr_aligned(ncp, rows / 2 - 5, NCALIGN_CENTER, wstr) < 0){
    return -1;
  }
  if(rows < 45){
    ncplane_set_fg_rgb(ncp, 0xc0, 0, 0x80);
    ncplane_set_bg_rgb(ncp, 0x20, 0x20, 0x20);
    ncplane_styles_on(ncp, CELL_STYLE_BLINK); // heh FIXME replace with pulse
    if(ncplane_putstr_aligned(ncp, 2, NCALIGN_CENTER, "demo runs best with at least 45 lines") < 0){
      return -1;
>>>>>>> a9e1829... merrrrge
    }
  }
  exit(status);
}

static demoresult*
ext_demos(struct notcurses* nc, const char* spec, bool ignore_failures){
  int ret = 0;
  results = malloc(sizeof(*results) * strlen(spec));
  if(results == NULL){
    return NULL;
  }
  memset(results, 0, sizeof(*results) * strlen(spec));
  democount = strlen(spec);
  struct timespec start, now;
  clock_gettime(CLOCK_MONOTONIC, &start);
  uint64_t prevns = timespec_to_ns(&start);
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    results[i].selector = spec[i];
  }
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    if(interrupted){
      break;
    }
<<<<<<< HEAD
    int idx = spec[i] - 'a';
#ifdef DFSG_BUILD
    if(demos[idx].dfsg_disabled){
      continue;
    }
#endif
#ifdef USE_OIIO
    if(demos[idx].oiio_disabled){
      continue;
    }
#endif
#ifndef USE_MULTIMEDIA
    if(demos[idx].mmeng_disabled){
      continue;
=======
    switch(demos[i]){
      case 'i': ret = intro(nc); break;
      case 'o': ret = outro(nc); break;
      case 's': ret = sliding_puzzle_demo(nc); break;
      case 'u': ret = unicodeblocks_demo(nc); break;
      case 't': ret = thermonuclear_demo(nc); break;
      case 'm': ret = maxcolor_demo(nc); break;
      case 'b': ret = box_demo(nc); break;
      case 'g': ret = grid_demo(nc); break;
      case 'l': ret = luigi_demo(nc); break;
      case 'v': ret = view_demo(nc); break;
      case 'e': ret = eagle_demo(nc); break;
      case 'x': ret = xray_demo(nc); break;
      case 'w': ret = witherworm_demo(nc); break;
      case 'p': ret = panelreel_demo(nc); break;
      default:
        fprintf(stderr, "Unknown demo specification: %c\n", *demos);
        ret = -1;
        break;
>>>>>>> a9e1829... merrrrge
    }
#endif
    hud_schedule(demos[idx].name);
    ret = demos[idx].fxn(nc);
    notcurses_reset_stats(nc, &results[i].stats);
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t nowns = timespec_to_ns(&now);
    results[i].timens = nowns - prevns;
    prevns = nowns;
    results[i].result = ret;
    hud_completion_notify(&results[i]);
    if(ret && !ignore_failures){
      break;
    }
  }
  return results;
}

// returns the demos to be run as a string. on error, returns NULL. on no
// specification, also returns NULL, heh. determine this by argv[optind];
// if it's NULL, there were valid options, but no spec.
static const char*
handle_opts(int argc, char** argv, notcurses_options* opts, bool* ignore_failures,
            FILE** json_output){
  bool constant_seed = false;
  *ignore_failures = false;
  char *renderfile = NULL;
  *json_output = NULL;
  int c;
  memset(opts, 0, sizeof(*opts));
  while((c = getopt(argc, argv, "VhickJ:l:r:d:f:p:m:")) != EOF){
    switch(c){
      case 'h':
        usage(*argv, EXIT_SUCCESS);
        break;
      case 'l':{
        int loglevel;
        if(sscanf(optarg, "%d", &loglevel) != 1){
          fprintf(stderr, "Couldn't get an int from %s\n", optarg);
          usage(*argv, EXIT_FAILURE);
        }
        opts->loglevel = loglevel;
        if(opts->loglevel < NCLOGLEVEL_SILENT || opts->loglevel > NCLOGLEVEL_TRACE){
          fprintf(stderr, "Invalid log level: %d\n", opts->loglevel);
          usage(*argv, EXIT_FAILURE);
        }
        break;
      }case 'm':{
        if(opts->margin_t || opts->margin_r || opts->margin_b || opts->margin_l){
          fprintf(stderr, "Provided margins twice!\n");
          usage(*argv, EXIT_FAILURE);
        }
        if(notcurses_lex_margins(optarg, opts)){
          usage(*argv, EXIT_FAILURE);
        }
        break;
      }case 'V':
        printf("notcurses-demo version %s\n", notcurses_version());
        exit(EXIT_SUCCESS);
      case 'J':
        if(*json_output){
          fprintf(stderr, "Supplied -J twice: %s\n", optarg);
          usage(*argv, EXIT_FAILURE);
        }
        if((*json_output = fopen(optarg, "wb")) == NULL){
          fprintf(stderr, "Error opening %s for JSON (%s?)\n", optarg, strerror(errno));
          usage(*argv, EXIT_FAILURE);
        }
        break;
      case 'c':
        constant_seed = true;
        break;
      case 'i':
        *ignore_failures = true;
        break;
      case 'k':
        opts->inhibit_alternate_screen = true;
        break;
      case 'f':
        if(opts->renderfp){
          fprintf(stderr, "-f may only be supplied once\n");
          usage(*argv, EXIT_FAILURE);
        }
        if((opts->renderfp = fopen(optarg, "wb")) == NULL){
          usage(*argv, EXIT_FAILURE);
        }
        break;
      case 'p':
        datadir = optarg;
        break;
      case 'r':
        renderfile = optarg;
        break;
      case 'd':{
        float f;
        if(sscanf(optarg, "%f", &f) != 1){
          fprintf(stderr, "Couldn't get a float from %s\n", optarg);
          usage(*argv, EXIT_FAILURE);
        }
        if(f < 0){
          fprintf(stderr, "Invalid multiplier: %f\n", f);
          usage(*argv, EXIT_FAILURE);
        }
        delaymultiplier = f;
        uint64_t ns = f * GIG;
        demodelay.tv_sec = ns / GIG;
        demodelay.tv_nsec = ns % GIG;
        break;
      }default:
        usage(*argv, EXIT_FAILURE);
    }
  }
  if(!constant_seed){
    srand(time(NULL)); // a classic blunder lol
  }
  if(renderfile){
    opts->renderfp = fopen(renderfile, "wb");
    if(opts->renderfp == NULL){
      fprintf(stderr, "Error opening %s for write\n", renderfile);
      usage(*argv, EXIT_FAILURE);
    }
  }
  const char* spec = argv[optind];
  return spec;
}

static int
table_segment_color(struct ncdirect* nc, const char* str, const char* delim, unsigned color){
  ncdirect_fg(nc, color);
  fputs(str, stdout);
  ncdirect_fg_rgb8(nc, 178, 102, 255);
  fputs(delim, stdout);
  return 0;
}

static int
table_segment(struct ncdirect* nc, const char* str, const char* delim){
  return table_segment_color(nc, str, delim, 0xffffff);
}

static int
table_printf(struct ncdirect* nc, const char* delim, const char* fmt, ...){
  ncdirect_fg_rgb8(nc, 0xD4, 0xAF, 0x37);
  va_list va;
  va_start(va, fmt);
  vfprintf(stdout, fmt, va);
  va_end(va);
  ncdirect_fg_rgb8(nc, 178, 102, 255);
  fputs(delim, stdout);
  return 0;
}

static int
summary_json(FILE* f, const char* spec, int rows, int cols){
  int ret = 0;
  ret |= (fprintf(f, "{\"notcurses-demo\":{\"spec\":\"%s\",\"TERM\":\"%s\",\"rows\":\"%d\",\"cols\":\"%d\",\"runs\":{",
                  spec, getenv("TERM"), rows, cols) < 0);
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    if(results[i].result || !results[i].stats.renders){
      continue;
    }
    ret |= (fprintf(f, "\"%s\":{\"bytes\":\"%ju\",\"frames\":\"%ju\",\"ns\":\"%ju\"}%s",
                    demos[results[i].selector - 'a'].name, results[i].stats.render_bytes,
                    results[i].stats.renders, results[i].timens, i < strlen(spec) - 1 ? "," : "") < 0);
  }
  ret |= (fprintf(f, "}}}\n") < 0);
  return ret;
}

static int
summary_table(struct ncdirect* nc, const char* spec){
  bool failed = false;
  uint64_t totalbytes = 0;
  long unsigned totalframes = 0;
  uint64_t totalrenderns = 0;
  printf("\n");
  table_segment(nc, "              runtime", "│");
  table_segment(nc, " frames", "│");
  table_segment(nc, "output(B)", "│");
  table_segment(nc, "rendering", "│");
  table_segment(nc, " %r", "│");
  table_segment(nc, "    FPS", "│");
  table_segment(nc, "TheoFPS", "║\n══╤═════════╤════════╪═══════╪═════════╪═════════╪═══╪═══════╪═══════╣\n");
  char timebuf[PREFIXSTRLEN + 1];
  char totalbuf[BPREFIXSTRLEN + 1];
  char rtimebuf[PREFIXSTRLEN + 1];
  uint64_t nsdelta = 0;
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    nsdelta += results[i].timens;
    qprefix(results[i].timens, GIG, timebuf, 0);
    qprefix(results[i].stats.render_ns, GIG, rtimebuf, 0);
    bprefix(results[i].stats.render_bytes, 1, totalbuf, 0);
    uint32_t rescolor;
    if(results[i].result != 0){
      rescolor = 0xff303c;
    }else if(!results[i].stats.renders){
      rescolor = 0xbbbbbb;
    }else{
      rescolor = 0x32CD32;
    }
    ncdirect_fg(nc, rescolor);
    printf("%2zu", i);
    ncdirect_fg_rgb8(nc, 178, 102, 255);
    printf("│");
    ncdirect_fg(nc, rescolor);
    printf("%9s", demos[results[i].selector - 'a'].name);
    ncdirect_fg_rgb8(nc, 178, 102, 255);
    printf("│%*ss│%7ju│%*s│ %*ss│%3jd│%7.1f│%7.1f║",
           PREFIXCOLUMNS, timebuf,
           (uintmax_t)(results[i].stats.renders),
           BPREFIXCOLUMNS, totalbuf,
           PREFIXCOLUMNS, rtimebuf,
           (uintmax_t)(results[i].timens ?
            results[i].stats.render_ns * 100 / results[i].timens : 0),
           results[i].timens ?
            results[i].stats.renders / ((double)results[i].timens / GIG) : 0.0,
           results[i].stats.renders ?
            GIG * (double)results[i].stats.renders / results[i].stats.render_ns : 0.0);
    ncdirect_fg(nc, rescolor);
    printf("%s\n", results[i].result < 0 ? "FAILED" :
            results[i].result > 0 ? "ABORTED" :
             !results[i].stats.renders ? "SKIPPED"  : "");
    if(results[i].result < 0){
      failed = true;
    }
    totalframes += results[i].stats.renders;
    totalbytes += results[i].stats.render_bytes;
    totalrenderns += results[i].stats.render_ns;
  }
  qprefix(nsdelta, GIG, timebuf, 0);
  bprefix(totalbytes, 1, totalbuf, 0);
  qprefix(totalrenderns, GIG, rtimebuf, 0);
  table_segment(nc, "", "══╧═════════╪════════╪═══════╪═════════╪═════════╪═══╪═══════╪═══════╝\n");
  printf("            │");
  table_printf(nc, "│", "%*ss", PREFIXCOLUMNS, timebuf);
  table_printf(nc, "│", "%7lu", totalframes);
  table_printf(nc, "│", "%*s", BPREFIXCOLUMNS, totalbuf);
  table_printf(nc, "│", " %*ss", PREFIXCOLUMNS, rtimebuf);
  table_printf(nc, "│", "%3ld", nsdelta ? totalrenderns * 100 / nsdelta : 0);
  table_printf(nc, "│", "%7.1f", nsdelta ? totalframes / ((double)nsdelta / GIG) : 0);
  printf("\n");
  ncdirect_fg_rgb8(nc, 0xff, 0xb0, 0xb0);
  fflush(stdout); // in case we print to stderr below, we want color from above
  if(failed){
    fprintf(stderr, "\nError running demo.\nIs \"%s\" the correct data path? Supply it with -p.\n", datadir);
  }
#ifdef DFSG_BUILD
  ncdirect_fg_rgb8(nc, 0xfe, 0x20, 0x76); // PANTONE Strong Red C + 3x0x20
  fflush(stdout); // in case we print to stderr below, we want color from above
  fprintf(stderr, "\nDFSG version. Some demos are unavailable.\n");
#elif !defined(USE_MULTIMEDIA) // don't double-print for DFSG
  ncdirect_fg_rgb8(nc, 0xfe, 0x20, 0x76); // PANTONE Strong Red C + 3x0x20
  fflush(stdout); // in case we print to stderr below, we want color from above
  fprintf(stderr, "\nNo multimedia support. Some demos are unavailable.\n");
#endif
  return failed;
}

static int
scrub_stdplane(struct notcurses* nc){
  struct ncplane* n = notcurses_stdplane(nc);
  uint64_t channels = 0;
  channels_set_fg(&channels, 0); // explicit black + opaque
  channels_set_bg(&channels, 0);
  if(ncplane_set_base(n, "", 0, channels)){
    return -1;
  }
  ncplane_erase(n);
  return 0;
}

int main(int argc, char** argv){
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale based on user preferences\n");
    return EXIT_FAILURE;
  }
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGWINCH);
  pthread_sigmask(SIG_SETMASK, &sigmask, NULL);
  const char* spec;
  FILE* json = NULL; // emit JSON summary to this file? (-J)
  bool ignore_failures; // continue after a failure? (-k)
  notcurses_options nopts;
  if((spec = handle_opts(argc, argv, &nopts, &ignore_failures, &json)) == NULL){
    if(argv[optind] != NULL){
      usage(*argv, EXIT_FAILURE);
    }
    spec = DEFAULT_DEMO;
  }
  for(size_t i = 0 ; i < strlen(spec) ; ++i){
    int nameidx = spec[i] - 'a';
    if(nameidx < 0 || nameidx > 25 || !demos[nameidx].name){
      fprintf(stderr, "Invalid demo specification: %c\n", spec[i]);
      return EXIT_FAILURE;
    }
  }
  struct timespec starttime;
  clock_gettime(CLOCK_MONOTONIC, &starttime);
  struct notcurses* nc;
  if((nc = notcurses_init(&nopts, NULL)) == NULL){
    return EXIT_FAILURE;
  }
  if(notcurses_mouse_enable(nc)){
    goto err;
  }
  if(input_dispatcher(nc)){
    goto err;
  }
  int dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  if(dimy < MIN_SUPPORTED_ROWS || dimx < MIN_SUPPORTED_COLS){
    goto err;
  }
  if(nopts.inhibit_alternate_screen){ // no one cares. 1s max.
    if(demodelay.tv_sec >= 1){
      sleep(1);
    }else{
      nanosleep(&demodelay, NULL);
    }
  }
  do{
    restart_demos = false;
    interrupted = false;
    notcurses_drop_planes(nc);
    if(scrub_stdplane(nc)){
      goto err;
    }
    if(!hud_create(nc)){
      goto err;
    }
    if(fpsgraph_init(nc)){
      goto err;
    }
    if(menu_create(nc) == NULL){
      goto err;
    }
    if(notcurses_render(nc)){
      goto err;
    }
    notcurses_reset_stats(nc, NULL);
    if(ext_demos(nc, spec, ignore_failures) == NULL){
      goto err;
    }
    if(hud_destroy()){ // destroy here since notcurses_drop_planes will kill it
      goto err;
    }
    if(fpsgraph_stop(nc)){
      goto err;
    }
  }while(restart_demos);
  if(stop_input()){
    goto err;
  }
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  if(nopts.renderfp){
    if(fclose(nopts.renderfp)){
      fprintf(stderr, "Warning: error closing renderfile\n");
    }
  }
  struct ncdirect* ncd = ncdirect_init(NULL, stdout);
  if(!ncd){
    return EXIT_FAILURE;
  }
  if(json && summary_json(json, spec, ncdirect_dim_y(ncd), ncdirect_dim_x(ncd))){
    return EXIT_FAILURE;
  }
  // reinitialize without alternate screen to do some coloring
  if(summary_table(ncd, spec)){
    ncdirect_stop(ncd);
    return EXIT_FAILURE;
  }
  free(results);
  ncdirect_stop(ncd);
  return EXIT_SUCCESS;

err:
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  notcurses_stop(nc);
  if(dimy < MIN_SUPPORTED_ROWS || dimx < MIN_SUPPORTED_COLS){
    fprintf(stderr, "At least an %dx%d terminal is required (current: %dx%d)\n",
            MIN_SUPPORTED_COLS, MIN_SUPPORTED_ROWS, dimx, dimy);
  }
  return EXIT_FAILURE;
}
