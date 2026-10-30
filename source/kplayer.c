/* K PLAYER native interface, GPL-2.0-or-later.
 * Uses the public libVLC 3 API from VideoLAN and SDL2.
 * The bundled, rebuilt VLC 3.0.23 engine is loaded at runtime.
 * Decoder/output plugins are supplied by the target Debian 13 system.
 */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <SDL.h>
#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <glob.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vlc/vlc.h>
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "dynamic.h"
#include "vendor/stb_image.h"
#include "vendor/stb_truetype.h"

#include "i18n.h"
#include "network_location.h"

#define W 640
#define H 480
#define MAX_FILES 4096
#define CACHE 96
static SDL_Window *window;
static SDL_Renderer *renderer;
static libvlc_instance_t *engine;
static libvlc_media_player_t *player;
static volatile sig_atomic_t running = 1;
static unsigned char *fontdata;
static stbtt_fontinfo font;
static unsigned char *bolddata;
static stbtt_fontinfo boldfont;
static int category;
static SDL_Color bg = {18, 18, 18, 255}, panel = {24, 24, 24, 255};
static SDL_Color textc = {255, 255, 255, 255}, muted = {179, 179, 179, 255},
                 accent = {29, 185, 84, 255};
static const char *root = "midias";
static char folder[PATH_MAX], media_name[512], status_note[256];
static int selected = 0, offset = 0, total = 0, mode = 3, option = 0;
static bool paused = false, show_osd = true, menu_dirty = true,
            options_from_video = false;
static bool loop_video = false;
static uint32_t osd_until = 0, note_until = 0;
static float rate = 1.0f;
static int fit = 0;
static SDL_Texture *video_texture;
static int texture_w = 0, texture_h = 0;
static struct {
  pthread_mutex_t mutex;
  unsigned char *pixels;
  unsigned width, height, pitch;
  bool ready;
} video = {.mutex = PTHREAD_MUTEX_INITIALIZER};

typedef struct {
  char *name;
  char *path;
  bool dir;
  long long size;
} Item;
static Item items[MAX_FILES];
typedef struct {
  char *s;
  int size;
  SDL_Texture *texture;
  int w, h;
  unsigned used;
} Text;
static Text cache[CACHE];
static unsigned cache_tick;
static void stop_signal(int sig) {
  (void)sig;
  running = 0;
}
static void color(SDL_Color c) {
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}
static void rect(int x, int y, int w, int h, SDL_Color c) {
  SDL_Rect r = {x, y, w, h};
  color(c);
  SDL_RenderFillRect(renderer, &r);
}
static void rounded(int x, int y, int w, int h, int r, SDL_Color c) {
  rect(x + r, y, w - 2 * r, h, c);
  rect(x, y + r, r, h - 2 * r, c);
  rect(x + w - r, y + r, r, h - 2 * r, c);
  color(c);
  for (int d = 0; d < r; d++) {
    int dx = (int)sqrt((double)r * r - (r - d - .5) * (r - d - .5));
    SDL_RenderDrawLine(renderer, x + r - dx, y + d, x + w - r + dx - 1, y + d);
    SDL_RenderDrawLine(renderer, x + r - dx, y + h - 1 - d, x + w - r + dx - 1,
                       y + h - 1 - d);
  }
}
static void triangle(int x, int y, int size, SDL_Color c) {
  color(c);
  for (int i = 0; i < size; i++) {
    int half = (size - i) / 2;
    SDL_RenderDrawLine(renderer, x + i, y + size / 2 - half, x + i,
                       y + size / 2 + half);
  }
}
static const unsigned char *decode(const unsigned char *s, int *cp) {
  unsigned a = *s++;
  if (a < 128) {
    *cp = a;
    return s;
  }
  int n = (a & 0xe0) == 0xc0   ? 1
          : (a & 0xf0) == 0xe0 ? 2
          : (a & 0xf8) == 0xf0 ? 3
                               : 0;
  if (!n) {
    *cp = '?';
    return s;
  }
  int value = a & ((1 << (6 - n)) - 1);
  for (int i = 0; i < n; i++) {
    if ((*s & 0xc0) != 0x80) {
      *cp = '?';
      return s;
    }
    value = (value << 6) | (*s++ & 63);
  }
  *cp = value;
  return s;
}
static int font_px(int size) { return size < 17 ? size + 2 : size + 1; }
static float measure(const char *s, int size) {
  float scale = stbtt_ScaleForPixelHeight(size >= 17 ? &boldfont : &font,
                                          (float)font_px(size)),
        x = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p;) {
    int c, a, b;
    p = decode(p, &c);
    stbtt_GetCodepointHMetrics(size >= 17 ? &boldfont : &font, c, &a, &b);
    x += a * scale;
  }
  return x;
}
static Text *text_texture(const char *s, int size) {
  cache_tick++;
  int slot = 0;
  for (int i = 0; i < CACHE; i++) {
    if (cache[i].s && cache[i].size == size && !strcmp(cache[i].s, s)) {
      cache[i].used = cache_tick;
      return &cache[i];
    }
    if (!cache[i].s || (cache[slot].s && cache[i].used < cache[slot].used))
      slot = i;
  }
  Text *t = &cache[slot];
  free(t->s);
  if (t->texture)
    SDL_DestroyTexture(t->texture);
  memset(t, 0, sizeof(*t));
  t->s = strdup(s);
  t->size = size;
  t->used = cache_tick;
  t->w = (int)ceil(measure(s, size)) + 4;
  t->h = font_px(size) + 8;
  unsigned char *pixels = calloc((size_t)t->w * t->h, 4);
  if (!pixels)
    return t;
  float scale = stbtt_ScaleForPixelHeight(size >= 17 ? &boldfont : &font,
                                          (float)font_px(size));
  int asc, desc, gap;
  stbtt_GetFontVMetrics(size >= 17 ? &boldfont : &font, &asc, &desc, &gap);
  float pen = 2;
  int baseline = (int)ceil(asc * scale) + 2;
  for (const unsigned char *p = (const unsigned char *)s; *p;) {
    int c, a, b, bw, bh, ox, oy;
    p = decode(p, &c);
    unsigned char *bitmap = stbtt_GetCodepointBitmap(
        size >= 17 ? &boldfont : &font, scale, scale, c, &bw, &bh, &ox, &oy);
    for (int y = 0; y < bh; y++)
      for (int x = 0; x < bw; x++) {
        int tx = (int)pen + ox + x, ty = baseline + oy + y;
        if (tx >= 0 && tx < t->w && ty >= 0 && ty < t->h) {
          size_t n = ((size_t)ty * t->w + tx) * 4;
          pixels[n] = pixels[n + 1] = pixels[n + 2] = 255;
          pixels[n + 3] = bitmap[y * bw + x];
        }
      }
    stbtt_FreeBitmap(bitmap, NULL);
    stbtt_GetCodepointHMetrics(size >= 17 ? &boldfont : &font, c, &a, &b);
    pen += a * scale;
  }
  t->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                 SDL_TEXTUREACCESS_STATIC, t->w, t->h);
  if (t->texture) {
    SDL_UpdateTexture(t->texture, NULL, pixels, t->w * 4);
    SDL_SetTextureBlendMode(t->texture, SDL_BLENDMODE_BLEND);
  }
  free(pixels);
  return t;
}
static void label(const char *s, int x, int y, int size, SDL_Color c,
                  int limit) {
  char clipped[1024];
  snprintf(clipped, sizeof(clipped), "%s", s);
  if (limit > 0 && measure(clipped, size) > limit) {
    size_t len = strlen(clipped);
    while (len) {
      do {
        len--;
      } while (len > 0 && ((unsigned char)clipped[len] & 0xc0) == 0x80);
      clipped[len] = 0;
      char trial[1028];
      memcpy(trial, clipped, len);
      memcpy(trial + len, "…", 4);
      if (measure(trial, size) <= limit) {
        if (len + 4 <= sizeof(clipped))
          memcpy(clipped, trial, len + 4);
        break;
      }
    }
  }
  Text *t = text_texture(clipped, size);
  if (!t->texture)
    return;
  SDL_SetTextureColorMod(t->texture, c.r, c.g, c.b);
  SDL_SetTextureAlphaMod(t->texture, c.a);
  SDL_Rect r = {x - 2, y - 2, t->w, t->h};
  SDL_RenderCopy(renderer, t->texture, NULL, &r);
}
/* Indeterminate: there is no known total until traversal has finished.  SDL
 * remains on the main thread; refresh periodically while entries are read. */
static uint32_t loading_last_frame;
static void loading_screen(const char *message, int found, bool force) {
  if (!renderer || !fontdata || !bolddata || !p_SDL_GetTicks) return;
  uint32_t tick = SDL_GetTicks();
  if (!force && tick - loading_last_frame < 80) return;
  loading_last_frame = tick;
  color(bg);
  SDL_RenderClear(renderer);
  rounded(88, 142, 464, 190, 12, panel);
  const char *title_text = tr(message);
  int title_x = (W - (int)measure(title_text, 23)) / 2;
  label(title_text, title_x, 174, 23, textc, 410);
  const char *subtitle = tr("Aguarde, procurando arquivos...");
  int sub_x = (W - (int)measure(subtitle, 14)) / 2;
  label(subtitle, sub_x, 211, 14, muted, 410);
  rounded(132, 248, 376, 14, 7, (SDL_Color){48, 48, 48, 255});
  int segment = (int)((tick / 6) % 594);
  if (segment > 297) segment = 594 - segment;
  rounded(134 + segment, 250, 76, 10, 5, accent);
  char count_text[96];
  snprintf(count_text, sizeof(count_text), "%s: %d", tr("Arquivos encontrados"), found);
  int count_x = (W - (int)measure(count_text, 13)) / 2;
  label(count_text, count_x, 278, 13, muted, 410);
  SDL_RenderPresent(renderer);
}
static void notify(const char *message) {
  snprintf(status_note, sizeof(status_note), "%s", message);
  note_until = SDL_GetTicks() + 4000;
  menu_dirty = true;
}
static void time_string(long long ms, char *out, size_t n) {
  long long s = ms > 0 ? ms / 1000 : 0;
  if (s >= 3600)
    snprintf(out, n, "%lld:%02lld:%02lld", s / 3600, s / 60 % 60, s % 60);
  else
    snprintf(out, n, "%02lld:%02lld", s / 60, s % 60);
}
static bool supported(const char *s) {
  const char *p = strrchr(s, '.');
  if (!p)
    return false;
  const char *ext[] = {".mp4", ".mkv", ".avi",  ".mov", ".webm",
                       ".m4v", ".mpg", ".mpeg", ".ts",  ".flv",
                       ".wmv", ".mp3", ".flac", ".ogg", ".wav",
                       ".m4a", ".aac", ".opus", NULL};
  for (int i = 0; ext[i]; i++)
    if (!strcasecmp(p, ext[i]))
      return true;
  return false;
}
static bool audio_file(const char *s) {
  const char *p = strrchr(s, '.');
  return p && (!strcasecmp(p, ".mp3") || !strcasecmp(p, ".flac") ||
               !strcasecmp(p, ".ogg") || !strcasecmp(p, ".wav") ||
               !strcasecmp(p, ".m4a") || !strcasecmp(p, ".aac") ||
               !strcasecmp(p, ".opus"));
}
static int sort_items(const void *a, const void *b) {
  const Item *x = a, *y = b;
  if (x->dir != y->dir)
    return y->dir - x->dir;
  return strcasecmp(x->name, y->name);
}
static void clear_items(void) {
  for (int i = 0; i < total; i++) {
    free(items[i].name);
    free(items[i].path);
  }
  total = 0;
}
#include "storage.h"
static void scan(void) {
  if (locations_view) {
    refresh_locations();
    return;
  }
  folder_error[0] = 0;
  loading_screen("Carregando arquivos", 0, true);
  clear_items();
  selected = offset = 0;
  DIR *d = opendir(folder);
  if (!d) {
    snprintf(folder_error, sizeof(folder_error), "%s",
             errno == EACCES || errno == EPERM
                 ? tr("Sem permissão de leitura nesta pasta")
                 : tr("Pasta indisponível ou cartão removido"));
    notify(folder_error);
    return;
  }
  struct dirent *e;
  bool capped = false;
  while ((e = readdir(d))) {
    loading_screen("Carregando arquivos", total, false);
    if (e->d_name[0] == '.')
      continue;
    char path[PATH_MAX];
    if (snprintf(path, sizeof(path), "%s/%s", folder, e->d_name) >=
        (int)sizeof(path))
      continue;
    struct stat st;
    if (stat(path, &st))
      continue;
    bool dir = S_ISDIR(st.st_mode);
    if (!dir && (!S_ISREG(st.st_mode) || !supported(e->d_name)))
      continue;
    if (!dir && ((category == 1 && !audio_file(e->d_name)) ||
                 (category == 2 && audio_file(e->d_name))))
      continue;
    if (total == MAX_FILES) {
      capped = true;
      break;
    }
    char *name = strdup(e->d_name), *stored_path = strdup(path);
    if (!name || !stored_path) {
      free(name);
      free(stored_path);
      break;
    }
    items[total++] = (Item){name, stored_path, dir, st.st_size};
  }
  closedir(d);
  qsort(items, total, sizeof(*items), sort_items);
  if (capped)
    notify(tr("Limite de 4096 itens: organize em subpastas"));
  menu_dirty = true;
}
static bool automatic_has_path(const char *path) {
  for (int i = 0; i < total; i++)
    if (items[i].path && !strcmp(items[i].path, path))
      return true;
  return false;
}
static void automatic_scan_tree(const char *base, int depth, bool *capped) {
  if (depth > 32 || total >= MAX_FILES) {
    *capped = total >= MAX_FILES;
    return;
  }
  DIR *d = opendir(base);
  if (!d)
    return;
  struct dirent *e;
  while ((e = readdir(d)) && total < MAX_FILES) {
    loading_screen(category == 1 ? "Carregando músicas" : "Carregando vídeos",
                   total, false);
    if (e->d_name[0] == '.')
      continue;
    char path[PATH_MAX];
    if (snprintf(path, sizeof(path), "%s/%s", base, e->d_name) >=
        (int)sizeof(path))
      continue;
    struct stat lst, st;
    if (lstat(path, &lst) || S_ISLNK(lst.st_mode) || stat(path, &st))
      continue;
    if (S_ISDIR(st.st_mode)) {
      automatic_scan_tree(path, depth + 1, capped);
      continue;
    }
    if (!S_ISREG(st.st_mode) || st.st_size < 50 * 1024 ||
        !supported(e->d_name) || (category == 1 && !audio_file(e->d_name)) ||
        (category == 2 && audio_file(e->d_name)) || automatic_has_path(path))
      continue;
    char *name = strdup(e->d_name), *stored_path = strdup(path);
    if (!name || !stored_path) {
      free(name);
      free(stored_path);
      *capped = true;
      break;
    }
    items[total++] = (Item){name, stored_path, false, st.st_size};
  }
  if (total >= MAX_FILES)
    *capped = true;
  closedir(d);
}
static void scan_automatic_media(void) {
  loading_screen(category == 1 ? "Carregando músicas" : "Carregando vídeos",
                 0, true);
  clear_items();
  selected = offset = 0;
  folder_error[0] = 0;
  automatic_view = true;
  locations_view = false;
  bool capped = false;
  for (size_t i = 0;
       i < sizeof(automatic_roots) / sizeof(*automatic_roots) && !capped; i++) {
    char resolved[PATH_MAX];
    if (realpath(automatic_roots[i], resolved))
      automatic_scan_tree(resolved, 0, &capped);
  }
  qsort(items, total, sizeof(*items), sort_items);
  if (!total)
    snprintf(folder_error, sizeof(folder_error), "%s",
             tr("Nenhuma mídia acima de 50 KB encontrada"));
  if (capped)
    notify(tr("Limite de 4096 itens: organize em subpastas"));
  mode = 0;
  menu_dirty = true;
}
static unsigned format_video(void **opaque, char *chroma, unsigned *width,
                             unsigned *height, unsigned *pitches,
                             unsigned *lines) {
  (void)opaque;
  unsigned w = *width, h = *height;
  if (!w || !h) {
    w = W;
    h = H;
  }
  double factor = fmin((double)W / w, (double)H / h);
  if (factor > 1)
    factor = 1;
  w = (unsigned)fmax(2, floor(w * factor));
  h = (unsigned)fmax(2, floor(h * factor));
  unsigned pitch = (w * 4 + 31) & ~31u;
  unsigned char *p = NULL;
  if (posix_memalign((void **)&p, 32, (size_t)pitch * h))
    return 0;
  memset(p, 0, (size_t)pitch * h);
  pthread_mutex_lock(&video.mutex);
  free(video.pixels);
  video.pixels = p;
  video.width = w;
  video.height = h;
  video.pitch = pitch;
  video.ready = false;
  pthread_mutex_unlock(&video.mutex);
  memcpy(chroma, "RV32", 4);
  *width = w;
  *height = h;
  *pitches = pitch;
  *lines = h;
  return 1;
}
static void cleanup_video(void *opaque) {
  (void)opaque;
  pthread_mutex_lock(&video.mutex);
  video.ready = false;
  pthread_mutex_unlock(&video.mutex);
}
static void *lock_video(void *opaque, void **planes) {
  (void)opaque;
  pthread_mutex_lock(&video.mutex);
  planes[0] = video.pixels;
  return NULL;
}
static void unlock_video(void *opaque, void *picture, void *const *planes) {
  (void)opaque;
  (void)picture;
  (void)planes;
  video.ready = true;
  pthread_mutex_unlock(&video.mutex);
}
#include "media_ui.h"
// Browser uses the media state above.
#include "music_browser.h"

static void stop_media(void) {
  active = false;
  playing_radio = false;
  if (player)
    libvlc_media_player_stop(player);
  pthread_mutex_lock(&video.mutex);
  video.ready = false;
  pthread_mutex_unlock(&video.mutex);
  if (video_texture) {
    SDL_DestroyTexture(video_texture);
    video_texture = NULL;
  }
  texture_w = texture_h = 0;
  paused = false;
  mode = 0;
  menu_dirty = true;
}
static void play_path(const char *path) {
  char saved[PATH_MAX];
  snprintf(saved, sizeof(saved), "%s", path);
  libvlc_media_t *m = libvlc_media_new_path(engine, saved);
  if (!m) {
    notify(tr("Não foi possível abrir a mídia"));
    return;
  }
  stop_media();
  snprintf(current_path, sizeof(current_path), "%s", saved);
  const char *name = strrchr(saved, '/');
  name = name ? name + 1 : saved;
  snprintf(media_name, sizeof(media_name), "%.511s", name);
  snprintf(title, sizeof(title), "%.511s", name);
  char *dot = strrchr(title, '.');
  if (dot)
    *dot = 0;
  artist[0] = album[0] = 0;
  playing_audio = audio_file(name);
  media_sample_rate = media_channels = media_bitrate_kbps = 0;
  media_ui_read_file_info(saved);
  load_lyrics(saved);
  load_cover(saved);
  apply_local_metadata(saved);
  plain_scroll = 0;
  queue_selected = queue_index;
  libvlc_media_player_set_media(player, m);
  libvlc_media_release(m);
  if (libvlc_media_player_play(player) < 0) {
    notify(tr("Falha ao iniciar. Consulte log.txt"));
    return;
  }
  active = true;
  meta_pending = true;
  meta_deadline = SDL_GetTicks() + 8000;
  meta_next = 0;
  libvlc_media_player_set_rate(player, rate);
  mode = playing_audio ? 4 : 1;
  osd_until = SDL_GetTicks() + 3000;
  show_osd = true;
}
static void start_media(void) {
  if (!total)
    return;
  const char *path = items[selected].path;
  if (!path)
    return;
  if (items[selected].dir) {
    enter_folder(path);
    return;
  }
  build_queue();
  play_path(path);
}
static void parent_folder(void) {
  if (automatic_view) {
    automatic_view = false;
    mode = 3;
    return;
  }
  if (!strcmp(folder, location_root) || !strcmp(folder, "/")) {
    show_locations();
    return;
  }
  char parent[PATH_MAX];
  snprintf(parent, sizeof(parent), "%s", folder);
  char *p = strrchr(parent, '/');
  if (p && p > parent)
    *p = 0;
  else
    snprintf(parent, sizeof(parent), "/");
  enter_folder(parent);
}
static void size_string(long long bytes, char *out, size_t n) {
  if (bytes >= 1024LL * 1024 * 1024)
    snprintf(out, n, "%.1f GB", bytes / (1024.0 * 1024 * 1024));
  else
    snprintf(out, n, "%.1f MB", bytes / (1024.0 * 1024));
}
#include "skin.h"
typedef struct {
  const char *name;
  SDL_Color background, panel_color, raised, text, secondary, highlight, subdued;
} KpTheme;
static const KpTheme themes[] = {
  {"Preto e verde", {18,18,18,255}, {24,24,24,255}, {36,36,36,255}, {255,255,255,255}, {179,179,179,255}, {29,185,84,255}, {145,145,145,255}},
  {"Preto e branco", {15,15,15,255}, {28,28,28,255}, {45,45,45,255}, {255,255,255,255}, {190,190,190,255}, {245,245,245,255}, {150,150,150,255}},
  {"Branco e preto", {248,248,248,255}, {233,233,233,255}, {218,218,218,255}, {20,20,20,255}, {70,70,70,255}, {25,25,25,255}, {90,90,90,255}},
  {"Verde e preto", {133,218,150,255}, {166,231,179,255}, {190,239,199,255}, {12,35,18,255}, {32,78,43,255}, {10,45,20,255}, {39,91,50,255}},
  {"Preto e azul", {11,17,29,255}, {19,29,46,255}, {31,45,68,255}, {245,249,255,255}, {178,193,212,255}, {102,182,255,255}, {149,165,184,255}},
  {"Preto e roxo", {22,15,30,255}, {35,24,45,255}, {52,38,65,255}, {252,245,255,255}, {204,187,215,255}, {211,153,255,255}, {170,150,184,255}},
  {"Preto e vermelho", {24,14,15,255}, {39,23,24,255}, {59,35,36,255}, {255,246,246,255}, {218,185,185,255}, {255,139,139,255}, {185,149,149,255}}
};
#define THEME_COUNT ((int)(sizeof(themes) / sizeof(themes[0])))
static int theme_index;
static void apply_theme(int index) {
  if (index < 0 || index >= THEME_COUNT) index = 0;
  theme_index = index;
  const KpTheme *t = &themes[index];
  bg = t->background; panel = t->panel_color; elevated = t->raised;
  textc = t->text; muted = t->secondary; accent = t->highlight; dim = t->subdued;
}
static void save_settings(void);
// Online radio uses the same skin primitives and libVLC player.
#include "radio.h"
// First-run screens use the skin drawing helpers.
#include "onboarding.h"
/* Update operations run off the SDL thread. Output is one fixed status line. */
static pthread_mutex_t update_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool update_busy = false;
static int update_status_code = 0; /* 0 idle, 1 new, 2 latest, 3 done, 4 error */
static char update_tag[32];
static void *update_worker(void *argument) {
  bool installing = (intptr_t)argument != 0;
  char cmd[160], tag[32];
  pthread_mutex_lock(&update_mutex);
  snprintf(tag, sizeof(tag), "%s", update_tag);
  pthread_mutex_unlock(&update_mutex);
  if (installing)
    snprintf(cmd, sizeof(cmd), "python3 ./updater.py install %.30s 2>>log.txt", tag);
  else
    snprintf(cmd, sizeof(cmd), "python3 ./updater.py check 2>>log.txt");
  FILE *f = popen(cmd, "r");
  char line[80] = {0};
  if (f && !fgets(line, sizeof(line), f)) line[0] = 0;
  int status = f ? pclose(f) : -1;
  pthread_mutex_lock(&update_mutex);
  if (status == 0 && !strncmp(line, "NEW v", 5)) {
    size_t n = strcspn(line + 4, "\r\n");
    bool valid = n > 1 && n < sizeof(update_tag);
    for (size_t i = 1; valid && i < n; i++)
      if (!isdigit((unsigned char)line[4 + i]) && line[4 + i] != '.') valid = false;
    if (valid) {
      memcpy(update_tag, line + 4, n); update_tag[n] = 0;
      update_status_code = 1;
    } else update_status_code = 4;
  } else if (status == 0 && !strncmp(line, "LATEST", 6)) update_status_code = 2;
  else if (status == 0 && !strncmp(line, "OK ", 3)) update_status_code = 3;
  else update_status_code = 4;
  update_busy = false;
  pthread_mutex_unlock(&update_mutex);
  return NULL;
}
static bool update_is_busy(void) {
  pthread_mutex_lock(&update_mutex);
  bool busy = update_busy;
  pthread_mutex_unlock(&update_mutex);
  return busy;
}
static void update_start(void) {
  pthread_mutex_lock(&update_mutex);
  if (update_busy) { pthread_mutex_unlock(&update_mutex); return; }
  bool install = update_status_code == 1;
  update_busy = true;
  pthread_mutex_unlock(&update_mutex);
  pthread_t worker;
  if (pthread_create(&worker, NULL, update_worker, (void *)(intptr_t)install) == 0)
    pthread_detach(worker);
  else {
    pthread_mutex_lock(&update_mutex);
    update_busy = false; update_status_code = 4;
    pthread_mutex_unlock(&update_mutex);
  }
}
/* Runs before the main menu; failed/offline checks never prevent startup. */
static bool startup_update(void) {
  bool installing = false, yes = false;
  update_start();
  for (;;) {
    bool busy;
    int status;
    char tag[sizeof(update_tag)];
    pthread_mutex_lock(&update_mutex);
    busy = update_busy;
    status = update_status_code;
    snprintf(tag, sizeof(tag), "%s", update_tag);
    pthread_mutex_unlock(&update_mutex);
    if (!busy && !installing && status != 1) return false;
    if (!busy && installing && status == 3) return true;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT && !busy) return false;
      if (e.type != SDL_KEYDOWN || e.key.repeat || busy) continue;
      SDL_Keycode k = e.key.keysym.sym;
      if (installing) {
        if (k == SDLK_RETURN || k == SDLK_BACKSPACE || k == SDLK_ESCAPE)
          return false;
      } else if (k == SDLK_LEFT || k == SDLK_RIGHT || k == SDLK_UP || k == SDLK_DOWN) {
        yes = !yes;
      } else if (k == SDLK_BACKSPACE || k == SDLK_ESCAPE) {
        return false;
      } else if (k == SDLK_RETURN) {
        if (!yes) return false;
        installing = true;
        update_start();
      }
    }
    color(bg);
    SDL_RenderClear(renderer);
    label("K PLAYER", 48, 48, 30, accent, 544);
    rounded(42, 120, 556, 240, 14, panel);
    if (busy) {
      label(tr(installing ? "Baixando e instalando..." : "Procurando atualizações..."), 68, 192, 24, textc, 505);
      label(tr("Aguarde..."), 68, 245, 18, muted, 500);
    } else if (installing) {
      label(tr("Não foi possível atualizar"), 68, 175, 24, textc, 500);
      label(tr("Erro; consulte log.txt"), 68, 225, 18, muted, 500);
      label(tr("B Continuar"), 68, 280, 18, accent, 500);
    } else {
      char line[100];
      snprintf(line, sizeof(line), "%s: %s", tr("Nova versão disponível"), tag);
      label(line, 68, 150, 22, accent, 500);
      label(tr("Deseja atualizar agora?"), 68, 200, 22, textc, 500);
      rounded(160, 260, 140, 45, 8, yes ? accent : elevated);
      rounded(340, 260, 140, 45, 8, !yes ? accent : elevated);
      label(tr("Sim"), 205, 272, 20, yes ? bg : textc, 80);
      label(tr("Não"), 385, 272, 20, !yes ? bg : textc, 80);
      label(tr("Esquerda/Direita: escolher    B: confirmar"), 62, 390, 17, muted, 520);
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(33);
  }
}
static void track_label(bool subtitle, char *out, size_t n) {
  int id =
      subtitle ? libvlc_video_get_spu(player) : libvlc_audio_get_track(player);
  if (subtitle && id < 0) {
    snprintf(out, n, "%s", tr("Desativada"));
    return;
  }
  libvlc_track_description_t *list =
      subtitle ? libvlc_video_get_spu_description(player)
               : libvlc_audio_get_track_description(player);
  snprintf(out, n, "%s", tr("Padrão"));
  for (libvlc_track_description_t *t = list; t; t = t->p_next)
    if (t->i_id == id) {
      snprintf(out, n, "%s", t->psz_name ? t->psz_name : tr("Faixa"));
      break;
    }
  libvlc_track_description_list_release(list);
}
static void draw_options(void) {
  color(bg);
  SDL_RenderClear(renderer);
  draw_header(tr("OPÇÕES"));
  label(tr("Opções"), 24, 52, 25, textc, 500);
  const char *names[] = {
      tr("Velocidade"),    tr("Formato da imagem"), tr("Faixa de áudio"),
      tr("Legendas"),      tr("Repetir mídia"),     tr("Idioma"),
      tr("Termos de uso"), tr("Favoritar faixa"),   tr("Sair do KPlayer"),
      tr("Tema"), tr("Atualização")};
  for (int i = 0; i < 11; i++) {
    int y = 85 + i * 29;
    rounded(24, y, 592, 27, 8, i == option ? elevated : panel);
    label(names[i], 40, y + 8, 18, i == option ? accent : textc, 280);
    char value[256];
    switch (i) {
    case 0:
      snprintf(value, sizeof(value), "%.2fx", rate);
      break;
    case 1:
      snprintf(value, sizeof(value), "%s",
               fit == 0   ? tr("Proporção original")
               : fit == 1 ? tr("Preencher (corte)")
                          : tr("Esticar"));
      break;
    case 2:
    case 3:
      if (options_from_video && !preview_mode)
        track_label(i == 3, value, sizeof(value));
      else
        snprintf(value, sizeof(value), "%s", tr("Abra uma mídia"));
      break;
    case 4:
      snprintf(value, sizeof(value), "%s",
               loop_video ? tr("Ativada") : tr("Desativada"));
      break;
    case 5:
      snprintf(value, sizeof(value), "%s", language_names[language]);
      break;
    case 6:
      snprintf(value, sizeof(value), "%s", tr("Abrir"));
      break;
    case 7:
      snprintf(value, sizeof(value), "%s",
               active && playing_audio
                   ? (is_favorite() ? tr("Ativada") : tr("Desativada"))
                   : tr("Abra uma mídia"));
      break;
    case 9:
      snprintf(value, sizeof(value), "%s", tr(themes[theme_index].name));
      break;
    case 10:
      pthread_mutex_lock(&update_mutex);
      if (update_busy) snprintf(value, sizeof(value), "%s", tr("Aguarde..."));
      else if (update_status_code == 1) snprintf(value, sizeof(value), "%s: %s", update_tag, tr("B Atualizar"));
      else if (update_status_code == 2) snprintf(value, sizeof(value), "%s", tr("Versão atual"));
      else if (update_status_code == 3) snprintf(value, sizeof(value), "%s", tr("Reinicie o KPlayer"));
      else if (update_status_code == 4) snprintf(value, sizeof(value), "%s", tr("Erro; consulte log.txt"));
      else snprintf(value, sizeof(value), "%s", tr("B Verificar"));
      pthread_mutex_unlock(&update_mutex);
      break;
    default:
      snprintf(value, sizeof(value), "%s", tr("Abrir"));
    }
    label(value, 344, y + 8, 17, muted, 253);
  }
  label(tr("Esquerda/direita ou B para alterar"), 24, 413, 13, muted, 590);
  footer(tr("A Voltar    SELECT Fechar opções"));
}
static void cycle_track(bool subtitle, int dir) {
  libvlc_track_description_t *list =
      subtitle ? libvlc_video_get_spu_description(player)
               : libvlc_audio_get_track_description(player);
  int ids[128], count = 0, index = 0,
                current = subtitle ? libvlc_video_get_spu(player)
                                   : libvlc_audio_get_track(player);
  for (libvlc_track_description_t *t = list; t && count < 128; t = t->p_next) {
    if (t->i_id == current)
      index = count;
    ids[count++] = t->i_id;
  }
  if (count) {
    int id = ids[(index + dir + count) % count];
    if (subtitle)
      libvlc_video_set_spu(player, id);
    else
      libvlc_audio_set_track(player, id);
  }
  libvlc_track_description_list_release(list);
}
static void change_option(int dir) {
  switch (option) {
  case 0: {
    float newrate = rate + dir * .25f;
    if (newrate < .5f)
      newrate = 2;
    if (newrate > 2)
      newrate = .5f;
    if (!options_from_video ||
        libvlc_media_player_set_rate(player, newrate) == 0)
      rate = newrate;
    break;
  }
  case 1:
    fit = (fit + dir + 3) % 3;
    break;
  case 2:
  case 3:
    if (options_from_video)
      cycle_track(option == 3, dir);
    break;
  case 4:
    loop_video = !loop_video;
    break;
  case 5: {
    int old = language;
    language = (language + dir + 3) % 3;
    if (!save_preferences(onboarding_done)) {
      language = old;
      notify(tr("Não foi possível salvar. Verifique o cartão."));
    }
    status_next = 0;
    break;
  }
  case 6:
    open_terms(2);
    break;
  case 7:
    if (active && playing_audio)
      toggle_favorite();
    else
      notify(tr("Abra uma mídia"));
    break;
  case 8:
    running = 0;
    break;
  case 9:
    apply_theme((theme_index + dir + THEME_COUNT) % THEME_COUNT);
    save_settings();
    break;
  case 10:
    update_start();
    break;
  }
  menu_dirty = true;
}
static void toggle_pause(void) {
  if (!active)
    return;
  paused = !paused;
  libvlc_media_player_set_pause(player, paused);
}
static void seek_media(long long delta) {
  if (!active || !libvlc_media_player_is_seekable(player))
    return;
  long long t = now_ms() + delta, len = duration_ms();
  if (t < 0)
    t = 0;
  if (len > 0 && t >= len)
    t = len - 1;
  libvlc_media_player_set_time(player, t);
}
static void open_options(void) {
  return_mode = mode;
  mode = 2;
  options_from_video = active;
  option = 0;
}
static void key(SDL_Keycode k, bool repeat) {
  if (update_is_busy()) {
    if (!repeat) notify(tr("Aguarde..."));
    return;
  }
  if (radio_play_is_busy()) {
    if (mode == 7 && k != SDLK_ESCAPE && k != SDLK_F1 && k != SDLK_TAB)
      radio_key(k, repeat);
    else if (!repeat)
      notify("Aguarde a conexão da rádio");
    return;
  }
  if (k == SDLK_ESCAPE) {
    running = 0;
    return;
  }
  if (k == SDLK_F1 && !repeat) {
    if (mode == 2)
      mode = return_mode;
    else
      open_options();
    return;
  }
  if (mode == 5) {
    if (k == SDLK_LEFT || k == SDLK_UP)
      language = (language + 2) % 3;
    else if (k == SDLK_RIGHT || k == SDLK_DOWN)
      language = (language + 1) % 3;
    else if (k == SDLK_m && !repeat)
      open_terms(5);
    else if (k == SDLK_RETURN && !repeat)
      finish_onboarding();
    return;
  }
  if (mode == 6) {
    if ((k == SDLK_BACKSPACE || k == SDLK_m) && !repeat)
      mode = terms_return;
    else if (k == SDLK_LEFT || k == SDLK_UP)
      terms_page = (terms_page + 2) % 3;
    else if (k == SDLK_RIGHT || k == SDLK_DOWN)
      terms_page = (terms_page + 1) % 3;
    return;
  }
  osd_until = SDL_GetTicks() + 3000;
  if (k == SDLK_TAB && !repeat) {
    toggle_pause();
    show_osd = true;
    return;
  }
  if (mode == 2) {
    if ((k == SDLK_BACKSPACE || k == SDLK_m) && !repeat)
      mode = return_mode;
    else if (k == SDLK_UP)
      option = (option + 10) % 11;
    else if (k == SDLK_DOWN)
      option = (option + 1) % 11;
    else if (k == SDLK_LEFT && option != 10)
      change_option(-1);
    else if ((k == SDLK_RIGHT && option != 10) || (k == SDLK_RETURN && !repeat))
      change_option(1);
    return;
  }
  if (mode == 3) {
    if (k == SDLK_LEFT || k == SDLK_UP)
      dash_focus = (dash_focus + 3) % 4;
    else if (k == SDLK_RIGHT || k == SDLK_DOWN)
      dash_focus = (dash_focus + 1) % 4;
    else if (k == SDLK_RETURN && !repeat) {
      category = dash_focus == 0 ? 1 : dash_focus == 1 ? 2 : 0;
      if (dash_focus < 2)
        scan_automatic_media();
      else if (dash_focus == 2)
        show_locations();
      else
        radio_open();
    } else if (k == SDLK_SPACE && !repeat && active)
      mode = playing_audio ? 4 : 1;
    else if (k == SDLK_m && !repeat)
      open_options();
    return;
  }
  if (mode == 7) {
    radio_key(k, repeat);
    return;
  }
  if (mode == 0) {
    if (locations_view) {
      if (k == SDLK_UP)
        location_selected = (location_selected + 3) % 4;
      else if (k == SDLK_DOWN)
        location_selected = (location_selected + 1) % 4;
      else if (k == SDLK_RETURN && !repeat)
        open_location();
      else if (k == SDLK_BACKSPACE && !repeat)
        mode = 3;
      else if (k == SDLK_LEFT || k == SDLK_RIGHT)
        category = (category + (k == SDLK_LEFT ? 2 : 1)) % 3;
      else if (k == SDLK_m && !repeat)
        refresh_locations();
      else if (k == SDLK_SPACE && !repeat && active)
        mode = playing_audio ? 4 : 1;
      return;
    }
    if (k == SDLK_UP && total)
      selected = (selected + total - 1) % total;
    else if (k == SDLK_DOWN && total)
      selected = (selected + 1) % total;
    else if (k == SDLK_RETURN && !repeat)
      start_media();
    else if (k == SDLK_BACKSPACE && !repeat) {
      parent_folder();
    } else if (k == SDLK_LEFT || k == SDLK_RIGHT) {
      category = (category + (k == SDLK_LEFT ? 2 : 1)) % 3;
      if (automatic_view)
        scan_automatic_media();
      else
        scan();
    } else if (k == SDLK_m && !repeat) {
      if (automatic_view)
        scan_automatic_media();
      else
        scan();
      notify(tr("Biblioteca atualizada"));
    } else if (k == SDLK_SPACE && !repeat && active)
      mode = playing_audio ? 4 : 1;
    return;
  }
  if (mode == 4) {
    if (k == SDLK_BACKSPACE && !repeat) {
      if (audio_panel == 2 && parent_music_folder())
        return;
      mode = 3;
      return;
    }
    if (k == SDLK_SPACE && !repeat)
      switch_audio_panel();
    else if (k == SDLK_RETURN && !repeat) {
      if (audio_panel == 1 && queue_count) {
        queue_index = queue_selected;
        play_path(queue_paths[queue_index]);
      } else if (audio_panel == 2)
        play_music_entry();
      else
        toggle_pause();
    } else if (k == SDLK_UP || k == SDLK_DOWN) {
      int d = k == SDLK_UP ? -1 : 1;
      if (audio_panel == 1 && queue_count)
        queue_selected = (queue_selected + d + queue_count) % queue_count;
      else if (audio_panel == 2 && music_total)
        music_selected = (music_selected + d + music_total) % music_total;
      else if (audio_panel == 0 && plain_count) {
        plain_scroll += d;
        if (plain_scroll < 0)
          plain_scroll = 0;
        if (plain_scroll >= plain_count)
          plain_scroll = plain_count - 1;
      }
    } else if (k == SDLK_m && !repeat) {
      audio_panel = 0;
      plain_scroll = 0;
    } else if ((k == SDLK_q || k == SDLK_e) && !repeat) {
      next_media(k == SDLK_q ? -1 : 1);
      queue_selected = queue_index;
    } else if (k == SDLK_LEFT || k == SDLK_RIGHT)
      seek_media(k == SDLK_LEFT ? -10000 : 10000);
    return;
  }
  if (k == SDLK_BACKSPACE && !repeat) {
    if (mode == 1)
      stop_media();
    mode = 3;
    return;
  }
  if (k == SDLK_RETURN && !repeat)
    toggle_pause();
  else if (k == SDLK_SPACE && !repeat) {
    {
      show_osd = !show_osd;
      return;
    }
  } else if (k == SDLK_m && !repeat) {
    if (mode == 4)
      toggle_favorite();
    else
      open_options();
  } else if ((k == SDLK_q || k == SDLK_e) && !repeat)
    next_media(k == SDLK_q ? -1 : 1);
  else if (k == SDLK_LEFT || k == SDLK_RIGHT)
    seek_media(k == SDLK_LEFT ? -10000 : 10000);
  else if (k == SDLK_PAGEUP || k == SDLK_PAGEDOWN)
    seek_media(k == SDLK_PAGEUP ? -5000 : 5000);
  else if (k == SDLK_UP || k == SDLK_DOWN) {
    /* Volume is controlled exclusively by the console firmware. */
  }
  show_osd = true;
}
static bool read_font(const char *path, unsigned char **data,
                      stbtt_fontinfo *info) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  rewind(f);
  if (n <= 0 || n > 20000000) {
    fclose(f);
    return false;
  }
  *data = malloc(n);
  if (!*data) {
    fclose(f);
    return false;
  }
  bool ok = fread(*data, 1, n, f) == (size_t)n;
  fclose(f);
  return ok &&
         stbtt_InitFont(info, *data, stbtt_GetFontOffsetForIndex(*data, 0));
}
static bool load_font(void) {
  return read_font("fonts/Interface.ttf", &fontdata, &font) &&
         read_font("fonts/Bold.ttf", &bolddata, &boldfont);
}
static void save_settings(void) {
  FILE *f = fopen("settings.tmp", "w");
  if (!f)
    return;
  fprintf(f, "KPLAYER_SETTINGS 2\nfit %d\nloop %d\nrate %.2f\ntheme %d\n", fit,
          loop_video, rate, theme_index);
  if (fclose(f) == 0)
    rename("settings.tmp", "settings.cfg");
}
static void load_settings(void) {
  FILE *f = fopen("settings.cfg", "r");
  if (!f)
    return;
  char line[160];
  int legacy_setting, ft, loop, saved_theme = 0;
  float r;
  bool valid = false;
  if (fgets(line, sizeof(line), f)) {
    if (!strcmp(line, "KPLAYER_SETTINGS 2\n")) {
      valid = fscanf(f, "fit %d\nloop %d\nrate %f", &ft, &loop, &r) == 3;
      if (valid && fscanf(f, "\ntheme %d", &saved_theme) != 1) saved_theme = 0;
    } else
      valid = sscanf(line, "%d %d %d %f", &legacy_setting, &ft, &loop, &r) == 4;
  }
  if (valid && ft >= 0 && ft <= 2 && (loop == 0 || loop == 1) && r >= .5f &&
      r <= 2) {
    fit = ft;
    loop_video = loop;
    rate = r;
    apply_theme(saved_theme);
  }
  fclose(f);
}
static bool screenshot(const char *path) {
  SDL_Surface *s =
      SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
  if (!s)
    return false;
  bool ok = SDL_RenderReadPixels(renderer, NULL, s->format->format, s->pixels,
                                 s->pitch) == 0 &&
            SDL_SaveBMP(s, path) == 0;
  SDL_FreeSurface(s);
  return ok;
}
int main(int argc, char **argv) {
  bool preview = argc > 1 && !strncmp(argv[1], "--preview", 9),
       check = argc > 1 && !strcmp(argv[1], "--check");
  signal(SIGINT, stop_signal);
  signal(SIGTERM, stop_signal);
  signal(SIGPIPE, SIG_IGN);
  if (!load_sdl())
    return 1;
  if (!preview && !load_vlc())
    return 1;
  if (check) {
    printf("K PLAYER 1.0.0 | SDL2 carregada | VLC %s\n", libvlc_get_version());
    return 0;
  }
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0) {
    fprintf(stderr, "SDL: %s\n", SDL_GetError());
    return 1;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  SDL_ShowCursor(SDL_DISABLE);
  window = SDL_CreateWindow("K PLAYER", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, W, H,
                            preview ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
  if (!window) {
    fprintf(stderr, "Janela SDL: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  if (!renderer) {
    fprintf(stderr, "Renderizador SDL: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_RenderSetLogicalSize(renderer, W, H);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  if (!load_font()) {
    fprintf(stderr, "Fonte fonts/Interface.ttf ausente ou inválida\n");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  if (preview) {
    preview_mode = true;
    if (argc > 3) {
      int l = atoi(argv[3]);
      if (l >= 0 && l < 3)
        language = l;
    }
    snprintf(folder, sizeof(folder), "midias");
    const char *names[] = {"Minhas músicas", "Meus vídeos",
                           "Noite tranquila.flac", "Viagem.mp4",
                           "Horizonte.mp3"};
    for (int i = 0; i < 5; i++)
      items[total++] =
          (Item){strdup(names[i]), strdup(names[i]), i < 2, 32500000};
    if (!strcmp(argv[1], "--preview-welcome"))
      draw_welcome();
    else if (!strcmp(argv[1], "--preview-terms")) {
      if (argc > 4)
        terms_page = atoi(argv[4]) % 3;
      draw_terms();
    } else if (!strcmp(argv[1], "--preview-audio")) {
      active = true;
      playing_audio = true;
      snprintf(title, sizeof(title), "Noite tranquila");
      snprintf(artist, sizeof(artist), "Faixa demonstrativa");
      lyrics[0] = (Lyric){0, "O dia vai ficando para trás"};
      lyrics[1] = (Lyric){30000, "E a música encontra seu lugar"};
      lyrics[2] = (Lyric){60000, "Um novo caminho para seguir"};
      lyric_count = 3;
      if (argc > 4) {
        char path[PATH_MAX];
        if (realpath(argv[4], path)) {
          snprintf(current_path, sizeof(current_path), "%s", path);
          media_ui_read_file_info(path);
          load_lyrics(path);
          load_cover(path);
          apply_local_metadata(path);
          queue_paths[0] = strdup(path);
          queue_count = 1;
          queue_index = queue_selected = 0;
          open_current_music_folder();
        }
      }
      if (argc > 5)
        audio_panel = atoi(argv[5]) % 3;
      if (argc > 6)
        preview_duration = atoll(argv[6]);
      draw_music();
    } else if (!strcmp(argv[1], "--preview-video")) {
      active = true;
      snprintf(media_name, sizeof(media_name), "Viagem.mp4");
      draw_video();
    } else if (!strcmp(argv[1], "--preview-options")) {
      if (argc > 4) apply_theme(atoi(argv[4]));
      option = 9;
      draw_options();
    } else if (!strcmp(argv[1], "--preview-radio")) {
      radio_count = 3;
      snprintf(radio_stations[0].name, sizeof(radio_stations[0].name), "Rádio Ilha 107.1");
      snprintf(radio_stations[1].name, sizeof(radio_stations[1].name), "Rádio Massa");
      snprintf(radio_stations[2].name, sizeof(radio_stations[2].name), "Aline FM");
      for (int i = 0; i < radio_count; i++)
        snprintf(radio_stations[i].codec, sizeof(radio_stations[i].codec), "STREAM");
      radio_filter = 5;
      draw_radio();
    } else if (!strcmp(argv[1], "--preview-locations")) {
      show_locations();
      draw_menu();
    } else if (!strcmp(argv[1], "--preview-library"))
      draw_menu();
    else
      draw_home();
    bool ok = screenshot(argc > 2 ? argv[2] : "preview.bmp");
    SDL_RenderPresent(renderer);
    SDL_Quit();
    return ok ? 0 : 1;
  }
  const char *args[] = {"--no-xlib", "--no-video-title-show", "--quiet",
                        "--aout=alsa"};
  engine = libvlc_new(4, args);
  if (!engine) {
    fprintf(stderr, "Falha libvlc_new: %s\n", libvlc_errmsg());
    SDL_Quit();
    return 1;
  }
  libvlc_set_user_agent(engine, "K PLAYER 1.0.0", "KPLAYER/1.0.0");
  player = libvlc_media_player_new(engine);
  if (!player) {
    libvlc_release(engine);
    SDL_Quit();
    return 1;
  }
  libvlc_video_set_callbacks(player, lock_video, unlock_video, NULL, NULL);
  libvlc_video_set_format_callbacks(player, format_video, cleanup_video);
  load_preferences();
  mode = onboarding_done ? 3 : 5;
  load_settings();
  load_favorites();
  bool restart_after_update = startup_update();
  if (restart_after_update) running = 0;
  else {
    network_location_enabled = true;
    network_location_start();
    mkdir(root, 0755);
    snprintf(folder, sizeof(folder), "%s", root);
    scan();
  }
  while (running) {
    uint32_t frame = SDL_GetTicks();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        if (!radio_play_is_busy() && !update_is_busy()) running = 0;
      }
      else if (e.type == SDL_KEYDOWN)
        key(e.key.keysym.sym, e.key.repeat != 0);
    }
    radio_poll_play();
    if (active) {
      update_metadata();
      libvlc_state_t state = libvlc_media_player_get_state(player);
      if (state == libvlc_Error) {
        bool was_radio = playing_radio;
        if (was_radio) { active = false; playing_radio = false; paused = false; }
        else stop_media();
        notify(tr("Falha na reprodução. Consulte log.txt"));
        if (was_radio)
          mode = 7;
      } else if (state == libvlc_Ended) {
        if (playing_radio) {
          active = false;
          paused = false;
          mode = 7;
        } else if (loop_video) {
          play_path(current_path);
        } else if (playing_audio && queue_index + 1 < queue_count)
          next_media(1);
        else {
          stop_media();
          mode = 3;
        }
      }
    }
    if (mode == 1)
      draw_video();
    else if (mode == 2)
      draw_options();
    else if (mode == 3)
      draw_home();
    else if (mode == 4)
      draw_music();
    else if (mode == 5)
      draw_welcome();
    else if (mode == 6)
      draw_terms();
    else if (mode == 7)
      draw_radio();
    else
      draw_menu();
    SDL_RenderPresent(renderer);
    uint32_t elapsed = SDL_GetTicks() - frame;
    if (elapsed < 33)
      SDL_Delay(33 - elapsed);
  }
  save_settings();
  stop_media();
  libvlc_media_player_release(player);
  libvlc_release(engine);
  free(video.pixels);
  clear_items();
  for (int i = 0; i < CACHE; i++) {
    free(cache[i].s);
    if (cache[i].texture)
      SDL_DestroyTexture(cache[i].texture);
  }
  free(fontdata);
  free(bolddata);
  clear_queue();
  clear_music_entries();
  if (cover_texture)
    SDL_DestroyTexture(cover_texture);
  if (radio_art_texture)
    SDL_DestroyTexture(radio_art_texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return restart_after_update ? 42 : 0;
}
