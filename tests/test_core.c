#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <stdatomic.h>
static unsigned char pixels[64*48*4];
static atomic_int frames;
static void *lock_frame(void *o, void **p){(void)o;p[0]=pixels;return NULL;}
static void unlock_frame(void *o,void *p,void *const *planes){(void)o;(void)p;(void)planes;atomic_fetch_add(&frames,1);}
#include <vlc/vlc.h>
#define LOAD(n) __typeof__(n) *p_##n=dlsym(h,#n);if(!p_##n){fprintf(stderr,"Missing %s\n",#n);return 1;}
int main(int argc,char **argv){
 void *h=dlopen("libvlc.so.5",RTLD_NOW|RTLD_GLOBAL);if(!h){puts(dlerror());return 1;}
 LOAD(libvlc_media_add_option);LOAD(libvlc_video_set_callbacks);LOAD(libvlc_video_set_format);LOAD(libvlc_new);LOAD(libvlc_release);LOAD(libvlc_get_version);LOAD(libvlc_media_player_new);LOAD(libvlc_media_new_path);LOAD(libvlc_media_player_set_media);LOAD(libvlc_media_release);LOAD(libvlc_media_player_play);LOAD(libvlc_media_player_get_state);LOAD(libvlc_media_player_stop);LOAD(libvlc_media_player_release);
 const char *args[]={"--quiet","--no-plugins-cache","--aout=adummy","--vout=dummy"};
 libvlc_instance_t *v=p_libvlc_new(4,args);if(!v){fprintf(stderr,"libvlc_new failed\n");return 1;}printf("VLC %s: initialized\n",p_libvlc_get_version());
 if(argc<2){p_libvlc_release(v);return 0;}libvlc_media_player_t *p=p_libvlc_media_player_new(v);libvlc_media_t *m=p_libvlc_media_new_path(v,argv[1]);if(!p||!m)return 1;int isvideo=strstr(argv[1],".yuv")!=NULL;
 if(isvideo){p_libvlc_media_add_option(m,":demux=rawvideo");p_libvlc_media_add_option(m,":rawvid-width=64");p_libvlc_media_add_option(m,":rawvid-height=48");p_libvlc_media_add_option(m,":rawvid-chroma=RV32");p_libvlc_media_add_option(m,":rawvid-fps=10");p_libvlc_video_set_callbacks(p,lock_frame,unlock_frame,NULL,NULL);p_libvlc_video_set_format(p,"RV32",64,48,64*4);}
 p_libvlc_media_player_set_media(p,m);p_libvlc_media_release(m);
 int result=p_libvlc_media_player_play(p);if(result)return 1;int pass=0;
 for(int i=0;i<200;i++){libvlc_state_t state=p_libvlc_media_player_get_state(p);if(state==libvlc_Ended){pass=1;break;}if(state==libvlc_Error)break;usleep(100000);}
 p_libvlc_media_player_stop(p);p_libvlc_media_player_release(p);p_libvlc_release(v);if(isvideo){printf("Frames decoded: %d\n",atomic_load(&frames));pass=pass&&atomic_load(&frames)>0;}puts(pass?"PASS: playback completed with rebuilt ARM64 core and Debian t64 plugins":"FAIL: playback did not complete");return pass?0:1;
}
