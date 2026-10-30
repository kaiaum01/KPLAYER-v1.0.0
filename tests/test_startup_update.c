#define main kplayer_app_main
#include "../source/kplayer.c"
#undef main

static void *press_keys(void *arg) {
  const char *scenario = arg;
  SDL_Delay(450);
  SDL_Event e = {0};
  e.type = SDL_KEYDOWN;
  if (!strcmp(scenario, "yes") || !strcmp(scenario, "failure")) {
    e.key.keysym.sym = SDLK_LEFT;
    SDL_PushEvent(&e);
  }
  e.key.keysym.sym = SDLK_RETURN;
  SDL_PushEvent(&e);
  if (!strcmp(scenario, "failure")) {
    SDL_Delay(450);
    SDL_PushEvent(&e);
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 2 || !load_sdl() || SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) return 2;
  window = SDL_CreateWindow("test", 0, 0, 640, 480, 0);
  renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE) : NULL;
  if (!renderer || !load_font()) return 3;
  pthread_t keys;
  bool send_keys = strcmp(argv[1], "latest") != 0;
  if (send_keys && pthread_create(&keys, NULL, press_keys, argv[1]) != 0) return 4;
  bool restart = startup_update();
  if (send_keys) pthread_join(keys, NULL);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return restart == !strcmp(argv[1], "yes") ? 0 : 1;
}
